#ifndef STR_H__
#define STR_H__
//#include "base_inc.h"

#include "stdint.h"
#include "base/arena.h"

// TODO: Should buffers become regular strings? (YES)

/*
Intro: These are _LENGTH-BASED_ strings, all the code uses these, yes its all C.
This helper is pretty standalone save for functions accepting Arenas, Maybe TODO: If Arena type not defined make this accept malloc_proc? Also clean up the types? Is this supposed to be standalone or not? IDK. 


currently strings _are_ null terminated .. sometimes?
*/

// A UTF-8 encoded string view
typedef struct {
  u8 *data;
  s64 count;
} str8;
#define STR8(S, C) (str8){(u8*)(S), C}
#define STR8L(S) (str8){(u8*)(S), sizeof(S) - 1}
#define STR8C(S) (str8){(u8*)(S), strlen(S) - 1}
#define STR8_VARG(S) (int)(S).count, (S).data


// FIXME: I don't like this because you cant do stuff like str8_substr(s, 0, str8_find_needle(s, "##")) because
// str8_find_needle could just be U64_MAX... and you have to do ternary operator bullshit.. fix this!
#define STR8_NO_MATCH U64_MAX

#ifndef STR_IMPLEMENTATION

s64 cstr_count(const char *s);
str8 str8_from_cstr(const char *cstr);
str8 upper_from_str8(Arena *arena, str8 base);
str8 lower_from_str8(Arena *arena, str8 base);
b32 str8_eq(str8 left, str8 right);
b32 str8_starts_with(str8 s, str8 prefix);
b32 str8_ends_with(str8 s, str8 prefix);
str8 str8_substr(str8 s, s64 start_incl, s64 end_incl);
u64 str8_find_needle(str8 haystack, str8 needle);


#else

//////////////////////////////
// common string helpers
//////////////////////////////

s64 cstr_count(const char *s) {
  s64 count = 0;
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

b32 char_is_alpha(char c) {
  return ((c >='a' && c<='z') || (c >= 'A' && c <= 'Z'));
}

b32 char_is_num(char c) {
  return (c >= '0' && c <= '9');
}

b32 char_is_alphanum(char c) {
  return (char_is_alpha(c) || char_is_num(c));
}

//////////////////////////////
// str8
//////////////////////////////

//typedef str8 buf;

str8 upper_from_str8(Arena *arena, str8 base) {
  str8 s = {
    .data = arena_push_array(arena, u8, base.count),
    .count = base.count
  };
  for (s32 i = 0; i < base.count; i+=1) {
    s.data[i] = char_to_upper(base.data[i]);
  }
  return s;
}

str8 lower_from_str8(Arena *arena, str8 base) {
  str8 s = {
    .data = arena_push_array(arena, u8, base.count),
    .count = base.count
  };
  for (s32 i = 0; i < base.count; i+=1) {
    s.data[i] = char_to_lower(base.data[i]);
  }
  return s;
}

b32 str8_eq(str8 left, str8 right) {
  return (M_CMP(left.data, right.data, sizeof(u8)*left.count) == 0);
}

// TODO: Maybe add match_flags or something?
u64 str8_find_needle(str8 haystack, str8 needle) {
  for (s64 i = 0; i < haystack.count - needle.count + 1; i+=1) {
    str8 haystack_candidate = STR8(&haystack.data[i], needle.count);
    if (str8_eq(haystack_candidate, needle)) return (u64)i;
  }

  return (u64)STR8_NO_MATCH;
}

b32 str8_starts_with(str8 s, str8 prefix) {
  if (s.count < prefix.count) return false;
  return str8_eq(STR8(s.data, prefix.count), prefix);
}

b32 str8_ends_with(str8 s, str8 prefix) {
  if (s.count < prefix.count) return false;
  return str8_eq(STR8(&s.data[s.count - prefix.count], prefix.count), prefix);
}

str8 str8_substr(str8 s, s64 start_incl, s64 end_incl) {
  start_incl = (start_incl > s.count) ? s.count : start_incl;
  end_incl = (end_incl > s.count) ? s.count : end_incl;

  s.data+=start_incl;
  s.count = (end_incl - start_incl)+1;

  return s;
}

// TODO: str8_list API?
// TODO: str8_path API?
// TODO: Fuzzy finding in a str8 producing a str8_list?

#endif


#endif
