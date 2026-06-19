#include "gui.h"
#include "core/core_inc.h"

static Gui_Context g_gui_ctx;

// Self-referential box used instead of nullptr to denote end/invalid
static Gui_Box g_nil_box = {
  .first = &g_nil_box,
  .last = &g_nil_box,
  .next = &g_nil_box,
  .prev = &g_nil_box,
  .parent = &g_nil_box,
};

void gui_context_init(Arena *temp_arena, Font_Info *font) {
  g_gui_ctx.font = font;

  g_gui_ctx.temp_arena = temp_arena;
  g_gui_ctx.persistent_arena = arena_make(MB(256));

  g_gui_ctx.slot_count = GUI_SLOT_COUNT;
  g_gui_ctx.slots = arena_push_array(g_gui_ctx.persistent_arena, Gui_Box_Hash_Slot, g_gui_ctx.slot_count);

  g_gui_ctx.root_panel = arena_push_array(g_gui_ctx.persistent_arena, Gui_Panel, 1);
  g_gui_ctx.root_panel->parent_pct = 1.0;
  g_gui_ctx.root_panel->split_axis = GUI_AXIS_Y;
  g_gui_ctx.root_panel->label = MAKE_STR("root_panel");
}

Gui_Context* gui_get_ctx() {
  return &g_gui_ctx;
}

Arena *gui_get_build_arena() {
	return gui_get_ctx()->temp_arena;
}
///////////////////////////////////
// Gui Keying mechanism
///////////////////////////////////

Gui_Key gui_key_make(u64 val){
	Gui_Key res = (Gui_Key){val};
	return res;
}

Gui_Key gui_key_zero(){
	return gui_key_make(0);
}

Gui_Key gui_key_from_str(buf s) {
	Gui_Key res = gui_key_zero();
  if (s.count > 0) {
    res = gui_key_make(djb2_buf(s));
    //printf("key: %.*s\n", (int)s.count, s.data);
  }
	return res;
}

b32 gui_key_match(Gui_Key a, Gui_Key b) {
	return ((u64)a == (u64)b);
}

///////////////////////////////////
// Gui Box Stuff
///////////////////////////////////

Gui_Box *gui_box_nil_id() {
	return &g_nil_box;
}

b32 gui_box_is_nil(Gui_Box *box) {
	return (box == 0 || box == gui_box_nil_id());
}

Gui_Box* gui_box_lookup_from_key(Gui_Box_Flags flags, Gui_Key key) {
	Gui_Box *res = gui_box_nil_id();

	if (!gui_key_match(key, gui_key_zero())) {
		u64 slot = key % gui_get_ctx()->slot_count;
		for (Gui_Box *box = gui_get_ctx()->slots[slot].hash_first; !gui_box_is_nil(box); box = box->hash_next) {
			if (gui_key_match(box->key, key)) {
				res = box;
				break;
			}
		}
	}
	return res;
}

Gui_Box *gui_box_build_from_key(Gui_Box_Flags flags, Gui_Key key, buf s) {
  Gui_Context *gctx = gui_get_ctx();
  Gui_Box *parent = gui_top_parent();

  // Look up to slot based cache to find the box
	Gui_Box *box = gui_box_lookup_from_key(flags, key);

	b32 box_first_time = gui_box_is_nil(box);
	b32 box_is_transient = gui_key_match(key,gui_key_zero());

  // If we find the box to have updated frame_idx, its a double creation of same idx..
  if(!box_first_time && box->last_used_frame_idx == gui_get_ctx()->frame_idx) {
		printf("Key [%.*s][%lu] has been detected twice, which means some box's hash to same ID!\n", (int)s.count, s.data, key);
		box = gui_box_nil_id();
		key = gui_key_zero();
		box_first_time = 1;
	}

  // If box was created this frame, allocate it, try to reuse Gui_Box's or allocate a new one
  if (box_first_time) {
		box = box_is_transient ? 0 : gui_get_ctx()->box_freelist;
		if (!gui_box_is_nil(box)) {
			sll_stack_pop(gui_get_ctx()->box_freelist);
		}
		else {
			box = arena_push_array_nz(box_is_transient? gui_get_build_arena() : gui_get_ctx()->persistent_arena, Gui_Box, 1);
		}
		M_ZERO_STRUCT(box);
	}

  // zero out per-frame data for box (will be recalculated)
	{
		box->first = box->last = box->next = box->prev = box->parent = gui_box_nil_id();
		box->child_count = 0;
		box->flags = 0;
		box->last_used_frame_idx = gui_get_ctx()->frame_idx;
		M_ZERO_ARRAY(box->pref_size);
	}

  // hook into persistent table
	if (box_first_time && !box_is_transient) {
		u64 hash_slot = (u64)key % gui_get_ctx()->slot_count;
		dll_insert_NPZ(&g_nil_box, gui_get_ctx()->slots[hash_slot].hash_first, gui_get_ctx()->slots[hash_slot].hash_last, gui_get_ctx()->slots[hash_slot].hash_last, box, hash_next, hash_prev);
	}

  // hook into tree structure
	if (!gui_box_is_nil(parent)) {
		dll_push_back_NPZ(gui_box_nil_id(), parent->first, parent->last, box, next, prev);
		parent->child_count += 1;
		box->parent = parent;
	}

  // fill the box's info stuff
	{
		box->key = key;
		box->flags |= flags;
		box->child_layout_axis = gui_top_child_layout_axis();
		// We are doing all layouting here, we should probably just traverse the hierarchy like Ryan says

		if (gctx->fixed_x_stack.top != &gctx->fixed_x_nil_stack_top) {
			box->fixed_pos.raw[GUI_AXIS_X] = gctx->fixed_x_stack.top->v;
			box->flags |= GB_FLAG_FIXED_X;
		}

		if (gctx->fixed_y_stack.top != &gctx->fixed_y_nil_stack_top) {
			box->fixed_pos.raw[GUI_AXIS_Y] = gctx->fixed_y_stack.top->v;
			box->flags |= GB_FLAG_FIXED_Y;
		}

		// FIXED_WIDTH/HEIGHT have NO pref size (GUI_SIZE_KIND_NULL) so their fixed_size will stay the same
		if (gctx->fixed_width_stack.top != &gctx->fixed_width_nil_stack_top) {
			box->fixed_size.raw[GUI_AXIS_X] = gctx->fixed_width_stack.top->v;
			box->flags |= GB_FLAG_FIXED_WIDTH;
		}else {
			box->pref_size[GUI_AXIS_X] = gui_top_pref_width();
		}

		if (gctx->fixed_height_stack.top != &gctx->fixed_height_nil_stack_top) {
			box->fixed_size.raw[GUI_AXIS_Y] = gctx->fixed_height_stack.top->v;
			box->flags |= GB_FLAG_FIXED_HEIGHT;
		}else {
			box->pref_size[GUI_AXIS_Y] = gui_top_pref_height();
		}

		box->color = gui_top_bg_color();
		box->text_color = gui_top_text_color();
		box->text_alignment = gui_top_text_alignment();
		box->font_scale = gui_top_font_scale();
	}

	gui_autopop_all_stacks();

  return box;
}


