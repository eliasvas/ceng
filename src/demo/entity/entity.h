#ifndef ENTITY_H__
#define ENTITY_H__
#include "base/base_inc.h"
#include "entity/entity_state_def.h"

// pretty much bitsquid with fat structs / entity megastruct!

struct Game_State;
struct World;
struct Entity;


typedef struct {
 v3 vel;
 v3 acc;

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
} Entity_Kind;

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

  b32 grounded;
  f32 dash_timer;
  v3 dash_dir;

  f32 angle;

  Entity_Kind kind;

  // Transforms
  transform local;
  transform world;
  m4 world_mat;

  // Tree links
  Entity *parent;
  Entity *first;
  Entity *last;
  Entity *next;
  Entity *prev;

  // Const Data (@NoSerialize)
  void (*update_fn)(struct World *world, Entity *e, f32 dt);
  void (*draw_fn)(struct World *world, Entity *e);
  void (*kill_fn)(struct World *world, Entity *e);
  void (*collide_fn)(struct World *world, Entity *e, Entity *other);
  union {
    // TODO: The callbacks should be in a static storage, why store per entity we dum?
    Hero_State_Machine hero_sm;
  };
};

static u64 entity_id(Entity_ID id) {
  return ((u64)id.generation << 32) | (id.index);
}

// FIXME: These should take a transform now !!!!!!
void entity_setup_const_data(Entity *e, Entity_Kind kind);
Entity *setup_none(Entity *e, transform xform);
Entity *setup_hero(Entity *e, transform xform);
Entity *setup_wall(Entity *e, transform xform);
Entity *setup_coin(Entity *e, transform xform);
Entity *setup_enemy(Entity *e, transform xform);
Entity *setup_bullet(Entity *e, transform xform);

// Helpers
bbox entity_get_collider_bbox(Entity *entity);
bbox bbox_from_phys_box(Phys_Box *box);

Entity *entity_add_child(Entity *parent, Entity *child);
m4 entity_get_world_transform(Entity *entity);

#endif
