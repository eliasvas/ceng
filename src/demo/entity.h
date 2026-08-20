#ifndef ENTITY_H__
#define ENTITY_H__

#include "game.h"
#include "base/base_inc.h"
#include "core/core_inc.h"

// Generation scheme taken from here, I think? https://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html

// TODO: Read this about serialization: https://jorenjoestar.github.io/post/serialization_for_games/ 

typedef struct {
 v3 pos;
 v3 vel;

 // Tihs hdim maybe should go inside entity?
 // not really connected to physics
 v3 hdim;

 // Collider offset and dimension
 v3 col_off;
 v3 col_hdim;
 f32 mass;
} Phys_Box;

typedef enum {
  ENTITY_KIND_HERO,
  ENTITY_KIND_WALL,
  ENTITY_KIND_COIN,
  ENTITY_KIND_ENEMY,
  ENTITY_KIND_BULLET,
}Entity_Kind;

typedef struct  {
  u32 index; // index to entity array
  u32 generation; // destroyed entities w/ same ID count 
} Entity_ID;

typedef struct Entity Entity;
struct Entity {
  Entity_ID id;
  color col;

  b32 dynamic;
  Phys_Box box;
  v3 move_dir;

  b32 grounded;
  f32 dash_timer;
  v3 dash_dir;

  Entity_Kind kind;
};

#define ENTITIES_PER_CHUNK 1024
typedef struct Entity_Chunk Entity_Chunk;
struct Entity_Chunk {
  Entity e[ENTITIES_PER_CHUNK];
  u32 gen[ENTITIES_PER_CHUNK];
  b32 alive[ENTITIES_PER_CHUNK];
  u32 *reuse_index_stack;
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
typedef struct Entity_Store {
  Arena *entity_arena;
  u64 next_id;

  // We need two maps one for id->entity and another for coords->entity
  Entity_Hash_Slot *slots;
  u64 slot_count;

  Entity_Chunk *entities;

  // More stuff
} Entity_Store;

Entity* entity_store_add(Entity_Store *store);
void entity_store_init(Entity_Store *store);
u64 entity_hash_id(Entity_Store *store, Entity_ID id);
b32 entity_collides(Entity_Store *store, Entity_ID id, v3 candidate_pos);
void entity_store_update_render(Game_State *gs, f32 dt);

Entity *setup_hero(Entity *e, v3 pos);
Entity *setup_wall(Entity *e, v3 pos);

#endif