Gui_Box *gui_box_build_from_str(Gui_Box_Flags flags, buf s) {
	Gui_Key key = gui_key_from_str(s);
	Gui_Box *box = gui_box_build_from_key(flags, key, s);
	if (s.count > 0){
    box->s = s;
	}
	return box;
}

Gui_Key gui_get_hot_box_key() {
	return gui_get_ctx()->hot_box_key;
}

Gui_Key gui_get_active_box_key(Input_Mouse_Button b) {
	return gui_get_ctx()->active_box_keys[b];
}


Gui_Signal gui_get_signal_for_box(Gui_Box *box) {
	Gui_Signal signal = {0};
	signal.box = box;
  v2 mp = input_get_mouse_pos(gui_get_ctx()->input_ref);

	rect r = box->r;

  // If parent has GB_FLAG_CLIP, we test mouse intersection only inside parent box
	b32 mouse_inside_box = rect_isect_point(r, mp);
  if (mouse_inside_box && (box->flags & (GB_FLAG_CLICKABLE|GB_FLAG_VIEW_SCROLL_X|GB_FLAG_VIEW_SCROLL_Y))) {
    for (Gui_Box *parent = box->parent; !gui_box_is_nil(parent); parent = parent->parent) {
      if (parent->flags & GB_FLAG_CLIP) {
        mouse_inside_box = rect_isect_point(parent->r, mp);
        break;
      } 
    }
  }

	// perform scrolling via scroll wheel if widget in focus
	//if (mouse_inside_box && (box->flags & GB_FLAG_SCROLL)) { signal.flags |= GUI_SIGNAL_FLAG_SCROLLED; }

	// FIXME -- What the FUck? ////////
	if (!(box->flags & GB_FLAG_CLICKABLE))return signal;
	///////////////////////////////////

	// if mouse inside box, the box is HOT

	if (mouse_inside_box && (box->flags & GB_FLAG_CLICKABLE)) {
		gui_get_ctx()->hot_box_key = box->key;
		signal.flags |= GUI_SIGNAL_FLAG_MOUSE_HOVER;
	}
	// if mouse inside box AND mouse button pressed, box is ACTIVE, PRESS event
	for (each_enumv(Input_Mouse_Button, INPUT_MOUSE, mk)) {
		if (mouse_inside_box && input_mkey_pressed(gui_get_ctx()->input_ref, mk)) {
			gui_get_ctx()->active_box_keys[mk] = box->key;
			// TODO -- This is pretty crappy logic, fix someday
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_PRESSED << mk);
			//gui_drag_set_current_mp();
		}
	}
	// if current box is active, set is as dragging
	for (each_enumv(Input_Mouse_Button, INPUT_MOUSE, mk)) {
		if (gui_key_match(gui_get_active_box_key(mk), box->key)) {
			signal.flags |= (GUI_SIGNAL_FLAG_DRAGGING);
		}
	}
	// if mouse inside box AND mouse button released and box was ACTIVE, reset hot/active RELEASE signal
	for (each_enumv(Input_Mouse_Button, INPUT_MOUSE, mk)) {
		if (mouse_inside_box && input_mkey_released(gui_get_ctx()->input_ref, mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
			gui_get_ctx()->hot_box_key = gui_key_zero();
			gui_get_ctx()->active_box_keys[mk]= gui_key_zero();
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_RELEASED << mk);
		}
	}
	// if mouse outside box AND mouse button released and box was ACTIVE, reset hot/active
	for (each_enumv(Input_Mouse_Button, INPUT_MOUSE, mk)) {
		if (!mouse_inside_box && input_mkey_released(gui_get_ctx()->input_ref, mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
			gui_get_ctx()->hot_box_key = gui_key_zero();
			gui_get_ctx()->active_box_keys[mk] = gui_key_zero();
		}
	}
	return signal;
}

///////////////////////////////////
// Gui Build
///////////////////////////////////

