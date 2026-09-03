#ifndef ASSET_MGR_H__
#define ASSET_MGR_H__

#include "asset_types.h"

extern Asset_Mgr g_am;

static Asset_Kind asset_kind_from_path(str8 path) {
  if (path.data[path.count-1] + path.data[path.count-2] + path.data[path.count-3] == 'j'+'p'+'g')return ASSET_KIND_TEX;
  return (path.count < 3) ?  ASSET_KIND_TEX :
   path.data[path.count-1] + path.data[path.count-2] + path.data[path.count-3]; // look at enum Asset_Kind
}

static Asset_Id asset_id_from_path(str8 path) {
  return (Asset_Id){djb2_buf(path.data, path.count), asset_kind_from_path(path)};
}

Asset_Node *asset_cache_get(Asset_Cache *mgr, Asset_Id id);
static Asset_Handle am_get(Asset_Id id) {
    return (Asset_Handle)asset_cache_get(g_am.cache, id);
}
// This is a great help actually!
#define AM_GET(id, field) (&((Asset_Node*)am_get(id))->field)

Asset_Id asset_cache_load_from_data(Asset_Cache *mgr, str8 asset_fullpath, str8 asset_data);
static Asset_Id am_load_from_data(str8 asset_path, str8 asset_data) {
  //Asset_Id id = asset_id_from_path(asset_path);
  return asset_cache_load_from_data(g_am.cache, asset_path, asset_data);
}

Asset_Id asset_cache_load_from_fullpath(Asset_Cache *mgr, str8 asset_fullpath);
static Asset_Id am_load_from_fullpath(str8 asset_fullpath) {
  //str8 file = str8_extract_filename(asset_path);
  //Asset_Id id = asset_id_from_path(file);
  //assert(id.kind == ASSET_KIND_MODEL);

  return asset_cache_load_from_fullpath(g_am.cache, asset_fullpath);
}

Asset_Cache* asset_cache_init(Asset_Mgr *parent);
static void am_init(Arena *arena, Arena *tarena) {
  g_am.arena = arena;
  g_am.tarena = tarena;

  // Initialize texture manager
  g_am.cache = asset_cache_init(&g_am);
}


#endif
