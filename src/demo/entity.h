#ifndef ENTITY_H__
#define ENTITY_H__

#include "game.h"
#include "base/base_inc.h"
#include "core/core_inc.h"

// Major inspiration: https://jorenjoestar.github.io/post/serialization_for_games/ 

// HACK
extern void platform_play_sound(const char *sound);

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
  ENTITY_KIND_ENEMY,
  ENTITY_KIND_BULLET,
}Entity_Kind;

typedef u64 Entity_Id;
typedef struct {
  Entity_Id id;
  color col;

  b32 dynamic;
  Phys_Box box;
  v3 move_dir;

  b32 grounded;
  f32 dash_timer;
  v3 dash_dir;

  Entity_Kind kind;
} Entity;

typedef struct Entity_Node Entity_Node;
struct Entity_Node {
  Entity_Node *hash_next;
  Entity_Node *hash_prev;

  Entity e;
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
  u32 slot_count;

  // More stuff
} Entity_Store;

u64 entity_hash_id(Entity_Store *store, u64 entity_id);
Entity* entity_store_add(Entity_Store *store);
void entity_store_init(Entity_Store *store);
Entity *entity_store_find(Entity_Store *store, Entity_Id id);
b32 entity_collides(Entity_Store *store, Entity_Id id, v3 candidate_pos);

void entity_store_update_render(Game_State *gs, f32 dt);


#endif
