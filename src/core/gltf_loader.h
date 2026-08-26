#ifndef GLTF_LOADER_H__
#define GLTF_LOADER_H__
#include "base/base_inc.h"
#include "core/json_util.h"
#include "core/base64.h"

// FIXME: embedded images don't currently work

// Ref: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

typedef enum {
  GLTF_ARRAY_BUFFER,         // vertex buffer
  GLTF_ELEMENT_ARRAY_BUFFER, // index buffer
} Gltf_Buffer_Kind;

//typedef struct { str8 data; s64 count; s64 offset; } Gltf_Buffer;
typedef struct {
  s32 buf_idx;
  s64 byte_offset;
  s64 byte_length;
  s64 byte_stride;
  Gltf_Buffer_Kind kind;
} Gltf_Buffer_View;

typedef struct {
  s32 bufv_idx;
  s64 byte_offset;
  s64 count;
  s32 bytes_per_elem;
  s32 comp_per_elem;
} Gltf_Accessor;

typedef struct {
  Asset_Id id;
} Gltf_Image;

typedef struct {
  s32 min_id;
  s32 mag_id;
  s32 wrap_s_id;
  s32 wrap_t_id;
} Gltf_Sampler;


typedef struct {
  s32 sampler_idx;
  s32 image_idx;
} Gltf_Texture;

typedef struct {
  v4 base_color_factor;
  f32 metallic_factor;
} Gltf_Material_PBR;

typedef struct {
  Gltf_Material_PBR pbr;
  v3 emissive_factor;
} Gltf_Material;

#define GLTF_PROPERTY_NOT_SPECIFIED (-1)
static b32 gltf_is_property_specified(s32 property_idx) {
  return (property_idx != GLTF_PROPERTY_NOT_SPECIFIED);
}

#define GLTF_MAX_ATTRIB_N 4
typedef struct {
  s32 pos_idx;
  s32 norm_idx;
  s32 tang_idx;
  // e.g TEXCOORD_0, TEXCOORD_1 etc.
  s32 texcoord_idx[GLTF_MAX_ATTRIB_N];
  s32 col_idx[GLTF_MAX_ATTRIB_N];
  s32 joints_idx[GLTF_MAX_ATTRIB_N];
  s32 weights_idx[GLTF_MAX_ATTRIB_N];
} Gltf_Attribs; 

typedef struct {
  Gltf_Attribs attribs;
  s32 indices_idx;
  s32 material_idx;
  s32 mode;
} Gltf_Prim;

typedef struct {
  Gltf_Prim *prims;
  s32 prim_count;
} Gltf_Mesh;


// Meshes are arrays of mesh primitives
typedef struct {
  Gltf_Mesh *meshes;
  s32 mesh_count;

  str8 *buffers;
  s32 buffer_count;

  Gltf_Buffer_View *buffer_views;
  s32 buffer_view_count;

  Gltf_Accessor *accessors;
  s32 accessor_count;

  Gltf_Image *images;
  s32 image_count;

  Gltf_Sampler *samplers;
  s32 sampler_count;

  Gltf_Texture *textures;
  s32 texture_count;

  Gltf_Material *materials;
  s32 material_count;

} Gltf_Info;



