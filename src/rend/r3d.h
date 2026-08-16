#ifndef _R3D_H__
#define _R3D_H__

#include "base/base_inc.h"
#include "frz/frz.h"
#include "ogl.h"

typedef FRZ_Vertex Tri_Vertex;

void rn_imm_cube(rect viewport, Ogl_Prim_Type, m4 *mvp, color c);
void rn_imm_verts(rect viewport, FRZ_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 *mvp);

// TODO: Maybe delete from here??
void r3d_try_load_shaders();


#endif
