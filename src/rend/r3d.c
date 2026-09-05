#include "rend/rend_inc.h"
#include "asset/asset_mgr.h"

// Maybe asset management should happen somewhere..
static Ogl_Render_Bundle tri_bundle = {};
static Ogl_Render_Bundle uber_bundle = {};

////////////////////////////////////////////////
// Triangle Shaders
////////////////////////////////////////////////

const char* tri_vs = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec2 tc;
layout(location=3) in vec4 color;

layout (std140) uniform BatchUbo { mat4 view_proj; };

out vec4 f_color;
out vec3 f_norm;
out vec2 f_tc;

void main() { 
	gl_Position = view_proj * vec4(pos, 1.0);
  f_color = color;
  f_norm = norm;
  f_tc = tc;
}
)";

const char* tri_fs= R"(#version 300 es
precision highp float;
layout(location = 0) out vec4 out_color;

in vec2 f_tc;
in vec4 f_color;
in vec3 f_norm;

void main() {
  out_color = f_color;
}
)";


const char* uber_vs = R"(#version 460 core
precision highp float;
layout(location=0)  in vec3 pos;
layout(location=1)  in vec3 norm;
layout(location=2)  in vec4 tang;
layout(location=3)  in vec2 tc_0;
layout(location=4)  in vec2 tc_1;
layout(location=5)  in vec2 tc_2;
layout(location=6)  in vec2 tc_3;
layout(location=7)  in vec4 col_0;
layout(location=8)  in vec4 col_1;
layout(location=9)  in vec4 col_2;
layout(location=10) in vec4 col_3;
layout(location=11) in vec4 joint_0;
//layout(location=12) in vec4 joint_1;
//layout(location=13) in vec4 joint_2;
//layout(location=14) in vec4 joint_3;
layout(location=12) in vec4 weight_0;

layout (std140) uniform PerFrameData { 
  mat4 view_proj;
  vec3 cam_pos;
  vec3 light_dir;
};
layout(std140) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  float metallic_factor;
  float roughness_factor;

  int base_tc_idx;
  int normal_tc_idx;
  int metallic_roughness_tc_idx;
  int emissive_tc_idx;
  int occlusion_tc_idx;

  mat4 model_matrix;
};

// FIXME: Why is max number of bones 32???? fix this
layout(std140) uniform JointMatrices {
  mat4 joint_mat[32];
};

out vec4 f_color;
out vec4 f_wp;
out vec3 f_view_dir;
out vec3 f_norm;
out vec3 f_tang;
out vec3 f_binorm;
out vec2 f_tc[4];

void main() { 
  f_color = col_0*base_color_factor;

#if 0
  mat4 skinMat =
    weight_0.x * joint_mat[int(joint_0.x)] +
    weight_0.y * joint_mat[int(joint_0.y)] +
    weight_0.z * joint_mat[int(joint_0.z)] +
    weight_0.w * joint_mat[int(joint_0.w)];
#else
  mat4 skinMat = mat4(1.0);
  #endif

	gl_Position = view_proj * model_matrix * skinMat * vec4(pos, 1.0);

  f_tc[0] = tc_0;
  f_tc[1] = tc_1;
  f_tc[2] = tc_2;
  f_tc[3] = tc_3;

  // Normal mapping bullshido
  mat3 M = mat3(model_matrix);
  mat3 normal_matrix = transpose(inverse(M));
  f_norm = normalize(normal_matrix * norm);
  vec3 T = normalize(M * tang.xyz);
  vec3 N = normalize(normal_matrix * norm);
  T = normalize(T - N * dot(N, T));
  vec3 B = normalize(cross(N, T)) * tang.w;
  f_tang = T;
  f_binorm = B;

  f_wp = model_matrix * vec4(pos, 1.0f);
  f_view_dir = normalize(cam_pos.xyz - f_wp.xyz);
}
)";

// ref: https://www.rastertek.com/gl4linuxtut52.html
const char* uber_fs= R"(#version 460 core
precision highp float;
layout(location = 0) out vec4 out_color;

layout (std140) uniform PerFrameData { 
  mat4 view_proj;
  vec3 cam_pos;
  vec3 light_dir;
};

layout(std140) uniform Material {
  vec4 base_color_factor;
  vec4 emissive_factor;
  float metallic_factor;
  float roughness_factor;

  int base_tc_idx;
  int normal_tc_idx;
  int metallic_roughness_tc_idx;
  int emissive_tc_idx;
  int occlusion_tc_idx;

  mat4 model_matrix;
};

