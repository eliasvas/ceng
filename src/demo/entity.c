#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "entity.h"

u64 entity_id(Entity_ID id) {
  return ((u64)id.generation << 32) | (id.index);
}

u64 entity_hash_id(Entity_Store *store, Entity_ID id) {
  return (entity_id(id) % store->slot_count);
}

Entity* entity_store_add(Entity_Store *store) {
  Entity_Chunk *entities = store->entities;

  // 0. Check if there is opportunity for reuse from reuse_index_stack
  // TBA

  // 1. If not make a new index
  u32 next_idx = entities->count;
  M_ZERO_STRUCT(&entities->e[next_idx]);
  entities->gen[next_idx]+=1;
  entities->alive[next_idx] = true;
  entities->e[next_idx].id =  (Entity_ID){
    .index = next_idx,
    .generation = entities->gen[next_idx],
  };
  entities->count+=1;

  // 2. Hook up to hash-map (Entity_id -> Entity*)
  Entity_Node *enode = arena_push_array(store->entity_arena, Entity_Node, 1);
  enode->e = &entities->e[next_idx];
  u64 hash_slot = entity_hash_id(store, enode->e->id);
  dll_insert_NPZ(nullptr, store->slots[hash_slot].hash_first, store->slots[hash_slot].hash_last, store->slots[hash_slot].hash_last, enode, hash_next, hash_prev);

  return enode->e;
}

void entity_store_init(Entity_Store *store) {
  store->entity_arena = arena_make(MB(256));

  store->entities = arena_push_array(store->entity_arena, Entity_Chunk, 1);

  store->slot_count = 64;
  store->slots = arena_push_array(store->entity_arena, Entity_Hash_Slot, store->slot_count);
}

Entity *entity_store_find(Entity_Store *store, Entity_ID id) {
  assert(id.index < ENTITIES_PER_CHUNK);

  Entity *e = &store->entities->e[id.index];
  b32 alive = store->entities->alive[id.index];
  u32 gen = store->entities->alive[id.index];
  if (gen == id.generation && alive) {
    return e;
  }
  return nullptr;
}

b32 pb_isect(Phys_Box *a, Phys_Box* b) {
  if (fabsf(a->pos.x - b->pos.x) > (a->col_hdim.x + b->col_hdim.x)) return false;
  if (fabsf(a->pos.y - b->pos.y) > (a->col_hdim.y + b->col_hdim.y)) return false;
  if (fabsf(a->pos.z - b->pos.z) > (a->col_hdim.z + b->col_hdim.z)) return false;
  return true;
}

// FIXME: Here especially we need a spatial partition..
b32 entity_collides(Entity_Store *store, Entity_ID id, v3 candidate_pos) {
  Entity *e = entity_store_find(store, id);
  Phys_Box col_box = e->box;
  col_box.pos = v3_add(candidate_pos, col_box.col_off);

  for (s64 idx = 0; idx < store->entities->count; idx+=1) {
    Entity *test = &store->entities->e[idx];
    b32 test_alive = store->entities->alive[idx];
    Phys_Box testbox = test->box;
    testbox.pos = v3_add(testbox.pos, testbox.col_off);

    if (entity_id(test->id) != entity_id(id) && test_alive) {
      if (pb_isect(&col_box, &testbox)) {
        return true;
      }
    }
  }
  return false;
}

////////////////////////////////////////////
// Hero Entity
////////////////////////////////////////////

