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
  v3 pos;
  v3 norm;
  v4 tangent;

  v2 tc_0;
  v2 tc_1;
  v2 tc_2;
  v2 tc_3;

  v4 col_0;
  v4 col_1;
  v4 col_2;
  v4 col_3;

  v4 joint_0;
  v4 joint_1;
  v4 joint_2;
  v4 joint_3;

  v4 weight_0;
  v4 weight_1;
  v4 weight_2;
  v4 weight_3;

} Uber_Vertex;

typedef struct {
  u64 id;
  Asset_Kind kind;
} Asset_Id;


typedef struct {
  Asset_Id tex_asset_id;
  s32 tc_idx; // e.g tc_0 or tc_1
  b32 active;
} Material_Tex; 

typedef struct {
  v4 base_color_factor;
  Material_Tex base_tex;

  f32 metallic_factor;
  f32 roughness_factor;
  Material_Tex metallic_roughness_tex;

  Material_Tex normal_tex;
  Material_Tex occlusion_tex;

  v3 emissive_factor;
  Material_Tex emissive_tex;

} Material_Info;

typedef struct {
  Ogl_Buf vbo;
  Ogl_Buf ibo;
  Ogl_Prim_Type type;
  Material_Info material;
  m4 model;
  v3 t;
  quat r;
  v3 s;
} Mesh_Primitive_Info;

typedef struct {
  Mesh_Primitive_Info *prims;
  s64 prim_count;
} Mesh_Info;

typedef enum {
  MESH_ANIMATION_KIND_TRANSLATION,
  MESH_ANIMATION_KIND_ROTATION,
  MESH_ANIMATION_KIND_SCALE,
} Mesh_Animation_Kind;

typedef enum {
  MESH_ANIMATION_INTERP_TYPE_LINEAR,
  MESH_ANIMATION_INTERP_TYPE_STEP,
  MESH_ANIMATION_INTERP_TYPE_CUBIC_SPLINE,
} Mesh_Animation_Interp_Type;

typedef struct {
  s32 mesh_idx;
  f32 *kf_timestamps;
  f32 *values;
  s32 kf_count;
  f32 max_duration;

  Mesh_Animation_Interp_Type type;
  Mesh_Animation_Kind kind;
} Mesh_Animation;

typedef struct Model_Info {
  Tri_Vertex *verts;
  s64 vert_count;

  Mesh_Info *meshes;
  s64 mesh_count;

  Mesh_Animation *animations;
  s64 animation_count;

  Asset_Id tex_id;
} Model_Info;
struct Ogl_Tex;

typedef struct Asset_Node Asset_Node;
struct Asset_Node {
  //union {
    Ogl_Tex tex;
    Model_Info model;
    // Font font
    // Render_Bundle?
  //};

  Asset_Id id;
  Asset_Node *next;
  Asset_Node *prev;
};



typedef struct {
  struct Asset_Node *first;
  struct Asset_Node *last;
} Asset_Node_Hash_Slot;

struct Asset_Cache;
typedef struct {
  Asset_Node_Hash_Slot *slots;
  s64 slot_count;
  Asset_Node default_value;
  struct Asset_Mgr *parent;
} Asset_Cache;


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

ASSET_TYPE_DEF(Model, Model_Info);

///////////////////////////////
// Asset_Mgr types
///////////////////////////////

typedef struct Asset_Mgr Asset_Mgr;
struct Asset_Mgr {
  Arena *arena;
  Arena *tarena;

  Asset_Cache *cache;
};

#endif