void gui_build_begin(void) {
	//Gui_Context *state = gui_get_ctx();
	// We init all stacks here because they are STRICTLY per-frame data structures
	gui_init_stacks();


	// build top level's root guiBox
  gui_set_next_fixed_width(gui_get_ctx()->screen_dim.x);
  gui_set_next_fixed_height(gui_get_ctx()->screen_dim.y);
	Gui_Box *root = gui_box_build_from_str(0, MAKE_STR("ImRootPlsDontPutSameHashSomewhereElse"));
	gui_push_parent(root);
  gui_get_ctx()->root = root;

	// reset hot if box pruned
	{
		Gui_Key hot_key = gui_get_hot_box_key();
		Gui_Box *box = gui_box_lookup_from_key(0, hot_key);
		b32 box_not_found = gui_box_is_nil(box);
		if (box_not_found) {
			gui_get_ctx()->hot_box_key = gui_key_zero();
		}
	}

	// reset active if box pruned
	b32 active_exists = false;
	for (each_enumv(Input_Mouse_Button, INPUT_MOUSE, mk)) {
		Gui_Key active_key = gui_get_active_box_key(mk);
		Gui_Box *box = gui_box_lookup_from_key(0, active_key);
		b32 box_not_found = gui_box_is_nil(box);
		if (box_not_found) {
			gui_get_ctx()->active_box_keys[mk] = gui_key_zero();
		}else {
			active_exists = true;
		}
	}

	// reset hot if there is no active
	if (!active_exists) {
		gui_get_ctx()->hot_box_key = gui_key_zero();
	}

  // build the panel hierarchy
  gui_panel_layout_panels_and_boundaries(gui_get_ctx()->root_panel, (rect){{0,0,gui_get_ctx()->screen_dim.x, gui_get_ctx()->screen_dim.y}});
}

void gui_build_end(void) {
	Gui_Context *state = gui_get_ctx();
	gui_pop_parent();

	// prune unused boxes
	for (u32 hash_slot = 0; hash_slot < state->slot_count; hash_slot+=1) {
		for (Gui_Box *box = state->slots[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			if (box->last_used_frame_idx < state->frame_idx) {
				dll_remove_NPZ(gui_box_nil_id(), state->slots[hash_slot].hash_first, state->slots[hash_slot].hash_last,box,hash_next,hash_prev);
				sll_stack_push(state->box_freelist, box);
			}
		}
	}

	// do layout pass
	gui_layout_root(state->root, GUI_AXIS_X);
	gui_layout_root(state->root, GUI_AXIS_Y);

	// print hierarchy if need-be
	// if (gui_input_mb_pressed(GUI_RMB)) {
	// 	print_gui_hierarchy();
	// }

	//gui_drag_set_current_mp();

	// do animations
	for (u32 hash_slot = 0; hash_slot < state->slot_count; hash_slot+=1) {
		for (Gui_Box *box = state->slots[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			// TODO -- do some logarithmic curve here, this is not very responsive!
			f32 trans_rate = 10 * state->dt;

			b32 is_box_hot = gui_key_match(box->key,gui_get_hot_box_key());
			b32 is_box_active = gui_key_match(box->key,gui_get_active_box_key(0));

			box->hot_t += trans_rate * (is_box_hot - box->hot_t);
			box->active_t += trans_rate * (is_box_active - box->active_t);
		}
	}

	// render eveything
  gui_render_hierarchy(gui_get_ctx()->root);

	// clear previous frame's arena + advance frame_index
	//arena_clear(gui_get_build_arena()); // We are currently using game's arena, so we should NOT clear it ourselves, EVER
	state->frame_idx += 1;
}


void gui_frame_begin(v2 screen_dim, Input *input, f64 dt) {
  g_gui_ctx.screen_dim = screen_dim;
  g_gui_ctx.dt = dt;
  g_gui_ctx.input_ref = input;
  gui_build_begin();
}

// Do NOT call this multiple times because (!!!)
void gui_frame_end() {
  // TBA: Rendering will be done here, actually.
  gui_build_end();
}

///////////////////////////////////
// Gui Layouting 
///////////////////////////////////

void gui_layout_calc_constant_sizes(Gui_Box *root, Gui_Axis axis) {
  Gui_Context *state = gui_get_ctx();
  // find the fixed size of the box
  if (root->pref_size[axis].kind == GUI_SIZE_KIND_PIXELS) {
      root->fixed_size.raw[axis] = root->pref_size[axis].value;
  }
  if (root->pref_size[axis].kind == GUI_SIZE_KIND_TEXT_CONTENT) {
      f32 padding = root->pref_size[axis].value;
      f32 text_size = 0;
      // TODO: make a bfont_measure_text_dim? :)
      if (axis == GUI_AXIS_X) {
        text_size = bfont_measure_text_width(state->font, root->s, root->font_scale);
      } else {
        text_size = bfont_measure_text_height(state->font, root->s, root->font_scale);
      }
      root->fixed_size.raw[axis] = padding + text_size;
  }
  // loop through all the hierarchy
  for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_calc_constant_sizes(child, axis);
  }
}

void gui_layout_calc_upward_dependent_sizes(Gui_Box *root, Gui_Axis axis) {
  Gui_Box *fixed_parent = gui_box_nil_id();
  if (root->pref_size[axis].kind == GUI_SIZE_KIND_PARENT_PCT) {
    fixed_parent = gui_box_nil_id();
    for(Gui_Box *box= root->parent; !gui_box_is_nil(box); box = box->parent) {
      if ( (box->flags & (GB_FLAG_FIXED_WIDTH<<axis)) ||
        box->pref_size[axis].kind == GUI_SIZE_KIND_PIXELS ||
        box->pref_size[axis].kind == GUI_SIZE_KIND_TEXT_CONTENT||
        box->pref_size[axis].kind == GUI_SIZE_KIND_PARENT_PCT) {
        fixed_parent = box;
        break;
      }
    }
    root->fixed_size.raw[axis] = fixed_parent->fixed_size.raw[axis] * root->pref_size[axis].value;
  }
	for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_calc_upward_dependent_sizes(child, axis);
	}
}

