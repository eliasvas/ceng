#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "world.h"
#include "world_serializer.h"

#define SAVEGAME_FILE STR8L(".savegame")

static u64 entity_hash_id(World *world, Entity_ID id) {
  return (entity_id(id) % world->slot_count);
}

void world_init(World *world) {
  world->entity_arena = arena_make(MB(256));
  world->frame_arena = arena_make(MB(16));

  world->entities = arena_push_array(world->entity_arena, Entity_Block, 1);
  world->entities->first_free_idx = -1;

  world->slot_count = 64;
  world->slots = arena_push_array(world->entity_arena, Entity_Hash_Slot, world->slot_count);

  world->pmgr = arena_push_array(world->entity_arena, Particle_Mgr, 1);
  particle_mgr_init(world->pmgr, world->entity_arena);

  world->rcommands = arena_push_array(world->entity_arena, Entity_Render_Command, ENTITIES_PER_BLOCK);
  world->rcommand_count = 0;
}

Entity* world_get_entity(World *world, Entity_ID id) {
  assert(id.index < ENTITIES_PER_BLOCK);
  Entity_Block *entities = world->entities;
  Entity *e = &entities->e[id.index];
  b32 alive = entities->alive[id.index];
  u32 generation = entities->gen[id.index];

  return (!alive || generation != e->id.generation) ? nullptr : e;
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

  return &entities->e[new_idx];
}

Entity* world_remove(World *world, Entity_ID eid) {
  Entity_Block *entities = world->entities;
  Entity* e = world_get_entity(world, eid);
  if (e) {
    e->kill_fn(world, e);
  }

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

u32 world_count_entities(World *world, Entity_Kind kind) {
  u32 count = 0;
  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *test = &world->entities->e[idx];
    if (world->entities->alive[idx] && test->kind == kind) count+=1;
  }

  return count;
}

// TODO: Not sure we need Game_State here, maybe world is ok? I mean the entity arena is what we fill right ?..
void world_serialize(Game_State *gs) {
  Arena *arena = gs->persistent_arena;
  assert(arena);

  World_Serializer s = wserializer_from_fullpath(arena, SAVEGAME_FILE);
  serialize_all_inc_version(&s, gs->world);
  wserializer_finish(&s);

  printf("SERIALIZE!!\n");
}

void world_deserialize(Game_State *gs) {
  Arena *arena = gs->persistent_arena;
  assert(arena);

  World_Serializer d = wdeserializer_from_fullpath(arena, SAVEGAME_FILE);
  serialize_all_inc_version(&d, gs->world);
  wdeserializer_finish(&d);

  printf("DESERIALIZE!!\n");
}


s32 bvh_count_levels(BVH_Node *node, s32 level_idx) {
  s32 max_level = level_idx;

  for (BVH_Node *sib = node->next; sib != nullptr; sib=sib->next) {
    s32 sib_level = bvh_count_levels(sib, level_idx);
    if (sib_level > max_level) {
      max_level = sib_level;
    }
  }

  for (BVH_Node *child = node->first; child!= nullptr; child=child->next) {
    s32 child_level = bvh_count_levels(child, level_idx+1);
    if (child_level > max_level) {
      max_level = child_level;
    }
  }

  return max_level;
}

s32 bvh_get_node_depth(BVH_Node *node) {
  s32 depth = 0;

  while (node) {
    depth+=1;
    node = node->parent;
  }

  return depth-1;
}

f32 ray_isect_bvh_node(BVH_Node *node, ray r) {
  f32 t_min = F32_MAX;
  v2 interseciont = ray_isect_bbox_t(r, node->box);
  if (interseciont.x < interseciont.y) {
    t_min = interseciont.x;
  }

  return t_min;
}

BVH_Node *bvh_pick(BVH_Node *node, ray r) {
  if (node->is_leaf) return node;

  f32 t_node = ray_isect_bvh_node(node, r);
  if (t_node != F32_MAX) {
    BVH_Node *left = bvh_pick(node->first, r);
    f32 t_left = (left) ? ray_isect_bvh_node(left, r) : F32_MAX;
    BVH_Node *right = bvh_pick(node->first->next, r);
    f32 t_right = (right) ? ray_isect_bvh_node(right, r) : F32_MAX;
    return (t_left < t_right) ? left : right;
  }
  return nullptr;
}


