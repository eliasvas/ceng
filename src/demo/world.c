#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "world.h"
#include "world_serializer.h"

// TODO: world_place(..) ?? for the acceleration structure?

static u64 entity_hash_id(World *world, Entity_ID id) {
  return (entity_id(id) % world->slot_count);
}

void world_init(World *world) {
  world->entity_arena = arena_make(MB(256));

  world->entities = arena_push_array(world->entity_arena, Entity_Block, 1);
  world->entities->first_free_idx = -1;

  world->slot_count = 64;
  world->slots = arena_push_array(world->entity_arena, Entity_Hash_Slot, world->slot_count);

  world->pmgr = arena_push_array(world->entity_arena, Particle_Mgr, 1);
  particle_mgr_init(world->pmgr, world->entity_arena);

  world->rcommands = arena_push_array(world->entity_arena, Entity_Render_Command, ENTITIES_PER_BLOCK);
  world->rcommand_count = 0;
}


Entity* world_add(World *world) {
  Entity_Block *entities = world->entities;

  // 0. Check if there is opportunity for reuse
  b32 entity_index_reuse = (entities->first_free_idx != -1);

  // 1. If not make a new index
  u32 new_idx = (entity_index_reuse) ? entities->first_free_idx : entities->count;
  if (entity_index_reuse) {
    entities->first_free_idx = entities->next_idx[new_idx];
  }
  M_ZERO_STRUCT(&entities->e[new_idx]);
  entities->gen[new_idx]+=1;
  entities->alive[new_idx] = true;
  entities->e[new_idx].id =  (Entity_ID){
    .index = new_idx,
    .generation = entities->gen[new_idx],
  };
  if (!entity_index_reuse) {
    entities->count+=1;
  }

#if 0
  // 2. Hook up to hash-map (Entity_id -> Entity*)
  Entity_Node *enode = arena_push_array(world->entity_arena, Entity_Node, 1);
  enode->e = &entities->e[new_idx];
  u64 hash_slot = entity_hash_id(world, enode->e->id);
  dll_insert_NPZ(nullptr, world->slots[hash_slot].hash_first, world->slots[hash_slot].hash_last, world->slots[hash_slot].hash_last, enode, hash_next, hash_prev);
#endif

  return &entities->e[new_idx];
}

Entity* world_remove(World *world, Entity_ID eid) {
  Entity_Block *entities = world->entities;
  Entity *e = &entities->e[eid.index];
  e->kill_fn(world, e);

  u32 prev_gen = entities->gen[eid.index];
  M_ZERO_STRUCT(&entities->e[eid.index]);
  entities->gen[eid.index] = prev_gen+1;
  entities->alive[eid.index] = false;
  entities->next_idx[eid.index] = entities->first_free_idx;
  entities->first_free_idx = eid.index;

  return nullptr;
}

s64 world_count_entity_chunks(World *world, s64 *entity_count) {
  s64 chunk_count = 0;
  Entity_Block *chunk = world->entities;

  while (chunk != nullptr) {
    chunk_count+=1;
    if (entity_count) {
      *(entity_count)+=chunk->count;
    }
    break;
    //chunk = chunk->next;
  }

  return chunk_count;
}



Entity *world_find(World *world, Entity_ID id) {
  assert(id.index < ENTITIES_PER_BLOCK);

  Entity *e = &world->entities->e[id.index];
  b32 alive = world->entities->alive[id.index];
  u32 gen = world->entities->gen[id.index];
  if (gen == id.generation && alive) {
    return e;
  }
  return nullptr;
}

u32 world_count_entities(World *world, Entity_Kind kind) {
  u32 count = 0;
  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *test = &world->entities->e[idx];
    if (world->entities->alive[idx] && test->kind == kind) count+=1;
  }

  return count;
}

b32 pb_isect(Phys_Box *a, Phys_Box* b) {
  if (fabsf(a->pos.x - b->pos.x) > (a->col_hdim.x + b->col_hdim.x)) return false;
  if (fabsf(a->pos.y - b->pos.y) > (a->col_hdim.y + b->col_hdim.y)) return false;
  if (fabsf(a->pos.z - b->pos.z) > (a->col_hdim.z + b->col_hdim.z)) return false;
  return true;
}


// FIXME: Here especially we need a spatial partition..
Entity *world_entity_collides(World *world, Entity_ID id, v3 candidate_pos) {
  Entity *e = world_find(world, id);
  Phys_Box col_box = e->box;
  col_box.pos = v3_add(candidate_pos, col_box.col_off);

  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *test = &world->entities->e[idx];
    if (world->entities->alive[idx] && entity_id(test->id) != entity_id(id) ) {
      b32 test_alive = world->entities->alive[idx];
      Phys_Box testbox = test->box;
      testbox.pos = v3_add(testbox.pos, testbox.col_off);

      if (entity_id(test->id) != entity_id(id) && test_alive) {
        if (pb_isect(&col_box, &testbox)) {
          return test;
        }
      }
    }
  }
  return nullptr;
}

// Simple linear search for now
Entity *world_pick_entity(World *world, ray r) {
  Entity *entity = nullptr;
  f32 entity_min_ray_t = F32_MAX;

  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *e = &world->entities->e[idx];


    if (world->entities->alive[idx]) {
      v3 entity_center = v3_add(e->box.pos, e->box.col_off);
      v2 intersection = ray_isect_bbox_t(r, bbox_from_center_hdim(entity_center, e->box.col_hdim));
      b32 intersected = (intersection.x < intersection.y);
      if (intersected) {
        f32 min_t = intersection.x;
        if (min_t < entity_min_ray_t) {
          entity_min_ray_t = min_t;
          entity = e;
        }
      }
    }
  }

  return entity;
}


void world_update_render(Game_State *gs, f32 dt) {
  World *world = gs->world;
  world->input = &gs->input;
  world->rcommand_count = 0;

  particle_mgr_update(world->pmgr, dt);

  // For serialization testing, not really needed tbh..
  s64 entity_count = 0;
  s64 chunk_count = world_count_entity_chunks(world, &entity_count);
  //printf("update_render %ld entities\n", entity_count);
  assert(chunk_count == 1);

  // Update all the entities
  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *e = &world->entities->e[idx];
    if (world->entities->alive[idx]) {
      e->update_fn(world, e, dt);
    }
  }

  // Draw all the entities
  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *e = &world->entities->e[idx];
    if (world->entities->alive[idx]) {
      e->draw_fn(world, e);
    }
  }


  particle_mgr_render(gs, world->pmgr);
  //for (s64 idx = 0; idx < world->entities->count; idx+=1) { Entity *e = &world->entities->e[idx]; e->draw_fn(gs, e); }
  // Cleanup to-be-deleted entities
  // TBA TBA TBA TBA TBA
}

void world_serialize(Game_State *gs) {
  Arena *arena = gs->persistent_arena;
  assert(arena);

  World_Serializer s = wserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all_inc_version(&s, gs->world);
  // FIXME: We need an api for serialization_finish or something, why call cstdlib here? we dum
  fclose(s.fptr);

  printf("SERIALIZE!!\n");
}

void world_deserialize(Game_State *gs) {
  Arena *arena = gs->persistent_arena;
  assert(arena);

  // FIXME: Should we retain the same arena? for the world? at least we need to clear? or no?
  World_Serializer d = wdeserializer_from_fullpath(arena, STR8L(".savegame"));
  serialize_all_inc_version(&d, gs->world);
  fclose(d.fptr);

  printf("DESERIALIZE!!\n");
}


