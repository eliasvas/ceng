#ifndef HASH_H__
#define HASH_H__
#include "helper.h"

static u64 djb2_buf(buf b) {
  u64 hash = 5381;
  int c;

  for (u32 i = 0; i < b.count; i+=1) {
    c = b.data[i];
    if (is_upper(c)) {
      c = c + 32;
    }
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

static inline u64 djb2(char *s) {
  return djb2_buf(MAKE_STR(s));
}

#endif
