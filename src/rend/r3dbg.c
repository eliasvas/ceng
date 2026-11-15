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

void r3dbg_tri(v3 p0, v3 p1, v3 p2, color c0, color c1, color c2) {
  r3dbg_add_vertex((D_Vertex){.p = p0, .c = c0});
  r3dbg_add_vertex((D_Vertex){.p = p1, .c = c1});
  r3dbg_add_vertex((D_Vertex){.p = p2, .c = c2});
}
