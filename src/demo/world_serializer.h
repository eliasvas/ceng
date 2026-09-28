#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "world.h"

// We do LBP serialization here.. maybe I should put this in the engine though
// GOATED reference: https://handmade.network/p/29/swedish-cubes-for-unity/blog/p/2723-how_media_molecule_does_serialization
// https://gist.github.com/OswaldHurlem/2a19e63760cba014b9884ff58205ea95/904b898c67d98e97da2733bce79da3f4439f5133#file-lbp_serialization-cpp-L210

enum : s32 {
    SV_INITIAL = 1,
    SV_ADDED_FOO,
    SV_REMOVED_FOO,
    SV_REMOVED_BAR,

    // Never remove dis
    SV_LATEST_PLUS_ONE
};
#define SV_LATEST (SV_LATEST_PLUS_ONE - 1)

typedef struct {
  s32 data_version;
  FILE *fptr;
  b32 is_writing;
  s32 counter;

  Arena *arena;
  World *world_ref;
} World_Serializer;

#define serialize_basic_type(_type, _data) \
if (wserializer->is_writing) { \
    fwrite(_data, sizeof(_type), 1, wserializer->fptr); \
} else { \
    fread(_data, sizeof(_type), 1, wserializer->fptr); \
}

static void serialize_s32(World_Serializer *wserializer, s32 *data) {
  if (wserializer->is_writing) {
    fwrite(data, sizeof(s32), 1, wserializer->fptr);
  } else {
    fread(data, sizeof(s32), 1, wserializer->fptr);
  }
}

#define ADD_BASIC(_fieldAdded, _type, _fieldName) \
if (wserializer->data_version >= (_fieldAdded)) { \
    serialize_basic_type(_type, &(data->_fieldName)); \
}

#define ADD(_fieldAdded, _type, _fieldName) \
if (wserializer->data_version >= (_fieldAdded)) { \
    serialize_##_type(wserializer, &(data->_fieldName)); \
}

#define ADD_LOCAL(_localAdded, _type, _localName, _defaultValue) \
_type _localName = (_defaultValue); \
if (wserializer->data_version >= (_localAdded)) { \
    serialize_##_type(wserializer, &(_localName)); \
}


#define ADD_BASIC_LOCAL(_localAdded, _type, _localName, _defaultValue) \
_type _localName = (_defaultValue); \
if (wserializer->data_version >= (_localAdded)) { \
    serialize_basic_type(_type, &(_localName)); \
}

#define REM(_fieldAdded, _fieldRemoved, _type, _fieldName, _defaultValue) \
_type _fieldName = (_defaultValue); \
if (wserializer->data_version >= (_fieldAdded) && wserializer->data_version < (_fieldRemoved)) { \
    serialize_##_type(wserializer, &(_fieldName)); \
}

#define CHECK_INTEGRITY(_checkAdded) \
if (wserializer->data_version >= (_checkAdded)) { \
    s32 check = wserializer->counter; \
    serialize_s32(wserializer, &check); \
    assert(check == wserializer->counter++); \
}

////////////////////////////////////////////////
// Entity serialization code
////////////////////////////////////////////////

static void serialize_Entity_ID(World_Serializer *wserializer, Entity_ID *data) {
  ADD_BASIC(SV_INITIAL, u32, index);
  ADD_BASIC(SV_INITIAL, u32, generation);
}

static void serialize_Phys_Box(World_Serializer *wserializer, Phys_Box *data) {
  ADD_BASIC(SV_INITIAL, v3, vel);
  ADD_BASIC(SV_INITIAL, v3, acc);

  ADD_BASIC(SV_INITIAL, v3, col_off);
  ADD_BASIC(SV_INITIAL, v3, col_hdim);
  ADD_BASIC(SV_INITIAL, f32, mass);
}

