#ifndef STR_H__
#define STR_H__
#include "base_inc.h"

// TODO: Should buffers become regular strings?

typedef struct {
  u8 *data;
  s64 count;
} str8;

#ifndef STR_IMPLEMENTATION

s64 cstr_count(const char *s);
s64 str8_count(str8 *s);
str8 str8_from_cstr(const char *cstr);

#else

//////////////////////////////
// cstr helpers
//////////////////////////////

s64 cstr_count(const char *s) {
  s64 count = 0;
  while (s[count]) count+=1;
  return count;
}

//////////////////////////////
// str8
//////////////////////////////

//typedef str8 buf;

s64 str8_count(str8 *s) {
  return (s) ? s->count : 0;
}

str8 str8_from_cstr(const char *cstr) {
  return (str8) {.data = (u8*)cstr, .count = cstr_count(cstr)};
}

#endif


#endif