void gui_layout_calc_downward_dependent_sizes(Gui_Box *root, Gui_Axis axis) {
  // loop through all the hierarchy
	for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
        gui_layout_calc_downward_dependent_sizes(child, axis);
	}
  if (root->pref_size[axis].kind == GUI_SIZE_KIND_CHILDREN_SUM) {
    f32 sum = 0; 
    for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
      if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
        if (axis == root->child_layout_axis) {
          sum += child->fixed_size.raw[axis];
        } else {
          sum = maximum(sum, child->fixed_size.raw[axis]);
        }
      }
    }
    root->fixed_size.raw[axis] = sum;
  }
}

void gui_layout_calc_solve_constraints(Gui_Box *root, Gui_Axis axis) {
  // fixup when we are NOT current layout axis
  if (axis != root->child_layout_axis && !(root->flags & (GB_FLAG_OVERFLOW_X<<axis))) {
    f32 max_allowed_size = root->fixed_size.raw[axis];
    for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
      if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
        f32 child_size = child->fixed_size.raw[axis];
        f32 fixup_needed = child_size - max_allowed_size;
        fixup_needed = maximum(0, minimum(fixup_needed, child_size));
        if (fixup_needed > 0) {
          child->fixed_size.raw[axis] -= fixup_needed;
        }
      }
    }
  }
  if (axis == root->child_layout_axis && !(root->flags & (GB_FLAG_OVERFLOW_X<<axis))) {
    f32 max_allowed_size = root->fixed_size.raw[axis];
    f32 total_size = 0;
    f32 total_weighed_size = 0;
    for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
      if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
        total_size += child->fixed_size.raw[axis];
        total_weighed_size += child->fixed_size.raw[axis] * (1.0f - child->pref_size[axis].strictness);
      }
    }
    f32 violation = total_size - max_allowed_size;

    if (violation > 0.0f) {
      f32 *child_fixup_array = arena_push_array(gui_get_build_arena(), f32, root->child_count);
      u32 child_idx = 0;
      for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next, ++child_idx) {
        if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
          f32 child_weighed_size = child->fixed_size.raw[axis] * (1.0f - child->pref_size[axis].strictness);
          child_weighed_size = maximum(0.0f, child_weighed_size);
          child_fixup_array[child_idx] = child_weighed_size;
        }
      }

      child_idx = 0;
      for (Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next, ++child_idx) {
        if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
          // this percentage will be applied to ALL child widgets
          f32 fixup_needed = (violation / (f32)total_weighed_size);
          fixup_needed = minimum(maximum(0.0f,fixup_needed),1.0f);
          child->fixed_size.raw[axis] -= fixup_needed * child_fixup_array[child_idx];
        }
      }
    }
  }

  // Re-adjust fixed size of children with Parent_Pct before solving any more constraints
  // Not sure this is needed..
  /*
  if (root->flags & (GB_FLAG_OVERFLOW_X<<axis)) {
    for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
      if (child->pref_size[axis].kind == GUI_SIZE_KIND_PARENT_PCT) {
        child->fixed_size.raw[axis] = root->fixed_size.raw[axis] * child->pref_size[axis].value;
      }
    }
  }
  */

  // do the same for all children nodes and their children in hierarchy
  for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
      gui_layout_calc_solve_constraints(child, axis);
  }
}

void gui_layout_calc_final_rects(Gui_Box *root, Gui_Axis axis) {
  f32 layout_pos = 0;
  // do layouting for all children (only root's children)
  for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    if (!(child->flags & (GB_FLAG_FIXED_X<<axis))) {
      child->fixed_pos.raw[axis] = layout_pos;
      // advance layout offset
      if (root->child_layout_axis == axis) {
        layout_pos += child->fixed_size.raw[axis];
      }
    }
    // HERE we view scroll (-=view_off)
    child->r.p0.raw[axis] = root->r.p0.raw[axis] + child->fixed_pos.raw[axis] - root->view_off.raw[axis];
    child->r.dim.raw[axis] = child->fixed_size.raw[axis];
  }

  // do the same for all nodes and their children in hierarchy
  for(Gui_Box *child = root->first; !gui_box_is_nil(child); child = child->next) {
    gui_layout_calc_final_rects(child, axis);
  }
}

void gui_layout_root(Gui_Box *root, Gui_Axis axis)  {
  gui_layout_calc_constant_sizes(root, axis);
  gui_layout_calc_upward_dependent_sizes(root,axis);
  gui_layout_calc_downward_dependent_sizes(root,axis);
  gui_layout_calc_solve_constraints(root,axis);
  gui_layout_calc_final_rects(root, axis);
}

///////////////////////////////////
// Gui Rendering (Using Game_State's Cmd Buffers)
///////////////////////////////////

void gui_draw_rect_clip(rect r, v4 c, rect clip_rect) {
  // push quad
  R_Quad quad = (R_Quad) { .dst_rect = r, .c = c, };
  rn_push_quad(rn_pass_front(), quad);
}

