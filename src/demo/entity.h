#include "game.h"

// TODO: add this to.. math? maybe?
typedef struct {
  s32 x,y,z;
} iv3;


typedef enum {
  ENTITY_KIND_HERO,
  ENTITY_KIND_GRASS,
  ENTITY_KIND_EMPTY,
  ENTITY_KIND_WALL,
}Entity_Kind;

typedef u64 Entity_Id;
typedef struct {
  Entity_Id id;
  iv3 coords;
  rect tex_coords;
  Entity_Kind kind;
  s32 layer; // 0..1 currently
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

// TODO: Temoval example (reuse via a freelist)
#if 0
	// prune unused boxes
	for (u32 hash_slot = 0; hash_slot < state->slot_count; hash_slot+=1) {
		for (Gui_Box *box = state->slots[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			if (box->last_used_frame_idx < state->frame_idx) {
				dll_remove_NPZ(gui_box_nil_id(), state->slots[hash_slot].hash_first, state->slots[hash_slot].hash_last,box,hash_next,hash_prev);
				sll_stack_push(state->box_freelist, box);
			}
		}
	}
#endif

Entity* entity_store_add() {
  // Allocate the entity node
  Entity_Node *en = arena_push_array(entity_store.entity_arena, Entity_Node, 1);
  en->e.id = entity_store.next_id++; 
  en->e.layer = 0; // bg_layer is 0, dynamic_layer is 1

  // Hook up to hash structure
  u64 hash_slot = entity_hash_id(en->e.id);
  dll_insert_NPZ(nullptr, entity_store.slots[hash_slot].hash_first, entity_store.slots[hash_slot].hash_last, entity_store.slots[hash_slot].hash_last, en, hash_next, hash_prev);

  return &en->e;
}

void entity_store_add_map(const char *map) {
  iv3 coords = {0,0,0};
  for (s32 c_idx = 0; c_idx < strlen(map); c_idx+=1) {
    char c = map[c_idx];
    Entity* e = nullptr;
    switch(c) {
      case '@':
        e = entity_store_add();
        e->tex_coords = rec(3*8,5*8,8,8);
        e->kind = ENTITY_KIND_WALL;
        e->coords = coords;
        coords.x+=1;
        break;
      case '#':
        e = entity_store_add();
        e->tex_coords = rec(4*8,5*8,8,8);
        e->kind = ENTITY_KIND_GRASS;
        e->coords = coords;
        coords.x+=1;
        break;
      case '$':
        e = entity_store_add();
        e->tex_coords = rec(5*8,5*8,8,8);
        e->kind = ENTITY_KIND_EMPTY;
        e->coords = coords;
        coords.x+=1;
        break;
      case '\n':
        coords.x = 0;
        coords.y += 1;
        break;
      default:
        break;
    }
  }
}

void entity_store_init() {
  entity_store.entity_arena = arena_make(MB(5));

  entity_store.slot_count = 64;
  entity_store.slots = arena_push_array(entity_store.entity_arena, Entity_Hash_Slot, entity_store.slot_count);
}

Entity *entity_store_find(Game_State *gs, iv3 coords) {
  for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
    Entity_Node *en = entity_store.slots[hash_slot].hash_first;
    while (en) {
      Entity *e = &(en->e);
      if (e->coords.x == coords.x && e->coords.y == coords.y && e->coords.z == coords.z) return e;
      en = en->hash_next;
    }
  }
  return nullptr;
}

void entity_store_update_render(Game_State *gs) {
  // TODO: layer range should be inside entity_store
  for (s32 layer = 0; layer <=1; layer+=1) {
    for (s64 hash_slot = 0; hash_slot < entity_store.slot_count; hash_slot+=1) {
      Entity_Node *en = entity_store.slots[hash_slot].hash_first;
      while (en) {
        Entity *e = &(en->e);
        if (e->layer == layer) {
          f32 tile_w_px = 8;

          switch(e->kind) {
            case ENTITY_KIND_HERO:

              iv3 next_tile_coords = e->coords;
              if (input_key_pressed(&gs->input, KEY_SCANCODE_RIGHT)) { next_tile_coords.x+=1; }
              else if (input_key_pressed(&gs->input, KEY_SCANCODE_LEFT)) { next_tile_coords.x-=1; }
              else if (input_key_pressed(&gs->input, KEY_SCANCODE_UP)) { next_tile_coords.y+=1; }
              else if (input_key_pressed(&gs->input, KEY_SCANCODE_DOWN)) { next_tile_coords.y-=1; }

              Entity *next_tile = entity_store_find(gs, next_tile_coords);
              if (next_tile && next_tile->kind != ENTITY_KIND_WALL) {
                e->coords = next_tile_coords;
              }
              break;
            case ENTITY_KIND_GRASS:
            case ENTITY_KIND_EMPTY:
            case ENTITY_KIND_WALL:
              break;
            default:
              break;
          }

          R2D_Quad quad = (R2D_Quad) {
              .src_rect = en->e.tex_coords,
              .dst_rect = rec(en->e.coords.x * tile_w_px, en->e.coords.y * tile_w_px, tile_w_px, tile_w_px),
              .c = col(1,1,1,1),
              .tex = gs->atlas,
              .rot_deg = 0,
          };

          R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
          r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
        }

        en = en->hash_next;
      }
    }
  }
}

