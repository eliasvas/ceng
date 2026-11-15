#ifndef R3DBG_H__
#define R3DBG_H__

#include "base/base_inc.h"

typedef struct {
  v3 p;
  color c;
} D_Vertex; 

#define VERTEX_CHUNK_COUNT 256
typedef struct Vertex_Chunk_Node Vertex_Chunk_Node;
struct Vertex_Chunk_Node {
  D_Vertex *verts;
  s64 count;
  s64 cap;

  Vertex_Chunk_Node *next;
  Vertex_Chunk_Node *prev;
};

typedef struct Vertex_Chunk_List {
  Vertex_Chunk_Node *first;
  Vertex_Chunk_Node *last;


  s64 vertex_count;
  s64 chunk_count;
} Vertex_Chunk_List;

// TODO: Lines are bad right now.. fix dis shit
typedef struct {
  Arena *arena;

  Vertex_Chunk_List tri_list;
  Vertex_Chunk_List line_list;
} R3DBG_Ctx;


void r3dbg_setup(Arena *arena);
R3DBG_Ctx* r3dbg_end();
void r3dbg_tri(v3 p0, v3 p1, v3 p2, color c0, color c1, color c2);
//void r3d_dbg_cube(m4 model);
//void r3d_dbg_frustum(m4 model);
//void r3d_dbg_line(v3);



#endif