void gui_draw_text_clip(rect r, v4 c, f32 font_scale, Gui_Text_Alignment text_alignment, buf s, rect clip_rect) {
  Gui_Context *gctx = gui_get_ctx();
  buf s_without_doublehash = buf_lcut(s, MAKE_STR("##"));

  rect label_rect = bfont_calc_text_rect(g_gui_ctx.font, s_without_doublehash, v2m(0,0), font_scale);
  rect fitted_rect = rect_fit_inside(label_rect, r, (Rect_Fit_Mode)text_alignment);
  v2 top_left = v2m(fitted_rect.x, fitted_rect.y);
  v2 baseline = v2_sub(top_left, label_rect.p0);

  rect viewport = rec(0,0,gctx->screen_dim.x, gctx->screen_dim.y);
  bfont_draw_text(gctx->font, gctx->temp_arena, viewport, clip_rect, s_without_doublehash, baseline, font_scale, c, false);
}


void gui_render_hierarchy(Gui_Box *box) {
  Gui_Context *gctx = gui_get_ctx();
	// Visualize hot_t, active_t values to plug to renderer
	if (box->flags & GB_FLAG_DRAW_ACTIVE_ANIMATION) {
		box->color.r += box->active_t/6.0f;
	}
	if (box->flags & GB_FLAG_DRAW_HOT_ANIMATION) {
		box->color.r += box->hot_t/6.0f;
	}

  rect clip_rect = rec(0,0,gctx->screen_dim.x, gctx->screen_dim.y);
  if (box->flags & (GB_FLAG_CLICKABLE|GB_FLAG_VIEW_SCROLL_X|GB_FLAG_VIEW_SCROLL_Y)) {
    for (Gui_Box *parent = box->parent; !gui_box_is_nil(parent); parent = parent->parent) {
      if (parent->flags & GB_FLAG_CLIP) {
        clip_rect = rect_bl_to_tl(parent->r, gctx->screen_dim.y);
        break;
      } 
    }
  }

	if (box->flags & GB_FLAG_DRAW_BACKGROUND) {
      gui_draw_rect_clip(box->r, box->color, clip_rect);
	}
	if (box->flags & GB_FLAG_DRAW_TEXT) {
    gui_draw_text_clip(box->r, box->text_color, box->font_scale, box->text_alignment, box->s, clip_rect);
  }

	// iterate through hierarchy
	for(Gui_Box *child = box->first; !gui_box_is_nil(child); child = child->next) {
		gui_render_hierarchy(child);
	}
}

///////////////////////////////////
// Gui Panels
///////////////////////////////////

Gui_Panel *gui_panel_traverse_dfs_preorder(Gui_Panel *panel) {
  Gui_Panel *itr = nullptr;
  if (panel->first != nullptr) { // we go down the hierarchy
    itr = panel->first;
  } else { // we go up the hierarchy
    Gui_Panel *p = panel;
    while (p != nullptr) {
      if (p->next != nullptr) {
        itr = p->next;
        break;
      }
      p = p->parent;
    }
  }
  return itr;
}

rect gui_panel_get_rect_from_parent_rect(Gui_Panel *panel, rect parent_rect) {
  rect r = parent_rect;
  Gui_Panel *parent = panel->parent;
  if (parent != nullptr) {
    Gui_Axis axis = parent->split_axis;
    Gui_Panel *child = parent->first;
    v2 running_pos = v2m(r.x, r.y);
    while (child != nullptr && child != panel) {
      if (axis == GUI_AXIS_X) running_pos.x += parent_rect.w * child->parent_pct;
      if (axis == GUI_AXIS_Y) running_pos.y += parent_rect.h * child->parent_pct;
      child = child->next;
    }
    r = (rect){{running_pos.x, running_pos.y, parent_rect.w * ((axis == GUI_AXIS_X) ? panel->parent_pct : 1), parent_rect.h * ((axis == GUI_AXIS_Y) ? panel->parent_pct : 1)}};
  }
  return r;
}

rect gui_panel_get_rect(Gui_Panel *panel, rect root_rect) {
  rect r = root_rect;
  Gui_Panel *itr = panel;
  while ((itr != nullptr) && (itr->parent != nullptr)) {
    gui_push_panel_itr((Gui_Panel_Itr){itr, itr->parent});
    itr = itr->parent;
  }

  while (!gui_empty_panel_itr()) {
    Gui_Panel_Itr panel_itr = gui_pop_panel_itr();
    r = gui_panel_get_rect_from_parent_rect(panel_itr.child, r);
  }

  return r;
}

