#include "r3dbg.h"
#include "rend/rend_inc.h"

static R3DBG_Ctx g_rdbg_ctx;

void r3dbg_setup(Arena *arena) {
  M_ZERO_STRUCT(&g_rdbg_ctx);
  g_rdbg_ctx.arena = arena;
}

R3DBG_Ctx* r3dbg_end() {
  return &g_rdbg_ctx;
}

void r3dbg_add_vertex(D_Vertex v) {
  Vertex_Chunk_List *tri_list = &g_rdbg_ctx.tri_list;

  // If we dont have enough space we must allocate a new chunk
  if (!tri_list->first || tri_list->first->count >= tri_list->first->cap) {
    Vertex_Chunk_Node *new_chunk = arena_push_array(g_rdbg_ctx.arena, Vertex_Chunk_Node, 1);
    new_chunk->count= 0;
    new_chunk->cap = VERTEX_CHUNK_COUNT;
    new_chunk->verts = arena_push_array_nz(g_rdbg_ctx.arena, D_Vertex, new_chunk->cap); 
    dll_push_back(tri_list->first, tri_list->last, new_chunk);
    tri_list->chunk_count+=1;
  }

  // Finally push the vertex to the non-filled chunk node
  Vertex_Chunk_Node *node = tri_list->last;
  node->verts[node->count++] = v;
  tri_list->vertex_count += 1;
}

void r3dbg_tri(D_Vertex v0, D_Vertex v1, D_Vertex v2) {
  r3dbg_add_vertex(v0);
  r3dbg_add_vertex(v1);
  r3dbg_add_vertex(v2);
}

void r3dbg_frustum(frustum f, color c) {
  D_Vertex frustum[8] = {
    (D_Vertex) {.p = f.points[0], .c = c},
    (D_Vertex) {.p = f.points[1], .c = c},
    (D_Vertex) {.p = f.points[2], .c = c},
    (D_Vertex) {.p = f.points[3], .c = c},
    (D_Vertex) {.p = f.points[4], .c = c},
    (D_Vertex) {.p = f.points[5], .c = c},
    (D_Vertex) {.p = f.points[6], .c = c},
    (D_Vertex) {.p = f.points[7], .c = c},
  };

  D_Vertex verts[] = {
    frustum[0], frustum[1], frustum[3],
    frustum[0], frustum[3], frustum[2],
    frustum[4], frustum[6], frustum[7],
    frustum[4], frustum[7], frustum[5],
    frustum[0], frustum[4], frustum[5],
    frustum[0], frustum[5], frustum[1],
    frustum[2], frustum[3], frustum[7],
    frustum[2], frustum[7], frustum[6],
    frustum[0], frustum[2], frustum[6],
    frustum[0], frustum[6], frustum[4],
    frustum[1], frustum[5], frustum[7],
    frustum[1], frustum[7], frustum[3],
  };

  for (s32 vidx= 0; vidx < (s32)ARRAY_COUNT(verts); vidx+=1) {
    r3dbg_add_vertex(verts[vidx]);
  }
}

// TODO: This is slow
void r3dbg_cube(m4 model, color c) { 
  D_Vertex cube_verts[36] = {
    (D_Vertex) {.p = v3m(-0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m( 0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f,-0.5f,-0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f, 0.5f), .c = c},
    (D_Vertex) {.p = v3m(-0.5f, 0.5f,-0.5f), .c = c},
  };

  for (s32 vidx = 0; vidx < (s32)ARRAY_COUNT(cube_verts); vidx+=1) {
    v4 v = m4_multv(model, v4_from_v3(cube_verts[vidx].p, 1));
    cube_verts[vidx].p = v3m(v.x/v.w, v.y/v.w, v.z/v.w);
    r3dbg_add_vertex(cube_verts[vidx]);
  }

}