in vec2 f_tc[4];
in vec4 f_color;
in vec4 f_wp;
in vec3 f_view_dir;
in vec3 f_norm;
in vec3 f_tang;
in vec3 f_binorm;

uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D metallic_roughness_tex;
uniform sampler2D emissive_tex;
uniform sampler2D occlusion_tex;

void main() {

#if 1
  ivec2 texture_size;
  vec2 tc;
  out_color = f_color * texture(base_color_tex, f_tc[base_tc_idx]);
  out_color += emissive_factor * texture(emissive_tex, f_tc[emissive_tc_idx]);
  out_color += 0.1 * texture(occlusion_tex, f_tc[occlusion_tc_idx]);
  out_color += 0.01 * texture(normal_tex, f_tc[normal_tc_idx]);
  out_color += 0.01 * texture(metallic_roughness_tex, f_tc[metallic_roughness_tc_idx]);
#else

  vec3 lD;
  vec3 albedo, metallic_rough, bump_map;
  vec3 bump_norm;
  float roughness, metallic;
  vec3 F0;
  vec3 half_dir;
  float NdotH, NdotV, NdotL, HdotV;
  float roughness_sqr, rough_sqr2, NdotHSqr, denominator, normal_distribution;
  float smithL, smithV, geom_shadow;
  vec3 fresnel;
  vec3 specularity;
  vec4 color;

  lD = -light_dir;

  // Sample the textures needed
  albedo = texture(base_color_tex, f_tc[base_tc_idx]).rgb;
  metallic_rough = texture(metallic_roughness_tex, f_tc[metallic_roughness_tc_idx]).rgb;
  bump_map = texture(normal_tex, f_tc[normal_tc_idx]).rgb;

  // Perform normal mapping
  bump_map = (bump_map * 2.0f) - 1.0f;
  bump_norm = (bump_map.x * f_tang) + (bump_map.y * f_binorm) + (bump_map.z * f_norm);
  bump_norm = normalize(bump_norm);

  // Retrieve metallic/roughness
  roughness = metallic_rough.r;
  metallic = metallic_rough.b;

  // Calculate fresnel factor
  F0 = vec3(0.04f, 0.04f, 0.04f);
  F0 = mix(F0, albedo, metallic);

  // Setup all needed vectors for lighting
  half_dir = normalize(f_view_dir + lD);
  NdotH = max(0.0f, dot(bump_norm, half_dir));
  NdotV = max(0.0f, dot(bump_norm, f_view_dir));
  NdotL = max(0.0f, dot(bump_norm, lD));
  HdotV = max(0.0f, dot(half_dir, f_view_dir));

  // GGX for normal distribution
  roughness_sqr = roughness * roughness;
  rough_sqr2 = roughness_sqr * roughness_sqr;
  NdotHSqr = NdotH * NdotH;
  denominator = (NdotHSqr * (rough_sqr2 - 1.0f) + 1.0f);
  denominator = 3.14159265359f * (denominator * denominator);
  normal_distribution = rough_sqr2 / denominator;

  // Schlick GGX for the geometric shadow
  smithL = NdotL / (NdotL * (1.0f - roughness_sqr) + roughness_sqr);
  smithV = NdotV / (NdotV * (1.0f - roughness_sqr) + roughness_sqr);
  geom_shadow = smithL * smithV;

  // Fresnel SChlick for fresnel calculation
  fresnel = F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);

  // Cook-Torrance specular BRDF
  specularity = (normal_distribution * fresnel * geom_shadow) / ((4.0f * (NdotL * NdotV)) + 0.00001f);
  color.rgb = albedo + specularity;
  color.rgb = color.rgb * NdotL;
  color = vec4(color.rgb, 1.0f);

  out_color = color;
#endif
}
)";

typedef struct {
  m4 view_proj;
  v3 cam_pos;
  f32 _padding;
  v3 light_dir;
  f32 _padding2;
} PerFrameData_UBO;

typedef struct {
  v4 base_color_factor;
  v4 emissive_factor;

  float metallic_factor;
  float roughness_factor;
  int base_tc_idx;
  int normal_tc_idx;

  int metallic_roughness_tc_idx;
  int emissive_tc_idx;
  int occlusion_tc_idx;
  int _padding;

  m4 model_matrix;
} Material_UBO;