static Gltf_Prim default_prim = (Gltf_Prim) {
  .attribs = {
    .pos_idx = GLTF_PROPERTY_NOT_SPECIFIED,
    .norm_idx = GLTF_PROPERTY_NOT_SPECIFIED,
    .tang_idx = GLTF_PROPERTY_NOT_SPECIFIED,
    .texcoord_idx[0] = GLTF_PROPERTY_NOT_SPECIFIED,
    .texcoord_idx[1] = GLTF_PROPERTY_NOT_SPECIFIED,
    .texcoord_idx[2] = GLTF_PROPERTY_NOT_SPECIFIED,
    .texcoord_idx[3] = GLTF_PROPERTY_NOT_SPECIFIED,

    .col_idx[0] = GLTF_PROPERTY_NOT_SPECIFIED,
    .col_idx[1] = GLTF_PROPERTY_NOT_SPECIFIED,
    .col_idx[2] = GLTF_PROPERTY_NOT_SPECIFIED,
    .col_idx[3] = GLTF_PROPERTY_NOT_SPECIFIED,

    .joints_idx[0] = GLTF_PROPERTY_NOT_SPECIFIED,
    .joints_idx[1] = GLTF_PROPERTY_NOT_SPECIFIED,
    .joints_idx[2] = GLTF_PROPERTY_NOT_SPECIFIED,
    .joints_idx[3] = GLTF_PROPERTY_NOT_SPECIFIED,

    .weights_idx[0] = GLTF_PROPERTY_NOT_SPECIFIED,
    .weights_idx[1] = GLTF_PROPERTY_NOT_SPECIFIED,
    .weights_idx[2] = GLTF_PROPERTY_NOT_SPECIFIED,
    .weights_idx[3] = GLTF_PROPERTY_NOT_SPECIFIED,
  },
  .indices_idx = GLTF_PROPERTY_NOT_SPECIFIED,
  .material_idx = 0,
  .mode = 4,
};

static s32 ogl_prim_type_from_gltf_prim_mode(s32 mode) {
  switch(mode) {
    case 0:  return OGL_PRIM_TYPE_POINT;
    case 1:  return OGL_PRIM_TYPE_LINE;
    case 2:  return OGL_PRIM_TYPE_LINE_LOOP;
    case 3:  return OGL_PRIM_TYPE_LINE_STRIP;
    case 4:  return OGL_PRIM_TYPE_TRIANGLE;
    case 5:  return OGL_PRIM_TYPE_TRIANGLE_STRIP;
    case 6:  return OGL_PRIM_TYPE_TRIANGLE_FAN;
    default: return OGL_PRIM_TYPE_TRIANGLE;
  }
}

static Gltf_Buffer_Kind gltf_get_buffer_view_kind(s32 target) {
  if (target == 34962) return GLTF_ARRAY_BUFFER;
  if (target == 34963) return GLTF_ELEMENT_ARRAY_BUFFER;
  return GLTF_ARRAY_BUFFER;
}

static s32 gltf_byte_count_from_comp_type(s32 comp_type) {
  if (comp_type == 5120) return 1; // s8
  if (comp_type == 5121) return 1; // u8
  if (comp_type == 5122) return 2; // s16
  if (comp_type == 5123) return 2; // u16
  if (comp_type == 5125) return 4; // u32
  if (comp_type == 5126) return 4; // f32
  return 1;
}

static s32 gltf_comp_count_from_type(str8 type) {
  if (str8_eq(type, STR8L("SCALAR"))) return 1;
  if (str8_eq(type, STR8L("VEC2")))   return 2;
  if (str8_eq(type, STR8L("VEC3")))   return 3;
  if (str8_eq(type, STR8L("VEC4")))   return 4;
  if (str8_eq(type, STR8L("MAT2")))   return 4;
  if (str8_eq(type, STR8L("MAT3")))   return 9;
  if (str8_eq(type, STR8L("MAT4")))   return 16;

  return 1;
}


static v3 json_parse_vec3(Json_Element *root) {
  return (root && root->first) ?
    v3m(
        str8_to_float(root->first->value), 
        str8_to_float(root->first->next->value),
        str8_to_float(root->first->next->next->value)
    ) : v3m(0,0,0);
}

static v4 json_parse_vec4(Json_Element *root) {
  return (root && root->first) ?
    v4m(
        str8_to_float(root->first->value), 
        str8_to_float(root->first->next->value),
        str8_to_float(root->first->next->next->value),
        str8_to_float(root->first->next->next->next->value)
    ) : v4m(0,0,0,0);
}

static unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len);
static Gltf_Info gltf_load(Arena *arena, str8 dir, str8 json_data) {
  Gltf_Info info = {};

  Json_Element *root = json_parse(arena, json_data);
  //Json_Element* scene = json_lookup(root, STR8L("scene"));
  //assert(str8_eq(scene->label, STR8L("scene")) && str8_to_int(scene->value) == 0);

  // 0. Parse meshes
  Json_Element* meshes_json = json_lookup(root, STR8L("meshes")); assert(meshes_json);
  info.mesh_count = json_count_children(meshes_json);
  info.meshes = arena_push_array(arena, Gltf_Mesh, info.mesh_count);
  s32 mesh_idx = 0;
  for (Json_Element *mesh = meshes_json->first; mesh != nullptr; mesh = mesh->next, mesh_idx+=1) {
    Json_Element* primitives_json = json_lookup(mesh, STR8L("primitives")); assert(primitives_json);

    info.meshes[mesh_idx].prim_count = json_count_children(primitives_json);
    info.meshes[mesh_idx].prims = arena_push_array(arena, Gltf_Prim, info.meshes[mesh_idx].prim_count);

    s32 prim_idx = 0;
    for (Json_Element *prim = primitives_json->first; prim != nullptr; prim = prim->next, prim_idx+=1) {
      Gltf_Prim *primitive = &info.meshes[mesh_idx].prims[prim_idx];
      *primitive = default_prim;

      Json_Element* attribs_json = json_lookup(prim, STR8L("attributes")); assert(attribs_json);
      for (Json_Element *attrib = attribs_json->first ; attrib != nullptr; attrib = attrib->next) {
        //printf("IAM ATTRIB %.*s -> %.*s \n", STR8_VARG(attrib->label), STR8_VARG(attrib->value));
        Gltf_Attribs *attribs = &primitive->attribs;
        if (str8_eq(attrib->label, STR8L("POSITION"))) {
          attribs->pos_idx = str8_to_int(attrib->value);
        } else if (str8_eq(attrib->label, STR8L("NORMAL"))) {
          attribs->norm_idx = str8_to_int(attrib->value);
        } else if (str8_eq(attrib->label, STR8L("TEXCOORD_0"))) {
          attribs->texcoord_idx[0] = str8_to_int(attrib->value);
        }
        // TODO: Add more attributes!
        // .....
        // .....
        // .....
      }

      Json_Element* indices = json_lookup(prim, STR8L("indices"));
      if (indices) primitive->indices_idx = str8_to_int(indices->value);
      Json_Element* materials = json_lookup(prim, STR8L("materials"));
      if (materials) primitive->material_idx = str8_to_int(materials->value);
      Json_Element* mode = json_lookup(prim, STR8L("mode"));
      if (mode) primitive->mode = str8_to_int(mode->value);
    }
  }


  // 1. Parse buffers
  Json_Element* buffers_json = json_lookup(root, STR8L("buffers")); assert(buffers_json);
  info.buffers = arena_push_array(arena, str8, json_count_children(buffers_json));
  s32 buf_idx = 0;
  for (Json_Element *b= buffers_json->first; b != nullptr; b = b->next, buf_idx+=1) {
    Json_Element* uri = json_lookup(b, STR8L("uri")); assert(uri);
    Json_Element* byte_len = json_lookup(b, STR8L("byteLength")); assert(byte_len);

    info.buffers[buf_idx] = str8_substr(uri->value, str8_find_needle(uri->value, STR8L(","))+1, uri->value.count); 

    if (str8_ends_with(uri->value, STR8L(".bin"))) {
      str8 bin_fullpath = str8_concat(arena, dir, uri->value);
      printf("reading %.*s from %.*s", STR8_VARG(uri->value), STR8_VARG(bin_fullpath));
      Temp_Arena temp = get_scratch(0,0);
      char* bin_fullpath_cstr = cstr_from_str8(temp.arena, bin_fullpath);
      u32 count = 0;
      // FIXME: leak
      u8 *bin_data = (u8*)read_whole_file_binary(bin_fullpath_cstr, &count);
      assert(bin_data);
      release_scratch(temp);
      info.buffers[buf_idx] = STR8(bin_data, count);
    } else {
      info.buffers[buf_idx] = my_base64_decode(arena, info.buffers[buf_idx]);
    }
  }

  // 2. Parse bufferViews
  Json_Element* buffer_views_json = json_lookup(root, STR8L("bufferViews")); assert(buffer_views_json);
  s32 buffer_view_count = json_count_children(buffer_views_json);
  info.buffer_views = arena_push_array(arena, Gltf_Buffer_View, buffer_view_count);
  s32 view_idx = 0;
  for (Json_Element *b= buffer_views_json->first; b != nullptr; b = b->next, view_idx+=1) {
    // Fill buffer_view array
    Json_Element* buf_idx = json_lookup(b, STR8L("buffer")); assert(buf_idx);
    Json_Element* offset  = json_lookup(b, STR8L("byteOffset"));
    Json_Element* length  = json_lookup(b, STR8L("byteLength")); assert(length);
    Json_Element* target = json_lookup(b, STR8L("target"));
    Json_Element* stride = json_lookup(b, STR8L("byteStride"));

    info.buffer_views[view_idx].buf_idx = str8_to_int(buf_idx->value);
    info.buffer_views[view_idx].byte_offset = (offset) ? str8_to_int(offset->value) : 0;
    info.buffer_views[view_idx].byte_length = str8_to_int(length->value);
    info.buffer_views[view_idx].kind = (target) ? gltf_get_buffer_view_kind(str8_to_int(target->value)) : 0;
    info.buffer_views[view_idx].byte_stride = (stride) ? str8_to_int(stride->value) : 0;

  }

  // 3. Parse accessors
  Json_Element* accessors_json = json_lookup(root, STR8L("accessors")); assert(accessors_json);
  info.accessors = arena_push_array(arena, Gltf_Accessor, json_count_children(accessors_json));
  s32 acc_idx = 0;
  for (Json_Element *a= accessors_json->first; a != nullptr; a = a->next, acc_idx+=1) {
    Json_Element* bufv_idx = json_lookup(a, STR8L("bufferView")); assert(bufv_idx);
    Json_Element* offset  = json_lookup(a, STR8L("byteOffset"));
    Json_Element* comp_type = json_lookup(a, STR8L("componentType")); assert(comp_type);
    Json_Element* count = json_lookup(a, STR8L("count")); assert(count);
    Json_Element* type = json_lookup(a, STR8L("type")); assert(type);
    //Json_Element* max = json_lookup(a, STR8L("max")); assert(max);
    //Json_Element* min = json_lookup(a, STR8L("min")); assert(min);

    info.accessors[acc_idx].bufv_idx = str8_to_int(bufv_idx->value);
    info.accessors[acc_idx].byte_offset = (offset) ? str8_to_int(offset->value) : 0;
    info.accessors[acc_idx].count = str8_to_int(count->value);
    info.accessors[acc_idx].bytes_per_elem = gltf_byte_count_from_comp_type(str8_to_int(comp_type->value));
    info.accessors[acc_idx].comp_per_elem = gltf_comp_count_from_type(type->value);
  }

  // 3. Parse Images
  Json_Element* images_json = json_lookup(root, STR8L("images"));
  info.image_count = (images_json) ? json_count_children(images_json) : 0;
  info.images = arena_push_array(arena, Gltf_Image, info.image_count); 
  if (images_json) {
    s32 image_idx = 0;
    for (Json_Element *i= images_json->first; i != nullptr; i = i->next, image_idx+=1) {
      Json_Element* uri = json_lookup(i, STR8L("uri"));

      char *img_data;
      u32 count = 0;

      if (str8_starts_with(uri->value, STR8L("data"))) {
        str8 img_b64_data = str8_substr(uri->value, str8_find_needle(uri->value, STR8L(","))+1, uri->value.count); 
        str8 decoded = my_base64_decode(arena, img_b64_data);
        count = decoded.count;
        img_data = (char*)decoded.data;
      } else {
        // Read the image data
        str8 img_fullpath = str8_concat(arena, dir, uri->value);
        printf("reading %.*s from %.*s", STR8_VARG(uri->value), STR8_VARG(img_fullpath));
        Temp_Arena temp = get_scratch(0,0);
        char* img_fullpath_cstr = cstr_from_str8(temp.arena, img_fullpath);
        // FIXME: leak
        img_data = (char*)read_whole_file_binary(img_fullpath_cstr, &count);
        assert(img_data);
        release_scratch(temp);
      }

      // Make a new texture asset from said data
      // FIXME: Only .png supported.. uri->value must end in .png
      // FIXME: We can't really modify sampler stuff with this asset system.. f-fix?
      Asset_Id id = tex_mgr_load_from_data(g_am.tm, uri->value, STR8(img_data, count));
      info.images[image_idx] = (Gltf_Image){id};
    }
  }
  // 3. Parse Samplers
  Json_Element* samplers_json = json_lookup(root, STR8L("samplers"));
  info.sampler_count = (samplers_json) ? json_count_children(samplers_json) : 0;
  info.samplers = arena_push_array(arena, Gltf_Sampler, info.sampler_count); 
  if (samplers_json) {
    s32 sampler_idx = 0;
    for (Json_Element *s= samplers_json->first; s != nullptr; s = s->next, sampler_idx+=1) {
      Json_Element *mag = json_lookup(s, STR8L("magFilter"));
      if (mag) info.samplers[sampler_idx].mag_id = str8_to_int(mag->value);

      Json_Element *minif = json_lookup(s, STR8L("minFilter"));
      if (minif) info.samplers[sampler_idx].min_id = str8_to_int(minif->value);

      Json_Element *wrap_s = json_lookup(s, STR8L("wrapS"));
      if (wrap_s) info.samplers[sampler_idx].wrap_s_id = str8_to_int(wrap_s->value);

      Json_Element *wrap_t = json_lookup(s, STR8L("wrapT"));
      if (wrap_t) info.samplers[sampler_idx].wrap_s_id = str8_to_int(wrap_t->value);
    }
  }
  // 3. Parse Textures
  Json_Element* textures_json = json_lookup(root, STR8L("textures"));
  info.texture_count = (textures_json) ? json_count_children(textures_json) : 0;
  info.textures = arena_push_array(arena, Gltf_Texture, info.texture_count); 
  if (textures_json) {
    s32 texture_idx = 0;
    for (Json_Element *t= textures_json->first; t != nullptr; t = t->next, texture_idx+=1) {
      Json_Element *sampler = json_lookup(t, STR8L("sampler"));
      if (sampler) info.textures[texture_idx].sampler_idx = str8_to_int(sampler->value);
      Json_Element *source = json_lookup(t, STR8L("source"));
      if (sampler) info.textures[texture_idx].image_idx = str8_to_int(source->value);

    }
  }

  // 4. Parse materials (In case no material specified we allocate one, the default empty material)
  Json_Element* materials_json = json_lookup(root, STR8L("materials"));
  info.material_count = (materials_json) ? json_count_children(materials_json) : 1;
  info.materials = arena_push_array(arena, Gltf_Material, info.material_count); 

  // FIXME: remove dis
  info.materials[0].pbr.base_color_factor = v4m(1,1,1,1);

  if (materials_json) {
    s32 material_idx = 0;
    for (Json_Element *m= materials_json->first; m != nullptr; m = m->next, material_idx+=1) {
      Json_Element* name = json_lookup(m, STR8L("name")); assert(name);
      printf("Material parsed: %.*s\n", STR8_VARG(name->value));
      Json_Element* emissive_factor = json_lookup(m, STR8L("emissiveFactor"));
      info.materials[material_idx].emissive_factor = json_parse_vec3(emissive_factor);

      Json_Element* pbr_mr = json_lookup(m, STR8L("pbrMetallicRoughness"));
      if (pbr_mr) {
        Json_Element* bcf = json_lookup(pbr_mr, STR8L("baseColorFactor"));
        if (bcf) {
          info.materials[material_idx].pbr.base_color_factor = json_parse_vec4(bcf);
        }

        Json_Element* mf = json_lookup(pbr_mr, STR8L("metallicFactor"));
        if (mf) {
          info.materials[material_idx].pbr.metallic_factor = str8_to_float(mf->value);
        }
      }
    }
  }

  return info;
}

