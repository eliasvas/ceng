#ifndef GLTF_LOADER_H__
#define GLTF_LOADER_H__
#include "base/base_inc.h"
#include "core/json_util.h"
#include "core/base64.h"

// Ref: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// Ref: https://github.khronos.org/glTF-Tutorials/gltfTutorial/

typedef struct {
  v3 t;
  quat r;
  v3 s;
} Gltf_Transform;

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

  m4 model_matrix;
  v3 t;
  quat r;
  v3 s;
} Gltf_Prim;

typedef struct {
  Gltf_Prim *prims;
  s32 prim_count;
} Gltf_Mesh;

typedef struct {
  s32 mesh_idx;
  s32 skin_idx;
  m4 model_matrix;

  v3 t;
  quat r;
  v3 s;

  s32 *children;
  s32 children_count;
  //str8 name;
} Gltf_Node_Info;

typedef enum {
  GLTF_ANIMATION_KIND_TRANSLATION,
  GLTF_ANIMATION_KIND_ROTATION,
  GLTF_ANIMATION_KIND_SCALE,
} Gltf_Animation_Kind;

typedef struct {
  s32 sampler_id;
  s32 target_node_id;
  Gltf_Animation_Kind kind;
} Gltf_Animation_Channel;

typedef enum {
  GLTF_INTERP_TYPE_LINEAR,
  GLTF_INTERP_TYPE_STEP,
  GLTF_INTERP_TYPE_CUBIC_SPLINE,
} Gltf_Animation_Sampler_Interp_Type;

typedef struct {
  Gltf_Animation_Sampler_Interp_Type type;
  s32 input;
  s32 output;
} Gltf_Animation_Sampler;

typedef struct {
  Gltf_Animation_Channel *channels;
  s32 channel_count;

  Gltf_Animation_Sampler *samplers;
  s32 sampler_count;
} Gltf_Animation;

typedef struct {
  s32 inverse_bind_matrices_accessor_id; // (OPTIONAL) where to get IBNs
  s32 skeleton_node_id; // the root node id
  s32 *joints; // joint -> node_idx
  s32 joint_count;
} Gltf_Skin;


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

  Gltf_Node_Info *nodes;
  s32 node_count;

  Gltf_Animation *animations;
  s32 animation_count;

  Gltf_Skin *skins;
  s32 skin_count;

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

static b32 json_parse_ints(Json_Element *root, str8 path, s32 *dst, s32 count) {
  Json_Element *node = json_lookup(root, path);
  if (!node) return false;

  s32 dst_idx = 0;
  for (Json_Element *n = node->first; n != nullptr && dst_idx < count; n=n->next, dst_idx+=1) {
    dst[dst_idx] = str8_to_int(n->value);
  }

  return true;
}

static f32 json_parse_float(Json_Element *root, str8 path, f32 def) {
  Json_Element *node = json_lookup(root, path);
  return (node) ? str8_to_float(node->value) : def;
}

static b32 json_parse_floats(Json_Element *root, str8 path, f32 *dst, s32 count) {
  Json_Element *node = json_lookup(root, path);
  if (!node) return false;

  s32 dst_idx = 0;
  for (Json_Element *n = node->first; n != nullptr && dst_idx < count; n=n->next, dst_idx+=1) {
    dst[dst_idx] = str8_to_float(n->value);
  }

  return true;
}

static v2 json_parse_vec2(Json_Element *root, str8 path, v2 def) {
  v2 v = def;
  json_parse_floats(root, path, (f32*)&v, 2);
  return v;
}

static v3 json_parse_vec3(Json_Element *root, str8 path, v3 def) {
  v3 v = def;
  json_parse_floats(root, path, (f32*)&v, 3);
  return v;
}

static v4 json_parse_vec4(Json_Element *root, str8 path, v4 def) {
  v4 v = def;
  json_parse_floats(root, path, (f32*)&v, 4);
  return v;
}

static quat json_parse_quat(Json_Element *root, str8 path, quat def) {
  quat q = def;
  json_parse_floats(root, path, (f32*)&q, 4);
  return q;
}

static m4 json_parse_mat4(Json_Element *root, str8 path, m4 def) {
  m4 m = def;
  json_parse_floats(root, path, (f32*)&m, 16);
  return m;
}

