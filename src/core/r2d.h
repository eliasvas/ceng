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
  rect src_rect, dst_rect;
  color c;
  f32 rot_deg;

  // TODO: Maybe this isn't the best way to conduct business.. Ogl_Tex is just a view
  // TODO: Maybe should be (void*) ? This could be a primitive Asset type thing, just a (void*)
  Ogl_Tex tex;
} R2D_Quad;

typedef struct R2D_Quad_Chunk_Node R2D_Quad_Chunk_Node;
struct R2D_Quad_Chunk_Node {
  R2D_Quad_Chunk_Node *next;
  R2D_Quad *arr;
  s64 cap;
  s64 count;
};

typedef struct R2D_Quad_Chunk_List R2D_Quad_Chunk_List;
struct R2D_Quad_Chunk_List {
  // This is a Singly Linked-List Queue,
  // so we can access from the front i.e iterate array style
  R2D_Quad_Chunk_Node *first;
  R2D_Quad_Chunk_Node *last;

  s64 node_count;
  s64 quad_count;
};

typedef struct R2D_Quad_Array R2D_Quad_Array;
struct R2D_Quad_Array {
  R2D_Quad *arr;
  s64 count;
};

typedef struct {
  R2D_Quad_Chunk_List list;
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
} R2D_Cam;

/////////////////////
// Low-Level (ogl-Based) API
/////////////////////

R2D* r2d_begin(Arena *arena, R2D_Cam *cam, rect viewport, rect clip_rect);
void r2d_end(R2D *rend);
void r2d_push_quad(R2D *rend, R2D_Quad q);

/////////////////////
// High-Level (Command-Based) API
/////////////////////

// TODO: Maybe make these 'push' to a stack instead of 'set'
typedef enum {
  R2D_CMD_KIND_SET_VIEWPORT,
  R2D_CMD_KIND_SET_SCISSOR,
  R2D_CMD_KIND_SET_CAMERA,
  R2D_CMD_KIND_ADD_QUAD,
} R2D_Cmd_Kind;

typedef struct {
  union {
    rect r;
    R2D_Cam c;
    R2D_Quad q;
  };
  R2D_Cmd_Kind kind;
} R2D_Cmd;

typedef struct R2D_Cmd_Chunk_Node R2D_Cmd_Chunk_Node;
struct R2D_Cmd_Chunk_Node {
  R2D_Cmd_Chunk_Node *next;
  R2D_Cmd *arr;
  u64 cap;
  u64 count;
};

typedef struct R2D_Cmd_Chunk_List R2D_Cmd_Chunk_List;
struct R2D_Cmd_Chunk_List {
  R2D_Cmd_Chunk_Node *first;
  R2D_Cmd_Chunk_Node *last;

  u64 node_count;
  u64 cmd_count;
};

static void r2d_push_cmd(Arena *arena, R2D_Cmd_Chunk_List *cmd_list, R2D_Cmd cmd, u64 cap) {
  R2D_Cmd_Chunk_Node *node = cmd_list->first;
  if (node == nullptr || node->count >= node->cap) {
    node = arena_push_struct(arena, R2D_Cmd_Chunk_Node);
    node->arr = arena_push_array(arena, R2D_Cmd, cap);
    node->cap = cap;

    sll_queue_push(cmd_list->first, cmd_list->last, node);
    cmd_list->node_count+=1;
  }
  node->arr[node->count] = cmd;
  node->count+=1;
  cmd_list->cmd_count+=1;
}


void r2d_render_cmds(Arena *arena, R2D_Cmd_Chunk_List *cmd_list);
void r2d_clear_cmds(R2D_Cmd_Chunk_List *cmd_list);

#endif
