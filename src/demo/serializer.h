#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "entity.h"
#include <errno.h>
#include <unistd.h>

// GOATED reference: https://handmade.network/p/29/swedish-cubes-for-unity/blog/p/2723-how_media_molecule_does_serialization

enum : int32_t {
    SV_Initial = 1,
    SV_AddedFoo,
    SV_RemovedFoo,

    // Never remove dis
    SV_LatestPlusOne
};
#define SV_LATEST (SV_LatestPlusOne - 1)


typedef struct {
  s32 data_version;
  FILE *fptr;
  b32 is_writing;
} Serializer;

void serialize_s32(Serializer *serializer, s32 *data) {
  if (serializer->is_writing) {
    fwrite(data, sizeof(s32), 1, serializer->fptr);
  } else {
    fread(data, sizeof(s32), 1, serializer->fptr);
  }
}

#define ADD(_type, _fieldAdded, _fieldName) \
if (serializer->data_version >= (_fieldAdded)) { \
    serialize_##_type(serializer, &(data->_fieldName)); \
}

typedef struct {
  s32 bar;
  s32 foo;
} My_Large_Type;

void serialize_my_large_type(Serializer *serializer, My_Large_Type *data) {
  ADD(s32, SV_Initial, bar);   
  ADD(s32, SV_AddedFoo, foo);   
}

b32 serialize_all_inc_version(Serializer *serializer, My_Large_Type *data) {
  if (serializer->is_writing) {
    serializer->data_version = SV_LATEST;
  }
  serialize_s32(serializer, &serializer->data_version);

  if (serializer->data_version > SV_LATEST) {
    return false;
  } else {
    serialize_my_large_type(serializer, data);
    return true;
  }
}

Serializer deserializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  Serializer s = (Serializer) {
    .is_writing = false,
    .data_version = SV_LATEST, // to be determined
  };
    s.fptr = fopen(fullpath_cstr, "rb");
  return s;
}

Serializer serializer_from_fullpath(Arena *arena, str8 fullpath) {
  Temp_Arena temp = get_scratch(&arena,1);
  char* fullpath_cstr = cstr_from_str8(temp.arena, fullpath);

  Serializer s = (Serializer) {
    .is_writing = true,
    .data_version = SV_LATEST,
  };
    s.fptr = fopen(fullpath_cstr, "wb");

  if (!s.fptr) {
    printf("fopen failed for: [%s]\n", fullpath_cstr);
    printf("errno = %d: %s\n", errno, strerror(errno));
}

  release_scratch(temp);
  return s;
}

void serializer_test() {
  My_Large_Type type;
  type.foo = 42;

  Serializer s = serializer_from_fullpath(get_scratch(0,0).arena, STR8L(".savegame"));
  type.foo = 43;
  serialize_all_inc_version(&s, &type);
  fclose(s.fptr);

  type.foo = 412;

  Serializer d = deserializer_from_fullpath(get_scratch(0,0).arena, STR8L(".savegame"));
  serialize_all_inc_version(&d, &type);
  fclose(d.fptr);
  assert(type.foo == 43);
}

