#ifndef _REND2D_H__
#define _REND2D_H__

// TODO: Texture handling is atrocious, make it possible (also Ogl side) to assign c-style texture sampler arrays to a slot
// TODO: Look at the batch fragment shader todos.. BEWARE!!

#include "base/base_inc.h"
#include "core/ogl.h"

#define REND_MAX_INSTANCES 512
#define REND_MAX_TEXTURES 4

typedef struct {
  v4 src_rect;
  v4 dst_rect;
  v4 color;
  f32 rot_rad;
} Batch_Vertex;

typedef struct {
  v3 pos;
  v3 norm;
  v2 tc;
  v4 color;
} Tri_Vertex;

typedef struct {
  rect src_rect, dst_rect;
  color c;
  f32 rot_deg;

  // TODO: Maybe this isn't the best way to conduct business.. Ogl_Tex is just a view
  // TODO: Maybe should be (void*) ? This could be a primitive Asset type thing, just a (void*)
  Ogl_Tex tex;
} R_Quad;

typedef struct R_Quad_Chunk_Node R_Quad_Chunk_Node;
struct R_Quad_Chunk_Node {
  R_Quad_Chunk_Node *next;
  R_Quad *arr;
  s64 cap;
  s64 count;
};

typedef struct R_Quad_Chunk_List R_Quad_Chunk_List;
struct R_Quad_Chunk_List {
  // This is a Singly Linked-List Queue,
  // so we can access from the front i.e iterate array style
  R_Quad_Chunk_Node *first;
  R_Quad_Chunk_Node *last;

  s64 node_count;
  s64 quad_count;
};

typedef struct R_Quad_Array R_Quad_Array;
struct R_Quad_Array {
  R_Quad *arr;
  s64 count;
};

typedef struct {
  R_Quad_Chunk_List list;
  Arena *arena;
  Ogl_Tex gtex;
} R2D;

// TODO: make the trick with the macro for scale initialization
typedef struct {
  v2 origin;
  v2 offset;
  float zoom;
  // TODO: make this rad?
  float rot_deg;
} R_Cam;

/////////////////////
// Low-Level (ogl-Based) API
/////////////////////

R2D* r_begin(Arena *arena, R_Cam *cam, rect viewport, rect clip_rect);
void r_end(R2D *rend);
void r_push_quad(R2D *rend, R_Quad q);

/////////////////////
// High-Level (Command-Based) API
/////////////////////

// TODO: Maybe make these 'push' to a stack instead of 'set'
typedef enum {
  R_CMD_KIND_SET_VIEWPORT,
  R_CMD_KIND_SET_SCISSOR,
  R_CMD_KIND_SET_CAMERA,
  R_CMD_KIND_ADD_QUAD,
} R_Cmd_Kind;

typedef struct {
  union {
    rect r;
    R_Cam c;
    R_Quad q;
  };
  R_Cmd_Kind kind;
} R_Cmd;

typedef struct R_Cmd_Chunk_Node R_Cmd_Chunk_Node;
struct R_Cmd_Chunk_Node {
  R_Cmd_Chunk_Node *next;
  R_Cmd *arr;
  u64 cap;
  u64 count;
};

typedef struct R_Cmd_Chunk_List R_Cmd_Chunk_List;
struct R_Cmd_Chunk_List {
  R_Cmd_Chunk_Node *first;
  R_Cmd_Chunk_Node *last;

  u64 node_count;
  u64 cmd_count;
};

void r_push_cmd(Arena *arena, R_Cmd_Chunk_List *cmd_list, R_Cmd cmd, u64 cap);
void r_render_cmds(Arena *arena, R_Cmd_Chunk_List *cmd_list);
void r_clear_cmds(R_Cmd_Chunk_List *cmd_list);

#endif