void r3d_try_load_shaders() {
  m4 m = {};
  if (tri_bundle.sp.impl_state == 0) {
    tri_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(tri_vs, tri_fs),
      .vbos = {
        [0] = {
          // the vertex buffer for this should probably be made after r_end has been called
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, nullptr, REND_MAX_INSTANCES, sizeof(Tri_Vertex)),
          .vattribs = {
            [0] = { .location = 0, .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Tri_Vertex, pos), .stride = sizeof(Tri_Vertex), .instanced = false, },
            [1] = { .location = 1, .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Tri_Vertex, norm), .stride = sizeof(Tri_Vertex), .instanced = false,  },
            [2] = { .location = 2, .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Tri_Vertex, uv),    .stride = sizeof(Tri_Vertex), .instanced = false,  },
            [3] = { .location = 3, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Tri_Vertex, color),  .stride = sizeof(Tri_Vertex), .instanced = false,  },
          },
        },
      },
      .ubos = { [0] = { .name = "BatchUbo", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, (m4[]) { m }, 1, sizeof(m4)), .start_offset = 0, .size = sizeof(m4) }, },
      //.rt = ogl_render_target_make(screen_dim.x, screen_dim.y, 2, OGL_TEX_FORMAT_RGBA8U, true),
      .dyn_state = (Ogl_Dyn_State){
//        .viewport = viewport,
//        .scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR,
      },
      .depth_state = (Ogl_Depth_State) {
        .dwrite = OGL_DEPTH_WRITE_ENABLED,
        .dfunc  = OGL_DFUNC_LEQUAL,
      },
    };
  }
  if (uber_bundle.sp.impl_state == 0) {
    uber_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(uber_vs, uber_fs),

      .textures[0] = (Ogl_Tex_Slot){ .name = "base_color_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[1] = (Ogl_Tex_Slot){ .name = "normal_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[2] = (Ogl_Tex_Slot){ .name = "metallic_roughness_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[3] = (Ogl_Tex_Slot){ .name = "emissive_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[4] = (Ogl_Tex_Slot){ .name = "occlusion_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .vbos = {
        [0] = {
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, nullptr, REND_MAX_INSTANCES, sizeof(Uber_Vertex)),
          .vattribs = {
            [0] = { .location = 0,   .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Uber_Vertex, pos),      .stride = sizeof(Uber_Vertex), .instanced = false, },
            [1] =  { .location = 1,  .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Uber_Vertex, norm),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [2] =  { .location = 2,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, tangent),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [3] =  { .location = 3,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_0),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [4] =  { .location = 4,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_1),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [5] =  { .location = 5,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_2),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [6] =  { .location = 6,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_3),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [7] =  { .location = 7,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_0),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [8] =  { .location = 8,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_1),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [9] =  { .location = 9,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_2),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [10] = { .location = 10, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_3),    .stride = sizeof(Uber_Vertex), .instanced = false, },

            [11] = { .location = 11, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [12] = { .location = 11, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            // FIXME: If we get only 2 joints and 2 weights max, 11 + 4 = 15 < 16 (Most OpenGL implementation MAX attribs) 
#if 0
            [11] = { .location = 11, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [12] = { .location = 12, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_1),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [13] = { .location = 13, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_2),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [14] = { .location = 14, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_3),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [15] = { .location = 15, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_0), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [16] = { .location = 16, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_1), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [17] = { .location = 17, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_2), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [18] = { .location = 18, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_3), .stride = sizeof(Uber_Vertex), .instanced = false, },
#endif
          },
        },
      },
      .ubos = { [0] = { .name = "PerFrameData", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(PerFrameData_UBO)), .start_offset = 0, .size = sizeof(PerFrameData_UBO) }, 
                [1] = { .name = "Material", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(Material_UBO)), .start_offset = 0, .size = sizeof(Material_UBO) },
                [2] = { .name = "JointMatrices", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(m4)*32), .start_offset = 0, .size = sizeof(m4)*32 },
      },
      //.rt = ogl_render_target_make(screen_dim.x, screen_dim.y, 2, OGL_TEX_FORMAT_RGBA8U, true),
      .dyn_state = (Ogl_Dyn_State){
//        .viewport = viewport,
//        .scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR,
      },
      .depth_state = (Ogl_Depth_State) {
        .dwrite = OGL_DEPTH_WRITE_ENABLED,
        .dfunc  = OGL_DFUNC_LEQUAL,
      },
    };
  }
}


void r3d_imm_verts(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 *mvp) {
  Ogl_Buf vbo = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, verts, 1, sizeof(Tri_Vertex)*vert_count);
  tri_bundle.vbos[0].buffer = vbo;
  ogl_buf_update(&tri_bundle.ubos[0].buffer, 0, mvp, 1, sizeof(m4));
  // Set dynamically before drawcall currently
  tri_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
  tri_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;
  ogl_render_bundle_draw(&tri_bundle, prim, vert_count, 1);

  // FIXME: This is retarded.. doing cleanup each invocation.. uhm.. what
  ogl_buf_deinit(&vbo);
}

