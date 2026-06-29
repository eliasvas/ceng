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
  Gui_Box *box_freelist;

  Gui_Box *root;
  s64 frame_idx;

} Gui_Ctx;

static Gui_Ctx ctx;

s32 gui_slot_idx_from_id(Gui_ID id) {
  return id % ctx.slot_count;
}

b32 gui_box_is_nil(Gui_Box *box) {
  return (box == nullptr);
}

Gui_Box *gui_box_make(buf label, Gui_ID id, Gui_Box_Flags flags) {
  Gui_Box *box = nullptr;
  s32 slot_idx = gui_slot_idx_from_id(id);
  // Try to find box in hashmap
  for (Gui_Box *b = ctx.slots[slot_idx].hash_first; b != nullptr; b=b->hash_next) {
    if (b->id == id) {
      box = b;
      break;
    }
  }
  // Try to reuse a Gui_Box from the freelist
  if (gui_box_is_nil(box)) {
    box = ctx.box_freelist;
    sll_stack_pop(ctx.box_freelist); 
  }
  // Finally (if freelist is empty) just allocate it from the arena..
  if (gui_box_is_nil(box)) {
    box = arena_push_array(ctx.arena, Gui_Box, 1);
  }
  // Plug the box to the persistent hashmap
  dll_push_back_NP(ctx.slots[slot_idx].hash_first, ctx.slots[slot_idx].hash_last, box, hash_next, hash_prev);

  // Clear some stuff (document this better)
  {
    box->id = id;
    box->label = label;
    box->flags = flags;
    box->local_coords[GUI_AXIS_X] = 0;
    box->local_coords[GUI_AXIS_Y] = 0;
    box->first = box->last = box->next = box->prev = box->parent = nullptr;
    box->color_mod = 1.0;
    box->last_frame_used = ctx.frame_idx;
  }
  return box;
}

b32 gui_button(buf label, Gui_Layout_Params params) {
  // 0. allocate/reuse/retain box pointer
  Gui_ID id = djb2_buf(label);
  Gui_Box *box = gui_box_make(label, id, params.flags);
  // 1. Fill the (immediate) params
  {
    box->color_mod = 1.0;
    if (box->flags & G2_BOX_FLAG_FIXED_X) {
      box->local_coords[GUI_AXIS_Y] = params.fixed_x;
    }
    if (box->flags & G2_BOX_FLAG_FIXED_Y) {
      box->local_coords[GUI_AXIS_Y] = params.fixed_y;
    }
    box->parent = (params.parent) ? params.parent : ctx.root;
  }
  // 2. Hook into the layout structure
  Gui_Box *parent = box->parent;
  dll_push_back(parent->first, parent->last, box);

  // 3. Button/Box logic (using previous frame's coords, AKA box->coords!)
  rect r = bfont_calc_text_rect(ctx.font, label, v2m(box->coords[GUI_AXIS_X], box->coords[GUI_AXIS_Y]), ctx.g_scale);
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
  ctx.frame_idx = 0;
  ctx.box_freelist = nullptr;

  // Initialize hash data structure
  ctx.slot_count = 256;
  ctx.slots = arena_push_array(ctx.arena, Gui_Box_Hash_Slot, ctx.slot_count);
}

void gui_begin(rect viewport) {
  // Advance frame index (used for box pruning)
  ctx.frame_idx+=1;

  ctx.viewport = viewport;
  // Initialize a root box
  buf root_label = MAKE_STR("ROOTBOX");
  Gui_ID root_id = djb2_buf(root_label);
  ctx.root = gui_box_make(root_label, root_id, G2_BOX_FLAG_FIXED_X | G2_BOX_FLAG_FIXED_Y);
  ctx.root->local_coords[0] = 300;
  ctx.root->local_coords[1] = 300;
}

void gui_layout(Gui_Box *root) {
  for (s32 layout_axis = GUI_AXIS_X; layout_axis <= GUI_AXIS_Y; layout_axis+=1) {
    root->coords[layout_axis] = root->local_coords[layout_axis] + ((root->parent) ? root->parent->coords[layout_axis] : 0);
  }
  for (Gui_Box *child = root->first; child != nullptr; child = child->next) {
    gui_layout(child);
  }
}

void gui_render(Gui_Box *root) {
  //printf("rendering box [%s] at %f %f\n", root->label.data, root->coords[0], root->coords[1]);
  bfont_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, root->label, v2m(root->coords[0], root->coords[1]), ctx.g_scale, col(root->color_mod, root->color_mod, root->color_mod,1), true);
  for (Gui_Box *child = root->first; child != nullptr; child = child->next) {
    gui_render(child);
  }
}


void gui_prune_unused_boxes() {
  for (s32 slot_idx = 0; slot_idx < ctx.slot_count; slot_idx +=1) {
    Gui_Box_Hash_Slot *slot = &ctx.slots[slot_idx];
    for (Gui_Box *box = slot->hash_first; box != nullptr;) {
      Gui_Box *next = box->hash_next; // This is here because the box could be deleted below
      if (box->last_frame_used != ctx.frame_idx) {
        dll_remove_NP(slot->hash_first, slot->hash_last, box, hash_next, hash_prev);
        M_ZERO_STRUCT(box);
        sll_stack_push(ctx.box_freelist, box); 
      }
      box = next;
    }
  }
}

void gui_end() {
  // Just to check for leaks
  //printf("GUI arena pos: %lu\n", arena_get_current_pos(ctx.arena));
  gui_layout(ctx.root);
  gui_prune_unused_boxes();
  gui_render(ctx.root);
  ctx.hot_id = 0;
}

