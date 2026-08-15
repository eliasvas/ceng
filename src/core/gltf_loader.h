#ifndef GLTF_LOADER_H__
#define GLTF_LOADER_H__
#include "base/base_inc.h"
#include "core/json_util.h"

// Ref: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// TODO: Make it possible to output regular Tri_Vertex's + Index info so that previous pipeline can be used

// extra INFO:
//When two or more vertex attribute accessors use the same bufferView, its byteStride MUST be defined.


typedef enum {
  GLTF_ARRAY_BUFFER,         // vertex buffer
  GLTF_ELEMENT_ARRAY_BUFFER, // index buffer
} Gltf_Buffer_Kind;

// This struct will pretty much contain all the gltf
// fields, can be used for serializing or deserializing data
typedef struct Mesh_Data {s32 pos_idx; s32 indices_idx;} Mesh_Data;
typedef struct Gltf2_Info {

  Mesh_Data *mdata;
  s32 mdata_count;

  str8 *buffers;
  s32 buffer_count;

  str8 *buffer_views;
  Gltf_Buffer_Kind *buffer_view_kinds;
  s32 *buffer_view_strides;
  s32 buffer_view_count;

  str8 *accessors;
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
    //info.buffers[buf_idx].count = str8_to_int(byte_len->value);

    size_t len = 0;
    unsigned char * raw = base64_decode(info.buffers[buf_idx].data, info.buffers[buf_idx].count, &len);
    info.buffers[buf_idx].data = raw;
    info.buffers[buf_idx].count = len;

    assert(len == (size_t)str8_to_int(byte_len->value));

    // aren't buffers always the length of the 'uri' string?
    //printf("len %.*s buf: %ld\n", STR8_VARG(byte_len->value), info.buffers[buf_idx].count);
    //assert(str8_to_int(byte_len->value) == buffers[buf_idx].count);
  }

  // 2. Parse bufferViews
  Json_Element* buffer_views_json = json_lookup(root, STR8L("bufferViews")); assert(buffer_views_json);
  s32 buffer_views_count = json_count_children(buffer_views_json);
  info.buffer_views = arena_push_array(arena, str8, buffer_views_count);
  info.buffer_view_kinds = arena_push_array(arena, Gltf_Buffer_Kind, buffer_views_count); 
  info.buffer_view_strides = arena_push_array(arena, s32, buffer_views_count);
  s32 view_idx = 0;
  for (Json_Element *b= buffer_views_json->first; b != nullptr; b = b->next, view_idx+=1) {
    // Fill buffer_view array
    Json_Element* buf_idx = json_lookup(b, STR8L("buffer")); assert(buf_idx);
    Json_Element* offset  = json_lookup(b, STR8L("byteOffset")); assert(offset);
    Json_Element* length  = json_lookup(b, STR8L("byteLength")); assert(length);
    info.buffer_views[view_idx].data = str8_to_int(offset->value) + info.buffers[str8_to_int(buf_idx->value)].data;
    info.buffer_views[view_idx].count = str8_to_int(length->value);

    // Fill buffer_view_kind array
    Json_Element* target = json_lookup(buffer_views_json, STR8L("target")); assert(target);
    Gltf_Buffer_Kind kind = gltf_get_buffer_view_kind(str8_to_int(target->value));
    info.buffer_view_kinds[view_idx] = kind;

    // Fill buffer_view_strides array (optional field)
    Json_Element* stride = json_lookup(buffer_views_json, STR8L("byteStride"));
    if (stride) {
      info.buffer_view_strides[view_idx] = str8_to_int(stride->value);
    }
  }

  // 3. Parse accessors
  Json_Element* accessors_json = json_lookup(root, STR8L("accessors")); assert(accessors_json);
  info.accessors = arena_push_array(arena, str8, json_count_children(accessors_json));
  s32 acc_idx = 0;
  for (Json_Element *a= accessors_json->first; a != nullptr; a = a->next, acc_idx+=1) {
    Json_Element* bufv_idx = json_lookup(a, STR8L("bufferView")); assert(bufv_idx);
    Json_Element* offset  = json_lookup(a, STR8L("byteOffset")); assert(offset);
    //Json_Element* compType = json_lookup(a, STR8L("componentTYpe")); assert(compType);
    Json_Element* count = json_lookup(a, STR8L("count")); assert(count);
    Json_Element* type = json_lookup(a, STR8L("type")); assert(type);
    //Json_Element* max = json_lookup(a, STR8L("max")); assert(max);
    //Json_Element* min = json_lookup(a, STR8L("min")); assert(min);

    info.accessors[acc_idx].data = str8_to_int(offset->value) + info.buffer_views[str8_to_int(bufv_idx->value)].data;
    info.accessors[acc_idx].count = str8_to_int(count->value);
  }


  return info;
}


static Tri_Vertex* gltf_to_basic_mesh_bundle(Arena *arena, Gltf2_Info info, s64 *vcount) {

  s64 actual_vcount = 0;
  for (s32 i = 0; i < info.mdata_count; i+=1) {
    Mesh_Data *md = &info.mdata[i];

    // FIXME: can't get buffer view from accessor right now
    // should probably model the whole thing.. ughh (We lose sizing info inside accessor)
    actual_vcount += info.accessors[md->indices_idx].count;
  }
  *vcount = actual_vcount;
  printf("vcount: %ld\n", *vcount);
  Tri_Vertex *verts = arena_push_array(arena, Tri_Vertex, actual_vcount);

  s64 vert_idx = 0;
  for (s32 i = 0; i < info.mdata_count; i+=1) {
    Mesh_Data *md = &info.mdata[i];

    s16 *indices = (s16*)info.accessors[md->indices_idx].data;
    s64 indices_count = info.accessors[md->indices_idx].count;
    f32 *positions = (f32*)info.accessors[md->pos_idx].data;

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



static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}
#endif
