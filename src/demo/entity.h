//#include "game.h"
#include "core/rend.h"

// HACK
extern void platform_play_sound(const char *sound);

typedef struct {
 HMM_Vec3 pos;
 HMM_Vec3 off; // collider offset
 HMM_Vec3 vel;
 HMM_Vec3 hdim;
 f32 mass;
} Phys_Box;

typedef enum {
  ENTITY_KIND_HERO,
  ENTITY_KIND_GRASS,
  ENTITY_KIND_EMPTY,
  ENTITY_KIND_WALL,
}Entity_Kind;

typedef u64 Entity_Id;
typedef struct {
  Entity_Id id;
  color col;

  b32 dynamic;
  Phys_Box box;
  b32 grounded;
  f32 dash_timer;
  HMM_Vec3 dash_dir;

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
typedef struct {
  Arena *entity_arena;
  u64 next_id;

  // We need two maps one for id->entity and another for coords->entity
  Entity_Hash_Slot *slots;
  u32 slot_count;

  // More stuff
} Entity_Store;

static Entity_Store entity_store;

u64 entity_hash_id(u64 entity_id) {
  return (entity_id % entity_store.slot_count);
}

Entity* entity_store_add() {
  // Allocate the entity node
  Entity_Node *en = arena_push_array(entity_store.entity_arena, Entity_Node, 1);
  en->e.id = entity_store.next_id++; 

  // Hook up to hash structure
  u64 hash_slot = entity_hash_id(en->e.id);
  dll_insert_NPZ(nullptr, entity_store.slots[hash_slot].hash_first, entity_store.slots[hash_slot].hash_last, entity_store.slots[hash_slot].hash_last, en, hash_next, hash_prev);

  return &en->e;
}

void entity_store_init() {
  entity_store.entity_arena = arena_make(MB(5));

  entity_store.slot_count = 64;
  entity_store.slots = arena_push_array(entity_store.entity_arena, Entity_Hash_Slot, entity_store.slot_count);
}

Entity *entity_store_find(Game_State *gs, Entity_Id id) {
  s64 hash_slot = entity_hash_id(id);
  Entity_Node *en = entity_store.slots[hash_slot].hash_first;
  while (en) {
    Entity *e = &(en->e);
    if (e->id == id) return e;
    en = en->hash_next;
  }
  return nullptr;
}



b32 pb_isect(Phys_Box *a, Phys_Box* b) {
  if (fabsf(a->pos.X - b->pos.X) > (a->hdim.X + b->hdim.X)) return false;
  if (fabsf(a->pos.Y - b->pos.Y) > (a->hdim.Y + b->hdim.Y)) return false;
  if (fabsf(a->pos.Z - b->pos.Z) > (a->hdim.Z + b->hdim.Z)) return false;
  return true;
}

b32 entity_collides(Game_State *gs, Entity_Id id, HMM_Vec3 candidate_pos) {
  Entity *e = entity_store_find(gs, id);
  Phys_Box ebox = e->box;
  ebox.pos = candidate_pos;
  ebox.pos = HMM_Add(ebox.pos, ebox.off);

  for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
    Entity_Node *en = entity_store.slots[hash_slot].hash_first;
    while (en) {
      Entity *test = &(en->e);
      Phys_Box testbox = test->box;
      testbox.pos = HMM_Add(testbox.pos, testbox.off);

      if (test->id != id) {
        if (pb_isect(&ebox, &testbox)) {
          return true;
        }
      }
      en = en->hash_next;
    }
  }
  return false;
}

void entity_store_update_render(Game_State *gs, f32 dt) {
  for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
    Entity_Node *en = entity_store.slots[hash_slot].hash_first;
    while (en != nullptr) {
      Entity *e = &(en->e);

      // Simulate dynamic entities
      if (e->dynamic) {
        // Movement dir
        HMM_Vec3 move_dir = HMM_V3(0,0,0);
        if (input_key_down(&gs->input, KEY_SCANCODE_RIGHT)) { move_dir.X+=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_LEFT)) { move_dir.X-=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_UP)) { move_dir.Z-=1; }
        if (input_key_down(&gs->input, KEY_SCANCODE_DOWN)) { move_dir.Z+=1; }
        //move_dir = HMM_Norm(move_dir);
        f32 speed = 5.0;
        e->box.vel.X = move_dir.X * speed;
        e->box.vel.Z = move_dir.Z * speed;

        // Dash logic
        if (e->dash_timer <= 0 && input_key_pressed(&gs->input, KEY_SCANCODE_LSHIFT)) {
          e->dash_timer = 0.1;
          e->dash_dir = move_dir;
        }
        if (e->dash_timer > 0) {
          f32 dash_scale = 50;

          e->box.vel.X = e->dash_dir.X * dash_scale; 
          e->box.vel.Z = e->dash_dir.Z * dash_scale; 

          e->dash_timer -= dt;
        } else {
          e->dash_timer = 0;
        }

        // Jump logic
        {
          // Jump logic
          f32 jump_scale = 5;
          if (input_key_pressed(&gs->input, KEY_SCANCODE_SPACE)) { 
            e->box.vel.Y = jump_scale;
          }
          f32 le_G = -9.8;
          e->box.vel.Y = lerp(e->box.vel.Y, le_G, dt);
        }
      }

      // Simple axis separated movement
      for (s32 axis = 0; axis < 3; axis += 1) {
        // TODO: Also check direction of collision ok?????

        HMM_Vec3 candidate_pos_axis = e->box.pos;
        candidate_pos_axis.Elements[axis] += e->box.vel.Elements[axis] * dt;
        b32 collides_axis = entity_collides(gs, e->id, candidate_pos_axis);
        if (!collides_axis) e->box.pos = candidate_pos_axis;
      }

      // Render le cube
      HMM_Mat4 model = HMM_MulM4(HMM_Translate(e->box.pos),HMM_Scale(HMM_Mul(e->box.hdim, 2.0f)));
      HMM_Mat4 mvp = HMM_Mul(HMM_Mul(gs->proj, gs->view), model);

      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);
      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp, v4m(0,0.0,0.0,1));

      // Iterate
      en = en->hash_next;
    }
  }
}

