#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "entity.h"

// GOATED reference: https://handmade.network/p/29/swedish-cubes-for-unity/blog/p/2723-how_media_molecule_does_serialization
// https://gist.github.com/OswaldHurlem/2a19e63760cba014b9884ff58205ea95/904b898c67d98e97da2733bce79da3f4439f5133#file-lbp_serialization-cpp-L210

enum : int32_t {
    SV_Initial = 1,
    SV_AddedFoo,
    SV_RemovedFoo,
    SV_RemovedBar,

    // Never remove dis
    SV_LatestPlusOne
};
#define SV_LATEST (SV_LatestPlusOne - 1)


typedef struct {
  s32 data_version;
  FILE *fptr;
  b32 is_writing;
  s32 counter;

  Arena *arena;
} Serializer;

#define serialize_basic_type(_type, _data) \
if (serializer->is_writing) { \
    fwrite(_data, sizeof(_type), 1, serializer->fptr); \
} else { \
    fread(_data, sizeof(_type), 1, serializer->fptr); \
}

void serialize_s32(Serializer *serializer, s32 *data) {
  if (serializer->is_writing) {
    fwrite(data, sizeof(s32), 1, serializer->fptr);
  } else {
    fread(data, sizeof(s32), 1, serializer->fptr);
  }
}

#define ADD_BASIC(_fieldAdded, _type, _fieldName) \
if (serializer->data_version >= (_fieldAdded)) { \
    serialize_basic_type(_type, &(data->_fieldName)); \
}

#define ADD(_fieldAdded, _type, _fieldName) \
if (serializer->data_version >= (_fieldAdded)) { \
    serialize_##_type(serializer, &(data->_fieldName)); \
}

#define ADD_LOCAL(_localAdded, _type, _localName, _defaultValue) \
_type _localName = (_defaultValue); \
if (serializer->data_version >= (_localAdded)) { \
    serialize_##_type(serializer, &(_localName)); \
}

#define REM(_fieldAdded, _fieldRemoved, _type, _fieldName, _defaultValue) \
_type _fieldName = (_defaultValue); \
if (serializer->data_version >= (_fieldAdded) && serializer->data_version < (_fieldRemoved)) { \
    serialize_##_type(serializer, &(_fieldName)); \
}

#define CHECK_INTEGRITY(_checkAdded) \
if (serializer->data_version >= (_checkAdded)) { \
    s32 check = serializer->counter; \
    serialize_s32(serializer, &check); \
    assert(check == serializer->counter++); \
}

////////////////////////////////////////////////
// Entity serialization code
////////////////////////////////////////////////

void serialize_Entity_ID(Serializer *serializer, Entity_ID *data) {
  ADD_BASIC(SV_Initial, u32, index);
  ADD_BASIC(SV_Initial, u32, generation);
}

void serialize_Phys_Box(Serializer *serializer, Phys_Box *data) {
  ADD_BASIC(SV_Initial, v3, pos);
  ADD_BASIC(SV_Initial, v3, vel);
  ADD_BASIC(SV_Initial, v3, hdim);

  ADD_BASIC(SV_Initial, v3, col_off);
  ADD_BASIC(SV_Initial, v3, col_hdim);
  ADD_BASIC(SV_Initial, f32, mass);
}


void serialize_Entity(Serializer *serializer, Entity *data) {
  ADD(SV_Initial, Entity_ID, id);
  ADD_BASIC(SV_Initial, v4, col);

  ADD_BASIC(SV_Initial, b32, dynamic);
  ADD(SV_Initial, Phys_Box, box);
  ADD_BASIC(SV_Initial, v3, move_dir); // TODO: This could not be serialized right?
  ADD_BASIC(SV_Initial, b32, grounded);
  ADD_BASIC(SV_Initial, f32, dash_timer);
  ADD_BASIC(SV_Initial, v3, dash_dir);
  ADD_BASIC(SV_Initial, s32, kind);
}

typedef struct {
  s32 count;
  Entity *e;
} Entities;
void serialize_entities(Serializer *serializer, Entities *data) {
  s32 prev_count = data->count;
  ADD_BASIC(SV_Initial, s32, count);

  // 'Realloc' the entity array if need be
  if (prev_count < data->count) {
    data->e = arena_push_array(serializer->arena, Entity, data->count);
  }

  for (s32 i = 0; i < data->count; i+=1) {
    ADD(SV_Initial, Entity, e[i]);
    // Pretty sure ADD_BASIC works currently on entities because its a POD
    // and also no migrations take place..
    //ADD_BASIC(SV_Initial, Entity, e[i]);
  }
}

b32 serialize_all_inc_version(Serializer *serializer, Entities *data) {
  if (serializer->is_writing) {
    serializer->data_version = SV_LATEST;
  }
  serialize_s32(serializer, &serializer->data_version);

  if (serializer->data_version > SV_LATEST) {
    return false;
  } else {
    serialize_entities(serializer, data);
    CHECK_INTEGRITY(serializer->counter);
    return true;
  }
}

Serializer deserializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  Serializer s = (Serializer) {
    .is_writing = false,
    .arena = arena,
  };
    s.fptr = fopen(fullpath_cstr, "rb");
  return s;
}

Serializer serializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  Serializer s = (Serializer) {
    .is_writing = true,
    .arena = arena,
  };
    s.fptr = fopen(fullpath_cstr, "wb");

  release_scratch(temp);
  return s;
}

void serializer_test(Arena *arena) {
  Entities entities = (Entities){
    .count = 2,
    .e = arena_push_array(arena, Entity, 2),
  };
  entities.e[0] = (Entity) {
    .col = col(1,0,0,0),
    .box.mass = 4.0,
  };
  entities.e[1] = (Entity) {
    .col = col(0,1,0,0),
    .box.mass = 2.0,
  };

#if 1
  Serializer s = serializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all_inc_version(&s, &entities);
  fclose(s.fptr);
#endif

  entities = (Entities){};
#if 1
  Serializer d = deserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all_inc_version(&d, &entities);
  printf("first: %f %f %f %f, mass: %f\n", entities.e[0].col.r, entities.e[0].col.g, entities.e[0].col.b, entities.e[0].col.a, entities.e[0].box.mass);
  printf("second: %f %f %f %f, mass: %f\n", entities.e[1].col.r, entities.e[1].col.g, entities.e[1].col.b, entities.e[1].col.a, entities.e[1].box.mass);
  fclose(s.fptr);
#endif

}

