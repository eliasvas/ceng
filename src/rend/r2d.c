#include "rend/rend_inc.h"
#include "core/asset_mgr.h"

// Maybe asset management should happen somewhere..
static Ogl_Render_Bundle batch_bundle = {};

// TODO: Should these globals be here? what about asset management (textures/materials/meshes etc..)
static Arena *__frame_arena;
static R2D_Pass_List __render_passes;

////////////////////////////////////////////////
// Batch Shaders
////////////////////////////////////////////////

// SDF ref: https://iquilezles.org/articles/distfunctions/

const char* batch_vs = R"(#version 460 core
layout(location=0) in vec4 src_rect;
layout(location=1) in vec4 dst_rect;
layout(location=2) in vec4 v_color;
layout(location=3) in int tidx;
layout(location=4) in float v_rot_rad;
layout(location=5) in float corner_radius;
layout(location=6) in float softness;

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



mat2 rotate2d(float angle) { return mat2(cos(angle), -sin(angle), sin(angle),  cos(angle)); }

out Vertex_Data {
  vec4 color;
  vec2 tc;

  flat int tidx;
  vec2 dst_pos;
  vec2 dst_hdim;
  float corner_radius;
  float softness;
} vdata;

void main() { 
  vec2 pos_offset = dst_rect.xy;
  vec2 dim = dst_rect.zw;

  vec2 pos = vertices[gl_VertexID]; // [-0.5, 0.5] range
  pos *= dim; // scale
  vec2 local_pos = pos;

  pos = rotate2d(v_rot_rad) * pos; // rotate

  pos += dim/2.0; // += hdim so that its centered on upper-left corner
  pos += pos_offset; // translate
  

	gl_Position = view_proj * vec4(pos, 0.0, 1.0);

  vdata.color = v_color;

  vec2 uv = tex_coords[gl_VertexID];
  vdata.tc = src_rect.xy + uv * src_rect.zw;

  vdata.dst_pos = local_pos;
  vdata.dst_hdim = dim / 2.0;
  vdata.corner_radius = corner_radius;
  vdata.softness = softness;
  vdata.tidx = tidx;
}
)";

const char* batch_fs = R"(#version 460 core
layout(location = 0) out vec4 out_color;

float sd_rect(vec2 p, vec2 hdim, float corner_radius) {
   p = abs(p) - hdim + corner_radius;
   return length(max(p, 0.0)) + min(max(p.x, p.y), 0.0) - corner_radius;
}

uniform sampler2D u_tex[4];

in Vertex_Data {
  vec4 color;
  vec2 tc;

  flat int tidx;
  vec2 dst_pos;
  vec2 dst_hdim;
  float corner_radius;
  float softness;
} vdata;

void main() {
  ivec2 texture_size;
  vec2 tc;

  float d = sd_rect(vdata.dst_pos, vdata.dst_hdim, vdata.corner_radius);
  float edge = 1.0 - smoothstep(0.0, vdata.softness, d);

  texture_size = textureSize(u_tex[vdata.tidx], 0);
  tc = vdata.tc / vec2(texture_size.x, texture_size.y);
  out_color = edge * vdata.color * texture(u_tex[vdata.tidx], tc);
}

)";

/////////////////////
// Quad chunk list implementation
/////////////////////
void r_quad_chunk_list_add_quad(Arena *arena, R_Quad_Chunk_List *list, R_Quad quad) {
  // 0. If we don't have empty node in the list, allocate a new one and append to list
  if (list->first == nullptr || (list->last->count >= list->last->cap)) {
    s32 quads_per_chunk = 256;
    R_Quad_Chunk_Node *node = arena_push_array(arena, R_Quad_Chunk_Node, 1);
    node->cap = quads_per_chunk;
    node->count = 0;
    node->quads = arena_push_array(arena, R_Quad, quads_per_chunk);
    dll_push_back(list->first, list->last, node);
    list->node_count += 1;
  } 
  // 1. Regular insertion logic
  R_Quad_Chunk_Node *node = list->last;
  node->quads[node->count++] = quad;
  list->quad_count += 1;
}

