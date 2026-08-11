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

// TODO: make this into a macro we gonna have a lot of these bois
typedef struct Tex_Node Tex_Node;
struct Tex_Node {
  Ogl_Tex data;
  Asset_Id id;

  Tex_Node *next;
  Tex_Node *prev;
};

typedef struct {
  Tex_Node *first;
  Tex_Node *last;
} Tex_Node_Hash_Slot;

struct Asset_Mgr;
typedef struct {
  Tex_Node_Hash_Slot *slots;
  s64 slot_count;

  Ogl_Tex default_value;

  struct Asset_Mgr *parent;
} Tex_Mgr;

///////////////////////////////
// Asset_Mgr types
///////////////////////////////

typedef struct Asset_Mgr Asset_Mgr;
struct Asset_Mgr {
  Arena *arena;
  Arena *tarena;

  Tex_Mgr *tm;
};

#endif