static Gltf_Transform gltf_get_transform_for_mesh(Gltf_Info *info, s32 mesh_idx) {
  Gltf_Transform trans = (Gltf_Transform){};
  for (s32 node_idx = 0; node_idx < info->node_count; node_idx+=1) {
    Gltf_Node_Info *node = &info->nodes[node_idx];
    if (node->mesh_idx == mesh_idx) {
#if 0
      return node->model_matrix;
#else
      Gltf_Transform trs = (Gltf_Transform) {
        .t = node->t,
        .r = node->r,
        .s = node->s,
      };
      trans = trs;
#endif
    }
  }

  return trans;
}
static m4 gltf_get_model_matrix_for_mesh(Gltf_Info *info, s32 mesh_idx) {
  Gltf_Transform trans = gltf_get_transform_for_mesh(info, mesh_idx);
  m4 trs = m4_mult(m4_translate(trans.t), m4_mult(m4_from_quat(trans.r), m4_scale(trans.s))); 
  return trs;
}

static Gltf_Transform gltf_matrix_to_trs(m4 m) {
  Gltf_Transform out = (Gltf_Transform){};

  out.t = v3m(m.col[3][0], m.col[3][1], m.col[3][2]);
  out.s = v3m(m.col[0][0],m.col[1][1],m.col[2][2]);
  // FIXME: We haven't translated rotations here!
  out.r = qu(0,0,0,1);

  return out;
}


static unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len);
static Gltf_Info gltf_load(Arena *arena, str8 dir, str8 json_data) {
  Gltf_Info info = {};

  Json_Element *root = json_parse(arena, json_data);
  //Json_Element* scene = json_lookup(root, STR8L("scene"));
  //assert(str8_eq(scene->label, STR8L("scene")) && str8_to_int(scene->value) == 0);


  // 0. Parse Nodes 
  Json_Element* nodes_json = json_lookup(root, STR8L("nodes"));
  info.node_count = (nodes_json) ? json_count_children(nodes_json) : 0;
  info.nodes = arena_push_array(arena, Gltf_Node_Info, info.node_count); 
  if (nodes_json) {
    s32 node_idx = 0;
    for (Json_Element *n= nodes_json->first; n != nullptr; n = n->next, node_idx+=1) {
      Gltf_Node_Info *node = &info.nodes[node_idx];

      node->mesh_idx = json_parse_int(n, STR8L("mesh"), 0);
      node->skin_idx = json_parse_int(n, STR8L("skin"), GLTF_PROPERTY_NOT_SPECIFIED);

      Json_Element* matrix_json = json_lookup(n, STR8L("matrix"));
      if (matrix_json) {
        m4 m = json_parse_mat4(n, STR8L("matrix"), m4d(1.0));
        Gltf_Transform trans = gltf_matrix_to_trs(m);
        node->model_matrix = m;
        node->t = trans.t;
        node->r = trans.r;
        node->s = trans.s;
      } else {
        v3 t =  json_parse_vec3(n, STR8L("translation"), v3m(0,0,0));
        quat r = json_parse_quat(n, STR8L("rotation"), qu(0,0,0,1));
        v3 s = json_parse_vec3(n, STR8L("scale"), v3m(1,1,1));
        m4 trs = m4_mult(m4_translate(t), m4_mult(m4_from_quat(r), m4_scale(s))); 
        node->model_matrix = trs;
        node->t = t;
        node->r = r;
        node->s = s;
      }

      node->children_count = json_count_children(n);
      node->children = arena_push_array(arena, s32, node->children_count);
      json_parse_ints(n, STR8L("children"), node->children, node->children_count);
    }
  }

  // 1. Parse meshes
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
      primitive->model_matrix = gltf_get_model_matrix_for_mesh(&info, mesh_idx);
      Gltf_Transform trans = gltf_get_transform_for_mesh(&info, mesh_idx);
      primitive->t = trans.t;
      primitive->r = trans.r;
      primitive->s = trans.s;

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

      material->emissive_factor = json_parse_vec3(m, STR8L("emissiveFactor"), v3_zero);
      material->normal_texture = json_parse_tex_info(m, STR8L("normalTexture"));
      material->emissive_texture = json_parse_tex_info(m, STR8L("emissiveTexture"));
      material->occlusion_texture = json_parse_tex_info(m, STR8L("occlusionTexture"));
    }
  }

  // 5. Parse animations
  Json_Element* animations_json = json_lookup(root, STR8L("animations"));
  info.animation_count = json_count_children(animations_json);
  info.animations = arena_push_array(arena, Gltf_Animation, info.animation_count); 

  if (info.animation_count) {
    s32 anim_idx = 0;
    for (Json_Element *a = animations_json->first; a != nullptr; a = a->next, anim_idx+=1) {
      //Json_Element *name = json_lookup(a, STR8L("name")); assert(name);
      Gltf_Animation *anim = &info.animations[anim_idx];

      // 5.1 Parse channels
      s32 channel_idx = 0;
      Json_Element *channels = json_lookup(a, STR8L("channels"));
      s32 channel_count = json_count_children(channels);
      anim->channels = arena_push_array(arena, Gltf_Animation_Channel, channel_count);
      anim->channel_count = channel_count;
      for (Json_Element *c = channels->first; c != nullptr; c = c->next, channel_idx+=1) {
        Gltf_Animation_Channel *channel = &anim->channels[channel_idx];
        channel->sampler_id = json_parse_int(c, STR8L("input"), 0);
        Json_Element *target = json_lookup(c, STR8L("target"));
        channel->target_node_id = json_parse_int(target, STR8L("node"), 0);
        Json_Element *path = json_lookup(target, STR8L("path"));
        if (str8_eq(path->value, STR8L("scale"))) {
          channel->kind = GLTF_ANIMATION_KIND_SCALE;
        } else if (str8_eq(path->value, STR8L("rotation"))) {
          channel->kind = GLTF_ANIMATION_KIND_ROTATION;
        } else {
          channel->kind = GLTF_ANIMATION_KIND_TRANSLATION;
        }
      }

      // 5.2 Parse samplers 
      s32 sampler_idx = 0;
      Json_Element *samplers = json_lookup(a, STR8L("samplers"));
      s32 sampler_count = json_count_children(samplers);
      anim->samplers = arena_push_array(arena, Gltf_Animation_Channel, sampler_count);
      anim->sampler_count = sampler_count;
  //printf("--- sampler count; %ld\n", anim->sampler_count);
      for (Json_Element *s = samplers->first; s != nullptr; s = s->next, sampler_idx+=1) {
        Gltf_Animation_Sampler *sampler = &anim->samplers[sampler_idx];
        sampler->input  = json_parse_int(s, STR8L("input"), 0);
        sampler->output = json_parse_int(s, STR8L("output"), 0);
        Json_Element *interp = json_lookup(s, STR8L("interpolation"));
        if (str8_eq(interp->value, STR8L("LINEAR"))) {
          sampler->type = GLTF_INTERP_TYPE_LINEAR;
        } else if (str8_eq(interp->value, STR8L("STEP"))) {
          sampler->type = GLTF_INTERP_TYPE_STEP;
        } else {
          sampler->type = GLTF_INTERP_TYPE_CUBIC_SPLINE;
        }
      }

    }
  }


  // 5. Parse skins 
  Json_Element* skins_json = json_lookup(root, STR8L("skins"));
  info.skin_count = json_count_children(skins_json);
  info.skins = arena_push_array(arena, Gltf_Skin, info.skin_count); 

  if (info.skin_count) {
    s32 skin_idx = 0;
    for (Json_Element *s = skins_json->first; s != nullptr; s = s->next, skin_idx+=1) {
      //Json_Element *name = json_lookup(a, STR8L("name")); assert(name);
      Gltf_Skin *skin = &info.skins[skin_idx];
      skin->inverse_bind_matrices_accessor_id = json_parse_int(s, STR8L("inverseBindMatrices"), GLTF_PROPERTY_NOT_SPECIFIED);
      skin->skeleton_node_id= json_parse_int(s, STR8L("skeleton"), 0);

      Json_Element *joints_json = json_lookup(s, STR8L("joints"));
      skin->joint_count = json_count_children(joints_json);
      skin->joints = arena_push_array(arena, s32, skin->joint_count);
      json_parse_ints(s, STR8L("joints"), skin->joints, skin->joint_count);
    }
  }


  return info;
}

static u8* gltf_data_from_accessor(Gltf_Info *info, s32 acc_idx, s32 *stride) {
  if (!gltf_is_property_specified(acc_idx)) return nullptr;

  s32 bufv_idx = info->accessors[acc_idx].bufv_idx;
  s32 buf_idx = info->buffer_views[bufv_idx].buf_idx;
  u8 *buf_data = info->buffers[buf_idx].data;
  s64 bufv_offset = info->buffer_views[bufv_idx].byte_offset;
  s64 acc_offset = info->accessors[acc_idx].byte_offset;
  if (stride) *(stride) = info->buffer_views[bufv_idx].byte_stride; 

  return (u8*)(buf_data + bufv_offset + acc_offset);
}

