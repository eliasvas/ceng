#ifndef GLTF_LOADER_H__
#define GLTF_LOADER_H__
#include "base/base_inc.h"
#include "core/json_util.h"
#include "core/base64.h"

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

// This struct will pretty much contain all the gltf
// fields, can be used for serializing or deserializing data
typedef struct Mesh_Data {s32 pos_idx; s32 indices_idx;} Mesh_Data;
typedef struct Gltf2_Info {

  Mesh_Data *mdata;
  s32 mdata_count;

  str8 *buffers;
  s32 buffer_count;

  Gltf_Buffer_View *buffer_views;
  s32 buffer_views_count;

  Gltf_Accessor *accessors;
  s32 accessor_count;

} Gltf2_Info;

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

// FIXME: Make a base64 encode/decode utility in core layer
static unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len);
static Gltf2_Info gltf2_load(Arena *arena, char *json_data) {
  Gltf2_Info info = {};

  Json_Element *root = json_parse(arena, json_data);
  Json_Element* scene = json_lookup(root, STR8L("scene"));
  assert(str8_eq(scene->label, STR8L("scene")) && str8_to_int(scene->value) == 0);

  // 0. Parse meshes
  Json_Element* meshes_json = json_lookup(root, STR8L("meshes_json")); assert(meshes_json);
  info.mdata_count = json_count_children(meshes_json);
  info.mdata = arena_push_array(arena, Mesh_Data, info.mdata_count);
  s32 mesh_idx = 0;
  for (Json_Element *mesh = meshes_json->first; mesh != nullptr; mesh = mesh->next, mesh_idx+=1) {
    Json_Element* primitives_json = json_lookup(meshes_json, STR8L("primitives")); assert(primitives_json);
    for (Json_Element *prim = primitives_json->first; prim != nullptr; prim = prim->next) {
      Json_Element* attribs_json = json_lookup(prim, STR8L("attributes")); assert(attribs_json);

      // FIXME: WHY attribs_json->first->first
      for (Json_Element *attrib = attribs_json->first->first ; attrib != nullptr; attrib = attrib->next) {
        //printf("IAM ATTRIB %.*s -> %.*s \n", STR8_VARG(attrib->label), STR8_VARG(attrib->value));
        if (str8_eq(attrib->label, STR8L("POSITION"))) {
          info.mdata[mesh_idx].pos_idx = str8_to_int(attrib->value);
        }
      }
      Json_Element* indices = json_lookup(attribs_json, STR8L("indices")); assert(indices);
      info.mdata[mesh_idx].indices_idx = str8_to_int(indices->value);
    }
  }


  // 1. Parse buffers
  Json_Element* buffers_json = json_lookup(root, STR8L("buffers")); assert(buffers_json);
  info.buffers = arena_push_array(arena, str8, json_count_children(buffers_json));
  s32 buf_idx = 0;
  for (Json_Element *b= buffers_json->first; b != nullptr; b = b->next, buf_idx+=1) {
    Json_Element* data = json_lookup(b, STR8L("uri")); assert(data);
    Json_Element* byte_len = json_lookup(b, STR8L("byteLength")); assert(byte_len);

    info.buffers[buf_idx] = str8_substr(data->value, str8_find_needle(data->value, STR8L(","))+1, data->value.count); 

    info.buffers[buf_idx] = my_base64_decode(arena, info.buffers[buf_idx]);
    assert(info.buffers[buf_idx].count == (s32)str8_to_int(byte_len->value));


    // aren't buffers always the length of the 'uri' string?
    //printf("len %.*s buf: %ld\n", STR8_VARG(byte_len->value), info.buffers[buf_idx].count);
    //assert(str8_to_int(byte_len->value) == buffers[buf_idx].count);
  }

  // 2. Parse bufferViews
  Json_Element* buffer_views_json = json_lookup(root, STR8L("bufferViews")); assert(buffer_views_json);
  s32 buffer_views_count = json_count_children(buffer_views_json);
  info.buffer_views = arena_push_array(arena, str8, buffer_views_count);
  s32 view_idx = 0;
  for (Json_Element *b= buffer_views_json->first; b != nullptr; b = b->next, view_idx+=1) {
    // Fill buffer_view array
    Json_Element* buf_idx = json_lookup(b, STR8L("buffer")); assert(buf_idx);
    Json_Element* offset  = json_lookup(b, STR8L("byteOffset")); assert(offset);
    Json_Element* length  = json_lookup(b, STR8L("byteLength")); assert(length);
    Json_Element* target = json_lookup(buffer_views_json, STR8L("target")); assert(target);
    Json_Element* stride = json_lookup(buffer_views_json, STR8L("byteStride"));

    info.buffer_views[view_idx].buf_idx = str8_to_int(buf_idx->value);
    info.buffer_views[view_idx].byte_offset = str8_to_int(offset->value);
    info.buffer_views[view_idx].byte_length = str8_to_int(length->value);
    info.buffer_views[view_idx].kind = gltf_get_buffer_view_kind(str8_to_int(target->value));
    if (stride) info.buffer_views[view_idx].byte_stride = str8_to_int(stride->value);

  }

  // 3. Parse accessors
  Json_Element* accessors_json = json_lookup(root, STR8L("accessors")); assert(accessors_json);
  info.accessors = arena_push_array(arena, Gltf_Accessor, json_count_children(accessors_json));
  s32 acc_idx = 0;
  for (Json_Element *a= accessors_json->first; a != nullptr; a = a->next, acc_idx+=1) {
    Json_Element* bufv_idx = json_lookup(a, STR8L("bufferView")); assert(bufv_idx);
    Json_Element* offset  = json_lookup(a, STR8L("byteOffset")); assert(offset);
    Json_Element* comp_type = json_lookup(a, STR8L("componentType")); assert(comp_type);
    Json_Element* count = json_lookup(a, STR8L("count")); assert(count);
    Json_Element* type = json_lookup(a, STR8L("type")); assert(type);
    //Json_Element* max = json_lookup(a, STR8L("max")); assert(max);
    //Json_Element* min = json_lookup(a, STR8L("min")); assert(min);

    info.accessors[acc_idx].bufv_idx = str8_to_int(bufv_idx->value);
    info.accessors[acc_idx].byte_offset = str8_to_int(offset->value);
    info.accessors[acc_idx].count = str8_to_int(count->value);
    info.accessors[acc_idx].bytes_per_elem = gltf_byte_count_from_comp_type(str8_to_int(comp_type->value));
    info.accessors[acc_idx].comp_per_elem = gltf_comp_count_from_type(type->value);
  }

  return info;
}

