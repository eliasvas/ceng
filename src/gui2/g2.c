#include "g2.h"

/*
FIXME FIXME FIXME
The  number of drawcalls explodes (one per rectangle/string of text) because:
- We do hardware clipping (We need to flush before continuing to apply the scissor rect) - We use 2 different textures for 'white' and text (and probably non-ASCII glyphs as well) We Should do the clipping on the fragment shader somehow (probably via discard) + combine our textures to one (or clamp to white??).
*/

#define GUI_STACKS_IMPLEMENTATION
#include "gui_stacks.h"

// Should this be here??
Gui_Ctx ctx;

static Gui_Box g_nil_box __attribute__((section(".rodata"))) = {
  .first = &g_nil_box,
  .last = &g_nil_box,
  .next = &g_nil_box,
  .prev = &g_nil_box,
  .parent = &g_nil_box,
};


Gui_ID gui_empty_id() { return 0; }
Gui_ID gui_get_id_from_label(str8 label) {
  return (label.count) ? djb2_buf(label.data, label.count) : gui_empty_id(); 
}

str8 gui_get_label_no_hh(str8 label) {
  u64 hh_pos = str8_find_needle(label, STR8L("##"));
  if (hh_pos != STR8_NO_MATCH) label.count = hh_pos;
  return label;
}


Gui_Box *gui_nil_box() {
  return (&g_nil_box);
}

s32 gui_slot_idx_from_id(Gui_ID id) {
  return id % ctx.slot_count;
}

b32 gui_box_is_nil(Gui_Box *box) {
  return ((box == nullptr) || (box == gui_nil_box()));
}

