#include "rend.h"
// maybe add an ifdef and make it configurable or something?
// TODO: Support for another bundle, BLURS!

// HMMMMMMM
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
out vec2 f_tc;

void main() { 
	gl_Position = view_proj * vec4(pos, 1.0);
  f_color = color;
  f_tc = tc;
}
)";

const char* tri_fs= R"(#version 300 es
precision highp float;
layout(location = 0) out vec4 out_color;

in vec2 f_tc;
in vec4 f_color;
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

static b32 r_cam_eq(R_Cam a, R_Cam b) {
  return (
      v2_eq(a.origin, b.origin) &&
      v2_eq(a.offset, b.offset) &&
      equalf(a.zoom, b.zoom, 0.001) &&
      equalf(a.rot_deg, b.rot_deg, 0.001)
      );
}
static m4 r_cam_make_view_mat(R_Cam *cam) {
  m4 rot = m4_rotate(cam->rot_deg, v3m(0,0,1));
  return m4_mult(m4_translate(v3m(cam->offset.x, cam->offset.y, 0)),m4_mult(rot,m4_mult(m4_scale(v3m(cam->zoom, cam->zoom,0)), m4_translate(v3m(-cam->origin.x, -cam->origin.y,0)))));
}

// Maybe the R_Quad should be passed by pointer, it might be YUGE!
static void rend_quad_chunk_list_push(Arena *arena, R_Quad_Chunk_List* list, u64 cap, R_Quad quad) {
  R_Quad_Chunk_Node *node = list->first;
  if (node == nullptr || node->count >= node->cap) {
    node = arena_push_struct(arena, R_Quad_Chunk_Node);
    node->arr = arena_push_array(arena, R_Quad, cap);

    node->cap = cap;
    sll_queue_push(list->first, list->last, node);
    list->node_count+=1;
  }
  node->arr[node->count] = quad;
  node->count+=1;
  list->quad_count+=1;
}

static R_Quad_Array rend_quad_chunk_list_to_array(Arena *arena, R_Quad_Chunk_List *list) {
  R_Quad_Array qa = {};

  qa.count = list->quad_count;
  qa.arr = arena_push_array(arena, R_Quad, qa.count);
  s64 itr = 0;
  for (R_Quad_Chunk_Node *node = list->first; node != nullptr; node = node->next) {
    M_COPY(&qa.arr[itr], node->arr, sizeof(R_Quad)*node->count);
    itr += node->count;
    assert(itr <= qa.count);
  }
  assert(itr == qa.count);

  return qa;
}

// TODO: We could save the prev arena offset and reset to that, no arena_pop nonsense!
static void r_flush(R2D *rend, Batch_Vertex *vertices, u64 count) {
  u64 arena_prev_pos = arena_get_current_pos(rend->arena); 
  buf sampler_name = arena_sprintf(rend->arena, "u_tex");
  batch_bundle.textures[0] = (Ogl_Tex_Slot){ .name = sampler_name.data, .tex = rend->gtex,};

  ogl_buf_update(&batch_bundle.vbos[0].buffer, 0, vertices, count, sizeof(Batch_Vertex));
  ogl_render_bundle_draw(&batch_bundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, count);
  arena_reset_to_pos(rend->arena, arena_prev_pos);
}

R2D* r_begin(Arena *arena, R_Cam *cam, rect viewport, rect scissor) {
  m4 proj = m4_ortho(0,viewport.w,0,viewport.h,-1,1);
  m4 view = r_cam_make_view_mat(cam);
  m4 m = m4_mult(proj, view);

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
      .ubos = {
        [0] = { .name = "BatchUbo", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, (m4[]) { m }, 1, sizeof(m4)), .start_offset = 0, .size = sizeof(m4) },
      },
      //.rt = ogl_render_target_make(screen_dim.x, screen_dim.y, 2, OGL_TEX_FORMAT_RGBA8U, true),
      .dyn_state = (Ogl_Dyn_State){
        .viewport = viewport,
        .scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR,
      }
    };
    white_tex = ogl_tex_make((u8[]){255,255,255,255}, 1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  }

  if (tri_bundle.sp.impl_state == 0) {
    tri_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(batch_vs, batch_fs),
      .vbos = {
        [0] = {
          // the vertex buffer for this should probably be made after r_end has been called
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, nullptr, REND_MAX_INSTANCES, sizeof(Tri_Vertex)),
          .vattribs = {
            [0] = { .location = 0, .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Tri_Vertex, pos), .stride = sizeof(Tri_Vertex), .instanced = false, },
            [1] = { .location = 1, .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Tri_Vertex, norm), .stride = sizeof(Tri_Vertex), .instanced = false,  },
            [2] = { .location = 2, .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Tri_Vertex, tc),    .stride = sizeof(Tri_Vertex), .instanced = false,  },
            [3] = { .location = 3, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Tri_Vertex, color),  .stride = sizeof(Tri_Vertex), .instanced = false,  },
          },
        },
      },
      .ubos = {
        [0] = { .name = "BatchUbo", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, (m4[]) { m }, 1, sizeof(m4)), .start_offset = 0, .size = sizeof(m4) },
      },
      //.rt = ogl_render_target_make(screen_dim.x, screen_dim.y, 2, OGL_TEX_FORMAT_RGBA8U, true),
      .dyn_state = (Ogl_Dyn_State){
        .viewport = viewport,
        .scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR,
      }
    };
  }


  batch_bundle.dyn_state.viewport = viewport;
  batch_bundle.dyn_state.scissor = scissor;

  ogl_buf_update(&batch_bundle.ubos[0].buffer, 0, &m, 1, sizeof(m4));


  R2D *rend = arena_push_array(arena, R2D, 1);
  rend->arena = arena;

  return rend;
}

