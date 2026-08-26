#ifndef STR_H__
#define STR_H__

// TODO: Should buffers become regular strings? (YES)

#ifndef STR_INCLUDE_BATTERIES
#include "base/arena.h"
#include "base/base_inc.h"
#include "stdint.h"
#else
// This is so str.h can be used standalone, without the need for Arena
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int foo;}Arena;
#define arena_push_array(a, t, c) malloc(sizeof(t)*c)
#define M_CMP(a,b,c) memcmp(a,b,c)
#endif

// A UTF-8 encoded string view
typedef struct {
  uint8_t *data;
  int64_t count;
} str8;
#define STR8(S, C) (str8){(uint8_t*)(S), C}
#define STR8L(S) (str8){(uint8_t*)(S), sizeof(S) - 1}
#define STR8C(S) (str8){(uint8_t*)(S), strlen(S) - 1}
#define STR8_VARG(S) (int)(S).count, (S).data


// FIXME: I don't like this because you cant do stuff like str8_substr(s, 0, str8_find_needle(s, "##")) because
// str8_find_needle could just be uint64_t_MAX... and you have to do ternary operator bullshit.. fix this!
#define STR8_NO_MATCH 99999999

#ifndef STR_IMPLEMENTATION

int64_t cstr_count(const char *s);
char* cstr_from_str8(Arena *arena, str8 s);
str8 upper_from_str8(Arena *arena, str8 base);
str8 lower_from_str8(Arena *arena, str8 base);
bool str8_eq(str8 left, str8 right);
bool str8_starts_with(str8 s, str8 prefix);
bool str8_ends_with(str8 s, str8 postfix);
str8 str8_substr(str8 s, int64_t start_incl, int64_t end_incl);
uint64_t str8_find_needle(str8 haystack, str8 needle);
uint64_t str8_find_char(str8 haystack, char needle, b32 backward_search);
str8 str8_sprintf(Arena *arena, const char* format, ...);
s64 str8_to_int(str8 s);
f64 str8_to_float(str8 s);
str8 str8_extract_path(str8 file_path);
str8 str8_extract_filename(str8 file_path);
str8 str8_concat(Arena *arena, str8 a, str8 b);

#else

//////////////////////////////
// common string helpers
//////////////////////////////

int64_t cstr_count(const char *s) {
  int64_t count = 0;
  while (s[count]) count+=1;
  return count;
}

char char_to_upper(char c) {
  if (c >= 'a' && c <= 'z') {
    return (c-'a') + 'A';
  }
  return c;
}

char char_to_lower(char c) {
  if (c >= 'A' && c <= 'Z') {
    return (c-'A') + 'a';
  }
  return c;
}

bool char_is_alpha(char c) {
  return ((c >='a' && c<='z') || (c >= 'A' && c <= 'Z'));
}

bool char_is_num(char c) {
  return (c >= '0' && c <= '9');
}

bool char_is_alphanum(char c) {
  return (char_is_alpha(c) || char_is_num(c));
}

//////////////////////////////
// str8
//////////////////////////////

//typedef str8 buf;

str8 upper_from_str8(Arena *arena, str8 base) {
  str8 s = {
    .data = arena_push_array(arena, uint8_t, base.count),
    .count = base.count
  };
  for (int64_t i = 0; i < base.count; i+=1) {
    s.data[i] = char_to_upper(base.data[i]);
  }
  return s;
}

str8 lower_from_str8(Arena *arena, str8 base) {
  str8 s = {
    .data = arena_push_array(arena, uint8_t, base.count),
    .count = base.count
  };
  for (int64_t i = 0; i < base.count; i+=1) {
    s.data[i] = char_to_lower(base.data[i]);
  }
  return s;
}

bool str8_eq(str8 left, str8 right) {
  return (M_CMP(left.data, right.data, sizeof(uint8_t)*left.count) == 0);
}

// TODO: Maybe add match_flags or something?
uint64_t str8_find_needle(str8 haystack, str8 needle) {
  for (int64_t i = 0; i < haystack.count - needle.count + 1; i+=1) {
    str8 haystack_candidate = STR8(&haystack.data[i], needle.count);
    if (str8_eq(haystack_candidate, needle)) return (uint64_t)i;
  }

  return (uint64_t)STR8_NO_MATCH;
}