// FIXME: This is INSANELY slow, especially for particles!
// FIXME: Probably cancel any rotation (M33) to be user facing right?
void r3d_imm_xy_face(rect viewport, Ogl_Prim_Type prim, m4 *mvp, color c) {
  Tri_Vertex cube_verts[6] = {
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.0f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.0f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.0f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.0f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.0f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.0f), .color = c},
  };
  r3d_imm_verts(viewport, cube_verts, array_count(cube_verts), prim, mvp);
}

void r3d_imm_cube(rect viewport, Ogl_Prim_Type prim, m4 *mvp, color c) {
  Tri_Vertex cube_verts[36] = {
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .color = c},
    (Tri_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .color = c}
  };
  r3d_imm_verts(viewport, cube_verts, array_count(cube_verts), prim, mvp);
}

void r3d_set_material(Mesh_Primitive_Info *info, m4 model) {
  Material_Info *material = &info->material;
  Ogl_Tex *texture = AM_GET(material->base_tex.tex_asset_id, tex);
  uber_bundle.textures[0] = (Ogl_Tex_Slot){.name = ("base_color_tex"), .tex = *(texture),};

  texture = AM_GET(material->normal_tex.tex_asset_id, tex);
  uber_bundle.textures[1] = (Ogl_Tex_Slot){.name = ("normal_tex"), .tex = *(texture),};

  texture = AM_GET(material->metallic_roughness_tex.tex_asset_id, tex);
  uber_bundle.textures[2] = (Ogl_Tex_Slot){.name = ("metallic_roughness_tex"), .tex = *(texture),};

  texture = AM_GET(material->emissive_tex.tex_asset_id, tex);
  uber_bundle.textures[3] = (Ogl_Tex_Slot){.name = ("emissive_tex"), .tex = *(texture),};

  texture = AM_GET(material->occlusion_tex.tex_asset_id, tex);
  uber_bundle.textures[4] = (Ogl_Tex_Slot){.name = ("occlusion_tex"), .tex = *(texture),};

  Material_UBO material_ubo = (Material_UBO) {
    .base_color_factor = material->base_color_factor,
    .metallic_factor = material->metallic_factor,
    .roughness_factor = material->roughness_factor,
    .emissive_factor = v4_from_v3(material->emissive_factor, 1.0),
    .base_tc_idx = material->base_tex.tc_idx,
    .metallic_roughness_tc_idx = material->metallic_roughness_tex.tc_idx,
    .normal_tc_idx = material->normal_tex.tc_idx,
    .occlusion_tc_idx = material->occlusion_tex.tc_idx,
    .model_matrix = model,
  };
  ogl_buf_update(&uber_bundle.ubos[1].buffer, 0, &material_ubo, 1, sizeof(Material_UBO));
}

void calc_global_transform(Mesh_Info *info, Mesh_Joint *root, m4 parent) {
  m4 local = m4_mult(m4_translate(root->t), m4_mult(m4_from_quat(root->r), m4_scale(root->s)));
  root->m = m4_mult(local, parent);
  for (s32 child_idx = 0; child_idx < root->children_count; child_idx +=1) {
    calc_global_transform(info, &info->joint_hierarchy.joints[root->children[child_idx]], root->m);
  }
}

m4 node_transform(Transform_Node *node) {
  return m4_mult(m4_translate(node->t), m4_mult(m4_from_quat(node->r), m4_scale(node->s)));
}

m4 calc_transform(Model_Info *info, s32 node_idx) {
  Transform_Node *node = &info->nodes[node_idx];
  m4 trans = node_transform(node);

  while (node->parent_idx != node_idx) {
    node_idx = node->parent_idx;
    node = &info->nodes[node->parent_idx];
    trans = m4_mult(node_transform(node), trans);
  }

  return trans;
}

