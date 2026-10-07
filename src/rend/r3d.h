#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

typedef FRZ_Vertex Tri_Vertex;

typedef enum {
  R3D_FLAG_IS_DEPTH_PASS = (0x1 << 0),
  R3D_FLAG_CLEAR_ALL     = (0x1 << 1),
} R3D_Ctx_Flags;


typedef struct {
  Arena *arena;
  rect viewport;
  m4 view;
  m4 proj;
  v3 cam_pos;
  v3 light_dir;

  Ogl_Render_Target *rt;
} R3D_Ctx;

R3D_Ctx* r3dc_begin(Arena *arena, rect viewport, m4 view, m4 proj, v3 cam_pos, v3 light_dir, R3D_Ctx_Flags flags);
void r3dc_end(R3D_Ctx *rctx);

void r3dc_imm_verts(R3D_Ctx *rctx, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model);
void r3dc_imm_xy_face(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);
void r3dc_imm_cube(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model);

struct Model_Info;
m4 *calc_joint_mats_for_animation(Arena *arena, struct Model_Info *info, s32 mesh_idx, s32 anim_idx, f32 time_sec);
void r3dc_imm_model(R3D_Ctx *rctx, struct Model_Info *info, m4 model, f32 time_sec, s32 anim_idx, color tint);

void r3d_try_load_shaders();

#endif
