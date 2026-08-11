#ifndef ASSET_MGR_H__
#define ASSET_MGR_H__
#include "base/base_inc.h"
#include "rend/rend_inc.h"
// @FIXME: Currently there is no asset deletion!

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

typedef struct Asset_Mgr Asset_Mgr;

typedef struct {
  Tex_Node_Hash_Slot *slots;
  s64 slot_count;

  Ogl_Tex default_value;

  Asset_Mgr *parent;
} Tex_Mgr;

struct Asset_Mgr {
  Arena *arena;
  Arena *tarena;
  Tex_Mgr tm;
};

#ifndef ASSET_MGR_IMPLEMENTATION

Asset_Handle am_get(Asset_Id id);
Asset_Id am_load_from_data(str8 asset_path, str8 asset_data);
void am_init(Arena *arena, Arena *tarena);
Asset_Id asset_id_from_path(str8 path);

#else

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static Asset_Mgr g_am = {};

///////////////////////////////////////
// Asset utilities
///////////////////////////////////////

void am_init(Arena *arena, Arena *tarena) {
  g_am.arena = arena;
  g_am.tarena = tarena;

  // Tex_Mgr initialization
  g_am.tm.default_value = ogl_tex_make((u8[]){255,255,255,255}, 
      1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});
  g_am.tm.slot_count = 32;
  g_am.tm.slots = arena_push_array(g_am.arena, Tex_Node_Hash_Slot, g_am.tm.slot_count); 
  g_am.tm.parent = &g_am;

}

Asset_Kind asset_kind_from_path(str8 path) {
  return (path.count < 3) ?  ASSET_KIND_TEX :
   path.data[path.count-1] + path.data[path.count-2] + path.data[path.count-3]; // look at enum Asset_Kind
}

Asset_Id asset_id_from_path(str8 path) {
  return (Asset_Id){djb2_buf(path.data, path.count), asset_kind_from_path(path)};
}

///////////////////////////////////////
// Texture Manager (use this as the macro base all managers should be like this ok?
///////////////////////////////////////

// TODO: Check if its a nil id no need for a full lookup for garbo stuff
Ogl_Tex *tex_mgr_find(Tex_Mgr *mgr, Asset_Id id) {
  s64 slot_idx = (id.id % mgr->slot_count);
  Tex_Node_Hash_Slot *slot = &mgr->slots[slot_idx];
  for (Tex_Node *node = slot->first; node != nullptr; node=node->next) {
    if (node->id.id == id.id) {
      return &(node->data); // So nice to have stable pointers :)
    }
  }
  return nullptr;
}

Ogl_Tex *tex_mgr_get(Tex_Mgr *mgr, Asset_Id id) {
  Ogl_Tex *tex = tex_mgr_find(mgr, id);
  return (tex) ? tex : &(mgr->default_value);
}

Asset_Id tex_mgr_load_from_data(Tex_Mgr *mgr, str8 asset_path, str8 asset_data) { // maybe pass this as argument we got from am_load_from_data(..)
  Asset_Id id = asset_id_from_path(asset_path); 

  Ogl_Tex tex = {};
  if (asset_data.count > 0) {
    // 0. Load via stb_image
    stbi_set_flip_vertically_on_load(true);
    s32 width, height, nr_channels;
    u8 *px_data = stbi_load_from_memory((u8*)asset_data.data, asset_data.count, 
        &width, &height, &nr_channels, STBI_rgb_alpha);

    // 1. Make the (internal) Ogl_Tex
    tex = ogl_tex_make(px_data, width, height, 
        OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  } else {
    // In case we provide empty data, the user can make an ogl_tex
    tex = (Ogl_Tex){};
  }
  

  // 2. Make a node and hook to hasmap
  Tex_Node *node = arena_push_array(mgr->parent->arena, Tex_Node, 1);
  node->data = tex;
  node->id = id;
  s64 slot_idx = (node->id.id % mgr->slot_count);
  dll_push_back(mgr->slots[slot_idx].first, mgr->slots[slot_idx].last, node);

  // 3. Return the id
  return id;
}


///////////////////////////////////////
// _Actual_ API
///////////////////////////////////////

Asset_Handle am_get(Asset_Id id) {
  switch(id.kind) {
    case ASSET_KIND_TEX:
      return (Asset_Handle)tex_mgr_get(&g_am.tm, id);
    case ASSET_KIND_FONT:
    case ASSET_KIND_AUDIO:
    case ASSET_KIND_MODEL:
    default:
      return (Asset_Handle){};
  }
}

Asset_Id am_load_from_data(str8 asset_path, str8 asset_data) {
  Asset_Id id = asset_id_from_path(asset_path);

  switch(id.kind) {
    case ASSET_KIND_TEX:
      return tex_mgr_load_from_data(&g_am.tm, asset_path, asset_data);
    case ASSET_KIND_FONT:
    case ASSET_KIND_AUDIO:
    case ASSET_KIND_MODEL:
    default:
      printf("YOYOYOYOOYO\n");
      return (Asset_Id){};
  }
}

// TODO am_load(...) via _Generic

#endif


#endif
