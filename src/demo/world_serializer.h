#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "world.h"

// We do LBP serialization here.. maybe I should put this in the engine though
// GOATED reference: https://handmade.network/p/29/swedish-cubes-for-unity/blog/p/2723-how_media_molecule_does_serialization
// https://gist.github.com/OswaldHurlem/2a19e63760cba014b9884ff58205ea95/904b898c67d98e97da2733bce79da3f4439f5133#file-lbp_serialization-cpp-L210

enum : s32 {
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
  ADD_BASIC(SV_Initial, u32, index);
  ADD_BASIC(SV_Initial, u32, generation);
}

static void serialize_Phys_Box(World_Serializer *wserializer, Phys_Box *data) {
  ADD_BASIC(SV_Initial, v3, pos);
  ADD_BASIC(SV_Initial, v3, vel);
  ADD_BASIC(SV_Initial, v3, hdim);

  ADD_BASIC(SV_Initial, v3, col_off);
  ADD_BASIC(SV_Initial, v3, col_hdim);
  ADD_BASIC(SV_Initial, f32, mass);
}


static void serialize_Entity(World_Serializer *wserializer, Entity *data) {
  ADD(SV_Initial, Entity_ID, id);
  ADD(SV_Initial, Phys_Box, box);
  ADD_BASIC(SV_Initial, color, col);

  ADD_BASIC(SV_Initial, b32, dynamic);
  ADD_BASIC(SV_Initial, v3, move_dir); // TODO: This could not be serialized right?
  ADD_BASIC(SV_Initial, b32, grounded);
  ADD_BASIC(SV_Initial, f32, dash_timer);
  ADD_BASIC(SV_Initial, v3, dash_dir);
  ADD_BASIC(SV_Initial, s32, kind);

  // Also load the 'member' functions
  switch (data->kind) {
    case ENTITY_KIND_HERO:
      data->update_fn = update_hero;
      data->draw_fn = draw_hero;
      data->kill_fn = kill_hero;
      data->collide_fn = collide_hero;
      break;
    case ENTITY_KIND_WALL:
      data->update_fn = update_wall;
      data->draw_fn = draw_wall;
      data->kill_fn = kill_wall;
      data->collide_fn = collide_wall;
      break;
    case ENTITY_KIND_COIN:
      data->update_fn = update_coin;
      data->draw_fn = draw_coin;
      data->kill_fn = kill_coin;
      data->collide_fn = collide_coin;
      break;
    case ENTITY_KIND_ENEMY:
      //data->update_fn = update_enemy;
      //data->draw_fn = draw_enemy;
      //data->kill_fn = kill_enemy;
      //data->collide_fn = collide_enemy;
      break;
    case ENTITY_KIND_BULLET:
      //data->update_fn = update_bullet;
      //data->draw_fn = draw_bullet;
      //data->kill_fn = kill_bullet;
      //data->collide_fn = collide_bullet;
    case ENTITY_KIND_NONE:
    default:
      break;
  }
}

static void serialize_world(World_Serializer *wserializer, World *data) {
  ADD_BASIC(SV_Initial, s32, next_id);

  //for (s32 block = 0; block < 1; block+=1) {
    // Parse entities array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD(SV_Initial, Entity, entities->e[idx]);
    }
    // Parse generation array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_Initial, u32, entities->gen[idx]);
    }
    // Parse alive array 
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_Initial, b32, entities->alive[idx]);
    }
    // Parse next_idx array
    for (s32 idx = 0; idx < ENTITIES_PER_BLOCK; idx+=1) {
      ADD_BASIC(SV_Initial, u32, entities->next_idx[idx]);
    }
    // Parse first_free_idx
    ADD_BASIC(SV_Initial, s64, entities->first_free_idx);

    // Parse count
    ADD_BASIC(SV_Initial, s64, entities->count);
  //}
}

static b32 serialize_all_inc_version(World_Serializer *wserializer, World *data) {
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
  serialize_all_inc_version(&s, &world);
  wserializer_finish(&s);

  world = (World){};
  world_init(&world);

  World_Serializer d = wdeserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all_inc_version(&d, &world);
  wdeserializer_finish(&d);

  Entity *d_e1 = &world.entities[0].e[lookup.index];
  printf("deserialized_e1: col(%f, %f, %f, %f)\n", d_e1->col.r, d_e1->col.g, d_e1->col.b, d_e1->col.a);
}
#endif
