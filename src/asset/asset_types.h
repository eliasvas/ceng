#ifndef _ASSET_TYPES_H__
#define _ASSET_TYPES_H__

#include "base/base_inc.h"
#include "rend/rend_inc.h"

///////////////////////////////
// Asset types
///////////////////////////////

typedef void* Asset_Handle;
typedef enum {
  ASSET_KIND_TEX   = ('p'+'n'+'g'),
  ASSET_KIND_FONT  = ('t'+'t'+'f'),
  ASSET_KIND_AUDIO = ('m'+'p'+'3'),
  ASSET_KIND_MODEL = ('l'+'t'+'f'), // TODO: Maybe make gltf strictly .glb
} Asset_Kind;

typedef struct {
  u64 id;
  Asset_Kind kind;
} Asset_Id;

///////////////////////////////
// Tex_Mgr types
///////////////////////////////

#define ASSET_TYPE_DEF(Name, DataType) \
  typedef struct Name##_Node Name##_Node; \
  struct Name##_Node { \
    DataType data; \
    Asset_Id id; \
    Name##_Node *next; \
    Name##_Node *prev; \
  }; \
  typedef struct { \
    struct Name##_Node *first; \
    struct Name##_Node *last; \
  } Name##_Node_Hash_Slot; \
  struct Asset_Mgr; \
  typedef struct { \
    Name##_Node_Hash_Slot *slots; \
    s64 slot_count; \
    DataType default_value; \
    struct Asset_Mgr *parent; \
  } Name##_Mgr; \

struct Ogl_Tex;
ASSET_TYPE_DEF(Tex, Ogl_Tex);


typedef struct {
  Tri_Vertex *verts;
  s64 vert_count;

  Asset_Id tex_id;
} Model_Info;
ASSET_TYPE_DEF(Model, Model_Info);

///////////////////////////////
// Asset_Mgr types
///////////////////////////////

typedef struct Asset_Mgr Asset_Mgr;
struct Asset_Mgr {
  Arena *arena;
  Arena *tarena;

  Tex_Mgr *tm;
  Model_Mgr *mm;
};

#endif
