#include "rend/rend_inc.h"
#include "asset/asset_mgr.h"

// Maybe asset management should happen somewhere..
static Ogl_Render_Bundle tri_bundle = {};

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

void r3d_try_load_shaders() {
  m4 m = {};
  if (tri_bundle.sp.impl_state == 0) {
    tri_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(tri_vs, tri_fs),

      .textures[0] = (Ogl_Tex_Slot){ .name = "u_tex", .tex = *(Ogl_Tex*)am_get(asset_id_from_path(STR8L("white.png")))},
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