void gui_panel_layout_panels_and_boundaries(Gui_Panel *root_panel, rect root_rect) {
#define BOUNDARY_THICKNESS 2

  Gui_Panel *itr = root_panel;
  while (itr != nullptr) {
    Gui_Panel *panel = itr;
    rect r = gui_panel_get_rect(panel, root_rect);

    if (panel->first == nullptr) {
      gui_set_next_fixed_x(r.x + BOUNDARY_THICKNESS);
      gui_set_next_fixed_y(r.y + BOUNDARY_THICKNESS);
      gui_set_next_fixed_width(r.w - BOUNDARY_THICKNESS*2);
      gui_set_next_fixed_height(r.h - BOUNDARY_THICKNESS*2);
      gui_set_next_child_layout_axis(GUI_AXIS_Y); // ?
      //gui_set_next_bg_color(v4m(0.2,0.2,0.2,0.7)); // TODO: styles?
                                                   
      assert(panel->label.count > 0);
      assert(panel->label.data != nullptr);
      buf name = arena_sprintf(gui_get_ctx()->temp_arena, "panel_%.*s", (int)panel->label.count, panel->label.data);
      Gui_Signal s = gui_pane(name);
      s.flags &= ~GB_FLAG_DRAW_TEXT;
    }

    itr = gui_panel_traverse_dfs_preorder(itr);
  }
   
  itr = root_panel;
  while (itr != nullptr) {
    Gui_Panel *panel = itr;
    // 1. find panel rect
    //rect r = gui_panel_get_rect(panel, root_rect);

    //printf("panel: %f has rect %f %f %f %f\n", panel->parent_pct, r.x,r.y,r.w,r.h);
    // 2. loop through every child that has a sibling next to it - otherwise no boundary
    Gui_Panel *child = panel->first;
    while (child != nullptr && child->next != nullptr) {
      rect child_rect = gui_panel_get_rect(child, root_rect);

      // 3. Calculate boundary rect and make a FIXED Gui_Box
      rect boundary_rect = child_rect;
      if (panel->split_axis == GUI_AXIS_X) {
        boundary_rect.x += child_rect.w - BOUNDARY_THICKNESS;
        boundary_rect.w = BOUNDARY_THICKNESS*2;
      } else {
        boundary_rect.y += child_rect.h - BOUNDARY_THICKNESS;
        boundary_rect.h = BOUNDARY_THICKNESS*2;
      } 
      gui_set_next_fixed_x(boundary_rect.x);
      gui_set_next_fixed_y(boundary_rect.y);
      gui_set_next_fixed_width(boundary_rect.w);
      gui_set_next_fixed_height(boundary_rect.h);
      gui_set_next_child_layout_axis(GUI_AXIS_Y); // ?
      gui_set_next_bg_color(v4m(0.1,0.6,0.8,1));

      buf name = arena_sprintf(gui_get_ctx()->temp_arena, "drag_boundary_%.*s", (int)child->label.count, child->label.data);
      Gui_Signal s = gui_button(name);
      s.box->flags &= ~GB_FLAG_DRAW_TEXT;

      // TODO: fix this dragging its.. HORRIBLE?
      if (gui_get_ctx()->active_box_keys[INPUT_MOUSE_LMB] == s.box->key) {
        //platform_set_cursor((child->split_axis == GUI_AXIS_X) ?  then NORTH_SOUTH else WEST_EAST);
        v2 mdelta = input_get_mouse_delta(gui_get_ctx()->input_ref);
        //f32 delta_on_split_axis = mdelta.raw[child->split_axis];
        f32 delta_on_split_axis = mdelta.raw[panel->split_axis];
        Gui_Panel *left_child = child;
        Gui_Panel *right_child = child->next;

        f32 parent_pct_movement = delta_on_split_axis * gui_get_ctx()->dt * 0.10;
        left_child->parent_pct += parent_pct_movement;
        left_child->parent_pct = clamp(left_child->parent_pct, 0.01, 0.99);
        right_child->parent_pct -= parent_pct_movement;
        right_child->parent_pct = clamp(right_child->parent_pct, 0.01, 0.99);
        //printf("dragging panel %s frame %lu w/ delta %f\n", panel->label, gui_get_ctx()->frame_idx, delta_on_split_axis);
      }

      child = child->next;
    }
    itr = gui_panel_traverse_dfs_preorder(itr);
  }

}

///////////////////////////////////
// Gui Widgets
///////////////////////////////////

Gui_Signal gui_button(buf s) {
	Gui_Box *w = gui_box_build_from_str( GB_FLAG_CLICKABLE |
									GB_FLAG_DRAW_TEXT |
									GB_FLAG_DRAW_BACKGROUND |
									GB_FLAG_DRAW_HOT_ANIMATION |
									GB_FLAG_DRAW_ACTIVE_ANIMATION,
									s);
	Gui_Signal signal = gui_get_signal_for_box(w);
	//if (signal.box->flags & GB_FLAG_HOVERING) { w->flags |= GB_FLAG_DRAW_BORDER; }
	return signal;
}

Gui_Signal gui_label(buf s) {
	Gui_Box *w = gui_box_build_from_str( GB_FLAG_DRAW_TEXT | GB_FLAG_DRAW_BACKGROUND | GB_FLAG_DRAW_HOT_ANIMATION, s);
	Gui_Signal signal = gui_get_signal_for_box(w);
	return signal;
}

Gui_Signal gui_pane(buf s) {
	Gui_Box *w = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, s);
	Gui_Signal signal = gui_get_signal_for_box(w);
	return signal;
}

Gui_Signal gui_spacer(Gui_Size size) {
	Gui_Box *parent = gui_top_parent();
	gui_set_next_pref_size(parent->child_layout_axis, size);
	Gui_Box *w = gui_box_build_from_str(0, buf_make(nullptr, 0));
	Gui_Signal signal = gui_get_signal_for_box(w);
	return signal;
}


