#include "asset/asset_mgr.h"

// FIXME: We should put this somehow in Game_State to support hot reloading
// Maybe we could inject this in platform layer with macro
Asset_Mgr g_am = {};

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Tex_Mgr* tex_mgr_init(Asset_Mgr *parent) {
  Tex_Mgr *tm = arena_push_array(parent->arena, Tex_Mgr, 1);

  tm->default_value = ogl_tex_make((u8[]){255,255,255,255}, 
      1,1, OGL_TEX_FORMAT_RGBA8U, 
      (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT}
  );

  tm->slot_count = 32;
  tm->slots = arena_push_array(parent->arena, Tex_Node_Hash_Slot, tm->slot_count); 
  tm->parent = parent;

  return tm;
}

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