R_Quad_Array r_quad_chunk_list_to_array(Arena *arena, R_Quad_Chunk_List *list) {
  R_Quad_Array qa = (R_Quad_Array) {
    .count = list->quad_count,
    .cap = list->quad_count,
  };

  s32 array_idx = 0;
  qa.quads = arena_push_array(arena, R_Quad, qa.cap);
  for (R_Quad_Chunk_Node *chunk_node = list->first; chunk_node != nullptr; chunk_node = chunk_node->next) {
    for (s32 quad_idx = 0; quad_idx < chunk_node->count; quad_idx+=1) {
      qa.quads[array_idx++] = chunk_node->quads[quad_idx];
    }
  }
  assert(array_idx == list->quad_count);

  return qa;
}


/////////////////////
// Actual Implementation
/////////////////////

void r2d_try_load_shaders() {
  m4 m = {};
  if (batch_bundle.sp.impl_state == 0) {
    batch_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make(batch_vs, batch_fs),
      .vbos = {
        [0] = {
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, nullptr, REND_MAX_INSTANCES, sizeof(Batch_Vertex)),
          .vattribs = {
            [0] = { .location = 0, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, src_rect),       .stride = sizeof(Batch_Vertex), .instanced = true, },
            [1] = { .location = 1, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, dst_rect),       .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [2] = { .location = 2, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Batch_Vertex, color),          .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [3] = { .location = 3, .type = OGL_DATA_TYPE_INT, .offset = offsetof(Batch_Vertex, tidx),        .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [4] = { .location = 4, .type = OGL_DATA_TYPE_FLOAT, .offset = offsetof(Batch_Vertex, rot_rad),        .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [5] = { .location = 5, .type = OGL_DATA_TYPE_FLOAT, .offset = offsetof(Batch_Vertex, corner_radius),  .stride = sizeof(Batch_Vertex),.instanced = true,  },
            [6] = { .location = 6, .type = OGL_DATA_TYPE_FLOAT, .offset = offsetof(Batch_Vertex, softness),       .stride = sizeof(Batch_Vertex),.instanced = true,  },
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

static m4 r_cam_make_view_mat(R_C2D *cam) {
  m4 rot = m4_rotate(cam->rot_deg, v3m(0,0,1));
  return m4_mult(m4_translate(v3m(cam->offset.x, cam->offset.y, 0)),m4_mult(rot,m4_mult(m4_scale(v3m(cam->zoom, cam->zoom,0)), m4_translate(v3m(-cam->origin.x, -cam->origin.y,0)))));
}

void r2d_begin(Arena *arena, rect dummy_viewport) {
  M_ZERO_STRUCT(&__render_passes);
  __frame_arena = arena;

  // Push a dummy render pass
  R_C2D dummy_cam = (R_C2D){
    .offset = v2m(0,0),
    .origin = v2m(0,0),
    .zoom = 1.0, 
    .rot_deg = 0
  };
  R2D_Pass *top_pass = r2d_push_pass(R2D_PASS_KIND_2D, dummy_cam, dummy_viewport);
  assert(top_pass);
  r2d_try_load_shaders();
  r3d_try_load_shaders();
}

void r2d_flush_verts(R2D_Pass *pass, Batch_Vertex *vertices, s32 vcount,  Ogl_Tex **tex_cache) {
  if (vcount <= 0) return;
  u64 arena_prev_pos = arena_get_current_pos(__frame_arena); 

  // set the textures
  for (s32 i = 0; i < OGL_MAX_ACTIVE_TEXTURES; i+=1) {
    if (tex_cache[i] != nullptr) {
      buf sampler_name = arena_sprintf(__frame_arena, "u_tex[%d]", i);
      batch_bundle.textures[i] = (Ogl_Tex_Slot){ .name = sampler_name.data, .tex = *tex_cache[i],};
    }
  }

  // set vertex buffer
  ogl_buf_update(&batch_bundle.vbos[0].buffer, 0, vertices, vcount, sizeof(Batch_Vertex));

  // set the ubo (currently only the VP matrix)
  m4 proj = m4_ortho(0,pass->viewport.w,0,pass->viewport.h,-1,1);
  m4 view = r_cam_make_view_mat(&pass->cam2d);
  m4 m = m4_mult(proj, view);
  ogl_buf_update(&batch_bundle.ubos[0].buffer, 0, &m, 1, sizeof(m4));

  // Set dynamically before drawcall currently
  batch_bundle.dyn_state.viewport = *(Ogl_rect *)&pass->viewport;
  batch_bundle.dyn_state.scissor = *(Ogl_rect *)&pass->viewport;

  ogl_render_bundle_draw(&batch_bundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, vcount);

  arena_reset_to_pos(__frame_arena, arena_prev_pos);


  //for (s32 i = 0; i < OGL_MAX_ACTIVE_TEXTURES; i+=1) { batch_bundle.textures[i] = (Ogl_Tex_Slot){}; }
  //M_ZERO(*tex_cache, sizeof(tex_cache[0]) * OGL_MAX_ACTIVE_TEXTURES);
}

void r2d_flush_all() {
  for (R2D_Pass *pass = __render_passes.last; pass != nullptr; pass = pass->prev) {
    R_Quad_Array quads = r_quad_chunk_list_to_array(__frame_arena, &pass->quads);
    Batch_Vertex *batch_vertices = arena_push_array(__frame_arena, Batch_Vertex,REND_MAX_INSTANCES);

    Ogl_Tex* tex_cache[OGL_MAX_ACTIVE_TEXTURES] = {};
    s64 vcount = 0;

    for (s64 quad_idx = 0; quad_idx < quads.count; quad_idx += 1) {
      R_Quad *q = &quads.quads[quad_idx];

      Batch_Vertex v = (Batch_Vertex){
        .src_rect = *(v4*)&q->src_rect,
        .dst_rect = *(v4*)&q->dst_rect,
        .color = q->c,
        .rot_rad = DEG2RAD(q->rot_deg),
        .corner_radius = q->corner_radius,
        .softness = q->softness,
      };

      // 0. Try to add texture to tex_cache - flush otherwise
      b32 tex_added = false;
      for (s32 i = 0; i < OGL_MAX_ACTIVE_TEXTURES; i += 1) {
        if (tex_cache[i] == nullptr || tex_cache[i] == q->tex) {
          tex_cache[i] = q->tex;
          v.tidx = i;
          tex_added = true;
          break;
        }
      }
      if (!tex_added) {
        r2d_flush_verts(pass, batch_vertices, vcount, tex_cache);
        vcount = 0;
        M_ZERO_ARRAY(tex_cache);
        tex_cache[0] = q->tex;
      }


      // 1. Check if instance array is full and flush if so
      b32 instance_array_full = (vcount == REND_MAX_INSTANCES);
      if (instance_array_full) {
        r2d_flush_verts(pass, batch_vertices, vcount, tex_cache);
        vcount = 0;
        M_ZERO_ARRAY(tex_cache);
        tex_cache[0] = q->tex;
        v.tidx = 0;
      }

      // 2. Add the vertex to instance array
      batch_vertices[vcount++] = v;

      // 3. Check if we are at the end of quads array - and flush if so
      b32 is_last_vertex = (quad_idx + 1 >= quads.count);
      if (is_last_vertex) {
        r2d_flush_verts(pass, batch_vertices, vcount, tex_cache);
      }
    }
  }

  // Cleanup render passes (They are now rendererd)
  //__render_passes.first = __render_passes.last = nullptr;
}

R2D_Pass *r2d_push_pass(R2D_Pass_Kind kind, R_C2D cam2d, rect viewport) {
  // 0. Allocate pass
  R2D_Pass *pass = arena_push_array(__frame_arena, R2D_Pass, 1);

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
R2D_Pass *r2d_pass_front() {
  return __render_passes.first;
}

R2D_Pass *r2d_pass_back() {
  return __render_passes.last;
}

void r2d_push_quad(R2D_Pass *pass, R_Quad q) {
  // white.png is just an invalid png name, which means that the default texture will be mapped (white)
  if (q.tex == nullptr) q.tex = ((Ogl_Tex*)am_get(asset_id_from_path(STR8L("white.png"))));
  r_quad_chunk_list_add_quad(__frame_arena, &pass->quads, q);
}