Gui_Signal gui_scroll_list_begin(buf s, Gui_Axis axis, Gui_Scroll_Data *sdata) {
  gui_push_bg_color(col(0.2,0.2,0.2,1.0));
  // Scroll list should fit in parent space right?
  gui_set_next_pref_size(axis, (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_child_layout_axis(gui_axis_flip(axis));
	Gui_Box *scroll_list = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, s);
  gui_push_parent(scroll_list);

  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_pref_size(axis, (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_child_layout_axis(axis);

  buf scroll_region_text = arena_sprintf(gui_get_ctx()->temp_arena, "%.*s_region", (int)s.count, s.data);
	Gui_Box *scroll_region = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND, scroll_region_text);

  f32 scroll_region_dim = (axis == GUI_AXIS_Y) ? scroll_region->r.h : scroll_region->r.w;
  f32 visible_items =  scroll_region_dim / sdata->item_px;
  f32 scroll_button_dim = scroll_region_dim * minimum(1.0, visible_items / (f32)sdata->item_count);

  f32 min_dim_px = 0;
  f32 max_dim_px = sdata->item_px * (sdata->item_count - visible_items); 


  if (visible_items < (f32)sdata->item_count) {
    gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZE_KIND_PIXELS, sdata->scroll_bar_px, 1.0});
    gui_set_next_pref_size(axis, (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
    gui_set_next_child_layout_axis(axis);
    gui_set_next_bg_color(v4_multf(gui_top_bg_color(), 0.9));

    buf scroll_bar_text = arena_sprintf(gui_get_ctx()->temp_arena, "%.*s_bar", (int)s.count, s.data);
    Gui_Box *scroll_bar= gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND, scroll_bar_text);

    gui_push_parent(scroll_bar);
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, sdata->scroll_percent, 0.0});
    gui_set_next_pref_size(axis, (Gui_Size){.kind = GUI_SIZE_KIND_PIXELS, scroll_button_dim, 1.0});
    gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
    gui_set_next_bg_color(sdata->scroll_button_color);
    buf scroll_button_text = arena_sprintf(gui_get_ctx()->temp_arena, "%.*s_sbutton", (int)s.count, s.data);
    Gui_Box *scroll_button = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLICKABLE, scroll_button_text);
    Gui_Signal scroll_button_sig = gui_get_signal_for_box(scroll_button);
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0 - sdata->scroll_percent, 0.0});
    if (gui_key_match(scroll_button_sig.box->key, gui_get_active_box_key(INPUT_MOUSE_LMB))) {
      sdata->scroll_percent += sdata->scroll_speed * input_get_mouse_delta(gui_get_ctx()->input_ref).raw[axis] * gui_get_ctx()->dt;
      sdata->scroll_percent = clamp(sdata->scroll_percent, 0, 1);
    }
    scroll_region->view_off.raw[axis] = lerp(min_dim_px, max_dim_px, sdata->scroll_percent);
    gui_pop_parent();

    gui_pop_parent();
    gui_pop_bg_color();

  }

	Gui_Signal sig = gui_get_signal_for_box(scroll_region);
  gui_push_parent(sig.box);
  gui_push_child_layout_axis(axis);
  gui_push_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_push_pref_size(axis, (Gui_Size){.kind = GUI_SIZE_KIND_PIXELS, sdata->item_px, 1.0});

	return sig;
}

void gui_scroll_list_end(buf s) {
  gui_pop_parent();
  gui_pop_child_layout_axis();
  gui_pop_pref_width();
  gui_pop_pref_height();
}

// TODO: make this fast, rn its dog slow ok?
Gui_Signal gui_multi_line_text(buf s, buf text) {
  assert(s.count);
  assert(text.count);
  gui_push_bg_color(col(0.2,0.2,0.2,1.0));
  // Scroll list should fit in parent space right?
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_child_layout_axis(GUI_AXIS_Y);
  Gui_Box *text_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP|GB_FLAG_OVERFLOW_Y, s);

  gui_push_parent(text_container);

  f32 max_x = text_container->r.w;
  buf mtext = text;

  while (max_x && mtext.count) {
    s64 glyphs_needed = bfont_count_glyphs_until_width(gui_get_ctx()->font, mtext, gui_top_font_scale(), max_x);
    glyphs_needed = maximum(glyphs_needed, 1); // in case no glyphs fit
    buf substr = buf_make(mtext.data, glyphs_needed);
    u32 text_h = bfont_measure_text_height(gui_get_ctx()->font, substr, gui_top_font_scale());
                         
    gui_set_next_pref_size(GUI_AXIS_X, (Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
    gui_set_next_pref_size(GUI_AXIS_Y, (Gui_Size){.kind = GUI_SIZE_KIND_PIXELS, text_h, 0.0});
    gui_label(substr);

    mtext.count -= glyphs_needed;
    mtext.data += glyphs_needed;
  }
  gui_pop_parent();
  gui_pop_bg_color();

  return gui_get_signal_for_box(text_container);
}

Gui_Dialog_State gui_dialog(buf id, buf person_name, buf prompt) {
  Gui_Dialog_State ds = {};
  gui_push_bg_color(col(0.2,0.2,0.2,1.0));

  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_child_layout_axis(GUI_AXIS_Y);
  Gui_Box *dialog_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, id);

  gui_push_parent(dialog_container);

  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 0.0});
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_text_color(col(0.3,0.9,0.9, 1.0));
  gui_label(arena_sprintf(gui_get_ctx()->temp_arena, "%.*s:##%.*s", (int)person_name.count, person_name.data, (int)id.count, id.data));

  gui_push_text_color(col(0.9,0.9,0.3,1.0));
  gui_multi_line_text(arena_sprintf(gui_get_ctx()->temp_arena, "%.*s__dialog", (int)id.count, id.data), prompt);
  gui_pop_text_color();

  gui_pop_text_alignment();

  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_CHILDREN_SUM, 0.0, 0.0});
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  Gui_Box *buttons_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, arena_sprintf(gui_get_ctx()->temp_arena, "%.*s__buttons", (int)id.count, id.data));
  gui_push_parent(buttons_container);
  {
    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 5.0, 0.0});
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 5.0, 1.0});
    gui_set_next_bg_color(col(0.7,0.5,0.4, 1.0));
    gui_set_next_text_color(col(0.9,0.9,0.3, 0.8));
    Gui_Signal s = gui_button(arena_sprintf(gui_get_ctx()->temp_arena, "Next##__next_button%.*s", (int)id.count, id.data));
    if (s.flags & GUI_SIGNAL_FLAG_LMB_PRESSED)ds = GUI_DIALOG_STATE_NEXT_PRESSED;

    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.0});

    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 5.0, 0.0});
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 5.0, 1.0});
    gui_set_next_bg_color(col(0.5,0.3,0.5, 1.0));
    gui_set_next_text_color(col(0.9,0.9,0.3, 0.8));
    s = gui_button(arena_sprintf(gui_get_ctx()->temp_arena, "Prev##__prev_button%.*s", (int)id.count, id.data));
    if (s.flags & GUI_SIGNAL_FLAG_LMB_PRESSED)ds = GUI_DIALOG_STATE_PREV_PRESSED;
  }
  gui_pop_parent();

  gui_pop_parent();
  gui_pop_bg_color();
  return ds;
}

