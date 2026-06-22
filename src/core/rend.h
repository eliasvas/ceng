#ifndef _REND2D_H__
#define _REND2D_H__

// TODO: Texture handling is atrocious, make it possible (also Ogl side) to assign c-style texture sampler arrays to a slot
// TODO: Look at the batch fragment shader todos.. BEWARE!!

#include "base/base_inc.h"
#include "frz/frz.h"
#include "core/ogl.h"

#define REND_MAX_INSTANCES 512
#define REND_MAX_TEXTURES 4

typedef struct {
  v4 src_rect;
  v4 dst_rect;
  v4 color;
  f32 rot_rad;
} Batch_Vertex;


typedef FRZ_Vertex Tri_Vertex;

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

void rn_begin(Arena *arena, rect dummy_viewport);
RN_Pass *rn_pass_front();
RN_Pass *rn_pass_back();
void rn_flush_all();
RN_Pass *rn_push_pass(RN_Pass_Kind kind, R_C2D cam2d, rect viewport);
void rn_push_quad(RN_Pass *pass, R_Quad q);
void rn_imm_tri(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model);

#endif
