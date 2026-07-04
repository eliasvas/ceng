//#include "game.h"
#include "core/rend.h"

// HACK
extern void platform_play_sound(const char *sound);

typedef enum {
  ENTITY_KIND_HERO,
  ENTITY_KIND_GRASS,
  ENTITY_KIND_EMPTY,
  ENTITY_KIND_WALL,
}Entity_Kind;

typedef u64 Entity_Id;
typedef struct {
  Entity_Id id;

  b32 dynamic;
  v3 size;

  // This way entity position is always animate-able property
  v3 pos;
  v3 vel;

  b32 grounded;
  f32 dash_timer;
  f32 dash_cooldown;
  v3 dash_dir;

  rect tex_coords;
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

Entity *entity_store_find(Game_State *gs, v3 coords) {
  for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
    Entity_Node *en = entity_store.slots[hash_slot].hash_first;
    while (en) {
      Entity *e = &(en->e);
      v3 anim_coords = e->pos;
      if (anim_coords.x == coords.x && anim_coords.y == coords.y && anim_coords.z == coords.z) return e;
      en = en->hash_next;
    }
  }
  return nullptr;
}

void entity_store_update_render(Game_State *gs, f32 dt) {
  for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
    Entity_Node *en = entity_store.slots[hash_slot].hash_first;
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
        move_dir = v3_norm(move_dir);
        f32 speed = 5.0;
        e->vel.x = move_dir.x * speed;
        e->vel.z = move_dir.z * speed;

        // Dash logic
        if (e->dash_timer <= 0 && input_key_pressed(&gs->input, KEY_SCANCODE_LSHIFT)) {
          e->dash_timer = 0.1;
          e->dash_dir = move_dir;
        }
        if (e->dash_timer > 0) {
          f32 dash_scale = 50;

          e->vel.x = e->dash_dir.x * dash_scale; 
          e->vel.z = e->dash_dir.z * dash_scale; 
          //e->vel.y = 0;

          e->dash_timer -= dt;
        } else {
          e->dash_timer = 0;
        }

        // Jump logic
        {
          // Jump logic
          f32 jump_scale = 5;
          if (e->grounded && input_key_pressed(&gs->input, KEY_SCANCODE_SPACE))
            { e->vel.y = jump_scale; e->grounded = false;}
          if (!e->grounded && e->pos.y >= 0) {
            f32 le_G = 9.8;
            e->vel.y -= le_G * dt;
            e->grounded = false;
          } else {
            e->grounded = true;
            e->pos.y = 0;
            e->vel.y = 0;
          }
        }

        // Calc final added position (needed also for collision checks right now
        e->pos = v3_add(e->pos, v3_multf(e->vel, dt));

      }

      // Render
      m4 model = m4_mult(m4_translate(e->pos), m4_translate(v3m(0.5, 0.5, 0.5)));
      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, gs->view_mat, model, v4m(1,0.2,0.2,1));
      rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, gs->view_mat, model, v4m(0,0.0,0.0,1));

      // Iterate
      en = en->hash_next;
    }
  }
}

