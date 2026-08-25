#include "asset/asset_mgr.h"
#include "core/core_inc.h"

Model_Mgr* model_mgr_init(Asset_Mgr *parent) {
  Model_Mgr *mm = arena_push_array(parent->arena, Model_Mgr, 1);


  Gltf_Info info = gltf_load(parent->tarena, (str8){},STR8(test_json_str, cstr_count(test_json_str)));
  s64 vcount = 0;
  Tri_Vertex* verts = gltf_to_basic_mesh_bundle(parent->arena, info, &vcount);

  mm->default_value = (Model_Info) {
    .verts = verts,
    .vert_count = vcount,
  };

  mm->slot_count = 32;
  mm->slots = arena_push_array(parent->arena, Model_Node_Hash_Slot, mm->slot_count); 
  mm->parent = parent;

  return mm;
}

Model_Info *model_mgr_find(Model_Mgr *mgr, Asset_Id id) {
  s64 slot_idx = (id.id % mgr->slot_count);
  Model_Node_Hash_Slot *slot = &mgr->slots[slot_idx];
  for (Model_Node *node = slot->first; node != nullptr; node=node->next) {
    if (node->id.id == id.id) {
      return &(node->data); // So nice to have stable pointers :)
    }
  }
  return nullptr;
}

Model_Info *model_mgr_get(Model_Mgr *mgr, Asset_Id id) {
  Model_Info *tex = model_mgr_find(mgr, id);
  return (tex) ? tex : &(mgr->default_value);
}

Asset_Id model_mgr_load_from_fullpath(Model_Mgr *mgr, str8 asset_fullpath) { 
  char* path = cstr_from_str8(mgr->parent->tarena, asset_fullpath);
  u32 count = 0;
  u8 *data = (u8*)read_whole_file_binary(path, &count);
  str8 asset_data = STR8(data, count);

  //str8 file = str8_extract_filename(asset_fullpath);
  return model_mgr_load_from_data(mgr, asset_fullpath, asset_data);
}

Asset_Id model_mgr_load_from_data(Model_Mgr *mgr, str8 asset_fullpath, str8 asset_data) { 
  str8 dir = str8_extract_path(asset_fullpath);
  str8 file = str8_extract_filename(asset_fullpath);
  Asset_Id id = asset_id_from_path(file); 
  Gltf_Info info = gltf_load(mgr->parent->tarena, dir, asset_data);
  s64 vcount = 0;
  Tri_Vertex* verts = gltf_to_basic_mesh_bundle(mgr->parent->arena, info, &vcount);

  Model_Info data = (Model_Info) {
    .verts = verts,
    .vert_count = vcount,
    .tex_id = (info.image_count) ? info.images[0].id : (Asset_Id){0, ASSET_KIND_TEX},
  };


  // 2. Make a node and hook to hasmap
  Model_Node *node = arena_push_array(mgr->parent->arena, Model_Node, 1);
  node->data = data;
  node->id = id;
  s64 slot_idx = (node->id.id % mgr->slot_count);
  dll_push_back(mgr->slots[slot_idx].first, mgr->slots[slot_idx].last, node);

  // 3. Return the id
  return id;
}