static u8* gltf_data_from_accessor(Gltf2_Info info, s32 acc_idx, s32 *stride) {
  s32 bufv_idx = info.accessors[acc_idx].bufv_idx;
  s32 buf_idx = info.buffer_views[bufv_idx].buf_idx;
  u8 *buf_data = info.buffers[buf_idx].data;
  s64 bufv_offset = info.buffer_views[bufv_idx].byte_offset;
  s64 acc_offset = info.accessors[acc_idx].byte_offset;
  if (stride) *(stride) = info.buffer_views[bufv_idx].byte_stride; 

  return (u8*)(buf_data + bufv_offset + acc_offset);
}

static Tri_Vertex* gltf_to_basic_mesh_bundle(Arena *arena, Gltf2_Info info, s64 *vcount) {
  s64 actual_vcount = 0;
  for (s32 i = 0; i < info.mdata_count; i+=1) {
    Mesh_Data *md = &info.mdata[i];

    // FIXME: We presuppose every mesh has indices.. is that expected? check spec
    actual_vcount += info.accessors[md->indices_idx].count;
  }
  *vcount = actual_vcount;

  Tri_Vertex *verts = arena_push_array(arena, Tri_Vertex, actual_vcount);

  s64 vert_idx = 0;
  for (s32 i = 0; i < info.mdata_count; i+=1) {
    Mesh_Data *md = &info.mdata[i];

    s16 *indices = (s16*)gltf_data_from_accessor(info, md->indices_idx, nullptr);
    s64 indices_count = info.accessors[md->indices_idx].count;
    f32 *positions = (f32*)gltf_data_from_accessor(info, md->pos_idx, nullptr);

    for (s64 idx = 0; idx < indices_count; idx+=1) {
      s32 vidx = indices[idx];
      v3 *vpos = (v3*)(&positions[3 * vidx]);
      verts[vert_idx].pos = *(vpos);

      // Only for testing rn
      verts[vert_idx].color = FRZ_RED;
      vert_idx+=1;
    }
  }

  return verts;
}

#endif
