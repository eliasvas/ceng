#ifndef ASSET_ID_H__
#define ASSET_ID_H__
#include "base/base_inc.h"

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

#endif
