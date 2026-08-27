#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

typedef FRZ_Vertex Tri_Vertex;

#include "asset/asset_mgr.h"
// FIXME: This should GO AWAY
void r3d_imm_change_tex(Ogl_Tex *tex);

void r3d_imm_cube(rect viewport, Ogl_Prim_Type, m4 *mvp, color c);
void r3d_imm_xy_face(rect viewport, Ogl_Prim_Type, m4 *mvp, color c);
void r3d_imm_verts(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 *mvp);

struct Model_Info;
void r3d_imm_model(rect viewport, struct Model_Info *info, m4 *mvp);

// TODO: Maybe delete from here??
void r3d_try_load_shaders();


#endif
