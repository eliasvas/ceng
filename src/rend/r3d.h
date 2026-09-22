#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

typedef FRZ_Vertex Tri_Vertex;

#include "asset/asset_mgr.h"

void r3d_imm_cube(rect viewport, Ogl_Prim_Type, m4 *mvp, color c);
void r3d_imm_xy_face(rect viewport, Ogl_Prim_Type, m4 *mvp, color c);
void r3d_imm_verts(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 *mvp);

struct Model_Info;
m4 *calc_joint_mats_for_animation(Arena *arena, struct Model_Info *info, s32 mesh_idx, s32 anim_idx, f32 time_sec);
void r3d_imm_model(rect viewport, struct Model_Info *info, m4 vp, m4 model, v3 cam_pos, f32 time_sec);

// TODO: Maybe delete from here??
void r3d_try_load_shaders();


#endif
