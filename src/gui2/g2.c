#include "g2.h"
static Gui_Box g_nil_box __attribute__((section(".rodata"))) = {
  .first = &g_nil_box,
  .last = &g_nil_box,
  .next = &g_nil_box,
  .prev = &g_nil_box,
  .parent = &g_nil_box,
};

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

Gui_Box *gui_nil_box() {
  return (&g_nil_box);
}

s32 gui_slot_idx_from_id(Gui_ID id) {
  return id % ctx.slot_count;
}

b32 gui_box_is_nil(Gui_Box *box) {
  return ((box == nullptr) || (box == gui_nil_box()));
}

Gui_Box *gui_box_make(buf label, Gui_ID id, Gui_Box_Flags flags) {
  Gui_Box *box = gui_nil_box();
  s32 slot_idx = gui_slot_idx_from_id(id);
  // Try to find box in hashmap
  for (Gui_Box *b = ctx.slots[slot_idx].hash_first; !gui_box_is_nil(b); b=b->hash_next) {
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
  dll_push_back_NPZ(gui_nil_box(), ctx.slots[slot_idx].hash_first, ctx.slots[slot_idx].hash_last, box, hash_next, hash_prev);

  // Clear some stuff (document this better)
  {
    box->id = id;
    box->label = label;
    box->flags = flags;
    box->local_rect = rec(0,0,0,0);
    box->first = box->last = box->next = box->prev = box->parent = gui_nil_box();
    box->color_mod = 1.0;
    box->last_frame_used = ctx.frame_idx;
  }
  return box;
}

// TODO: This should probably be a signal_return and step 3 with button logic should become generic-er
b32 gui_button(buf label, Gui_Layout_Params params) {
  // 0. allocate/reuse/retain box pointer
  Gui_ID id = djb2_buf(label);
  Gui_Box *box = gui_box_make(label, id, params.flags);
  // 1. Fill the (immediate) params
  {
    box->parent = (params.parent) ? params.parent : ctx.root;
    box->major_layout_axis = params.major_layout_axis;
  }

  // FIXME: Should this happen along with push to persistent hashmap (why is it done here)
  // 2. Hook into the layout structure
  Gui_Box *parent = box->parent;
  dll_push_back_NPZ(gui_nil_box(), parent->first, parent->last, box, next, prev);

  // 3. Button/Box logic (using previous frame's final_rect.p.raw, AKA box->final_rect.p.raw!)
  rect r = bfont_calc_text_rect(ctx.font, label, v2m(box->final_rect.p.raw[GUI_AXIS_X], box->final_rect.p.raw[GUI_AXIS_Y]), ctx.g_scale);
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
  ctx.root = gui_box_make(root_label, root_id, 
    GUI_BOX_FLAG_FIXED_X | GUI_BOX_FLAG_FIXED_Y | 
    GUI_BOX_FLAG_FIXED_WIDTH | GUI_BOX_FLAG_FIXED_HEIGHT | 
    GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT
  );
  f32 pad_px = 100;
  ctx.root->local_rect = rec(
      ctx.viewport.x + pad_px/2,
      ctx.viewport.y + pad_px/2,
      ctx.viewport.w - pad_px,
      ctx.viewport.h - pad_px
  );
  ctx.root->major_layout_axis = GUI_AXIS_X;
}

void gui_layout_axis(Gui_Box *root, Gui_Axis axis) {
  // FIXME: To NOT have this ternary operator we could make a self referential struct instead for nullptr in our lists 
  f32 prev_box_layout_pos = (!gui_box_is_nil(root->prev)) ? root->prev->final_rect.p.raw[axis] : root->parent->final_rect.p.raw[axis];
  f32 prev_box_layout_dim = root->prev->final_rect.dim.raw[axis];
  Gui_Axis parent_layout_axis = root->parent->major_layout_axis;
  root->final_rect.p.raw[axis] = root->local_rect.p.raw[axis] + prev_box_layout_pos + ((parent_layout_axis == axis) ? prev_box_layout_dim : 0);

  if ( root->flags & (GUI_BOX_FLAG_FIXED_WIDTH << axis) ) {
    root->final_rect.dim.raw[axis] = root->local_rect.dim.raw[axis];
  } else if (root->label.count > 0) { // FIXME: Currently only other way to get width is via text length
    rect text_rect = bfont_calc_text_rect(ctx.font, root->label, v2m(0,0), ctx.g_scale);
    root->final_rect.dim.raw[axis] = text_rect.dim.raw[axis];
  } else {
    printf("HUHUHUHUUHUHUHUUHHHH?!\n");
  }

  for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_axis(child, axis);
  }
}

void gui_render(Gui_Box *root) {
  // 0. Draw the Regular box
  if (root->flags & GUI_BOX_FLAG_DRAW_BOX) {
    R_Quad quad = (R_Quad) {
        .dst_rect = root->final_rect,
        .c = col(0.2,0.2,0.2,0.95),
    };
    rn_push_quad(rn_pass_front(), quad);
  }

  // 1. Draw the text
  // FIXME: If the box is fixed (meaning layout not based on text for now), put the label in the middle of the container
  if (root->flags & GUI_BOX_FLAG_DRAW_TEXT) {
    if (root->flags & GUI_BOX_FLAG_FIXED_WIDTH || root->flags & GUI_BOX_FLAG_FIXED_HEIGHT) {
      rect r = bfont_calc_text_rect(ctx.font, root->label, v2m(0,0), ctx.g_scale);
      bfont_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, root->label, 
          v2m((root->final_rect.p.raw[0] + root->final_rect.dim.raw[0]/2.0 - r.dim.raw[0]/2.0), 
            (root->final_rect.p.raw[1] + root->final_rect.dim.raw[1]/2.0 - r.dim.raw[1]/2.0)),
          ctx.g_scale, col(root->color_mod, root->color_mod, root->color_mod,1), false
      );
    } else {
      bfont_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, root->label, v2m(root->final_rect.p.raw[0], root->final_rect.p.raw[1]), ctx.g_scale, col(root->color_mod, root->color_mod, root->color_mod,1), false);
    }
  }
  

  // 2. Proceed to render the remaining hierarch (back-to-front)
  for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    gui_render(child);
  }
}

void gui_prune_unused_boxes() {
  for (s32 slot_idx = 0; slot_idx < ctx.slot_count; slot_idx +=1) {
    Gui_Box_Hash_Slot *slot = &ctx.slots[slot_idx];
    for (Gui_Box *box = slot->hash_first; !gui_box_is_nil(box);) {
      Gui_Box *next = box->hash_next; // This is here because the box could be deleted below
      if (box->last_frame_used != ctx.frame_idx) {
        dll_remove_NPZ(gui_nil_box(), slot->hash_first, slot->hash_last, box, hash_next, hash_prev);
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
  gui_layout_axis(ctx.root, GUI_AXIS_X);
  gui_layout_axis(ctx.root, GUI_AXIS_Y);
  gui_prune_unused_boxes();
  gui_render(ctx.root);
  ctx.hot_id = 0;
}

