#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"
#include "camera.h"

#define MAX_CASCADES 4

//typedef FRZ_Vertex Tri_Vertex;
//#define TRI_VERTEX_VARG(vt) vt.pos.x, vt.pos.y, vt.pos.z


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
  CSM_SPLIT_UNIFORM,
  CSM_SPLIT_LOGARITHMIC,
  CSM_SPLIT_PRACTICAL,
  CSM_SPLIT_MANUAL,
  CSM_SPLIT_SDSM, // SDSM
} CSM_Split_Policy;

typedef enum {
  CSM_FIT_TIGHT,
  CSM_FIT_APPLY_ZMULT,
  CSM_FIT_ISECT_BVH,
} CSM_Fitting_Flags;

typedef struct {

  // Fitting strategy
  f32 zmult;
  CSM_Fitting_Flags fflags;

  // Split strategy
  CSM_Split_Policy split_policy;
  union {
    f32 lambda;
    v4 bounds;
  };
  
  s32 cascade_count;
  v3 light_dir;
} CSM_Opt;

static CSM_Opt csm_opt_default() {
  CSM_Opt default_options = (CSM_Opt) {
    .zmult = 1.0,
    //.split_policy = CSM_SPLIT_MANUAL,
    //.split_policy = CSM_SPLIT_LOGARITHMIC,
    //.split_policy = CSM_SPLIT_UNIFORM,
    .split_policy = CSM_SPLIT_PRACTICAL,
    .lambda = 0.5,
    .fflags = CSM_FIT_APPLY_ZMULT | CSM_FIT_ISECT_BVH,
    .cascade_count = MAX_CASCADES,
    .light_dir = v3_norm(v3m(0.1,1,1)),
  };

  return default_options;
}

typedef enum {
  R3D_FLAG_RENDER_GRID   = (0x1 << 0),
  R3D_FLAG_RENDER_MESHES = (0x1 << 1),
  R3D_FLAG_RENDER_BOXES  = (0x1 << 2),
  R3D_FLAG_CASCADE_TEST  = (0x1 << 3),
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
  m4 lsms[MAX_CASCADES];

  Ogl_RT *rt;

} R3D_Ctx;

void r3d_clear();

R3D_Ctx* r3dc_begin_forward(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_offscreen(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_transparent(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_depth_i(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, s32 cascade_idx, R3D_Ctx_Flags flags);
void r3dc_end(R3D_Ctx *rctx);

void r3dc_imm_verts(R3D_Ctx *rctx, Uber_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model);
void r3dc_imm_cube(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);

void r3dc_imm_xy_face(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);
void r3dc_imm_frustum(R3D_Ctx *rctx, m4 view, m4 proj, color c, Ogl_Prim_Type prim);

struct Model_Info;
m4 *calc_joint_mats_for_animation(Arena *arena, struct Model_Info *info, s32 mesh_idx, s32 anim_idx, f32 time_sec);
void r3dc_imm_model(R3D_Ctx *rctx, struct Model_Info *info, m4 model, f32 time_sec, s32 anim_idx, color tint);

void r3d_try_load_shaders(rect viewport);
void r3d_do_postproc(rect viewport);

// Just exposed for testing by game.c
v4 r3d_get_cascade_bounds(CSM_Opt *opt);
color r3d_get_cascade_idx_color(s32 cascade_idx);
//m4 r3d_calc_lsm_i(R3D_Ctx *rctx, s32 index);

#endif
