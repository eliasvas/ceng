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
uniform sampler2D u_tex;

void main() {
  ivec2 texture_size;
  vec2 tc;

  out_color = f_color * texture(u_tex, f_tc);
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
//layout(location=11) in vec4 joint_0;
//layout(location=12) in vec4 joint_1;
//layout(location=13) in vec4 joint_2;
//layout(location=14) in vec4 joint_3;
//layout(location=15) in vec4 weight_0;

layout (std140) uniform BatchUbo { mat4 view_proj; };

out vec4 f_color;
out vec3 f_norm;
out vec2 f_tc;

void main() { 
	gl_Position = view_proj * vec4(pos, 1.0);
  f_color = col_0;
  f_norm = norm;
  f_tc = tc_0;
}
)";

const char* uber_fs= R"(#version 460 core
precision highp float;
layout(location = 0) out vec4 out_color;

in vec2 f_tc;
in vec4 f_color;
in vec3 f_norm;
uniform sampler2D u_tex;

void main() {
  ivec2 texture_size;
  vec2 tc;

  out_color = f_color * texture(u_tex, f_tc);
}
)";


void r3d_try_load_shaders() {
  m4 m = {};
  if (tri_bundle.sp.impl_state == 0) {
    tri_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(tri_vs, tri_fs),

      .textures[0] = (Ogl_Tex_Slot){ .name = "u_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},

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

      .textures[0] = (Ogl_Tex_Slot){ .name = "u_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
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
}

void r3d_imm_change_tex(Ogl_Tex *tex) {
  tri_bundle.textures[0] = (Ogl_Tex_Slot){.name = ("u_tex"), .tex = *(tex),};
  uber_bundle.textures[0] = (Ogl_Tex_Slot){.name = ("u_tex"), .tex = *(tex),};
}

void r3d_imm_verts(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 *mvp) {
  //u64 arena_prev_pos = arena_get_current_pos(__frame_arena); 
  //buf sampler_name = arena_sprintf(__frame_arena, "u_tex");

  Ogl_Buf vbo = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, verts, 1, sizeof(Tri_Vertex)*vert_count);
  tri_bundle.vbos[0].buffer = vbo;

  //m4 proj = m4_persp(45.0, viewport.w/(f32)viewport.h, 0.1, 100);
  //m4 view = m4_view(v3m(0,0,0), v3m(0,0,-1), v3m(0,1,0));

  ogl_buf_update(&tri_bundle.ubos[0].buffer, 0, mvp, 1, sizeof(m4));

  // Set dynamically before drawcall currently
  tri_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
  tri_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;

  ogl_render_bundle_draw(&tri_bundle, prim, vert_count, 1);
  //arena_reset_to_pos(__frame_arena, arena_prev_pos);
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


void r3d_imm_model(rect viewport, struct Model_Info *info, m4 *mvp) {
  for (s64 mesh_idx = 0; mesh_idx < info->mesh_count; mesh_idx+=1) {
    Mesh_Info *mesh = &info->meshes[mesh_idx];
    for (s64 primitive_idx = 0; primitive_idx < mesh->prim_count; primitive_idx+=1) {
      Mesh_Primitive_Info *prim =  &mesh->prims[primitive_idx];
      uber_bundle.vbos[0].buffer = prim->vbo;
      uber_bundle.index_buffer = prim->ibo;
      uber_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
      uber_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;
      // Also the material should be filled here (UBO1) right? maybe make another one along mvp
      ogl_buf_update(&uber_bundle.ubos[0].buffer, 0, mvp, 1, sizeof(m4));
      if (prim->ibo.count) {
        ogl_render_bundle_draw_indexed(&uber_bundle, prim->type, prim->ibo.count);
      } else {
        ogl_render_bundle_draw(&uber_bundle, prim->type, prim->vbo.count, 1);
      }
    }
  }
}
