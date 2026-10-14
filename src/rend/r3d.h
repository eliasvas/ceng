#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

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
  R3D_FLAG_CLEAR_ALL     = (0x1 << 0),
  // ADD MORE
} R3D_Ctx_Flags;


typedef struct {
  Arena *arena;
  rect viewport;
  m4 view;
  m4 proj;
  m4 lsms[4];
  v3 cam_pos;
  v3 light_dir;

  Ogl_Render_Target *rt;
} R3D_Ctx;

R3D_Ctx* r3dc_begin_color(Arena *arena, rect viewport, m4 view, m4 proj, v3 cam_pos, v3 light_dir, R3D_Ctx_Flags flags);
R3D_Ctx* r3dc_begin_depth_i(Arena *arena, rect viewport, m4 view, m4 proj, v3 cam_pos, v3 light_dir, s32 cascade_idx, R3D_Ctx_Flags flags);
void r3dc_end(R3D_Ctx *rctx);

void r3dc_imm_verts(R3D_Ctx *rctx, Uber_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model);
void r3dc_imm_cube(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);

void r3dc_imm_xy_face(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);
void r3dc_imm_frustum(R3D_Ctx *rctx, v3 eye, m4 view, m4 proj, Ogl_Prim_Type prim);

struct Model_Info;
m4 *calc_joint_mats_for_animation(Arena *arena, struct Model_Info *info, s32 mesh_idx, s32 anim_idx, f32 time_sec);
void r3dc_imm_model(R3D_Ctx *rctx, struct Model_Info *info, m4 model, f32 time_sec, s32 anim_idx, color tint);

void r3d_try_load_shaders(rect viewport);
void r3d_do_fxaa(rect viewport);

#endif
