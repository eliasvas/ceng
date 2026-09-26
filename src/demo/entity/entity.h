#ifndef ENTITY_H__
#define ENTITY_H__
#include "base/base_inc.h"
#include "entity/entity_state_def.h"

struct Game_State;
struct World;
struct Entity;


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

typedef enum : s32 {
  ENTITY_KIND_NONE,
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
  Phys_Box box;

  color col;
  b32 dynamic;
  v3 move_dir;

  b32 grounded;
  f32 dash_timer;
  v3 dash_dir;

  Entity_Kind kind;

  // Const Data (@NoSerialize)
  void (*update_fn)(struct World *world, Entity *e, f32 dt);
  void (*draw_fn)(struct World *world, Entity *e);
  void (*kill_fn)(struct World *world, Entity *e);
  void (*collide_fn)(struct World *world, Entity *e, Entity *other);
  union {
    Hero_State_Machine hero_sm;
  };
};

static u64 entity_id(Entity_ID id) {
  return ((u64)id.generation << 32) | (id.index);
}

void entity_setup_const_data(Entity *e, Entity_Kind kind);
Entity *setup_hero(Entity *e, v3 pos);
Entity *setup_wall(Entity *e, v3 pos);
Entity *setup_coin(Entity *e, v3 pos);
Entity *setup_enemy(Entity *e, v3 pos);
Entity *setup_bullet(Entity *e, v3 pos);

// Helpers
bbox entity_get_collider_bbox(Entity *entity);
bbox bbox_from_phys_box(Phys_Box *box);

#endif
