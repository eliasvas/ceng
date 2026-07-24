#ifndef HASH_H__
#define HASH_H__
#include "helper.h"

static u64 djb2_buf(u8 *data, s64 count) {
  u64 hash = 5381;
  int c;

  for (u32 i = 0; i < count; i+=1) {
    c = data[i];
    if (is_upper(c)) {
      c = c + 32;
    }
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

#endif