static u8* gltf_data_from_accessor(Gltf_Info info, s32 acc_idx, s32 *stride) {
  s32 bufv_idx = info.accessors[acc_idx].bufv_idx;
  s32 buf_idx = info.buffer_views[bufv_idx].buf_idx;
  u8 *buf_data = info.buffers[buf_idx].data;
  s64 bufv_offset = info.buffer_views[bufv_idx].byte_offset;
  s64 acc_offset = info.accessors[acc_idx].byte_offset;
  if (stride) *(stride) = info.buffer_views[bufv_idx].byte_stride; 

  return (u8*)(buf_data + bufv_offset + acc_offset);
}

static Tri_Vertex* gltf_to_basic_mesh_bundle(Arena *arena, Gltf_Info info, s64 *vcount) {
  s64 actual_vcount = 0;
  for (s32 i = 0; i < info.mesh_count; i+=1) {
    Gltf_Mesh *mesh = &info.meshes[i];

    // add either the count of indices or the count of position to final mesh vertex count
    for (s32 prim_idx = 0; prim_idx < mesh->prim_count; prim_idx+=1) {
      Gltf_Prim *primitive = &mesh->prims[prim_idx];
      actual_vcount += (gltf_is_property_specified(primitive->indices_idx)) ? 
        info.accessors[mesh->prims[prim_idx].indices_idx].count : 
        info.accessors[mesh->prims[prim_idx].attribs.pos_idx].count;
    }
  }
  *vcount = actual_vcount;

  Tri_Vertex *verts = arena_push_array(arena, Tri_Vertex, actual_vcount);

  s64 vert_idx = 0;
  for (s32 i = 0; i < info.mesh_count; i+=1) {
    Gltf_Mesh *mesh = &info.meshes[i];

    for (s32 prim_idx = 0; prim_idx < mesh->prim_count; prim_idx+=1) {
      f32 *positions = (f32*)gltf_data_from_accessor(info, mesh->prims[prim_idx].attribs.pos_idx, nullptr);
      f32 *texcoords = (f32*)gltf_data_from_accessor(info, mesh->prims[prim_idx].attribs.texcoord_idx[0], nullptr);
      b32 index_data_available = gltf_is_property_specified(mesh->prims[prim_idx].indices_idx);

      if (index_data_available) { // In case of index-based primitive
        s16 *indices = (s16*)gltf_data_from_accessor(info, mesh->prims[prim_idx].indices_idx, nullptr);
        s64 indices_count = info.accessors[mesh->prims[prim_idx].indices_idx].count;
        for (s64 idx = 0; idx < indices_count; idx+=1) {
          s32 vidx = indices[idx];
          v3 *vpos = (v3*)(&positions[3 * vidx]);
          v2 *tc = (v2*)(&texcoords[2 * vidx]);
          verts[vert_idx].pos = *(vpos);
          verts[vert_idx].uv = *(tc);

          verts[vert_idx].color = info.materials[mesh->prims[prim_idx].material_idx].pbr.base_color_factor;
          vert_idx+=1;
        }
      } else { // In case of vertex-only primitive
        s64 vertices_count = info.accessors[mesh->prims[prim_idx].attribs.pos_idx].count;
        for (s64 idx = 0; idx < vertices_count; idx+=1) {
          s32 vidx = idx; 
          v3 *vpos = (v3*)(&positions[3 * vidx]);
          verts[vert_idx].pos = *(vpos);

          verts[vert_idx].color = info.materials[mesh->prims[prim_idx].material_idx].pbr.base_color_factor;
          vert_idx+=1;
        }
      }
    }
  }

  return verts;
}

#endif
