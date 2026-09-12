#ifndef WORLD_H__
#define WORLD_H__

#include "game.h"
#include "base/base_inc.h"
#include "core/core_inc.h"
#include "entity.h"

#define ENTITIES_PER_CHUNK 1024
typedef struct Entity_Chunk Entity_Chunk;
struct Entity_Chunk {
  Entity e[ENTITIES_PER_CHUNK];
  u32 gen[ENTITIES_PER_CHUNK];
  b32 alive[ENTITIES_PER_CHUNK];
  u32 next_idx[ENTITIES_PER_CHUNK];
  s64 first_free_idx;
  s64 count;

  Entity_Chunk *next;
};

typedef struct Entity_Node Entity_Node;
struct Entity_Node {
  Entity_Node *hash_next;
  Entity_Node *hash_prev;

  Entity *e;
};

typedef struct Entity_Hash_Slot Entity_Hash_Slot;
struct Entity_Hash_Slot {
  Entity_Node *hash_first;
  Entity_Node *hash_last;
};

// TODO: Make this a hash structure
typedef struct World {
  Arena *entity_arena; // @NoSerialize

  // These are per frame I think, so no need to actually serialize
  // We need two maps one for id->entity and another for coords->entity
  Entity_Hash_Slot *slots; // @NoSerialize
  u64 slot_count; // @NoSerialize

  Entity_Chunk *entities; // @Serialize
  s32 chunk_count;

  u64 next_id; // @Serialize

  // More stuff
} World;

void world_init(World *world);

Entity* world_add(World *world);
Entity* world_remove(World *world, Entity_ID id);
u32 world_count_entities(World *world, Entity_Kind kind);
void world_update_render(Game_State *gs, f32 dt);

void world_serialize(Game_State *gs);
void world_deserialize(Game_State *gs);

#endif
