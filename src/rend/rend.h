#ifndef _REND2D_H__
#define _REND2D_H__

// TODO: Texture handling is atrocious, make it possible (also Ogl side) to assign c-style texture sampler arrays to a slot
// TODO: Look at the batch fragment shader todos.. BEWARE!!

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

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

  Ogl_Tex *tex;
} R_Quad;

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

// Quad chunked list (optional but useful for big batches)
typedef struct R_Quad_Chunk_Node R_Quad_Chunk_Node;
struct R_Quad_Chunk_Node {
  R_Quad *quads;
  s64 count;
  s64 cap;

  R_Quad_Chunk_Node *next;
  R_Quad_Chunk_Node *prev;
};
typedef R_Quad_Chunk_Node R_Quad_Array;

typedef struct {
  R_Quad_Chunk_Node *first;
  R_Quad_Chunk_Node *last;

  s64 node_count;
  s64 quad_count;
} R_Quad_Chunk_List ;

typedef enum {
  RN_PASS_KIND_2D,
  RN_PASS_KIND_3D, // TBA
} RN_Pass_Kind;

typedef struct RN_Pass RN_Pass;
struct RN_Pass {
#define RN_MAX_CMD 256
  R_Quad_Chunk_List quads;
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

void rn_begin(Arena *arena, rect dummy_viewport);
RN_Pass *rn_pass_front();
RN_Pass *rn_pass_back();
void rn_flush_all();
RN_Pass *rn_push_pass(RN_Pass_Kind kind, R_C2D cam2d, rect viewport);
void rn_push_quad(RN_Pass *pass, R_Quad q);

// Maybe delete from here??
void r3d_load_shaders();


#endif