uint64_t str8_find_char(str8 haystack, char needle, b32 backward_search) {
  int64_t start_idx = (backward_search) ? haystack.count-1 : 0;
  int64_t end_idx = (backward_search) ? 0 : haystack.count;
  int64_t step = (backward_search) ? -1 : 1;
  for (int64_t i = start_idx; labs(i - end_idx-step) != 0; i+=step) {
    if (haystack.data[i] == needle) return i;
  }

  return (uint64_t)STR8_NO_MATCH;
}

bool str8_starts_with(str8 s, str8 prefix) {
  if (s.count < prefix.count) return false;
  return str8_eq(STR8(s.data, prefix.count), prefix);
}

bool str8_ends_with(str8 s, str8 postfix) {
  if (s.count < postfix.count) return false;
  return str8_eq(STR8(&s.data[s.count - postfix.count], postfix.count), postfix);
}

str8 str8_substr(str8 s, int64_t start_incl, int64_t end_incl) {
  start_incl = (start_incl > s.count) ? s.count : start_incl;
  end_incl = (end_incl > s.count) ? s.count : end_incl;

  s.data+=start_incl;
  s.count = (end_incl - start_incl)+1;

  return s;
}

str8 str8_sprintf(Arena *arena, const char* format, ...) {
  va_list args;
  va_start(args, format);

  // +1 byte for null terminator (Do we need this? FIXME)
  s32 size = vsnprintf(NULL, 0, format, args)+1;

  // Also because of push_array_nz, the 'null' terminator is not nulled ??
  char *mem = arena_push_array_nz(arena, u8, size);
  assert(mem);

  va_end(args);
  va_start(args, format);
  vsnprintf(mem, size, format, args);
  va_end(args);

  return STR8(mem, size-1);

}

// TODO: str8_list API?
// TODO: str8_path API?
// TODO: Fuzzy finding in a str8 producing a str8_list?

s64 str8_to_int(str8 s) {
  s64 counter = 0;
  for (s64 idx = s.count-1; idx >= 0; idx-=1) {
    counter += (s.data[s.count-idx-1] - '0') * pwr(10,idx);
  }
  return counter;
}

f64 str8_to_float(str8 s) {
  f64 counter = 0.0;
  s64 dot_idx = -1;
  s64 sign = 1;

  if (s.count == 0) return 0.0;

  // handle sign
  u64 start = 0;
  if (s.data[0] == '-') {
    sign = -1;
    start = 1;
  } else if (s.data[0] == '+') {
    start = 1;
  }

  // find dot index
  for (s64 i = start; i < s.count; i++) {
    if (s.data[i] == '.') {
      dot_idx = i;
      break;
    } else if (s.data[i] == 'e' || s.data[i] == 'E') {
      // cop out with scientific notation
      return 0.0;
    }
  }

  if (dot_idx == -1) {
    counter = (f64)str8_to_int(STR8(s.data + start, s.count - start));
  } else {
    // integer part
    u32 p = 0;
    for (s64 i = (s64)dot_idx - 1; i >= (s64)start; i-=1, p+=1) {
      counter += (s.data[i] - '0') * pwr(10, p);
    }

    // fractional part
    p = 1;
    for (s64 i = dot_idx + 1; i < s.count; i+=1, p+=1) {
      counter += (s.data[i] - '0') / pwr(10, p);
    }
  }

  return counter * sign;
}

char* cstr_from_str8(Arena *arena, str8 s) {
  char *buffer = arena_push_array_nz(arena, char, s.count+1);
  for (s32 idx = 0; idx < s.count; idx+=1) {
    // FIXME: make a memcpy oK?
    buffer[idx] = s.data[idx];
  }
  buffer[s.count] = '\0';

  return buffer;
}

// FIXME: We need a real str8 path api
str8 str8_extract_path(str8 file_path) {
  // FIXME: Why are paths always with '/'?
  // support delimeter with all path separatos
  s64 last_slash_idx = maximum(0,str8_find_char(file_path, '/', true));
  return str8_substr(file_path, 0, last_slash_idx);
}

str8 str8_extract_filename(str8 file_path) {
uint64_t str8_find_needle(str8 haystack, str8 needle);
  s64 last_slash_idx = maximum(0,str8_find_char(file_path, '/', true));
  return str8_substr(file_path, last_slash_idx+1, file_path.count-1);
}

str8 str8_concat(Arena *arena, str8 a, str8 b) {
  return str8_sprintf(arena, "%.*s%.*s", STR8_VARG(a), STR8_VARG(b));
}

#endif // STR_IMPLEMENTATION

#endif // STR_H__