void r_end(R2D *rend) {
  if (rend) {
    R_Quad_Array quads = rend_quad_chunk_list_to_array(rend->arena, &rend->list);
    if (quads.count) {
      Batch_Vertex *batch_vertices = arena_push_array(rend->arena, Batch_Vertex,REND_MAX_INSTANCES);

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

        if (vertex_idx >= REND_MAX_INSTANCES || quad_idx+1 >= quads.count) {
          r_flush(rend, batch_vertices, vertex_idx);
          vertex_idx = 0;
        }
      }
    }
  }
}

void r_push_quad(R2D *rend, R_Quad q) {
  // Then push it to the chunk list as normal
  rend_quad_chunk_list_push(rend->arena, &rend->list, 256, q);
}

void r_clear_cmds(R_Cmd_Chunk_List *cmd_list) {
  cmd_list->first = nullptr;
  cmd_list->last = nullptr;

  cmd_list->node_count = 0;
  cmd_list->cmd_count = 0;
}

// This is platform specific, thus is inside the implementation file
void r_render_cmds(Arena *arena, R_Cmd_Chunk_List *cmd_list) {
  // TODO: We could make the stacks here, so that we will be able to push/pop these properties
  // We will flush if a new camera/scissor/viewport is inserted and add it for subsequent calls
  R2D *rend = nullptr;  
  R_Cam c = {}; // Should get the default from the nil stack!! 
  rect viewport = {}; // Should get the default from the nil stack!!
  rect scissor = {}; // Should get the default from the nil stack!!
  for (R_Cmd_Chunk_Node *node = cmd_list->first; node != nullptr; node = node->next) {
    for (u64 idx = 0; idx < node->count; idx+=1) {
      R_Cmd cmd = node->arr[idx];
      switch (cmd.kind) {
        case R_CMD_KIND_SET_VIEWPORT: 
          if (!rect_equals(viewport, cmd.r)) {
            r_end(rend);
            viewport = cmd.r;
            rend = r_begin(arena, &c, viewport, scissor);
          }
          break;
        case R_CMD_KIND_SET_SCISSOR:
          if (!rect_equals(scissor, cmd.r)) {
            r_end(rend);
            scissor = cmd.r;
            rend = r_begin(arena, &c, viewport, scissor);
          }
          break;
        case R_CMD_KIND_SET_CAMERA: 
          if (!r_cam_eq(c ,cmd.c)) {
            r_end(rend);
            c = cmd.c;
            rend = r_begin(arena, &c, viewport, scissor);
          }
          break;
        case R_CMD_KIND_ADD_QUAD: 
          if (cmd.q.tex.impl_state == 0) {
            rend->gtex = white_tex;
            cmd.q.tex = white_tex;
          }
          if (rend->gtex.impl_state != 0 && cmd.q.tex.impl_state != rend->gtex.impl_state) {
            r_end(rend);
            rend = r_begin(arena, &c, viewport, scissor);
          }
          rend->gtex = cmd.q.tex;
          r_push_quad(rend, cmd.q);
          break;
        default:
          break;
      }
    }
  }
  r_end(rend);
  r_clear_cmds(cmd_list);
}

void r_push_cmd(Arena *arena, R_Cmd_Chunk_List *cmd_list, R_Cmd cmd, u64 cap) {
  R_Cmd_Chunk_Node *node = cmd_list->first;
  if (node == nullptr || node->count >= node->cap) {
    node = arena_push_struct(arena, R_Cmd_Chunk_Node);
    node->arr = arena_push_array(arena, R_Cmd, cap);
    node->cap = cap;

    sll_queue_push(cmd_list->first, cmd_list->last, node);
    cmd_list->node_count+=1;
  }
  node->arr[node->count] = cmd;
  node->count+=1;
  cmd_list->cmd_count+=1;
}

