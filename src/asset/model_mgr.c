#include "asset/asset_mgr.h"
#include "core/core_inc.h"

Model_Mgr* model_mgr_init(Asset_Mgr *parent) {
  Model_Mgr *mm = arena_push_array(parent->arena, Model_Mgr, 1);


  Gltf_Info info = gltf_load(parent->tarena, STR8(test_json_str, cstr_count(test_json_str)));
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

Asset_Id model_mgr_load_from_data(Model_Mgr *mgr, str8 asset_path, str8 asset_data) { 
  Asset_Id id = asset_id_from_path(asset_path); 
  Gltf_Info info = gltf_load(mgr->parent->tarena, asset_data);
  s64 vcount = 0;
  Tri_Vertex* verts = gltf_to_basic_mesh_bundle(mgr->parent->arena, info, &vcount);

  Model_Info data = (Model_Info) {
    .verts = verts,
    .vert_count = vcount,
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