Gui_Box *gui_box_make(str8 label, Gui_Box_Flags flags) {
  Gui_ID id = gui_get_id_from_label(label);
  Gui_Box *box = gui_nil_box();
  b32 is_transient = (id == gui_empty_id());

  // Try to find box in hashmap
  b32 first_time = true;
  s32 slot_idx = gui_slot_idx_from_id(id);
  for (Gui_Box *b = ctx.slots[slot_idx].hash_first; !gui_box_is_nil(b); b=b->hash_next) {
    if (b->id == id) {
      box = b;
      first_time = false;
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

  // If this box is a spacer, DON'T plug into the persistent hierarchy 
  if (first_time && !is_transient) {
    // Plug the box to the persistent hashmap
    dll_push_back_NPZ(gui_nil_box(), ctx.slots[slot_idx].hash_first, ctx.slots[slot_idx].hash_last, box, hash_next, hash_prev);
  }

  // Clear some stuff (document this better)
  {
    box->fixed_pos.raw[GUI_AXIS_X]  = 0;
    box->fixed_pos.raw[GUI_AXIS_Y]  = 0;
    box->fixed_size.raw[GUI_AXIS_X] = 0;
    box->fixed_size.raw[GUI_AXIS_Y] = 0;
    box->first = box->last = box->next = box->prev = box->parent = gui_nil_box();
    box->last_frame_used = ctx.frame_idx;
    box->child_count = 0;
    box->id = id;
    //box->label = label;
    box->label = gui_get_label_no_hh(label); // we chop any ##abcd diversifiers
    box->flags = flags;

    box->text_align = gui_top_text_alignment();
    box->bg_color = gui_top_bg_color();
    box->text_color = gui_top_text_color();

    box->parent = gui_top_parent();
    box->major_layout_axis = gui_top_child_layout_axis(); 

    if (ctx.fixed_width_stack.top != &ctx.fixed_width_nil_stack_top) {
      box->flags |= GUI_BOX_FLAG_FIXED_WIDTH;
      box->fixed_size.raw[GUI_AXIS_X] = gui_top_fixed_width();
    } else {
      box->pref_size[GUI_AXIS_X] = gui_top_pref_width();
    }

    if (ctx.fixed_height_stack.top != &ctx.fixed_height_nil_stack_top) {
      box->flags |= GUI_BOX_FLAG_FIXED_HEIGHT;
      box->fixed_size.raw[GUI_AXIS_Y] = gui_top_fixed_height();
    } else {
      box->pref_size[GUI_AXIS_Y] = gui_top_pref_height();
    }

    if (ctx.fixed_x_stack.top != &ctx.fixed_x_nil_stack_top) {
      box->flags |= GUI_BOX_FLAG_FIXED_X;
      box->fixed_pos.raw[GUI_AXIS_X] = gui_top_fixed_x();
    }
    if (ctx.fixed_y_stack.top != &ctx.fixed_y_nil_stack_top) {
      box->flags |= GUI_BOX_FLAG_FIXED_Y;
      box->fixed_pos.raw[GUI_AXIS_Y] = gui_top_fixed_y();
    }
  }

  // Hook box to the per-frame hierarchy
  Gui_Box *parent = box->parent;
  dll_push_back_NPZ(gui_nil_box(), parent->first, parent->last, box, next, prev);
  if (parent != gui_nil_box()) { parent->child_count+=1; }

  gui_autopop_all_stacks();
  return box;
}

Gui_Signal gui_signal_from_box(Gui_Box *box) {
  Gui_Signal sig = {
    .box = box,
    .sflags = 0,
  };

  if (box->flags & GUI_BOX_FLAG_CLICKABLE) {
    // 0. Clicking logic
    rect r = box->final_rect;
    v2 bl_mp = input_get_mouse_pos(ctx.input);
    bl_mp.y = (ctx.viewport.h - bl_mp.y);
    b32 collides = rect_isect_point(r, bl_mp);

    for (s32 mbtn_idx = 0; mbtn_idx < 3; mbtn_idx+=1) {
      b32 mb_pressed = input_mkey_pressed(ctx.input, INPUT_MOUSE_LMB+mbtn_idx);
      b32 mb_released = input_mkey_released(ctx.input, INPUT_MOUSE_LMB+mbtn_idx);

      s32 mbtn_idx = 0;
      if (collides) {
        ctx.hot_id = box->id;
        if (mb_pressed) {
          ctx.active_id = box->id;
          sig.sflags |= (GUI_SIGNAL_FLAG_LMB_PRESSED << mbtn_idx);
          break; // is this correct?
        }
      }
      if (mb_released) {
        if (ctx.hot_id == box->id) {
          sig.sflags |= (GUI_SIGNAL_FLAG_LMB_RELEASED << mbtn_idx);
          break; // is this correct?
        }
        ctx.active_id = 0;
      }
    }

    // 1. hot/active animations (these are here bc hot_id is set on step 0)
    {
      // FIXME: these should happen at gui_end right?
      f32 anim_rate = 1.0 - pow_f32(2.0, (-20.0f * ctx.dt));
      b32 is_hot = (ctx.hot_id == box->id);
      box->hot_t += ((f32)is_hot - box->hot_t) * anim_rate;
      b32 is_active = (ctx.active_id == box->id);
      box->active_t += ((f32)is_active - box->active_t) * anim_rate;
    }
  }

  return sig;
}

Gui_Signal gui_button(str8 label) {
  u32 flags = (GUI_BOX_FLAG_CLICKABLE | GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT);
  Gui_Box *box = gui_box_make(label, flags);
  return gui_signal_from_box(box);
}

Gui_Signal gui_spacer(Gui_Size size) {
  Gui_Axis layout_axis = gui_top_child_layout_axis(); 
  if (layout_axis == GUI_AXIS_X) gui_set_next_pref_width(size);
  if (layout_axis == GUI_AXIS_Y) gui_set_next_pref_height(size);
  Gui_Box *box = gui_box_make(STR8L(""), 0);
  return gui_signal_from_box(box);
}

Gui_Signal gui_panel(str8 label) {
  u32 flags = (GUI_BOX_FLAG_DRAW_BOX);
  Gui_Box *box = gui_box_make(label, flags);
  return gui_signal_from_box(box);
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

void gui_init_stacks();

void gui_begin(rect viewport, f32 dt) {
  // Advance frame index (used for box pruning)
  gui_init_stacks();
  ctx.frame_idx+=1;
  ctx.dt = dt;

  gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_push_bg_color(v4m(0.4,0.4,0.4,0.9));
  ctx.viewport = viewport;
  // Initialize a root box
  // TODO: ROOTBOX should finally be the whole screen space and this rect should be in user-code
  gui_set_next_fixed_x(5);
  gui_set_next_fixed_y(10);
  gui_set_next_fixed_width(300);
  gui_set_next_fixed_height(300);
  //ctx.root = gui_box_make(STR8L("ROOTBOX"), GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT);
  ctx.root = gui_box_make(STR8L("ROOTBOX"), GUI_BOX_FLAG_DRAW_BOX);

  gui_push_parent(ctx.root);
}

void gui_layout_constant_sizes(Gui_Box *node, Gui_Axis axis) {
  // 0. for SIZEKIND_PIXELS, fixed_size is just the value
  if (node->pref_size[axis].kind == GUI_SIZEKIND_PIXELS) {
      node->fixed_size.raw[axis] = node->pref_size[axis].value;
  }
  // 1. for SIZEKIND_TEXT_CONTENT, fixed_size is the size of label text + padding (value)
  else if (node->pref_size[axis].kind == GUI_SIZEKIND_TEXT_CONTENT) {
      f32 padding = node->pref_size[axis].value;
      rect text_rect = bfont_calc_text_rect(ctx.font, node->label, v2m(0,0), ctx.g_scale);
      node->fixed_size.raw[axis] = text_rect.dim.raw[axis] + padding;
  }

  // 2. Recurse
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_constant_sizes(child, axis);
  }
}

void gui_layout_upward_dependent_sizes(Gui_Box *node, Gui_Axis axis) {
  if (node->pref_size[axis].kind == GUI_SIZEKIND_PERCENT_OF_PARENT) {
    // 0. Find nearest parent with fixed size 
    f32 parent_size = 0;
    for (Gui_Box *parent = node->parent; !gui_box_is_nil(parent); parent = parent->parent) {
      if (parent->pref_size[axis].kind != GUI_SIZEKIND_SUM_OF_CHILDREN) {
        parent_size = parent->fixed_size.raw[axis];
        break;
      }
    }
    // 1. Final fixed size is a percent (value) of that
    node->fixed_size.raw[axis] = parent_size * node->pref_size[axis].value;
  }

  // 3. Recurse
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_upward_dependent_sizes(child, axis);
  }
}

void gui_layout_downward_dependent_sizes(Gui_Box *node, Gui_Axis axis) {
  // 0. Recurse
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_downward_dependent_sizes(child, axis);
  }

  if (node->pref_size[axis].kind == GUI_SIZEKIND_SUM_OF_CHILDREN) {
    // 1. If along box's layout axis, calculate the fixed size as sum of children
    if (axis == node->major_layout_axis) {
      f32 child_sum_size = 0;
      for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
        child_sum_size += child->fixed_size.raw[axis];
      }
      node->fixed_size.raw[axis] = child_sum_size;
    } 
    // 2. If along non-major axis, fixed size is the largest box (max) along axis
    else {
      f32 max_size = 0;
      for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
        max_size = maximum(max_size, child->fixed_size.raw[axis]);
      }
      node->fixed_size.raw[axis] = max_size;
    }
  }
}

