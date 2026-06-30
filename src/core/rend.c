#include "rend.h"

// HMMMMMMM
// Maybe asset management should happen somewhere..
static Ogl_Render_Bundle batch_bundle = {};
static Ogl_Render_Bundle tri_bundle = {};
static Ogl_Tex white_tex = {};

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

////////////////////////////////////////////////
// Batch Shaders
////////////////////////////////////////////////
const char* batch_vs = R"(#version 300 es
precision highp float;
layout(location=0) in vec4 src_rect;
layout(location=1) in vec4 dst_rect;
layout(location=2) in vec4 v_color;
layout(location=3) in float v_rot_rad;

layout (std140) uniform BatchUbo { mat4 view_proj; };

vec2 vertices[4] = vec2[](
  vec2(-0.5,+0.5),
  vec2(-0.5,-0.5),
  vec2(+0.5,-0.5),
  vec2(+0.5,+0.5)
);

vec2 tex_coords[4] = vec2[](
  vec2(0.0,1.0),
  vec2(0.0,0.0),
  vec2(1.0,0.0),
  vec2(1.0,1.0)
);

out vec4 f_color;
out vec2 f_tc;

mat2 rotate2d(float angle) { return mat2(cos(angle), -sin(angle), sin(angle),  cos(angle)); }

void main() { 
  vec2 pos_offset = dst_rect.xy;
  vec2 dim = dst_rect.zw;

  vec2 hdim = vec2(0.5,0.5);

  vec2 pos = vertices[gl_VertexID]; // [-0.5, 0.5] range
  pos *= dim; // scale
  pos = rotate2d(v_rot_rad) * pos; // rotate

  pos += hdim * dim; // += hdim so that its centered on upper-left corner

  pos += pos_offset; // translate

	gl_Position = view_proj * vec4(pos, 0.0, 1.0);

  f_color = v_color;

  vec2 uv = tex_coords[gl_VertexID];
  f_tc = src_rect.xy + uv * src_rect.zw;
}
)";

const char* batch_fs = R"(#version 300 es
precision highp float;
layout(location = 0) out vec4 out_color;

in vec2 f_tc;
in vec4 f_color;
uniform sampler2D u_tex;

void main() {
  ivec2 texture_size;
  vec2 tc;

  // FIXME: GLES30 doesn't support c-style sampler2D array indexing so we have to use max 1 texture
  texture_size = textureSize(u_tex, 0);
  tc = f_tc / vec2(texture_size.x, texture_size.y);
  out_color = f_color * texture(u_tex, tc);
  //out_color = f_color;
}

)";

/////////////////////
// Actual Implementation
/////////////////////

static m4 r_cam_make_view_mat(R_C2D *cam) {
  m4 rot = m4_rotate(cam->rot_deg, v3m(0,0,1));
  return m4_mult(m4_translate(v3m(cam->offset.x, cam->offset.y, 0)),m4_mult(rot,m4_mult(m4_scale(v3m(cam->zoom, cam->zoom,0)), m4_translate(v3m(-cam->origin.x, -cam->origin.y,0)))));
}

// TODO: Should these globals be here? what about asset management (textures/materials/meshes etc..)
static Arena *__frame_arena;
static RN_Pass_List __render_passes;

void rn_begin(Arena *arena, rect dummy_viewport) {
  M_ZERO_STRUCT(&__render_passes);
  __frame_arena = arena;

  // Push a dummy render pass
  R_C2D dummy_cam = (R_C2D){
    .offset = v2m(0,0),
    .origin = v2m(0,0),
    .zoom = 1.0, 
    .rot_deg = 0
  };
  RN_Pass *top_pass = rn_push_pass(RN_PASS_KIND_2D, dummy_cam, dummy_viewport);
  assert(top_pass);

  m4 m = {};
  if (batch_bundle.sp.impl_state == 0) {
    batch_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(batch_vs, batch_fs),
      .vbos = {
        [0] = {
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC,nullptr, REND_MAX_INSTANCES, sizeof(Batch_Vertex)),
          .vattribs = {
            [0] = { .location = 0, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, src_rect), .stride = sizeof(Batch_Vertex), .instanced = true, },
            [1] = { .location = 1, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, dst_rect), .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [2] = { .location = 2, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, color),    .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [3] = { .location = 3, .type = OGL_DATA_TYPE_FLOAT, .offset = offsetof(Batch_Vertex, rot_rad),  .stride = sizeof(Batch_Vertex),.instanced = true,  },
          },
        },
      },
      .ubos = { [0] = { .name = "BatchUbo", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, (m4[]) { m }, 1, sizeof(m4)), .start_offset = 0, .size = sizeof(m4) }, },
      //.rt = ogl_render_target_make(screen_dim.x, screen_dim.y, 2, OGL_TEX_FORMAT_RGBA8U, true),
      .dyn_state = (Ogl_Dyn_State){
//        .viewport = viewport,
//        .scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR,
      }
    };
    white_tex = ogl_tex_make((u8[]){255,255,255,255}, 1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  }

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
      }
    };
  }
}

