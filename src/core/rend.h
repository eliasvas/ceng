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

typedef struct R_Quad_Array R_Quad_Array;
struct R_Quad_Array {
  R_Quad *arr;
  s64 count;
};


#if 0
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


typedef struct {
  R_Quad_Chunk_List list;
  Arena *arena;
  Ogl_Tex gtex;

  rect viewport;
  rect scissor;
  union {
    R_C2D cam2d;
    R_C3D cam3d;
  };
  R_Cam_Mode c_mode;
} R2D;
#endif

typedef enum {
  R_CAM_MODE_2D = 0,
  R_CAM_MODE_3D = 1,
} R_Cam_Mode;

// Camera2D
typedef struct {
  v2 origin;
  v2 offset;
  f32 zoom;
  // TODO: make this rad?
  f32 rot_deg;
} R_C2D;

// Camera3D
typedef struct {
  v3 pos;
  v3 pitch;
  v3 raw;
  v3 yaw;
  f32 zoom;
} R_C3D;

/////////////////////
// Low-Level (ogl-Based) API
/////////////////////

//R2D* r_begin2d(Arena *arena, R_C2D cam, rect viewport, rect clip_rect);
//void r_end(R2D *rend);
//void r_push_quad(R2D *rend, R_Quad q);

#if 0
// TODO: Maybe make these 'push' to a stack instead of 'set'
typedef enum {
  R_CMD_KIND_SET_VIEWPORT,
  R_CMD_KIND_SET_SCISSOR,
  R_CMD_KIND_SET_CAMERA_2D,
  R_CMD_KIND_SET_CAMERA_3D,
  R_CMD_KIND_ADD_QUAD,
} R_Cmd_Kind;

typedef struct {
  union {
    rect r;
    R_C2D c;
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
#endif

//void r_push_cmd(Arena *arena, R_Cmd_Chunk_List *cmd_list, R_Cmd cmd, u64 cap);
//void r_render_cmds(Arena *arena, R_Cmd_Chunk_List *cmd_list);
//void r_clear_cmds(R_Cmd_Chunk_List *cmd_list);


typedef enum {
  RN_PASS_KIND_2D,
  RN_PASS_KIND_3D, // TBA
} RN_Pass_Kind;

typedef struct RN_Pass RN_Pass;
struct RN_Pass {
#define RN_MAX_CMD 256
  // TODO: Chunked array as well right?
  R_Quad cmds[RN_MAX_CMD];
  s32 cmd_count;
  RN_Pass_Kind kind;

  R_C2D cam2d;
  rect viewport;


  RN_Pass *next;
  RN_Pass *prev;
};

typedef struct {
  RN_Pass *first;
  RN_Pass *last;
  s32 count;
} RN_Pass_List;

// NEW API
void rn_begin(Arena *arena, rect dummy_viewport);
RN_Pass *rn_pass_front();
RN_Pass *rn_pass_back();
void rn_flush_all();
RN_Pass *rn_push_pass(RN_Pass_Kind kind, R_C2D cam2d, rect viewport);
void rn_push_quad(RN_Pass *pass, R_Quad q);

#endif