int gui_choice_box(buf id, buf *choices, int count) {
  int ret = -1;
  //gui_push_bg_color(col(0.2,0.2,0.2,1.0));
  gui_pop_bg_color(); // There is a bg_color running wild somewhere

  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
  gui_set_next_child_layout_axis(GUI_AXIS_Y);
  Gui_Box *master_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, id);
  gui_push_parent(master_container);

  gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.0});
  for (int i = 0; i < count; ++i) {
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_CHILDREN_SUM, 1.0, 1.0});
    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 0.0});
    gui_set_next_child_layout_axis(GUI_AXIS_X);
    Gui_Box *sub_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, arena_sprintf(gui_get_ctx()->temp_arena, "%.*s_subcontainer##%i", (int)id.count, id.data, i));
    gui_push_parent(sub_container);

    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.0});

    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 15.0, 1.0});
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, 15.0, 1.0});
    gui_set_next_bg_color(col(0.1,0.1,0.1,1.0));
    gui_set_next_text_color(col(0.9,0.9,0.3, 0.8));
    Gui_Signal s = gui_button(arena_sprintf(gui_get_ctx()->temp_arena, "%.*s##%i", (int)choices[i].count, choices[i].data, i));
    if (s.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
      ret = i;
    }

    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.0});

    gui_pop_parent();
  }
  gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.0});

  gui_pop_parent();
  gui_pop_text_alignment();

  return ret;
}

void gui_centered_region_begin(buf s, f32 stretch_percent, b32 x_axis, b32 y_axis) {
  if (x_axis) {
    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, stretch_percent, 1.0});
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
    gui_set_next_child_layout_axis(GUI_AXIS_X);
    Gui_Box *horizontal_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, arena_sprintf(gui_get_ctx()->temp_arena, "horizontal##%.*s", (int)s.count, s.data));
    gui_push_parent(horizontal_container);
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.5});
  }

  if (y_axis) { gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
    gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, stretch_percent, 1.0});
    gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
    gui_set_next_child_layout_axis(GUI_AXIS_Y);
    Gui_Box *vertical_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, arena_sprintf(gui_get_ctx()->temp_arena, "vertical##%.*s", (int)s.count, s.data));
    gui_push_parent(vertical_container);
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.5});
  }
}

void gui_centered_region_end(b32 x_axis, b32 y_axis) {
  if (y_axis) {
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.5});
    gui_pop_parent(); // vertical container
  }

  if (x_axis) {
    gui_spacer((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1, 0.5});
    gui_pop_parent(); // horizontal container
  }
}

void gui_simple_game_options_menu(buf s, Simple_Game_Options *opt) {
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
  gui_centered_region_begin(arena_sprintf(gui_get_ctx()->temp_arena, "center_reg##%.*s", (int)s.count, s.data), 0.5,1,1);
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_bg_color(col(0.3,0.3,0.3,0.3));
  gui_set_next_child_layout_axis(GUI_AXIS_Y);
  Gui_Box *master_container = gui_box_build_from_str(GB_FLAG_DRAW_BACKGROUND|GB_FLAG_CLIP, arena_sprintf(gui_get_ctx()->temp_arena, "master##%.*s", (int)s.count, s.data));
  gui_push_parent(master_container);

  v2 pad = v2m(10,10);

  //gui_centered_region_begin(MAKE_STR("first"), 1.0, 1, 0);
  gui_set_next_bg_color(col(0.6,0.2,0.2,1.0));
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, pad.x, 1.0});
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, pad.y, 1.0});
  Gui_Signal start_sig = gui_button(arena_sprintf(gui_get_ctx()->temp_arena, "start##%.*s", (int)s.count, s.data));
  if (start_sig.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
    opt->start_btn_pressed = true;
  } else {
    opt->start_btn_pressed = false;
  }
  //gui_centered_region_end(1,0);

  //gui_centered_region_begin(MAKE_STR("second"), 1.0, 1, 0);
  gui_set_next_bg_color(col(0.5,0.2,0.3,1.0));
  gui_set_next_pref_width((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, pad.x, 1.0});
  gui_set_next_pref_height((Gui_Size){.kind = GUI_SIZE_KIND_TEXT_CONTENT, pad.y, 1.0});
  Gui_Signal exit_sig = gui_button(arena_sprintf(gui_get_ctx()->temp_arena, "exit##%.*s", (int)s.count, s.data));
  if (exit_sig.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
    opt->exit_btn_pressed = true;
  } else {
    opt->exit_btn_pressed = false;
  }
  //gui_centered_region_end(1,0);

  gui_pop_parent(); // master container
  gui_centered_region_end(1,1);

  gui_pop_text_alignment();
}


