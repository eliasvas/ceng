#include "rend/rend_inc.h"
#include "core/core_inc.h"
#include "game.h"
#include "entity.h"

u64 entity_hash_id(Entity_Store *store, u64 entity_id) {
  return (entity_id % store->slot_count);
}

Entity* entity_store_add(Entity_Store *store) {
  // Allocate the entity node
  Entity_Node *en = arena_push_array(store->entity_arena, Entity_Node, 1);
  en->e.id = store->next_id++; 

  // Hook up to hash structure
  u64 hash_slot = entity_hash_id(store, en->e.id);
  dll_insert_NPZ(nullptr, store->slots[hash_slot].hash_first, store->slots[hash_slot].hash_last, store->slots[hash_slot].hash_last, en, hash_next, hash_prev);

  return &en->e;
}

void entity_store_init(Entity_Store *store) {
  store->entity_arena = arena_make(MB(5));

  store->slot_count = 64;
  store->slots = arena_push_array(store->entity_arena, Entity_Hash_Slot, store->slot_count);
}

Entity *entity_store_find(Entity_Store *store, Entity_Id id) {
  s64 hash_slot = entity_hash_id(store, id);
  Entity_Node *en = store->slots[hash_slot].hash_first;
  while (en) {
    Entity *e = &(en->e);
    if (e->id == id) return e;
    en = en->hash_next;
  }
  return nullptr;
}

b32 pb_isect(Phys_Box *a, Phys_Box* b) {
  if (fabsf(a->pos.x - b->pos.x) > (a->col_hdim.x + b->col_hdim.x)) return false;
  if (fabsf(a->pos.y - b->pos.y) > (a->col_hdim.y + b->col_hdim.y)) return false;
  if (fabsf(a->pos.z - b->pos.z) > (a->col_hdim.z + b->col_hdim.z)) return false;
  return true;
}

b32 entity_collides(Entity_Store *store, Entity_Id id, v3 candidate_pos) {
  Entity *e = entity_store_find(store, id);

  Phys_Box col_box = e->box;
  col_box.pos = v3_add(candidate_pos, col_box.col_off);

  for (s64 hash_slot = 0; hash_slot < store->slot_count; hash_slot+=1) {
    Entity_Node *en = store->slots[hash_slot].hash_first;
    while (en) {
      Entity *test = &(en->e);
      Phys_Box testbox = test->box;
      testbox.pos = v3_add(testbox.pos, testbox.col_off);

      if (test->id != id) {
        if (pb_isect(&col_box, &testbox)) {
          return true;
        }
      }
      en = en->hash_next;
    }
  }
  return false;
}

void entity_store_update_render(Game_State *gs, f32 dt) {
  Entity_Store *store = gs->entity_store;
  for (s64 hash_slot = 0; hash_slot < store->slot_count; hash_slot+=1) {
    Entity_Node *en = store->slots[hash_slot].hash_first;
    while (en != nullptr) {
      Entity *e = &(en->e);

      // Simulate dynamic entities
      if (e->dynamic) {
        // Movement dir
        v3 move_dir = v3m(0,0,0);
        if (input_key_down(&gs->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }
        f32 speed = 5.0;
        e->box.vel.x = move_dir.x * speed;
        e->box.vel.z = move_dir.z * speed;

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
      }

      // Simple axis separated movement
      e->move_dir = v3_norm(e->box.vel);
      for (s32 axis = 0; axis < 3; axis += 1) {
        v3 candidate_pos_axis = e->box.pos;
        candidate_pos_axis.raw[axis] += e->box.vel.raw[axis] * dt;
        b32 collides_axis = entity_collides(store, e->id, candidate_pos_axis);
        if (!collides_axis) e->box.pos = candidate_pos_axis;
      }

      // Render le cube
      m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
      m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

      // Render le collider
      m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
          m4_mult(m4_translate(e->box.pos),
          m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
      );
      m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
          v4m(e->dynamic,e->dynamic,e->dynamic,1));

      // Iterate
      en = en->hash_next;
    }
  }
}
