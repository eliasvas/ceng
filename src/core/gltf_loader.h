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
  s32 index;
  s32 tex_coord;
  b32 active;
} Gltf_Material_Tex_Info; 

typedef struct {
  v4 base_color_factor;
  Gltf_Material_Tex_Info base_color_texture;
  f32 metallic_factor;
  f32 roughness_factor;
  Gltf_Material_Tex_Info metallic_roughness_texture;
} Gltf_Material_PBR;

typedef struct {
  Gltf_Material_PBR pbr;
  Gltf_Material_Tex_Info normal_texture;
  Gltf_Material_Tex_Info emissive_texture;
  Gltf_Material_Tex_Info occlusion_texture;
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

static Gltf_Material_Tex_Info json_parse_tex_info(Json_Element *root, str8 path) {
  Gltf_Material_Tex_Info info = {};
  Json_Element *bct = json_lookup(root, path);
  if (bct) {
    Json_Element *index = json_lookup(bct, STR8L("index"));
    if (index) info.index = str8_to_int(index->value);
    Json_Element *tex_coord = json_lookup(bct, STR8L("texCoord"));
    if (tex_coord) info.tex_coord = str8_to_int(tex_coord->value);
    if (index || tex_coord) info.active = true;
  }
  return info;
}

static s32 json_parse_int(Json_Element *root, str8 path, s32 def) {
  Json_Element *node = json_lookup(root, path);
  return (node) ? str8_to_int(node->value) : def;
}

static f32 json_parse_float(Json_Element *root, str8 path, f32 def) {
  Json_Element *node = json_lookup(root, path);
  return (node) ? str8_to_float(node->value) : def;
}

static v2 json_parse_vec2(Json_Element *root, str8 path, v2 def) {
  Json_Element *node = json_lookup(root, path);
  return (node && node->first) ?
    v2m(
        str8_to_float(node->first->value), 
        str8_to_float(node->first->next->value)
    ) : def;
}

static v3 json_parse_vec3(Json_Element *root, str8 path, v3 def) {
  Json_Element *node = json_lookup(root, path);
  return (node && node->first) ?
    v3m(
        str8_to_float(node->first->value), 
        str8_to_float(node->first->next->value),
        str8_to_float(node->first->next->next->value)
    ) : def;
}

static v4 json_parse_vec4(Json_Element *root, str8 path, v4 def) {
  Json_Element *node = json_lookup(root, path);
  return (node && node->first) ?
    v4m(
        str8_to_float(node->first->value), 
        str8_to_float(node->first->next->value),
        str8_to_float(node->first->next->next->value),
        str8_to_float(node->first->next->next->next->value)
    ) : def;
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
        if (str8_eq(attrib->label, STR8L("POSITION")))        attribs->pos_idx =         str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("NORMAL")))     attribs->norm_idx =        str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("TANGENT")))    attribs->tang_idx =        str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("TEXCOORD_0"))) attribs->texcoord_idx[0] = str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("TEXCOORD_1"))) attribs->texcoord_idx[1] = str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("TEXCOORD_2"))) attribs->texcoord_idx[2] = str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("TEXCOORD_3"))) attribs->texcoord_idx[3] = str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("COLOR_0")))    attribs->col_idx[0] =      str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("COLOR_1")))    attribs->col_idx[1] =      str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("COLOR_2")))    attribs->col_idx[2] =      str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("COLOR_3")))    attribs->col_idx[3] =      str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("JOINTS_0")))   attribs->joints_idx[0] =   str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("JOINTS_1")))   attribs->joints_idx[1] =   str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("JOINTS_2")))   attribs->joints_idx[2] =   str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("JOINTS_3")))   attribs->joints_idx[3] =   str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("WEIGHTS_0")))  attribs->weights_idx[0] =  str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("WEIGHTS_1")))  attribs->weights_idx[1] =  str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("WEIGHTS_2")))  attribs->weights_idx[2] =  str8_to_int(attrib->value);
        else if (str8_eq(attrib->label, STR8L("WEIGHTS_3")))  attribs->weights_idx[3] =  str8_to_int(attrib->value);
      }
      primitive->indices_idx = json_parse_int(prim, STR8L("indices"), GLTF_PROPERTY_NOT_SPECIFIED);
      primitive->material_idx = json_parse_int(prim, STR8L("materials"), GLTF_PROPERTY_NOT_SPECIFIED);
      primitive->mode = json_parse_int(prim, STR8L("mode"), 4);
    }
  }


  // 1. Parse buffers
  Json_Element* buffers_json = json_lookup(root, STR8L("buffers")); assert(buffers_json);
  info.buffers = arena_push_array(arena, str8, json_count_children(buffers_json));
  s32 buf_idx = 0;
  for (Json_Element *b= buffers_json->first; b != nullptr; b = b->next, buf_idx+=1) {
    Json_Element* uri = json_lookup(b, STR8L("uri")); assert(uri);
    Json_Element* byte_len = json_lookup(b, STR8L("byteLength")); assert(byte_len);

    s64 data_idx = str8_find_needle(uri->value, STR8L(","))+1;
    info.buffers[buf_idx] = str8_substr(uri->value, data_idx, uri->value.count); 

    if (str8_ends_with(uri->value, STR8L(".bin"))) {
      Temp_Arena temp = get_scratch(&arena,1);
      str8 bin_fullpath = str8_concat(temp.arena, str8_concat(temp.arena, dir, STR8L("/")), uri->value);
      //printf("reading %.*s from %.*s", STR8_VARG(uri->value), STR8_VARG(bin_fullpath));
      str8 bin_data = str8_read_file_binary(arena, bin_fullpath);
      assert(bin_data.count);
      release_scratch(temp);
      info.buffers[buf_idx] = bin_data;
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
    Gltf_Buffer_View *view = &info.buffer_views[view_idx];

    view->buf_idx = json_parse_int(b, STR8L("buffer"), 0);
    view->byte_offset = json_parse_int(b, STR8L("byteOffset"), 0);
    view->byte_length = json_parse_int(b, STR8L("byteLength"), 0);
    view->kind = gltf_get_buffer_view_kind(json_parse_int(b, STR8L("target"), 0));
    view->byte_stride = json_parse_int(b, STR8L("byteStride"), 0);
  }

  // 3. Parse accessors
  Json_Element* accessors_json = json_lookup(root, STR8L("accessors")); assert(accessors_json);
  info.accessors = arena_push_array(arena, Gltf_Accessor, json_count_children(accessors_json));
  s32 acc_idx = 0;
  for (Json_Element *a= accessors_json->first; a != nullptr; a = a->next, acc_idx+=1) {
    Gltf_Accessor *accessor = &info.accessors[acc_idx];

    accessor->bufv_idx = json_parse_int(a, STR8L("bufferView"), 0);
    accessor->byte_offset = json_parse_int(a, STR8L("byteOffset"), 0);
    accessor->bytes_per_elem = gltf_byte_count_from_comp_type(json_parse_int(a, STR8L("componentType"), 0));
    accessor->count = json_parse_int(a, STR8L("count"), 0);

    Json_Element* type = json_lookup(a, STR8L("type")); assert(type);
    accessor->comp_per_elem = gltf_comp_count_from_type(type->value);
  }

  // 3. Parse Images
  Json_Element* images_json = json_lookup(root, STR8L("images"));
  info.image_count = (images_json) ? json_count_children(images_json) : 0;
  info.images = arena_push_array(arena, Gltf_Image, info.image_count); 
  if (images_json) {
    s32 image_idx = 0;
    for (Json_Element *i= images_json->first; i != nullptr; i = i->next, image_idx+=1) {
      Json_Element* uri = json_lookup(i, STR8L("uri"));

      str8 img_data;
      if (str8_starts_with(uri->value, STR8L("data"))) {
        s64 data_idx = str8_find_needle(uri->value, STR8L(","))+1;
        str8 img_b64_data = str8_substr(uri->value, data_idx, uri->value.count); 
        str8 decoded = my_base64_decode(arena, img_b64_data);
        img_data = STR8((char*)decoded.data, decoded.count);
      } else {
        // Read the image data
        //printf("reading %.*s from %.*s", STR8_VARG(uri->value), STR8_VARG(img_fullpath));
        Temp_Arena temp = get_scratch(&arena,1);
        str8 img_fullpath = str8_concat(temp.arena, str8_concat(temp.arena, dir, STR8L("/")), uri->value);
        img_data = str8_read_file_binary(arena, img_fullpath);
        release_scratch(temp);
      }

      Asset_Id id = am_load_from_data(uri->value, img_data);
      info.images[image_idx] = (Gltf_Image){id};
    }
  }
  // 3. Parse Samplers
  Json_Element* samplers_json = json_lookup(root, STR8L("samplers"));
  info.sampler_count = (samplers_json) ? json_count_children(samplers_json) : 0;
  info.samplers = arena_push_array(arena, Gltf_Sampler, info.sampler_count); 
  if (samplers_json) {
    s32 sampler_idx = 0;
    for (Json_Element *s = samplers_json->first; s != nullptr; s = s->next, sampler_idx+=1) {
      Gltf_Sampler *sampler = &info.samplers[sampler_idx];
      //FIXME: Make conversions for all these fields when time comes
      sampler->mag_id = json_parse_int(s, STR8L("magFilter"), 0);
      sampler->min_id = json_parse_int(s, STR8L("minFilter"), 0);
      sampler->wrap_s_id = json_parse_int(s, STR8L("wrapS"), 0);
      sampler->wrap_t_id = json_parse_int(s, STR8L("wrapT"), 0);
    }
  }
  // 3. Parse Textures
  Json_Element* textures_json = json_lookup(root, STR8L("textures"));
  info.texture_count = (textures_json) ? json_count_children(textures_json) : 0;
  info.textures = arena_push_array(arena, Gltf_Texture, info.texture_count); 
  if (textures_json) {
    s32 texture_idx = 0;
    for (Json_Element *t= textures_json->first; t != nullptr; t = t->next, texture_idx+=1) {
      Gltf_Texture *texture = &info.textures[texture_idx];
      texture->sampler_idx = json_parse_int(t, STR8L("sampler"), 0);
      texture->image_idx = json_parse_int(t, STR8L("source"), 0);
    }
  }

  // 4. Parse materials (In case no material specified we allocate one, the default empty material)
  Json_Element* materials_json = json_lookup(root, STR8L("materials"));
  info.material_count = (materials_json) ? json_count_children(materials_json) : 1;
  info.materials = arena_push_array(arena, Gltf_Material, info.material_count); 


  if (materials_json) {
    s32 material_idx = 0;
    for (Json_Element *m= materials_json->first; m != nullptr; m = m->next, material_idx+=1) {
      Json_Element* name = json_lookup(m, STR8L("name")); assert(name);
      Gltf_Material *material = &info.materials[material_idx];

      Json_Element* pbr_mr = json_lookup(m, STR8L("pbrMetallicRoughness"));
      if (pbr_mr) {
        material->pbr.base_color_factor = json_parse_vec4(pbr_mr, STR8L("baseColorFactor"), v4_one);
        material->pbr.metallic_factor = json_parse_float(pbr_mr, STR8L("metallicFactor"), 1.0);
        material->pbr.roughness_factor = json_parse_float(pbr_mr, STR8L("roughnessFactor"), 1.0);

        material->pbr.base_color_texture = json_parse_tex_info(pbr_mr, STR8L("baseColorTexture"));
        material->pbr.metallic_roughness_texture = json_parse_tex_info(pbr_mr, STR8L("metallicRoughnessTexture"));
      }

      material->emissive_factor = json_parse_vec3(m, STR8L("emissive_factor"), v3_zero);
      material->normal_texture = json_parse_tex_info(m, STR8L("normalTexture"));
      material->emissive_texture = json_parse_tex_info(m, STR8L("emissiveTexture"));
      material->occlusion_texture = json_parse_tex_info(m, STR8L("occlusionTexture"));
    }
  }

  return info;
}