void r3d_imm_model(rect viewport, struct Model_Info *info, m4 vp, m4 model, v3 cam_pos, f32 time_sec) {

  //----------------
  // Do animations!
  //----------------
  for (s64 anim_idx = 0; anim_idx < info->animation_count; anim_idx+=1) {
    Node_Anim *anim = &info->animations[anim_idx];

    f32 anim_time = fmodf(time_sec, anim->max_duration);
    s32 prev_kf_idx = 0;
    s32 next_kf_idx = anim->kf_count-1;
    for (s32 kf_idx = 0; kf_idx < anim->kf_count; kf_idx+=1) {
      f32 timestamp = anim->kf_timestamps[kf_idx];
      if (timestamp < anim_time) prev_kf_idx = kf_idx;
      if (timestamp > anim_time) {
        next_kf_idx = kf_idx;
        break;
      }
    }

    f32 prev_timestamp = anim->kf_timestamps[prev_kf_idx];
    f32 next_timestamp = anim->kf_timestamps[next_kf_idx];
    f32 percent = 0;
    if (next_timestamp - prev_timestamp != 0.0) percent = (anim_time - prev_timestamp) / (next_timestamp - prev_timestamp);


    // FIXME: Interpolate based on Interp_Type, dont do always linear!
    Transform_Node *tn = &info->nodes[anim->node_idx];
    //printf("timestamps: %f - %f - %f\n", prev_timestamp, anim_time, next_timestamp);
    v3 prev, next, interp;
    quat prev4, next4, interp4;
    switch(anim->kind) {
      case NODE_ANIM_KIND_TRANSLATION:
        prev = ((v3*)(anim->values))[prev_kf_idx];
        next = ((v3*)(anim->values))[next_kf_idx];
        //printf("[node:%ld] trans: (%f %f %f)\n", anim->node_idx, tn->t.x, tn->t.y, tn->t.z);
        interp = v3_lerp(prev, next, percent);
        tn->t = interp;
        break;
      case NODE_ANIM_KIND_ROTATION:
        prev4 = ((quat*)(anim->values))[prev_kf_idx];
        next4 = ((quat*)(anim->values))[next_kf_idx];
        interp4 = quat_nlerp(prev4, next4, percent);
        tn->r = interp4;
        //printf("[node:%ld] rot: (%f %f %f %f)\n", anim->node_idx, tn->r.x, tn->r.y, tn->r.z, tn->r.w);
        break;
      case NODE_ANIM_KIND_SCALE:
        prev = ((v3*)(anim->values))[prev_kf_idx];
        next = ((v3*)(anim->values))[next_kf_idx];
        interp = v3_lerp(prev, next, percent);
        tn->s = interp;
        break;
      default:
        break;
    }
  }


  for (s64 mesh_idx = 0; mesh_idx < info->mesh_count; mesh_idx+=1) {
    Mesh_Info *mesh = &info->meshes[mesh_idx];
    m4 global_transform = calc_transform(info, mesh->node_idx);

    if (mesh->joint_hierarchy.joint_count > 0) {
      m4 root_bone_transform = m4d(1.0);
      //m4_mult(m4_mult(m4_translate(root->t), m4_mult(m4_from_quat(root->r), m4_scale(root->s))), root->ibn); 
      // FIXME: joint[0] isnt always the first joint right? or no idk, maybe the root has many such children
      calc_global_transform(mesh, &mesh->joint_hierarchy.joints[0], root_bone_transform);
    }


    for (s64 primitive_idx = 0; primitive_idx < mesh->prim_count; primitive_idx+=1) {
      Mesh_Primitive_Info *prim =  &mesh->prims[primitive_idx];

      // FIXME why is the node TRS multiplied to the model matrix? This is for the whole mesh right?
      m4 model_matrix = m4_mult(model, global_transform);

      //m4 mvp = m4_mult(vp, model_matrix);
      uber_bundle.vbos[0].buffer = prim->vbo;
      uber_bundle.index_buffer = prim->ibo;
      uber_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
      uber_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;

      PerFrameData_UBO pf_ubo = (PerFrameData_UBO) {
        .view_proj = vp,
        .cam_pos = cam_pos,
        .light_dir = v3_norm(v3m(0.2,1,-1)),
      };
      ogl_buf_update(&uber_bundle.ubos[0].buffer, 0, &pf_ubo, 1, sizeof(pf_ubo));

      // We could set update frequence to 'per mesh' here
      m4 joint_matrices[32];
      for (s32 i = 0; i < (s32)array_count(joint_matrices); i+=1) {
        joint_matrices[i] = m4d(1.0);
        //joint_matrices[i] = mesh->joint_hierarchy.joints[i].ibn;
        //joint_matrices[i] = m4_mult(mesh->joint_hierarchy.joints[i].m, mesh->joint_hierarchy.joints[i].ibn);
      }
      ogl_buf_update(&uber_bundle.ubos[2].buffer, 0, joint_matrices, 1, sizeof(joint_matrices));

      r3d_set_material(prim, model_matrix);

      if (prim->ibo.count) {
        ogl_render_bundle_draw_indexed(&uber_bundle, prim->type, prim->ibo.count);
      } else {
        ogl_render_bundle_draw(&uber_bundle, prim->type, prim->vbo.count, 1);
      }
    }
  }
}
