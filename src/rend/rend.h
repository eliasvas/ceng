#ifndef _R2D_H__
#define _R2D_H__

// TODO: Texture handling is atrocious, make it possible (also Ogl side) to assign c-style texture sampler arrays to a slot
// TODO: Look at the batch fragment shader todos.. BEWARE!!

/*
 * General idea is to have low-level and high-level API, do fs-side clips
 * */
#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

#define REND_MAX_INSTANCES 512

typedef struct {
  v4 src_rect;
  v4 dst_rect;
  v4 color;
  u32 tidx; // texture index
  f32 rot_rad;
  f32 corner_radius;
  f32 softness;
} Batch_Vertex;


typedef struct {
  rect src_rect, dst_rect;
  color c;
  f32 rot_deg;
  f32 corner_radius;
  f32 softness;

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
} R_Quad_Chunk_List;

typedef enum {
  R2D_PASS_KIND_2D,
  R2D_PASS_KIND_3D, // TBA
} R2D_Pass_Kind;

typedef struct R2D_Pass R2D_Pass;
struct R2D_Pass {
#define R2D_MAX_CMD 256
  R_Quad_Chunk_List quads;
  s32 cmd_count;
  R2D_Pass_Kind kind;

  R_C2D cam2d;
  rect viewport;


  R2D_Pass *next;
  R2D_Pass *prev;
};

typedef struct {
  R2D_Pass *first;
  R2D_Pass *last;
  s32 count;
} R2D_Pass_List;

void r2d_begin(Arena *arena, rect dummy_viewport);
R2D_Pass *r2d_pass_front();
R2D_Pass *r2d_pass_back();
void r2d_flush_all();
R2D_Pass *r2d_push_pass(R2D_Pass_Kind kind, R_C2D cam2d, rect viewport);
void r2d_push_quad(R2D_Pass *pass, R_Quad q);

// Maybe delete from here??
void r3d_load_shaders();




#endif