static Model_Info gltf_to_model(Arena *arena, Gltf_Info info) {
  Model_Info model = {};


  // FIXME FIXME FIXME: Why do we separate animations per channel
  s32 animation_count = 0;
  for (s32 anim_idx = 0; anim_idx < info.animation_count; anim_idx+=1) {
    Gltf_Animation *anim = &info.animations[anim_idx];
    for (s32 sampler_idx = 0; sampler_idx < anim->sampler_count; sampler_idx+=1) {
      animation_count += 1; 
    }
  }

  s32 running_anim_idx = 0;

  model.animation_count = animation_count;
  model.animations = arena_push_array(arena, Mesh_Animation, model.animation_count);
  for (s32 anim_idx = 0; anim_idx < info.animation_count; anim_idx+=1) {
    Gltf_Animation *anim = &info.animations[anim_idx];
    for (s32 channel_idx = 0; channel_idx < anim->channel_count; channel_idx+=1) {
      Gltf_Animation_Channel *channel = &anim->channels[channel_idx];
      s32 mesh_idx = info.nodes[channel->target_node_id].mesh_idx;
      s32 sampler_idx = channel->sampler_id;
      Gltf_Animation_Sampler *sampler = &anim->samplers[sampler_idx];
      f32 *timestamps = (f32*)gltf_data_from_accessor(&info, sampler->input, nullptr);
      s64 timestamp_count = info.accessors[sampler->input].count;
      f32 *values = (f32*)gltf_data_from_accessor(&info, sampler->output, nullptr);
      Mesh_Animation_Kind kind = MESH_ANIMATION_KIND_TRANSLATION;
      s32 component_count = 3;
      switch(channel->kind) {
        case GLTF_ANIMATION_KIND_TRANSLATION:
          kind = MESH_ANIMATION_KIND_TRANSLATION;
          component_count = 3;
          break;
        case GLTF_ANIMATION_KIND_ROTATION:
          kind = MESH_ANIMATION_KIND_ROTATION;
          component_count = 4;
          break;
        case GLTF_ANIMATION_KIND_SCALE:
          kind = MESH_ANIMATION_KIND_SCALE;
          component_count = 3;
          break;
        default:
          break;
      }
      Mesh_Animation_Interp_Type type = MESH_ANIMATION_INTERP_TYPE_LINEAR;
      switch(sampler->type) {
        case GLTF_INTERP_TYPE_LINEAR:
          type = MESH_ANIMATION_INTERP_TYPE_LINEAR;
          break;
        case GLTF_INTERP_TYPE_STEP:
          type = MESH_ANIMATION_INTERP_TYPE_STEP;
          break;
        case GLTF_INTERP_TYPE_CUBIC_SPLINE:
          type = MESH_ANIMATION_INTERP_TYPE_CUBIC_SPLINE;
          break;
        default:
          break;
      }
      Mesh_Animation new_anim = (Mesh_Animation) {
        .mesh_idx = mesh_idx,
        .kf_timestamps = arena_push_array(arena, f32, timestamp_count),
        .kf_count = timestamp_count,
        .values = arena_push_array(arena, f32, component_count*timestamp_count),
        .type = type,
        .kind = kind,
      };
      for (s32 sample = 0; sample < timestamp_count; sample+=1) {
        new_anim.kf_timestamps[sample] = timestamps[sample];
        for (s32 comp = 0; comp < component_count; comp+=1) {
          new_anim.values[component_count*sample + comp] = values[component_count*sample + comp];
        }
      }
      new_anim.max_duration = new_anim.kf_timestamps[new_anim.kf_count - 1];
      model.animations[running_anim_idx++] = new_anim;
    }
  }


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
      prim->model = gprim->model_matrix;
      prim->t = gprim->t;
      prim->r = gprim->r;
      prim->s = gprim->s;

      // FIXME: This should be used for the material description ok?! Make a UBO and everything!
      Gltf_Material *material = &info.materials[gprim->material_idx];
      assert(material);

      prim->type = ogl_prim_type_from_gltf_prim_mode(gprim->mode);
      f32 *positions = (f32*)gltf_data_from_accessor(&info, gprim->attribs.pos_idx, nullptr);

      // FIXME: For normals we have to compute them if not available..
      f32 *normals   = (f32*)gltf_data_from_accessor(&info, gprim->attribs.norm_idx, nullptr);
      f32 *tangents = (f32*)gltf_data_from_accessor(&info, gprim->attribs.tang_idx, nullptr);

      f32 *texcoords_0 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.texcoord_idx[0], nullptr);
      f32 *texcoords_1 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.texcoord_idx[1], nullptr);
      f32 *texcoords_2 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.texcoord_idx[2], nullptr);
      f32 *texcoords_3 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.texcoord_idx[3], nullptr);

      f32 *colors_0 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.col_idx[0], nullptr);
      f32 *colors_1 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.col_idx[1], nullptr);
      f32 *colors_2 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.col_idx[2], nullptr);
      f32 *colors_3 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.col_idx[3], nullptr);

      f32 *joints_0 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.joints_idx[0], nullptr);
      f32 *joints_1 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.joints_idx[1], nullptr);
      f32 *joints_2 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.joints_idx[2], nullptr);
      f32 *joints_3 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.joints_idx[3], nullptr);

      f32 *weights_0 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.weights_idx[0], nullptr);
      f32 *weights_1 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.weights_idx[1], nullptr);
      f32 *weights_2 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.weights_idx[2], nullptr);
      f32 *weights_3 = (f32*)gltf_data_from_accessor(&info, gprim->attribs.weights_idx[3], nullptr);

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
        u8 *indices = (u8*)gltf_data_from_accessor(&info, gprim->indices_idx, nullptr);
        s64 indices_count = info.accessors[gprim->indices_idx].count;
        Ogl_Buf ibo = ogl_buf_make(OGL_BUF_KIND_INDEX, OGL_BUF_HINT_STATIC, indices, indices_count, index_buf_element_size);
        prim->ibo = ibo;
      }

      // FIXME: Material handling.. GWE
      if (info.image_count > 0 && info.texture_count >0 && info.material_count > 0) {
        model.tex_id = info.images[info.textures[info.materials[0].pbr.base_color_texture.index].image_idx].id;
      }

      // Material Parsing here!
      Gltf_Material *gmat = &info.materials[gprim->material_idx];
      if (gmat && info.texture_count && info.image_count) {
        Material_Info *material = &prim->material;
        M_ZERO_STRUCT(material);

        // Base color
        material->base_color_factor = gmat->pbr.base_color_factor;
        if (gltf_is_property_specified(gmat->pbr.base_color_texture.index) &&
            gltf_is_property_specified(info.textures[gmat->pbr.base_color_texture.index].image_idx)) {
          material->base_tex = (Material_Tex) {
            .tex_asset_id = info.images[info.textures[gmat->pbr.base_color_texture.index].image_idx].id,
            .tc_idx = gmat->pbr.base_color_texture.tex_coord,
            .active = gmat->pbr.base_color_texture.active,
          };
        }

        // Metallic Roughness
        material->metallic_factor = gmat->pbr.metallic_factor;
        material->roughness_factor = gmat->pbr.roughness_factor;
        if (gltf_is_property_specified(gmat->pbr.metallic_roughness_texture.index) &&
            gltf_is_property_specified(info.textures[gmat->pbr.metallic_roughness_texture.index].image_idx)) {
          material->metallic_roughness_tex = (Material_Tex) {
            .tex_asset_id = info.images[info.textures[gmat->pbr.metallic_roughness_texture.index].image_idx].id,
            .tc_idx = gmat->pbr.metallic_roughness_texture.tex_coord,
            .active = gmat->pbr.metallic_roughness_texture.active,
          };
        }


        // Normal texture
        if (gltf_is_property_specified(gmat->normal_texture.index) &&
            gltf_is_property_specified(info.textures[gmat->normal_texture.index].image_idx)) {
          material->normal_tex = (Material_Tex) {
            .tex_asset_id = info.images[info.textures[gmat->normal_texture.index].image_idx].id,
            .tc_idx = gmat->normal_texture.tex_coord,
            .active = gmat->normal_texture.active,
          };
        }

        // Occlusion
        if (gltf_is_property_specified(gmat->occlusion_texture.index) &&
            gltf_is_property_specified(info.textures[gmat->occlusion_texture.index].image_idx)) {
          material->occlusion_tex = (Material_Tex) {
            .tex_asset_id = info.images[info.textures[gmat->occlusion_texture.index].image_idx].id,
            .tc_idx = gmat->occlusion_texture.tex_coord,
            .active = gmat->occlusion_texture.active,
          };
        }

        // Emissive
        material->emissive_factor = gmat->emissive_factor;
        if (gltf_is_property_specified(gmat->emissive_texture.index) &&
            gltf_is_property_specified(info.textures[gmat->emissive_texture.index].image_idx)) {
          material->emissive_tex = (Material_Tex) {
            .tex_asset_id = info.images[info.textures[gmat->emissive_texture.index].image_idx].id,
            .tc_idx = gmat->emissive_texture.tex_coord,
            .active = gmat->emissive_texture.active,
          };
        }

      }

      release_scratch(temp);
    }
  }

  return model;
}

#endif
