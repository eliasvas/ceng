#ifndef ASSET_MGR_H__
#define ASSET_MGR_H__

#include "asset_types.h"

extern Asset_Mgr g_am;

static Asset_Kind asset_kind_from_path(str8 path) {
  return (path.count < 3) ?  ASSET_KIND_TEX :
   path.data[path.count-1] + path.data[path.count-2] + path.data[path.count-3]; // look at enum Asset_Kind
}

static Asset_Id asset_id_from_path(str8 path) {
  return (Asset_Id){djb2_buf(path.data, path.count), asset_kind_from_path(path)};
}

Ogl_Tex *tex_mgr_get(Tex_Mgr *mgr, Asset_Id id);
static Asset_Handle am_get(Asset_Id id) {
  switch(id.kind) {
    case ASSET_KIND_TEX:
      return (Asset_Handle)tex_mgr_get(g_am.tm, id);
    case ASSET_KIND_FONT:
    case ASSET_KIND_AUDIO:
    case ASSET_KIND_MODEL:
    default:
      printf("YOYOYOYOOYO\n");
      return (Asset_Handle){};
  }
}

Asset_Id tex_mgr_load_from_data(Tex_Mgr *mgr, str8 asset_path, str8 asset_data);
static Asset_Id am_load_from_data(str8 asset_path, str8 asset_data) {
  Asset_Id id = asset_id_from_path(asset_path);

  switch(id.kind) {
    case ASSET_KIND_TEX:
      return tex_mgr_load_from_data(g_am.tm, asset_path, asset_data);
    case ASSET_KIND_FONT:
    case ASSET_KIND_AUDIO:
    case ASSET_KIND_MODEL:
    default:
      printf("YOYOYOYOOYO\n");
      return (Asset_Id){};
  }
}

Tex_Mgr* tex_mgr_init(Asset_Mgr *parent);
static void am_init(Arena *arena, Arena *tarena) {
  g_am.arena = arena;
  g_am.tarena = tarena;

  // Initialize texture manager
  g_am.tm = tex_mgr_init(&g_am);
}


#endif