b32 bvh_collide(World *world, BVH_Node *node, bbox box, Entity_ID self_id) {
  if (!node || entity_id(node->id) == entity_id(self_id)) return false;

  f32 box_overlap = v3_min_comp(bbox_calc_overlap(node->box, box));
  if (box_overlap > 0 && node->is_leaf) {
    // Perform the collision
    Entity *e = &world->entities->e[self_id.index];
    Entity *other = &world->entities->e[node->id.index];
    e->collide_fn(world, e, other);
    return true;
  } else if (box_overlap > 0) {
    b32 lres = bvh_collide(world, node->first, box, self_id);
    b32 rres = bvh_collide(world, node->first->next, box, self_id);
    return (lres || rres);
  }
  return false;
}


void world_bvh_calc(World *world, BVH_Node *node, Entity *entities, s32 count, BVH_Split_Axis axis);
// TODO: For going FAST with bvh we need integer coordinates AND radix sort
void world_build_bvh(World *world) {
  // 0. Allocate root node
  world->bvh_root = arena_push_array(world->frame_arena, BVH_Node, 1);
  //world->bvh_root->box = bbox_from_center_hdim(v3m(0,0,0), v3m(5,5,5));
  //world->bvh_root->is_leaf = false;

  // 1. Allocate and populat the (to be) sorted entity array
  s32 ecount = world->entities->count;
  Entity *entities = arena_push_array(world->frame_arena, Entity, ecount);
  s32 alloc_idx = 0;
  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *e = &world->entities->e[idx];
    if (world->entities->alive[idx]) {
      entities[alloc_idx++] = *(e);
    }
  }

  // 2. Calculate the actual BVH structure .. (!!)
  world_bvh_calc(world, world->bvh_root, entities, ecount, BVH_AXIS_X);
}


int entity_compare_x(void *a, void *b) {
    Entity *e_a = (Entity *)a;
    Entity *e_b = (Entity *)b;

    v3 a_center = v3_add(e_a->box.pos, e_a->box.col_off);
    v3 b_center = v3_add(e_b->box.pos, e_b->box.col_off);
    if (a_center.x < b_center.x) {
      return -1;
    } else {
      return +1;
    }
}

int entity_compare_y(void *a, void *b) {
    Entity *e_a = (Entity *)a;
    Entity *e_b = (Entity *)b;

    v3 a_center = v3_add(e_a->box.pos, e_a->box.col_off);
    v3 b_center = v3_add(e_b->box.pos, e_b->box.col_off);
    if (a_center.y < b_center.y) {
      return -1;
    } else {
      return +1;
    }
}

int entity_compare_z(void *a, void *b) {
    Entity *e_a = (Entity *)a;
    Entity *e_b = (Entity *)b;

    v3 a_center = v3_add(e_a->box.pos, e_a->box.col_off);
    v3 b_center = v3_add(e_b->box.pos, e_b->box.col_off);
    if (a_center.z < b_center.z) {
      return -1;
    } else {
      return +1;
    }
}

