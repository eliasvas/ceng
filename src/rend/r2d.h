#ifndef _R2D_H__
#define _R2D_H__

#include "base/base_inc.h"
#include "ogl.h"
#include "asset/asset_id.h"

#define REND_MAX_INSTANCES 512

typedef struct {
  v4 src_rect;
  v4 dst_rect;
  v4 clip_rect;
  v4 color;
  s32 tidx; // texture index
  f32 rot_rad;
  f32 corner_radius;
  f32 softness;
} Batch_Vertex;


typedef struct {
  rect src_rect, dst_rect, clip_rect;
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

// Low-level API
void r2d_begin(Arena *arena, rect dummy_viewport);
R2D_Pass *r2d_pass_front();
R2D_Pass *r2d_pass_back();
void r2d_flush_all();
R2D_Pass *r2d_push_pass(R2D_Pass_Kind kind, R_C2D cam2d, rect viewport);
void r2d_push_quad(R2D_Pass *pass, R_Quad q);

// High-level helpers
// TODO: viewport is also part of R2D_Pass, do we really need it here?
// TODO: maybe all the sizing info could be in a struct, its getting pretty long!
void r2d_push_text(R2D_Pass *pass, Asset_Id font_id, rect viewport, rect clip_rect, str8 text, v2 pos, f32 scale, color col);


#endif
