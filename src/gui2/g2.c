#include "g2.h"

typedef struct {
  rect viewport;
  f32 g_scale;

  Arena *arena;
  Arena *temp_arena;

  Font_Info *font;
  Input *input;

  Gui_ID hot_id;
  Gui_ID active_id;

  Gui_Box_Hash_Slot *slots;
  s32 slot_count;

  Gui_Box *root;

} Gui_Ctx;

static Gui_Ctx ctx;


s32 gui_slot_idx_from_id(Gui_ID id) {
  return id % ctx.slot_count;
}

b32 gui_box_is_nil(Gui_Box *box) {
  return (box == nullptr);
}

Gui_Box *gui_box_make(Gui_ID id) {
  Gui_Box *box = nullptr;
  s32 slot_idx = gui_slot_idx_from_id(id);
  // Try to find box in hashmap
  for (Gui_Box *b = ctx.slots[slot_idx].hash_first; b != nullptr; b=b->hash_next) {
    if (b->id == id) {
      box = b;
      break;
    }
  }
  // TODO: try to pop ctx.free_boxes or somethign?
  if (gui_box_is_nil(box)) {

  }
  // Finally just allocate it..
  if (gui_box_is_nil(box)) {
    box = arena_push_array(ctx.arena, Gui_Box, 1);
    dll_push_back_NP(ctx.slots[slot_idx].hash_first, ctx.slots[slot_idx].hash_last, box, hash_next, hash_prev);
  }

  // Clear some stuff (document this better)
  {
    box->local_coords[0] = 0;
    box->local_coords[1] = 0;
    box->first = box->last = box->next = box->prev = box->parent = nullptr;
  }
  return box;
}

b32 gui_button(buf label, Gui_Layout_Params params) {
  // 0. allocate/reuse/retain box pointer
  Gui_ID id = djb2_buf(label);
  Gui_Box *box = gui_box_make(id);
  // 1. Fill the (immediate) params
  {
    box->id = id; 
    box->label = label;
    box->flags = params.flags;
    box->color_mod = 1.0;
    if (box->flags & G2_BOX_FLAG_FIXED_X) {
      box->local_coords[0] = params.fixed_x;
    }
    if (box->flags & G2_BOX_FLAG_FIXED_Y) {
      box->local_coords[1] = params.fixed_y;
    }
    box->parent = (params.parent) ? params.parent : ctx.root;
  }
  // 2. Hook into the layout structure
  Gui_Box *parent = box->parent;
  dll_push_back(parent->first, parent->last, box);

  // Button/Box logic (using previous frame's coords, AKA box->coords!)
  rect r = bfont_calc_text_rect(ctx.font, label, v2m(box->coords[0], box->coords[1]), ctx.g_scale);
  v2 bl_mp = input_get_mouse_pos(ctx.input);
  bl_mp.y = (ctx.viewport.h - bl_mp.y);
  b32 collides = rect_isect_point(r, bl_mp);
  b32 lmb_pressed = input_mkey_pressed(ctx.input, INPUT_MOUSE_LMB);
  b32 lmb_released = input_mkey_released(ctx.input, INPUT_MOUSE_LMB);

  b32 res = false;
  if (collides) {
    ctx.hot_id = id;
    if (lmb_pressed) {
      ctx.active_id = id;
    }
  }
  if (lmb_released) {
    if (ctx.hot_id == id) {
      res = true;
    }
    ctx.active_id = 0;
  }

  b32 is_hot = (ctx.hot_id == id);
  b32 is_active = (ctx.active_id == id);

  // Some color modulation to see in which state we are in
  f32 color_mod = 1.0;
  if (is_hot) color_mod = 0.3;
  if (is_active) color_mod = 0.1;
  box->color_mod = color_mod;

  return res;
}

void gui_init(Arena *tarena, Font_Info *font, Input *input) {
  ctx.arena = arena_make(MB(256));
  ctx.temp_arena = tarena;
  ctx.g_scale = 1.0;
  ctx.font = font;
  ctx.input = input;

  // Initialize hash data structure
  ctx.slot_count = 256;
  ctx.slots = arena_push_array(ctx.arena, Gui_Box_Hash_Slot, ctx.slot_count);
}

void gui_begin(rect viewport) {
  ctx.viewport = viewport;

  // Initialize a root box
  buf root_label = MAKE_STR("I_AM_ROOT");
  Gui_ID root_id = djb2_buf(root_label);
  ctx.root = gui_box_make(root_id);
  ctx.root->flags = (G2_BOX_FLAG_FIXED_X | G2_BOX_FLAG_FIXED_Y);
  ctx.root->label = root_label;
  ctx.root->local_coords[0] = 300;
  ctx.root->local_coords[1] = 300;
}

void gui_layout(Gui_Box *root) {
  if (root->flags & G2_BOX_FLAG_FIXED_X) {
    root->coords[0] = root->local_coords[0] + ((root->parent) ? root->parent->coords[0] : 0);
  }
  if (root->flags & G2_BOX_FLAG_FIXED_Y) {
    root->coords[1] = root->local_coords[1] + ((root->parent) ? root->parent->coords[1] : 0);
  }

  for (Gui_Box *child = root->first; child != nullptr; child = child->next) {
    gui_layout(child);
  }
}

void gui_render(Gui_Box *root) {
  printf("rendering box [%s] at %f %f\n", root->label.data, root->coords[0], root->coords[1]);
  bfont_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, root->label, v2m(root->coords[0], root->coords[1]), ctx.g_scale, col(root->color_mod, root->color_mod, root->color_mod,1), true);
  for (Gui_Box *child = root->first; child != nullptr; child = child->next) {
    gui_render(child);
  }
}

void gui_end() {
  gui_layout(ctx.root);
  gui_render(ctx.root);
  ctx.hot_id = 0;
}

