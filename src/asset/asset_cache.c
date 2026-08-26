#include "asset/asset_mgr.h"
#include "core/core_inc.h"
// FIXME: put this in game_state
Asset_Mgr g_am = {};

extern u8* platform_img_to_raw(Arena *arena, str8 image_data_png, v2 *out_dim);


Asset_Cache* asset_cache_init(Asset_Mgr *parent) {
  Asset_Cache *ac = arena_push_array(parent->arena, Asset_Cache, 1);

  // Default texture
  ac->default_value.tex = ogl_tex_make((u8[]){255,255,255,255}, 
      1,1, OGL_TEX_FORMAT_RGBA8U, 
      (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT}
  );

  // Default model
  Gltf_Info info = gltf_load(parent->tarena, (str8){},STR8(test_json_str, cstr_count(test_json_str)));
  s64 vcount = 0;
  Tri_Vertex* verts = gltf_to_basic_mesh_bundle(parent->arena, info, &vcount);
  ac->default_value.model = (Model_Info) {
    .verts = verts,
    .vert_count = vcount,
  };

  ac->slot_count = 32;
  ac->slots = arena_push_array(parent->arena, Tex_Node_Hash_Slot, ac->slot_count); 
  ac->parent = parent;

  return ac;
}

// Asset_Node;

// TODO: Check if its a nil id no need for a full lookup for garbo stuff
Asset_Node *asset_cache_find(Asset_Cache *mgr, Asset_Id id) {
  s64 slot_idx = (id.id % mgr->slot_count);
  Asset_Node_Hash_Slot *slot = &mgr->slots[slot_idx];
  for (Asset_Node *node = slot->first; node != nullptr; node=node->next) {
    if (node->id.id == id.id) {
      return node; // So nice to have stable pointers :)
    }
  }
  return nullptr;
}

Asset_Node *asset_cache_get(Asset_Cache *mgr, Asset_Id id) {
  Asset_Node *node = asset_cache_find(mgr, id);
  return (node) ? node : &(mgr->default_value);
}

Asset_Id asset_cache_load_from_data(Asset_Cache *mgr, str8 asset_fullpath, str8 asset_data) { // maybe pass this as argument we got from am_load_from_data(..)
  str8 dir = str8_extract_path(asset_fullpath);
  str8 file = str8_extract_filename(asset_fullpath);
  printf("fullpath:%.*s\n", STR8_VARG(asset_fullpath));
  printf("filepath:%.*s\n", STR8_VARG(file));
  Asset_Id id = asset_id_from_path(file); 
  Asset_Node *node = arena_push_array(mgr->parent->arena, Asset_Node, 1);
  node->id = id;
  s64 slot_idx = (node->id.id % mgr->slot_count);

  switch(id.kind) {
    case ASSET_KIND_TEX:
      Ogl_Tex tex = {};
      if (asset_data.count > 0) {
        v2 img_dim = {};
        u8 *px_data = platform_img_to_raw(mgr->parent->tarena, asset_data, &img_dim);
        tex = ogl_tex_make(px_data, img_dim.x, img_dim.y, 
            OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
      } else {
        tex = (Ogl_Tex){};
      }
      node->tex = tex; // <------- Important part here! :)
      break;
    case ASSET_KIND_MODEL:
      Gltf_Info info = gltf_load(mgr->parent->tarena, dir, asset_data);
      s64 vcount = 0;
      Tri_Vertex* verts = gltf_to_basic_mesh_bundle(mgr->parent->arena, info, &vcount);

      Model_Info model = (Model_Info) {
        .verts = verts,
        .vert_count = vcount,
        .tex_id = (Asset_Id){0, ASSET_KIND_TEX},
      };
      if (info.image_count > 0 && info.texture_count >0 && info.material_count > 0) {
        model.tex_id = info.images[info.textures[info.materials[0].pbr.base_color_texture.index].image_idx].id;
      }
      node->model = model; // <------- Important part here! :)
      break;
    case ASSET_KIND_FONT:
    case ASSET_KIND_AUDIO:
    default:
      break;
  }

  dll_push_back(mgr->slots[slot_idx].first, mgr->slots[slot_idx].last, node);

  // 3. Return the id
  return id;
}

Asset_Id asset_cache_load_from_fullpath(Asset_Cache *mgr, str8 asset_fullpath) { 
  char* path = cstr_from_str8(mgr->parent->tarena, asset_fullpath);
  u32 count = 0;
  u8 *data = (u8*)read_whole_file_binary(path, &count);
  str8 asset_data = STR8(data, count);

  //str8 file = str8_extract_filename(asset_fullpath);
  return asset_cache_load_from_data(mgr, asset_fullpath, asset_data);
}