void rn_flush_all() {
  for (RN_Pass *pass = __render_passes.last; pass != nullptr; pass = pass->prev) {
    R_Quad_Array quads = (R_Quad_Array) {
      pass->cmds,
      pass->cmd_count,
    };
    Batch_Vertex *batch_vertices = arena_push_array(__frame_arena, Batch_Vertex,REND_MAX_INSTANCES);

    s64 vertex_idx  = 0;
    for (s64 quad_idx = 0; quad_idx < quads.count; ++quad_idx) {
      R_Quad *q = &quads.arr[quad_idx];

      Batch_Vertex v = (Batch_Vertex){
        .src_rect = *(v4*)&q->src_rect,
        .dst_rect = *(v4*)&q->dst_rect,
        .color = q->c,
        .rot_rad = DEG2RAD(q->rot_deg),
      };

      batch_vertices[vertex_idx] = v;
      vertex_idx+=1;

      if (vertex_idx >= REND_MAX_INSTANCES || quad_idx+1 >= quads.count || quads.arr[quad_idx+1].tex.impl_state != q->tex.impl_state) {

        u64 arena_prev_pos = arena_get_current_pos(__frame_arena); 
        buf sampler_name = arena_sprintf(__frame_arena, "u_tex");
        batch_bundle.textures[0] = (Ogl_Tex_Slot){ .name = sampler_name.data, .tex = q->tex,};

        // set vertex buffer
        // @BEWARE only for 2D rendering, for 3D we gotta branch
        ogl_buf_update(&batch_bundle.vbos[0].buffer, 0, batch_vertices, vertex_idx, sizeof(Batch_Vertex));

        // set the ubo (currently only the VP matrix)
        m4 proj = m4_ortho(0,pass->viewport.w,0,pass->viewport.h,-1,1);
        m4 view = r_cam_make_view_mat(&pass->cam2d);
        m4 m = m4_mult(proj, view);
        ogl_buf_update(&batch_bundle.ubos[0].buffer, 0, &m, 1, sizeof(m4));

        // Set dynamically before drawcall currently
      batch_bundle.dyn_state.viewport = *(Ogl_rect *)&pass->viewport;
      batch_bundle.dyn_state.scissor = *(Ogl_rect *)&pass->viewport;

        ogl_render_bundle_draw(&batch_bundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, vertex_idx);
        arena_reset_to_pos(__frame_arena, arena_prev_pos);
        vertex_idx = 0;
      }
    }
  }
}

RN_Pass *rn_push_pass(RN_Pass_Kind kind, R_C2D cam2d, rect viewport) {
  // 0. Allocate pass
  RN_Pass *pass = arena_push_array(__frame_arena, RN_Pass, 1);

  // 1. Hook it up to our g_list
  dll_push_back(__render_passes.first, __render_passes.last, pass);
  __render_passes.count += 1;

  // 2. Fill some info
  pass->kind = kind;
  pass->viewport = viewport;
  pass->cam2d = cam2d;

  return pass;
}

// TODO: Should become better
RN_Pass *rn_pass_front() {
  return __render_passes.first;
}

RN_Pass *rn_pass_back() {
  return __render_passes.last;
}

// FIXME FIXME FIXME FIXME
void rn_push_quad(RN_Pass *pass, R_Quad q) {
  if (q.tex.impl_state == 0) q.tex = white_tex;
  assert(pass->cmd_count < RN_MAX_CMD && "Didn't I say FIXME FIXME, make this static array a chunklist or some shit");
  pass->cmds[pass->cmd_count++] = q; 
}

void rn_imm_tri(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model) {

  u64 arena_prev_pos = arena_get_current_pos(__frame_arena); 
  buf sampler_name = arena_sprintf(__frame_arena, "u_tex");
  tri_bundle.textures[0] = (Ogl_Tex_Slot){ .name = sampler_name.data, .tex = white_tex,};

  Ogl_Buf vbo = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, verts, 1, sizeof(Tri_Vertex)*vert_count);
  tri_bundle.vbos[0].buffer = vbo;

  m4 proj = m4_persp(45.0, viewport.w/(f32)viewport.h, 0.1, 100);
  m4 view = m4_view(v3m(0,0,0), v3m(0,0,-1), v3m(0,1,0));
  m4 m = m4_mult(proj, m4_mult(view, model));
  // Apply a test model matrix
  ogl_buf_update(&tri_bundle.ubos[0].buffer, 0, &m, 1, sizeof(m4));

  // Set dynamically before drawcall currently
  tri_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
  tri_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;

  ogl_render_bundle_draw(&tri_bundle, prim, vert_count, 1);
  arena_reset_to_pos(__frame_arena, arena_prev_pos);
}
