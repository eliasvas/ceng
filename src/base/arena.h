#ifndef _ARENA_H__
#define _ARENA_H__
#include "helper.h"
#include <stdarg.h>

#define ARENA_DEFAULT_CHUNK_SIZE KB(4)

typedef struct {
  void *backing_memory;
  u64 alignment; //typically 64
  u64 current;
  u64 reserved;
  u64 committed;
} Arena;

// AFAIK only C23 feature, we do C99 now :|
// static_assert(sizeof(Arena) < ARENA_DEFAULT_CHUNK_SIZE);

static void arena_align_forward(Arena *arena) {
  arena->current = align_pow2(UINT_FROM_PTR(arena->backing_memory) + arena->current, arena->alignment) - UINT_FROM_PTR(arena->backing_memory);
}

static void arena_destroy(Arena *arena) {
  ASAN_UNPOISON_MEMORY_REGION(arena->backing_memory, arena->reserved);
  M_RELEASE(arena->backing_memory, arena->reserved);
}

static void* arena_push_nz(Arena *arena, u64 size_in_bytes) {
  // Check if current allocation fits inside whole arena, return nullptr, TODO: we should do the linked list of arenas here sometime
  u64 remaining_reserved_bytes = arena->reserved - arena->current;
  assert(size_in_bytes <= remaining_reserved_bytes && "Allocation exceeding Arena reserved space");

  // First align forward to ensure allocation will be aligned
  arena_align_forward(arena);

  // Commit as many chunks needed (maybe 0) + Posion it
  if (arena->current + size_in_bytes > arena->committed) {
    u64 extra_bytes_needed = size_in_bytes - (arena->committed - arena->current);
    u64 chunks_needed = (extra_bytes_needed/ARENA_DEFAULT_CHUNK_SIZE) + 1;
    M_COMMIT(PTR_FROM_UINT(UINT_FROM_PTR(arena->backing_memory) + arena->committed), chunks_needed*ARENA_DEFAULT_CHUNK_SIZE);
    ASAN_POISON_MEMORY_REGION(PTR_FROM_UINT(UINT_FROM_PTR(arena->backing_memory) + arena->committed), chunks_needed*ARENA_DEFAULT_CHUNK_SIZE);
    //ASAN_UNPOISON_MEMORY_REGION(PTR_FROM_UINT(UINT_FROM_PTR(arena->backing_memory) + arena->committed), chunks_needed*ARENA_DEFAULT_CHUNK_SIZE);
    arena->committed += chunks_needed * ARENA_DEFAULT_CHUNK_SIZE;
  }

  // Return chunk of memory, increment allocator to account for it + UnPoison the region
  ASAN_UNPOISON_MEMORY_REGION(PTR_FROM_UINT(UINT_FROM_PTR(arena->backing_memory) + arena->current), size_in_bytes);
  void *ret = PTR_FROM_UINT(UINT_FROM_PTR(arena->backing_memory) + arena->current);
  arena->current += size_in_bytes;

  assert(UINT_FROM_PTR(ret) <= UINT_FROM_PTR(arena->backing_memory) + arena->current);

  assert(arena->current <= arena->committed && "Arena's current idx due to align forward has exceeded committed idx");
  assert(arena->current <= arena->reserved);
  
  return ret;
}

static void * arena_push(Arena *arena, u64 size_in_bytes) {
  void *bytes = arena_push_nz(arena, size_in_bytes);
  M_ZERO(bytes, size_in_bytes);
  return bytes;
}

// maybe we should use M_RELEASE(base, bytes) if we exceed a committed chunk boundary??
static void arena_pop(Arena *arena, u64 bytes_to_pop) {
  arena->current -= bytes_to_pop;
}

static void arena_reset_to_pos(Arena *arena, u64 pos) {
  assert(arena->current >= pos);
  arena->current = pos;
}

static u64 arena_get_current_pos(Arena *arena) {
  return arena->current;
}

#define arena_push_array(arena, s, count) arena_push((arena), sizeof(s)*(count))
#define arena_push_array_nz(arena, s, count) arena_push_nz((arena), sizeof(s)*(count))

#define arena_push_struct(arena, s) arena_push_array(arena, s, 1)
#define arena_push_struct_nz(arena, s) arena_push_array_nz(arena, s, 1)


// TODO: Maybe we should also mem_release if we crossed committed chunk boundaries
static void arena_clear(Arena *arena) {
  arena->current = sizeof(Arena);
  arena_align_forward(arena);
}

#include "stdio.h"
static Arena* arena_make_with_alignment(u64 size_in_bytes, u64 alignment) {
  u64 size_to_alloc = (size_in_bytes < ARENA_DEFAULT_CHUNK_SIZE) ? ARENA_DEFAULT_CHUNK_SIZE : size_in_bytes;

  // Reserve the whole memory and Commit only the first chunk
  void *memory = M_RESERVE(size_to_alloc);
  M_COMMIT(memory, ARENA_DEFAULT_CHUNK_SIZE);

  Arena *arena = memory;
  arena->backing_memory = memory;
  arena->alignment = alignment;
  arena->reserved = size_to_alloc;
  arena->committed = ARENA_DEFAULT_CHUNK_SIZE;

  // Use our handy arena_push to advance the current idx and align forward
  arena_push_struct_nz(arena, Arena);

  // Return the arena
  return arena;
}

static Arena* arena_make(u64 size_in_bytes) {
  return arena_make_with_alignment(size_in_bytes, 64);
}

// Here we allocated the null terminator but don't return
// the actual size, its a bit obfuscated if we wanna pop
// right after, we could leak 1 byte for each pop..
static buf arena_sprintf(Arena *arena, const char* format, ...) {
  va_list args;
  va_start(args, format);

  // +1 byte for null terminator
  s32 size = vsnprintf(NULL, 0, format, args)+1;

  // Also because of push_array_nz, the 'null' terminator is not nulled
  char *mem = arena_push_array_nz(arena, u8, size);
  assert(mem);

  va_end(args);
  va_start(args, format);
  vsnprintf(mem, size, format, args);
  va_end(args);

  return buf_make(mem, size-1);
}

#if 0
  // Arena test
  Arena* test_arena = arena_make(MB(256));
  u32 elem_count = 500;

  f32 *my_floats = arena_push(test_arena, sizeof(float)*elem_count);
  assert(my_floats);
  for (u32 i = 0; i < elem_count; ++i) { my_floats[i] = (f32)i; }

  f32 *my_floats2 = arena_push(test_arena, sizeof(float)*elem_count*2);
  assert(my_floats2);
  for (u32 i = 0; i < elem_count*2; ++i) { my_floats2[i] = (f32)i; }

  Ogl_Render_Bundle *my_unused_render_bundle = arena_push_struct(test_arena, Ogl_Render_Bundle);
  assert(my_unused_render_bundle);

  arena_clear(test_arena);
  arena_destroy(test_arena);
#endif 


#endif
