#ifndef WORLD_H__
#define WORLD_H__

#include "entity.h"
#include "game.h"
#include "base/base_inc.h"
#include "core/core_inc.h"

typedef struct Particle_Mgr Particle_Mgr;
#include "particle/particle_inc.h"


/////////////////////////////////////////
// Entity basics
/////////////////////////////////////////

#define ENTITIES_PER_BLOCK 1024
typedef struct Entity_Block Entity_Block;
struct Entity_Block {
  Entity e[ENTITIES_PER_BLOCK];
  u32 gen[ENTITIES_PER_BLOCK];
  b32 alive[ENTITIES_PER_BLOCK];
  u32 next_idx[ENTITIES_PER_BLOCK];
  s64 first_free_idx;
  s64 count;

  b32 allocated;
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

/////////////////////////////////////////
// Entity Rendering
/////////////////////////////////////////

typedef struct {
  transform xform;
  Asset_Id asset_id;
  color col;

  transform collider_xform;
  color collider_col;
} Entity_Render_Command;

/////////////////////////////////////////
// BVH stuff
/////////////////////////////////////////

typedef enum {
  BVH_AXIS_X,
  BVH_AXIS_Y,
  BVH_AXIS_Z,
} BVH_Split_Axis;

typedef struct BVH_Node BVH_Node;
struct BVH_Node {
  bbox box;
  BVH_Split_Axis split_axis;

  b32 is_leaf;
  Entity_ID id;

  BVH_Node *first; // Should have only two children (if !is_leaf)
  BVH_Node *next; // Should have at most one sibling
  BVH_Node *parent;
};

typedef enum {
  BVH_RENDER_EVERYTHING,
  BVH_RENDER_LEVEL_BY_LEVEL,
} BVH_Render_Kind;

typedef struct {
#define BVH_RENDER_COLOR_COUNT 16
  color colors[BVH_RENDER_COLOR_COUNT];

  f32 seconds_per_level;
  f32 running_time_sec;

  s32 clr_idx;

  BVH_Render_Kind kind;
} BVH_Render_Config;

/////////////////////////////////////////
// World
/////////////////////////////////////////

typedef struct World {
  Arena *entity_arena; // @NoSerialize

  // These are per frame I think, so no need to actually serialize
  // We need two maps one for id->entity and another for coords->entity
  Entity_Hash_Slot *slots; // @NoSerialize
  u64 slot_count; // @NoSerialize

  // TODO: For now only 1 block supported, maybe increase this to multiple, when the time is right...
  Entity_Block *entities; // @Serialize

  s32 next_id; // @Serialize

  Particle_Mgr *pmgr; // @NoSerialize for now

  Entity_Render_Command *rcommands; // FIXME: make this a chunked array
  s32 rcommand_count;

  // @NoSerialize: These are STRICTLY per-frame data!!
  BVH_Node *bvh_root;

  Input *input; // Stolen from Game_State for now!
} World;

// World interaction
void world_init(World *world);
Entity* world_add(World *world);
Entity* world_remove(World *world, Entity_ID id);
void world_update_render(Game_State *gs, f32 dt);

// Entity queries
Entity *world_pick_entity(World *world, ray r);
Entity *world_entity_collides(World *world, Entity_ID id, v3 candidate_pos);
u32 world_count_entities(World *world, Entity_Kind kind);

// World serialization
void world_serialize(Game_State *gs);
void world_deserialize(Game_State *gs);

// World BVH stuff
void world_build_bvh(World *world);
void world_render_bvh(World *world, BVH_Node *node, m4 vp, rect viewport, BVH_Render_Config rc);

#endif