void world_bvh_calc(World *world, BVH_Node *node, Entity *entities, s32 count, BVH_Split_Axis axis) {
  // 0. Calculate the bbox for the entity slice
  bbox super_box = entity_get_collider_bbox(&entities[0]);
  for (s32 entity_idx = 0; entity_idx < count; entity_idx +=1) {
    Entity *e = &entities[entity_idx];
    super_box = bbox_union(super_box, entity_get_collider_bbox(e));
  }

  // 1. Set other entity properties
  node->is_leaf = (count == 1);
  node->box = super_box;
  node->split_axis = axis;

  // 2. Sort entity slice based on axis
  void *comp = nullptr;
  switch (axis) {
    case BVH_AXIS_X: 
      comp = entity_compare_x;
      break;
    case BVH_AXIS_Y: 
      comp = entity_compare_y;
      break;
    case BVH_AXIS_Z: 
      comp = entity_compare_z;
      break;
    default:
      break;
  }
  qsort(entities, count, sizeof(entities[0]), comp);

  // 3. If its not leaf add left/right children + recurse
  if (!node->is_leaf) {
    BVH_Node *left = arena_push_array(world->frame_arena, BVH_Node, 1);
    left->parent = node;
    BVH_Node *right = arena_push_array(world->frame_arena, BVH_Node, 1);
    right->parent = node;

    BVH_Split_Axis new_split_axis = (axis+1) % 3;

    // because its a stack, so left is first
    sll_stack_push(node->first, right);
    sll_stack_push(node->first, left);

    world_bvh_calc(world, left, entities, count/2, new_split_axis);
    world_bvh_calc(world, right, (entities+count/2), count - (count/2), new_split_axis);
  } else {
    node->id = entities[0].id;
  }
}

void world_render_bvh(World *world, BVH_Node *node, m4 vp, rect viewport, BVH_Render_Config rc) {
  rc.clr_idx += 1;

  if (node) {
    v3 hdim = bbox_get_hdim(node->box);
    hdim = v3_multf(hdim, 2.0); // this is because the default cube is [-0.5, 0.5]
    v3 trans = bbox_get_center(node->box);
    m4 worldmat = m4_mult(m4_translate(trans), m4_scale(hdim));
    m4 mvp = m4_mult(vp, worldmat);

    for (BVH_Node *child = node->first; child != nullptr; child=child->next) {
      world_render_bvh(world, child, vp, viewport, rc);
    }

    if (rc.kind == BVH_RENDER_EVERYTHING) {
      r3d_imm_cube(viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, rc.colors[rc.clr_idx % ARRAY_COUNT(rc.colors)]);
    } else if (rc.kind == BVH_RENDER_LEVEL_BY_LEVEL) {
      // TODO: level_count is calculated for every node, VERY wasteful, optimize this!!!
      s32 level_count = bvh_count_levels(world->bvh_root, 0);

      s32 depth = bvh_get_node_depth(node);
      s32 wanted_depth = (s32)(rc.running_time_sec / rc.seconds_per_level) % (level_count+1); 
      if (depth == wanted_depth || (node->is_leaf && depth < wanted_depth)) {
        r3d_imm_cube(viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, rc.colors[rc.clr_idx % ARRAY_COUNT(rc.colors)]);
      }
    }
    //world_render_bvh(world, node->next, vp, viewport, rc);
  }
}

b32 world_entity_collides(World *world, Entity_ID id, v3 candidate_pos) {
  Entity *e = world_get_entity(world, id);
  Phys_Box col_box = e->box;
  col_box.pos = candidate_pos;

#if 1
  return bvh_collide(world, world->bvh_root, bbox_from_phys_box(&col_box), id);
#else

  for (s64 idx = 0; idx < world->entities->count; idx+=1) {
    Entity *test = &world->entities->e[idx];
    if (world->entities->alive[idx] && entity_id(test->id) != entity_id(id) ) {
      b32 test_alive = world->entities->alive[idx];
      Phys_Box testbox = test->box;

      if (entity_id(test->id) != entity_id(id) && test_alive) {
        if (bbox_isect(bbox_from_phys_box(&col_box), bbox_from_phys_box(&testbox))) {
          return test;
        }
      }
    }
  }
  return nullptr;
#endif

}

Entity *world_pick_entity(World *world, ray r) {
  BVH_Node *bvh_node = bvh_pick(world->bvh_root, r);
  if (bvh_node && bvh_node->is_leaf) {
    Entity_ID id = bvh_node->id;
    Entity *e = world_get_entity(world, id);
    return e;
  } else {
    return nullptr;
  }
}


void world_update_render(Game_State *gs, f32 dt) {
  World *world = gs->world;
  world->input = &gs->input;
  world->rcommand_count = 0;

  world_build_bvh(world);
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
  arena_clear(world->frame_arena);
}