static void serialize_Entity(World_Serializer *wserializer, Entity *data) {
  ADD(SV_INITIAL, Entity_ID, id);
  ADD(SV_INITIAL, Phys_Box, box);
  ADD_BASIC(SV_INITIAL, color, col);

  ADD_BASIC(SV_INITIAL, b32, dynamic);
  ADD_BASIC(SV_INITIAL, f32, angle);
  ADD_BASIC(SV_INITIAL, b32, grounded);
  ADD_BASIC(SV_INITIAL, f32, dash_timer);
  ADD_BASIC(SV_INITIAL, v3, dash_dir);
  ADD_BASIC(SV_INITIAL, s32, kind);
  ADD_BASIC(SV_INITIAL, transform, local);
  ADD_BASIC(SV_INITIAL, transform, world);
  ADD_BASIC(SV_INITIAL, m4, world_mat);

  // TODO: This is _Kinda_ hacky.. maybe do a cleanup
  // Entity pointer serialization
#define INVALID_SERIAL_IDX U64_MAX
  if (wserializer->is_writing) {
    ADD_BASIC_LOCAL(SV_INITIAL, u64, parent_idx, (data->parent) ? UINT_FROM_PTR(data->parent) - UINT_FROM_PTR(wserializer->world_ref->entities->e) : INVALID_SERIAL_IDX);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, first_idx, (data->first) ? UINT_FROM_PTR(data->first) - UINT_FROM_PTR(wserializer->world_ref->entities->e) : INVALID_SERIAL_IDX);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, last_idx, (data->last) ? UINT_FROM_PTR(data->last) - UINT_FROM_PTR(wserializer->world_ref->entities->e) : INVALID_SERIAL_IDX);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, next_idx, (data->next) ? UINT_FROM_PTR(data->next) - UINT_FROM_PTR(wserializer->world_ref->entities->e) : INVALID_SERIAL_IDX);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, prev_idx, (data->prev) ? UINT_FROM_PTR(data->prev) - UINT_FROM_PTR(wserializer->world_ref->entities->e) : INVALID_SERIAL_IDX);
  } else {
    ADD_BASIC_LOCAL(SV_INITIAL, u64, parent_idx, 0);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, first_idx, 0);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, last_idx, 0);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, next_idx, 0);
    ADD_BASIC_LOCAL(SV_INITIAL, u64, prev_idx, 0);

    data->parent = (parent_idx == INVALID_SERIAL_IDX) ? nullptr : &wserializer->world_ref->entities->e[parent_idx/sizeof(Entity)];
    data->first = (first_idx == INVALID_SERIAL_IDX) ? nullptr : &wserializer->world_ref->entities->e[first_idx/sizeof(Entity)];
    data->last = (last_idx == INVALID_SERIAL_IDX) ? nullptr : &wserializer->world_ref->entities->e[last_idx/sizeof(Entity)];
    data->next = (next_idx == INVALID_SERIAL_IDX) ? nullptr : &wserializer->world_ref->entities->e[next_idx/sizeof(Entity)];
    data->prev = (prev_idx == INVALID_SERIAL_IDX) ? nullptr : &wserializer->world_ref->entities->e[prev_idx/sizeof(Entity)];
  }

  entity_setup_const_data(data, data->kind);
}

static void serialize_world(World_Serializer *wserializer, World *data) {
  ADD_BASIC(SV_INITIAL, s32, next_id);

  //for (s32 block = 0; block < 1; block+=1) {
    // Parse entities array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD(SV_INITIAL, Entity, entities->e[idx]);
    }
    // Parse generation array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_INITIAL, u32, entities->gen[idx]);
    }
    // Parse alive array 
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_INITIAL, b32, entities->alive[idx]);
    }
    // Parse next_idx array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_INITIAL, u32, entities->next_idx[idx]);
    }
    // Parse first_free_idx
    ADD_BASIC(SV_INITIAL, s64, entities->first_free_idx);

    // Parse count
    ADD_BASIC(SV_INITIAL, s64, entities->count);
  //}
}

static b32 serialize_all(World_Serializer *wserializer, World *data) {
  wserializer->world_ref = data;
  if (wserializer->is_writing) {
    wserializer->data_version = SV_LATEST;
  }
  serialize_s32(wserializer, &wserializer->data_version);

  if (wserializer->data_version > SV_LATEST) {
    return false;
  } else {
    serialize_world(wserializer, data);
    //CHECK_INTEGRITY(wserializer->counter);
    return true;
  }
}

static World_Serializer wserializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  World_Serializer s = (World_Serializer) {
    .is_writing = true,
    .arena = arena,
  };
  s.fptr = fopen(fullpath_cstr, "wb");

  release_scratch(temp);
  return s;
}
static void wserializer_finish(World_Serializer *serializer) {
  fclose(serializer->fptr);
}

static World_Serializer wdeserializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  World_Serializer s = (World_Serializer) {
    .is_writing = false,
    .arena = arena,
  };
  s.fptr = fopen(fullpath_cstr, "rb");
  release_scratch(temp);

  return s;
}

static void wdeserializer_finish(World_Serializer *deserializer) {
  wserializer_finish(deserializer);
}

#if 0
static void wserializer_test(Arena *arena) {
  World world;
  world_init(&world);

  Entity *e0 = world_add(&world);
  e0->col = v4m(1,0,0,0);
  Entity *e1 = world_add(&world);
  e1->col = v4m(0,1,0,0);
  Entity *e2 = world_add(&world);
  e2->col = v4m(0,0,1,0);
  Entity_ID lookup = e1->id;

  World_Serializer s = wserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all(&s, &world);
  wserializer_finish(&s);

  world = (World){};
  world_init(&world);

  World_Serializer d = wdeserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all(&d, &world);
  wdeserializer_finish(&d);

  Entity *d_e1 = &world.entities[0].e[lookup.index];
  printf("deserialized_e1: col(%f, %f, %f, %f)\n", d_e1->col.r, d_e1->col.g, d_e1->col.b, d_e1->col.a);
}
#endif
