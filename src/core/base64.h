#ifndef _BASE64_H__
#define _BASE64_H__

#include "base/base_inc.h"
#include "core/core_inc.h"

// TODO: Where is bas64 encode huh????? LAZY
static str8 my_base64_encode(Arena *arena, str8 data) {
  // TBA
  return data;
}

static const u8 base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static str8 my_base64_decode(Arena *arena, str8 data) {
  s32 dec_len = 3*(data.count / 4);
  // FIXME: padding cases here right?

  u8 ascii_to_b64[256];
  for (s32 i = 0; i < 256; i += 1) {
      ascii_to_b64[i] = 0x80;
  }
  for (s32 i = 0; i < (s32)array_count(base64_table); i += 1) {
      ascii_to_b64[base64_table[i]] = (u8)i;
  }


  s32 leftover = 0;
  if (str8_find_needle(data, STR8L("==")) != STR8_NO_MATCH) leftover = 2;
  if (str8_find_needle(data, STR8L("=")) != STR8_NO_MATCH) leftover = 1;

  str8 decoded = {
    .data = arena_push_array(arena, u8, dec_len),
    .count = 0,
  };
  u8 q[3] = {};
  // Get a 4 character sequence -> treat it as ASCII -> Keep the F going
  for (s32 quad_idx = 0; quad_idx < data.count/4; quad_idx+=1) {
    u8 a = ascii_to_b64[data.data[4*quad_idx + 0]];
    u8 b = ascii_to_b64[data.data[4*quad_idx + 1]];
    u8 c = ascii_to_b64[data.data[4*quad_idx + 2]];
    u8 d = ascii_to_b64[data.data[4*quad_idx + 3]];
    q[0] = (a<<2) | (b>>4);
    q[1] = (b<<4) | (c>>2);
    q[2] = (c<<6) | (d>>0);


    decoded.data[decoded.count++] = q[0];

    b32 is_last_quad = (quad_idx+1 >= data.count/4);
    if (!(leftover == 2 && is_last_quad)) {
      decoded.data[decoded.count++] = q[1];
    }

    if (!(leftover > 0 && is_last_quad)) {
      decoded.data[decoded.count++] = q[2];
    }
  }


  return decoded;
}


static void base_64_test(Arena *arena) {
  assert(str8_eq(STR8L("Man"),my_base64_decode(arena, STR8L("TWFu"))));
  assert(str8_eq(STR8L("Ma"),my_base64_decode(arena, STR8L("TWE="))));
  assert(str8_eq(STR8L("M"),my_base64_decode(arena, STR8L("TQ=="))));
  assert(str8_eq(
      STR8L("Many hands make light work."),
      my_base64_decode(arena, STR8L("TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu")))
  );
}

#endif