void gui_layout_enforce_size_constraints(Gui_Box *node, Gui_Axis axis) {
  // 0. Calculate fixed size of this node (named parent) and all its children 
  b32 overflow_allowed = (node->flags & (GUI_BOX_FLAG_ALLOW_OVERFLOW_X<<axis));
  f32 parent_size = node->fixed_size.raw[axis];
  f32 children_size = 0;
  f32 children_max_size = 0;
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    if (!(child->flags & GUI_BOX_FLAG_FIXED_X<<axis)) {
      children_size += child->fixed_size.raw[axis];
      children_max_size = maximum(children_max_size, child->fixed_size.raw[axis]);
    }
  }

  // 1.1 For major axis we have to substract an amount for each box
  if (axis == node->major_layout_axis && !overflow_allowed) {
    // 1.2 If we have an overflow for this axis, calculate how much size every child is willing to lose + overall size
    if (children_size > parent_size) {
      f32 needed_size = children_size - parent_size;
      u64 arena_pos = arena_get_current_pos(ctx.temp_arena);
      f32 *available_sizes = arena_push_array(ctx.temp_arena, f32, node->child_count);

      f32 available_size_sum = 0;
      s32 child_idx = 0;
      for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
        if (!(child->flags & GUI_BOX_FLAG_FIXED_X<<axis)) {
          f32 willing_to_lose = (1.0 - child->pref_size[axis].strictness) * child->fixed_size.raw[axis];
          available_sizes[child_idx] = willing_to_lose;
          available_size_sum += willing_to_lose;
          child_idx+=1;
        }
      }
      // 1.3 Change the size of all children so that they fit the parent fixed_size by taking a proportion of their available size

      child_idx = 0;
      for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
        child->fixed_size.raw[axis] -= needed_size * (available_sizes[child_idx] / available_size_sum);
        child_idx+=1;
      }
      arena_reset_to_pos(ctx.temp_arena, arena_pos);
    }
  } 

  // 2.1 For non-major axis just scale children that are not fully strict
  if (axis != node->major_layout_axis && !overflow_allowed) {
    if (children_max_size > parent_size) {
      for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
        if (!(child->flags & GUI_BOX_FLAG_FIXED_X<<axis)) {
          f32 willing_to_lose = (1.0 - child->pref_size[axis].strictness) * child->fixed_size.raw[axis];
          f32 child_fixed_size = child->fixed_size.raw[axis];
          if (child_fixed_size > parent_size && willing_to_lose > 0) {
            child->fixed_size.raw[axis] = parent_size;
          }
        }
      }
    }
  }

  // 3. If node is children-sum and a child is percent-of-parent the child fixed size
  // is calculated wrt a fixed rect from up the hierarchy if the children-sum node allows overflow,
  // its fixed size is known so it has to propagate to children with percent-of-parent 
  if (overflow_allowed) {
    for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
      if (child->pref_size[axis].kind == GUI_SIZEKIND_PERCENT_OF_PARENT) {
        child->fixed_size.raw[axis] = child->pref_size[axis].value * node->fixed_size.raw[axis];
      }
    }
  }

  
  // 1. Recurse
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_enforce_size_constraints(child, axis);
  }
}


