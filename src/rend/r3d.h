#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "ogl.h"
#include "camera.h"
#include "csm_utils.h"

/*
This renderer works in 4 modes:
  0. Forward mode: Anywhere after the ogl_clear(..) you can issue immediate commands that will draw immediately, used for debug purposes
  1. Opaque mode: opaque objects can be drawn and they will go to the 'offscreen_rt' 
  2. Transparent mode: transparent objects will be drawn to 'transparent_rt'
  3. Shadow mode: Scene can be rendered in shadow mode, results go to 'shadow_rt'
*/

typedef enum {
  R3D_FWD, // forward
  R3D_OPQ, // Opaque
  R3D_TRN, // Transparent
  R3D_SHD, // Shadow
} R3D_Render_Mode ;

typedef struct {
  v3 pos;
  v3 norm;
  v4 tangent;

  v2 tc_0;
  v2 tc_1;
  v2 tc_2;
  v2 tc_3;

  v4 col_0;
  v4 col_1;
  v4 col_2;
  v4 col_3;

  iv4 joint_0;
  iv4 joint_1;
  iv4 joint_2;
  iv4 joint_3;

  v4 weight_0;
  v4 weight_1;
  v4 weight_2;
  v4 weight_3;

} Uber_Vertex;

typedef enum {
  R3D_FLAG_RENDER_GRID   = (0x1 << 0),
  R3D_FLAG_RENDER_MESHES = (0x1 << 1),
  R3D_FLAG_RENDER_BOXES  = (0x1 << 2),
  R3D_FLAG_CASCADE_VIS   = (0x1 << 3),
  R3D_FLAG_BVH_TEST      = (0x1 << 4),
  // ADD MORE
} R3D_Ctx_Flags;


typedef struct {
  Arena *arena;
  rect viewport;
  s32 cascade_idx;

  Camera cam;
  CSM_Opt opt;
  // Not really needed right?
  m4 lsms[CSM_MAX_CASCADES];

  R3D_Render_Mode rmode;
} R3D_Ctx;

void r3d_try_load_shaders(rect viewport);
void r3d_clear();

///////////////////////
// Drawing API
///////////////////////

R3D_Ctx* r3dc_begin(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Render_Mode rmode, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_forward(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_offscreen(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_transparent(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_depth_i(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, s32 cascade_idx, R3D_Ctx_Flags flags);
void r3dc_end(R3D_Ctx *rctx);


////////////////////////
// Immediate where?
////////////////////////
void r3dc_imm_verts(R3D_Ctx *rctx, Uber_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model);
void r3dc_imm_cube(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);
void r3dc_imm_xy_face(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);
struct Model_Info;
void r3dc_imm_model(R3D_Ctx *rctx, struct Model_Info *info, m4 model, f32 time_sec, s32 anim_idx, color tint);


// Just exposed for testing by game.c
void r3d_do_postproc(rect viewport);
v4 r3d_get_cascade_bounds(CSM_Opt *opt);

// Trash
v2 r3d_compute_depth_range(R3D_Ctx *rctx);
frustum r3d_compute_cascade_frustum(R3D_Ctx *rctx, v2 drange, s32 cascade_idx);
Ogl_RT* r3d_rt_from_render_mode(R3D_Render_Mode rmode);

#endif