static u8* gltf_data_from_accessor(Gltf_Info info, s32 acc_idx, s32 *stride) {
  if (!gltf_is_property_specified(acc_idx)) return nullptr;

  s32 bufv_idx = info.accessors[acc_idx].bufv_idx;
  s32 buf_idx = info.buffer_views[bufv_idx].buf_idx;
  u8 *buf_data = info.buffers[buf_idx].data;
  s64 bufv_offset = info.buffer_views[bufv_idx].byte_offset;
  s64 acc_offset = info.accessors[acc_idx].byte_offset;
  if (stride) *(stride) = info.buffer_views[bufv_idx].byte_stride; 

  return (u8*)(buf_data + bufv_offset + acc_offset);
}

static Model_Info gltf_to_model(Arena *arena, Gltf_Info info) {
  Model_Info model = {};

  model.mesh_count = info.mesh_count;
  model.meshes = arena_push_array(arena, Mesh_Info, model.mesh_count); 
  for (s32 mesh_idx = 0; mesh_idx < info.mesh_count; mesh_idx+=1) {
    Gltf_Mesh *gmesh = &info.meshes[mesh_idx];
    Mesh_Info *mesh = &model.meshes[mesh_idx];
    // FIXME: We currently don't do anything w/ primitives, this has to change!
    mesh->prim_count = gmesh->prim_count;
    mesh->prims = arena_push_array(arena, Mesh_Primitive_Info, mesh->prim_count);
    for (s32 prim_idx = 0; prim_idx < mesh->prim_count; prim_idx+=1) {
      Gltf_Prim *gprim = &gmesh->prims[prim_idx];
      Mesh_Primitive_Info *prim = &mesh->prims[prim_idx];

      // FIXME: This should be used for the material description ok?! Make a UBO and everything!
      Gltf_Material *material = &info.materials[gprim->material_idx];
      assert(material);

      prim->type = ogl_prim_type_from_gltf_prim_mode(gprim->mode);
      f32 *positions = (f32*)gltf_data_from_accessor(info, gprim->attribs.pos_idx, nullptr);

      // FIXME: For normals we have to compute them if not available..
      f32 *normals   = (f32*)gltf_data_from_accessor(info, gprim->attribs.norm_idx, nullptr);
      f32 *tangents = (f32*)gltf_data_from_accessor(info, gprim->attribs.tang_idx, nullptr);

      f32 *texcoords_0 = (f32*)gltf_data_from_accessor(info, gprim->attribs.texcoord_idx[0], nullptr);
      f32 *texcoords_1 = (f32*)gltf_data_from_accessor(info, gprim->attribs.texcoord_idx[1], nullptr);
      f32 *texcoords_2 = (f32*)gltf_data_from_accessor(info, gprim->attribs.texcoord_idx[2], nullptr);
      f32 *texcoords_3 = (f32*)gltf_data_from_accessor(info, gprim->attribs.texcoord_idx[3], nullptr);

      f32 *colors_0 = (f32*)gltf_data_from_accessor(info, gprim->attribs.col_idx[0], nullptr);
      f32 *colors_1 = (f32*)gltf_data_from_accessor(info, gprim->attribs.col_idx[1], nullptr);
      f32 *colors_2 = (f32*)gltf_data_from_accessor(info, gprim->attribs.col_idx[2], nullptr);
      f32 *colors_3 = (f32*)gltf_data_from_accessor(info, gprim->attribs.col_idx[3], nullptr);

      f32 *joints_0 = (f32*)gltf_data_from_accessor(info, gprim->attribs.joints_idx[0], nullptr);
      f32 *joints_1 = (f32*)gltf_data_from_accessor(info, gprim->attribs.joints_idx[1], nullptr);
      f32 *joints_2 = (f32*)gltf_data_from_accessor(info, gprim->attribs.joints_idx[2], nullptr);
      f32 *joints_3 = (f32*)gltf_data_from_accessor(info, gprim->attribs.joints_idx[3], nullptr);

      f32 *weights_0 = (f32*)gltf_data_from_accessor(info, gprim->attribs.weights_idx[0], nullptr);
      f32 *weights_1 = (f32*)gltf_data_from_accessor(info, gprim->attribs.weights_idx[1], nullptr);
      f32 *weights_2 = (f32*)gltf_data_from_accessor(info, gprim->attribs.weights_idx[2], nullptr);
      f32 *weights_3 = (f32*)gltf_data_from_accessor(info, gprim->attribs.weights_idx[3], nullptr);

      s64 vcount = info.accessors[gprim->attribs.pos_idx].count;
      Temp_Arena temp = get_scratch(&arena,1);
      Uber_Vertex *verts = arena_push_array_nz(temp.arena, Uber_Vertex, vcount);
      for (s64 vidx = 0; vidx < vcount; vidx+=1) {
        verts[vidx].pos = *((v3*)&positions[3 * vidx]);
        verts[vidx].norm = (normals) ? *((v3*)&normals[3 * vidx]) : v3m(0,1,0);
        verts[vidx].tangent = (tangents) ? *((v4*)&tangents[4 * vidx]) : v4m(0,0,0,0);

        verts[vidx].tc_0 = (texcoords_0) ? *((v2*)&texcoords_0[2 * vidx]) : v2m(0,0);
        verts[vidx].tc_1 = (texcoords_1) ? *((v2*)&texcoords_1[2 * vidx]) : v2m(0,0);
        verts[vidx].tc_2 = (texcoords_2) ? *((v2*)&texcoords_2[2 * vidx]) : v2m(0,0);
        verts[vidx].tc_3 = (texcoords_3) ? *((v2*)&texcoords_3[2 * vidx]) : v2m(0,0);

        verts[vidx].col_0 = (colors_0) ? *((v4*)&colors_0[4 * vidx]) : v4m(1,1,1,1);
        verts[vidx].col_1 = (colors_1) ? *((v4*)&colors_1[4 * vidx]) : v4m(1,1,1,1);
        verts[vidx].col_2 = (colors_2) ? *((v4*)&colors_2[4 * vidx]) : v4m(1,1,1,1);
        verts[vidx].col_3 = (colors_3) ? *((v4*)&colors_3[4 * vidx]) : v4m(1,1,1,1);

        verts[vidx].joint_0 = (joints_0) ? *((v4*)&joints_0[4 * vidx]) : v4m(1,0,0,0);
        verts[vidx].joint_1 = (joints_1) ? *((v4*)&joints_1[4 * vidx]) : v4m(1,0,0,0);
        verts[vidx].joint_2 = (joints_2) ? *((v4*)&joints_2[4 * vidx]) : v4m(1,0,0,0);
        verts[vidx].joint_3 = (joints_3) ? *((v4*)&joints_3[4 * vidx]) : v4m(1,0,0,0);

        verts[vidx].weight_0 = (weights_0) ? *((v4*)&weights_0[4 * vidx]) : v4m(0,0,0,0);
        verts[vidx].weight_1 = (weights_1) ? *((v4*)&weights_1[4 * vidx]) : v4m(0,0,0,0);
        verts[vidx].weight_2 = (weights_2) ? *((v4*)&weights_2[4 * vidx]) : v4m(0,0,0,0);
        verts[vidx].weight_3 = (weights_3) ? *((v4*)&weights_3[4 * vidx]) : v4m(0,0,0,0);
      }

      b32 mesh_has_idx = gltf_is_property_specified(gprim->indices_idx);

      Ogl_Buf vbo = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_STATIC, verts, 1, sizeof(Uber_Vertex)*vcount);
      prim->vbo = vbo;

      if (mesh_has_idx) {
        s32 index_buf_element_size = info.accessors[gprim->indices_idx].bytes_per_elem;
        u8 *indices = (u8*)gltf_data_from_accessor(info, gprim->indices_idx, nullptr);
        s64 indices_count = info.accessors[gprim->indices_idx].count;
        Ogl_Buf ibo = ogl_buf_make(OGL_BUF_KIND_INDEX, OGL_BUF_HINT_STATIC, indices, indices_count, index_buf_element_size);
        prim->ibo = ibo;
      }

      // FIXME: Material handling.. GWE
      if (info.image_count > 0 && info.texture_count >0 && info.material_count > 0) {
        model.tex_id = info.images[info.textures[info.materials[0].pbr.base_color_texture.index].image_idx].id;
      }
      release_scratch(temp);
    }
  }

  return model;
}

#endif