void gui_layout_calc_fixed_pos_and_final_rects(Gui_Box *node, Gui_Axis axis) {
  f32 node_pos = node->fixed_pos.raw[axis];
  f32 layout_pos = 0;

  // Calculate final rect for the box
  node->final_rect.p.raw[axis] = node->fixed_pos.raw[axis];
  node->final_rect.dim.raw[axis] = node->fixed_size.raw[axis];

  // Calculate this box's children fixed positions
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    if (!(child->flags & GUI_BOX_FLAG_FIXED_X<<axis)) {
      child->fixed_pos.raw[axis] = node_pos + layout_pos;

      if (axis == node->major_layout_axis) {
        layout_pos += child->fixed_size.raw[axis];
      }
    }
  }

  // Recurse
  for (Gui_Box *child = node->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_calc_fixed_pos_and_final_rects(child, axis);
  }
}


void gui_render(Gui_Box *root) {
  // 0. Draw the Regular box
  if (root->flags & GUI_BOX_FLAG_DRAW_BOX) {
    R_Quad quad = (R_Quad) {
        .dst_rect = root->final_rect,
        .c = v4_add(root->bg_color, v4m(0.2 * root->hot_t, 0.2*root->active_t,0,0)),
    };
    rn_push_quad(rn_pass_front(), quad);
  }

  // 1. Draw the text
  if (root->flags & GUI_BOX_FLAG_DRAW_TEXT) {
    rect r = bfont_calc_text_rect(ctx.font, root->label, v2m(0,0), ctx.g_scale);
    v2 text_draw_pos = {};
    switch(root->text_align) {
      case GUI_TEXT_ALIGNMENT_LEFT:
        text_draw_pos = v2m(root->final_rect.p.raw[0], (root->final_rect.p.raw[1] + root->final_rect.dim.raw[1]/2.0 - r.dim.raw[1]/2.0));
        break;
      case GUI_TEXT_ALIGNMENT_RIGHT:
        text_draw_pos = v2m(root->final_rect.p.raw[0] + root->final_rect.dim.raw[0] - r.dim.raw[0], (root->final_rect.p.raw[1] + root->final_rect.dim.raw[1]/2.0 - r.dim.raw[1]/2.0));
        break;
      case GUI_TEXT_ALIGNMENT_CENTER:
        text_draw_pos = v2m((root->final_rect.p.raw[0] + root->final_rect.dim.raw[0]/2.0 - r.dim.raw[0]/2.0), (root->final_rect.p.raw[1] + root->final_rect.dim.raw[1]/2.0 - r.dim.raw[1]/2.0));
        break;
    }
    bfont_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, root->label, text_draw_pos, ctx.g_scale, root->text_color, false);
  }

  // 2. Proceed to render the remaining hierarchy (back-to-front)
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
  for (s32 axis = GUI_AXIS_X; axis <= GUI_AXIS_Y; axis+=1) {
    gui_layout_constant_sizes(ctx.root, axis);
    gui_layout_upward_dependent_sizes(ctx.root, axis);
    gui_layout_downward_dependent_sizes(ctx.root, axis);
    gui_layout_enforce_size_constraints(ctx.root, axis);
    gui_layout_calc_fixed_pos_and_final_rects(ctx.root, axis);
  }
  gui_prune_unused_boxes();
  gui_render(ctx.root);
  ctx.hot_id = 0;
}