void update_hero(Game_State *gs, Entity *e, f32 dt) {
  Entity_Store *store = gs->entity_store;
  // Movement dir
  v3 move_dir = v3m(0,0,0);
  if (input_key_down(&gs->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }
  f32 speed = 5.0;
  e->box.vel.x = move_dir.x * speed;
  e->box.vel.z = move_dir.z * speed;

  // For reload testing
  //e->col = col(1,1,1,1);

  // Dash logic
  if (e->dash_timer <= 0 && input_key_pressed(&gs->input, KEY_SCANCODE_LSHIFT)) {
    e->dash_timer = 0.1;
    e->dash_dir = move_dir;
  }
  if (e->dash_timer > 0) {
    f32 dash_scale = 30;

    e->box.vel.x = e->dash_dir.x * dash_scale; 
    e->box.vel.z = e->dash_dir.z * dash_scale; 

    e->dash_timer -= dt;
  } else {
    e->dash_timer = 0;
  }

  // Jump logic
  {
    // Jump logic
    f32 jump_scale = 5;
    if (input_key_pressed(&gs->input, KEY_SCANCODE_SPACE)) { 
      e->box.vel.y = jump_scale;
    }
    f32 le_G = -9.8;
    e->box.vel.y = lerp(e->box.vel.y, le_G, dt);
  }
  // Simple axis separated movement
  e->move_dir = v3_norm(e->box.vel);
  for (s32 axis = 0; axis < 3; axis += 1) {
    v3 candidate_pos_axis = e->box.pos;
    candidate_pos_axis.raw[axis] += e->box.vel.raw[axis] * dt;
    b32 collides_axis = entity_collides(store, e->id, candidate_pos_axis);
    if (!collides_axis) e->box.pos = candidate_pos_axis;
  }
}

void draw_hero(Game_State *gs, Entity *e) {
  // Render le cube
  m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
  m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

  // Render le collider
  m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
      m4_mult(m4_translate(e->box.pos),
      m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
  );
  m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
      v4m(e->dynamic,e->dynamic,e->dynamic,1));
}

Entity *setup_hero(Entity *e, v3 pos) {
  e->kind = ENTITY_KIND_HERO;
  e->dynamic = true;
  e->box = (Phys_Box) {
    .pos = pos,
    .col_off = v3m(0,0,0),
    .hdim = v3m(0.3, 0.5, 0.3),
    .col_hdim = v3m(0.5,0.5,0.5),
  };
  e->col = v4m(0.9,0.4,0.3,1.0);
  return e;
}


////////////////////////////////////////////
// Wall Entity
////////////////////////////////////////////

void update_wall(Game_State *gs, Entity *e, f32 dt) {
  // TBA
}

void draw_wall(Game_State *gs, Entity *e) {
  // Render le cube
  m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
  m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

  // Render le collider
  m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
      m4_mult(m4_translate(e->box.pos),
      m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
  );
  m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
      v4m(e->dynamic,e->dynamic,e->dynamic,1));
}

Entity *setup_wall(Entity *e, v3 pos) {
  e->kind = ENTITY_KIND_WALL;
  e->dynamic = false;
  e->box = (Phys_Box) {
    .pos = pos,
    .col_off = v3m(0,0,0),
    .col_hdim = v3m(0.5,0.5,0.5),
    .hdim = v3m(0.5,0.5,0.5),
  };
  e->col = v4m(0.2,0.4,0.9,1.0);
  return e;
}


void entity_store_update_render(Game_State *gs, f32 dt) {
  Entity_Store *store = gs->entity_store;
  // Update all the entities
  for (s64 idx = 0; idx < store->entities->count; idx+=1) {
    Entity *e = &store->entities->e[idx];
    //e->update_fn(gs, e, dt);
    switch(e->kind) {
      case ENTITY_KIND_HERO:
        update_hero(gs, e, dt);
        break;
      case ENTITY_KIND_WALL:
        update_wall(gs, e, dt);
        break;
      case ENTITY_KIND_COIN:
      case ENTITY_KIND_BULLET:
      case ENTITY_KIND_ENEMY:
      default:
        break;
    }
  }

  // Draw all the entities
  for (s64 idx = 0; idx < store->entities->count; idx+=1) {
    Entity *e = &store->entities->e[idx];
    //e->update_fn(gs, e, dt);
    switch(e->kind) {
      case ENTITY_KIND_HERO:
        draw_hero(gs, e);
        break;
      case ENTITY_KIND_WALL:
        draw_wall(gs, e);
        break;
      case ENTITY_KIND_COIN:
      case ENTITY_KIND_BULLET:
      case ENTITY_KIND_ENEMY:
      default:
        break;
    }
  }
  //for (s64 idx = 0; idx < store->entities->count; idx+=1) { Entity *e = &store->entities->e[idx]; e->draw_fn(gs, e); }
  // Cleanup to-be-deleted entities
  // TBA TBA TBA TBA TBA
}
