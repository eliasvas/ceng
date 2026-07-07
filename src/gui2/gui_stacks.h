#include "g2.h"

#ifndef GUI_STACKS_IMPLEMENTATION

Gui_Box *gui_push_parent(Gui_Box *box);
Gui_Box *gui_set_next_parent(Gui_Box *box);
Gui_Box *gui_pop_parent(void);
Gui_Box *gui_top_parent(void);

f32 gui_push_fixed_x(f32 v);
f32 gui_set_next_fixed_x(f32 v);
f32 gui_pop_fixed_x(void);
f32 gui_top_fixed_x(void);

f32 gui_push_fixed_y(f32 v);
f32 gui_set_next_fixed_y(f32 v);
f32 gui_pop_fixed_y(void);
f32 gui_top_fixed_y(void);

f32 gui_push_fixed_width(f32 v);
f32 gui_set_next_fixed_width(f32 v);
f32 gui_pop_fixed_width(void);
f32 gui_top_fixed_width(void);

f32 gui_push_fixed_height(f32 v);
f32 gui_set_next_fixed_height(f32 v);
f32 gui_pop_fixed_height(void);
f32 gui_top_fixed_height(void);

Gui_Size gui_push_pref_width(Gui_Size v);
Gui_Size gui_set_next_pref_width(Gui_Size v);
Gui_Size gui_pop_pref_width(void);
Gui_Size gui_top_pref_width(void);

Gui_Size gui_push_pref_height(Gui_Size v);
Gui_Size gui_set_next_pref_height(Gui_Size v);
Gui_Size gui_pop_pref_height(void);
Gui_Size gui_top_pref_height(void);

v4 gui_top_bg_color(void);
v4 gui_set_next_bg_color(v4 v);
v4 gui_push_bg_color(v4 v);
v4 gui_pop_bg_color(void);

v4 gui_top_text_color(void);
v4 gui_set_next_text_color(v4 v);
v4 gui_push_text_color(v4 v);
v4 gui_pop_text_color(void);

f32 gui_top_font_scale(void);
f32 gui_set_next_font_scale(f32 v);
f32 gui_push_font_scale(f32 v);
f32 gui_pop_font_scale(void);

Gui_Text_Alignment gui_top_text_alignment(void);
Gui_Text_Alignment gui_set_next_text_alignment(Gui_Text_Alignment v);
Gui_Text_Alignment gui_push_text_alignment(Gui_Text_Alignment v);
Gui_Text_Alignment gui_pop_text_alignment(void);


#else

extern Gui_Ctx ctx;

Arena *gui_get_build_arena() {
  return ctx.temp_arena;
}

#define gui_stack_top_impl(state, name_upper, name_lower) return ctx.name_lower##_stack.top->v;

#define gui_stack_bottom_impl(state, name_upper, name_lower) return ctx.name_lower##_stack.bottom_val;

#define gui_stack_push_impl(state, name_upper, name_lower, type, new_value) \
Gui_##name_upper##_Node *node = ctx.name_lower##_stack.free;\
if(node != 0) {sll_stack_pop(ctx.name_lower##_stack.free);}\
else {node = arena_push_array(gui_get_build_arena(), Gui_##name_upper##_Node, 1);}\
type old_value = ctx.name_lower##_stack.top->v;\
node->v = new_value;\
sll_stack_push(ctx.name_lower##_stack.top, node);\
if(node->next == &ctx.name_lower##_nil_stack_top)\
{\
ctx.name_lower##_stack.bottom_val = (new_value);\
}\
ctx.name_lower##_stack.auto_pop = 0;\
return old_value;

#define gui_stack_pop_impl(state, name_upper, name_lower) \
Gui_##name_upper##_Node *popped = ctx.name_lower##_stack.top;\
if(popped != &ctx.name_lower##_nil_stack_top)\
{\
sll_stack_pop(ctx.name_lower##_stack.top);\
sll_stack_push(ctx.name_lower##_stack.free, popped);\
ctx.name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define gui_stack_set_next_impl(state, name_upper, name_lower, type, new_value) \
Gui_##name_upper##_Node *node = ctx.name_lower##_stack.free;\
if(node != 0) {sll_stack_pop(ctx.name_lower##_stack.free);}\
else {node = arena_push_array(gui_get_build_arena(), Gui_##name_upper##_Node, 1);}\
type old_value = ctx.name_lower##_stack.top->v;\
node->v = new_value;\
sll_stack_push(ctx.name_lower##_stack.top, node);\
ctx.name_lower##_stack.auto_pop = 1;\
return old_value;\

#define gui_stack_empty_impl(state, name_upper, name_lower) \
Gui_##name_upper##_Node *top = ctx.name_lower##_stack.top;\
return (top == &ctx.name_lower##_nil_stack_top);\
//-----------------------------------------------------------------------------



// This function should be huge and initialize ALL the UI stacks to empty
// TODO -- MAYBE this should be just some macro (maybe), declarations too?
void gui_init_stacks() {
	// -- parent stack initialization
	ctx.parent_nil_stack_top.v = gui_nil_box();
	ctx.parent_stack.top = &ctx.parent_nil_stack_top;
	ctx.parent_stack.bottom_val = gui_nil_box();
	ctx.parent_stack.free = 0;
	ctx.parent_stack.auto_pop = 0;
	// -- fixed_x stack initialization
	ctx.fixed_x_nil_stack_top.v = 0;
	ctx.fixed_x_stack.top = &ctx.fixed_x_nil_stack_top;
	ctx.fixed_x_stack.bottom_val = 0;
	ctx.fixed_x_stack.free = 0;
	ctx.fixed_x_stack.auto_pop = 0;
	// -- fixed_y stack initialization
	ctx.fixed_y_nil_stack_top.v = 0;
	ctx.fixed_y_stack.top = &ctx.fixed_y_nil_stack_top;
	ctx.fixed_y_stack.bottom_val = 0;
	ctx.fixed_y_stack.free = 0;
	ctx.fixed_y_stack.auto_pop = 0;
	// -- fixed_width stack initialization
	ctx.fixed_width_nil_stack_top.v = 0;
	ctx.fixed_width_stack.top = &ctx.fixed_width_nil_stack_top;
	ctx.fixed_width_stack.bottom_val = 0;
	ctx.fixed_width_stack.free = 0;
	ctx.fixed_width_stack.auto_pop = 0;
	// -- fixed_height stack initialization
	ctx.fixed_height_nil_stack_top.v = 0;
	ctx.fixed_height_stack.top = &ctx.fixed_height_nil_stack_top;
	ctx.fixed_height_stack.bottom_val = 0;
	ctx.fixed_height_stack.free = 0;
	ctx.fixed_height_stack.auto_pop = 0;
	// -- pref_width stack initialization
	ctx.pref_width_nil_stack_top.v = (Gui_Size){GUI_SIZEKIND_PIXELS,250.0f,1.0};
	ctx.pref_width_stack.top = &ctx.pref_width_nil_stack_top;
	ctx.pref_width_stack.bottom_val = ctx.pref_width_nil_stack_top.v;
	ctx.pref_width_stack.free = 0;
	ctx.pref_width_stack.auto_pop = 0;
	// -- pref_height stack initialization
	ctx.pref_height_nil_stack_top.v = (Gui_Size){GUI_SIZEKIND_PIXELS,40.0f,1.0};
	ctx.pref_height_stack.top = &ctx.pref_height_nil_stack_top;
	ctx.pref_height_stack.bottom_val = ctx.pref_height_nil_stack_top.v;
	ctx.pref_height_stack.free = 0;
	ctx.pref_height_stack.auto_pop = 0;
	// -- bg_color stack initialization
	ctx.bg_color_nil_stack_top.v = v4m(0,0,0,0);
	ctx.bg_color_stack.top = &ctx.bg_color_nil_stack_top;
	ctx.bg_color_stack.bottom_val = ctx.bg_color_nil_stack_top.v;
	ctx.bg_color_stack.free = 0;
	ctx.bg_color_stack.auto_pop = 0;
	// -- text_color stack initialization
	ctx.text_color_nil_stack_top.v = v4m(1,1,1,1);
	ctx.text_color_stack.top = &ctx.text_color_nil_stack_top;
	ctx.text_color_stack.bottom_val = ctx.text_color_nil_stack_top.v;
	ctx.text_color_stack.free = 0;
	ctx.text_color_stack.auto_pop = 0;
	// -- text_alignment stack initialization
	ctx.text_alignment_nil_stack_top.v = GUI_TEXT_ALIGNMENT_CENTER;
	ctx.text_alignment_stack.top = &ctx.text_alignment_nil_stack_top;
	ctx.text_alignment_stack.bottom_val = ctx.text_alignment_nil_stack_top.v;
	ctx.text_alignment_stack.free = 0;
	ctx.text_alignment_stack.auto_pop = 0;
	// -- font_scale stack initialization
	ctx.font_scale_nil_stack_top.v = 0.5;
	ctx.font_scale_stack.top = &ctx.font_scale_nil_stack_top;
	ctx.font_scale_stack.bottom_val = ctx.font_scale_nil_stack_top.v;
	ctx.font_scale_stack.free = 0;
	ctx.font_scale_stack.auto_pop = 0;
	// -- child_layout_axis stack initialization
	ctx.child_layout_axis_nil_stack_top.v = GUI_AXIS_X;
	ctx.child_layout_axis_stack.top = &ctx.child_layout_axis_nil_stack_top;
	ctx.child_layout_axis_stack.bottom_val = ctx.child_layout_axis_nil_stack_top.v;
	ctx.child_layout_axis_stack.free = 0;
	ctx.child_layout_axis_stack.auto_pop = 0;
}

Gui_Box *gui_top_parent(void) { gui_stack_top_impl(gui_get_ctx(), Parent, parent); }
Gui_Box *gui_set_next_parent(Gui_Box *box) { gui_stack_set_next_impl(gui_get_ctx(), Parent, parent, Gui_Box*, box); }
Gui_Box *gui_push_parent(Gui_Box *box) { gui_stack_push_impl(gui_get_ctx(), Parent, parent, Gui_Box*, box); }
Gui_Box *gui_pop_parent(void) { gui_stack_pop_impl(gui_get_ctx(), Parent, parent); }

f32 gui_top_fixed_x(void) { gui_stack_top_impl(gui_get_ctx(), Fixed_X, fixed_x); }
f32 gui_set_next_fixed_x(f32 v) { gui_stack_set_next_impl(gui_get_ctx(), Fixed_X, fixed_x, f32, v); }
f32 gui_push_fixed_x(f32 v) { gui_stack_push_impl(gui_get_ctx(), Fixed_X, fixed_x, f32, v); }
f32 gui_pop_fixed_x(void) { gui_stack_pop_impl(gui_get_ctx(), Fixed_X, fixed_x); }

f32 gui_top_fixed_y(void) { gui_stack_top_impl(gui_get_ctx(), Fixed_Y, fixed_y); }
f32 gui_set_next_fixed_y(f32 v) { gui_stack_set_next_impl(gui_get_ctx(), Fixed_Y, fixed_y, f32, v); }
f32 gui_push_fixed_y(f32 v) { gui_stack_push_impl(gui_get_ctx(), Fixed_Y, fixed_y, f32, v); }
f32 gui_pop_fixed_y(void) { gui_stack_pop_impl(gui_get_ctx(), Fixed_Y, fixed_y); }

f32 gui_top_fixed_width(void) { gui_stack_top_impl(gui_get_ctx(), Fixed_Width, fixed_width); }
f32 gui_set_next_fixed_width(f32 v) { gui_stack_set_next_impl(gui_get_ctx(), Fixed_Width, fixed_width, f32, v); }
f32 gui_push_fixed_width(f32 v) { gui_stack_push_impl(gui_get_ctx(), Fixed_Width, fixed_width, f32, v); }
f32 gui_pop_fixed_width(void) { gui_stack_pop_impl(gui_get_ctx(), Fixed_Width, fixed_width); }

f32 gui_top_fixed_height(void) { gui_stack_top_impl(gui_get_ctx(), Fixed_Height, fixed_height); }
f32 gui_set_next_fixed_height(f32 v) { gui_stack_set_next_impl(gui_get_ctx(), Fixed_Height, fixed_height, f32, v); }
f32 gui_push_fixed_height(f32 v) { gui_stack_push_impl(gui_get_ctx(), Fixed_Height, fixed_height, f32, v); }
f32 gui_pop_fixed_height(void) { gui_stack_pop_impl(gui_get_ctx(), Fixed_Height, fixed_height); }

Gui_Size gui_top_pref_width(void) { gui_stack_top_impl(gui_get_ctx(), Pref_Width, pref_width); }
Gui_Size gui_set_next_pref_width(Gui_Size v) { gui_stack_set_next_impl(gui_get_ctx(), Pref_Width, pref_width, Gui_Size, v); }
Gui_Size gui_push_pref_width(Gui_Size v) { gui_stack_push_impl(gui_get_ctx(), Pref_Width, pref_width, Gui_Size, v); }
Gui_Size gui_pop_pref_width(void) { gui_stack_pop_impl(gui_get_ctx(), Pref_Width, pref_width); }

Gui_Size gui_top_pref_height(void) { gui_stack_top_impl(gui_get_ctx(), Pref_Height, pref_height); }
Gui_Size gui_set_next_pref_height(Gui_Size v) { gui_stack_set_next_impl(gui_get_ctx(), Pref_Height, pref_height, Gui_Size, v); }
Gui_Size gui_push_pref_height(Gui_Size v) { gui_stack_push_impl(gui_get_ctx(), Pref_Height, pref_height, Gui_Size, v); }
Gui_Size gui_pop_pref_height(void) { gui_stack_pop_impl(gui_get_ctx(), Pref_Height, pref_height); }

v4 gui_top_bg_color(void) { gui_stack_top_impl(gui_get_ctx(), Bg_Color, bg_color); }
v4 gui_set_next_bg_color(v4 v) { gui_stack_set_next_impl(gui_get_ctx(), Bg_Color, bg_color, v4, v); }
v4 gui_push_bg_color(v4 v) { gui_stack_push_impl(gui_get_ctx(), Bg_Color, bg_color, v4, v); }
v4 gui_pop_bg_color(void) { gui_stack_pop_impl(gui_get_ctx(), Bg_Color, bg_color); }

v4 gui_top_text_color(void) { gui_stack_top_impl(gui_get_ctx(), Text_Color, text_color); }
v4 gui_set_next_text_color(v4 v) { gui_stack_set_next_impl(gui_get_ctx(), Text_Color, text_color, v4, v); }
v4 gui_push_text_color(v4 v) { gui_stack_push_impl(gui_get_ctx(), Text_Color, text_color, v4, v); }
v4 gui_pop_text_color(void) { gui_stack_pop_impl(gui_get_ctx(), Text_Color, text_color); }

f32 gui_top_font_scale(void) { gui_stack_top_impl(gui_get_ctx(), Font_Scale, font_scale); }
f32 gui_set_next_font_scale(f32 v) { gui_stack_set_next_impl(gui_get_ctx(), Font_Scale, font_scale, f32, v); }
f32 gui_push_font_scale(f32 v) { gui_stack_push_impl(gui_get_ctx(), Font_Scale, font_scale, f32, v); }
f32 gui_pop_font_scale(void) { gui_stack_pop_impl(gui_get_ctx(), Font_Scale, font_scale); }

Gui_Text_Alignment gui_top_text_alignment(void) { gui_stack_top_impl(gui_get_ctx(), Text_Alignment, text_alignment); }
Gui_Text_Alignment gui_set_next_text_alignment(Gui_Text_Alignment v) { gui_stack_set_next_impl(gui_get_ctx(), Text_Alignment, text_alignment, Gui_Text_Alignment, v); }
Gui_Text_Alignment gui_push_text_alignment(Gui_Text_Alignment v) { gui_stack_push_impl(gui_get_ctx(), Text_Alignment, text_alignment, Gui_Text_Alignment, v); }
Gui_Text_Alignment gui_pop_text_alignment(void) { gui_stack_pop_impl(gui_get_ctx(), Text_Alignment, text_alignment); }

Gui_Axis gui_top_child_layout_axis(void) { gui_stack_top_impl(gui_get_ctx(), Child_Layout_Axis, child_layout_axis); }
Gui_Axis gui_set_next_child_layout_axis(Gui_Axis v) { gui_stack_set_next_impl(gui_get_ctx(), Child_Layout_Axis, child_layout_axis, Gui_Axis, v); }
Gui_Axis gui_push_child_layout_axis(Gui_Axis v) { gui_stack_push_impl(gui_get_ctx(), Child_Layout_Axis, child_layout_axis, Gui_Axis, v); }
Gui_Axis gui_pop_child_layout_axis(void) { gui_stack_pop_impl(gui_get_ctx(), Child_Layout_Axis, child_layout_axis); }

Gui_Size gui_push_pref_size(Gui_Axis axis, Gui_Size v) {
  Gui_Size result;
  switch(axis)
  {
    case GUI_AXIS_X: {result = gui_push_pref_width(v);}break;
    case GUI_AXIS_Y: {result = gui_push_pref_height(v);}break;
    default: break;
  }
  return result;
}

Gui_Size gui_pop_pref_size(Gui_Axis axis) {
  Gui_Size result;
  switch(axis)
  {
    case GUI_AXIS_X: {result = gui_pop_pref_width();}break;
    case GUI_AXIS_Y: {result = gui_pop_pref_height();}break;
    default: break;
  }
  return result;
}

Gui_Size gui_set_next_pref_size(Gui_Axis axis, Gui_Size v) {
  if (axis == GUI_AXIS_X){
    return gui_set_next_pref_width(v);
  }
  return gui_set_next_pref_height(v);
}


void gui_push_rect(rect r) {
  gui_push_fixed_x(r.p.x);
  gui_push_fixed_y(r.p.y);
  gui_push_fixed_width(r.dim.x);
  gui_push_fixed_height(r.dim.y);
}

void gui_pop_rect(void) {
  gui_pop_fixed_x();
  gui_pop_fixed_y();
  gui_pop_fixed_width();
  gui_pop_fixed_height();
}

void gui_set_next_rect(rect r) {
  gui_set_next_fixed_x(r.p.x);
  gui_set_next_fixed_y(r.p.y);
  gui_set_next_fixed_width(r.dim.x);
  gui_set_next_fixed_height(r.dim.y);
}

void gui_autopop_all_stacks() {
	if (ctx.parent_stack.auto_pop) { gui_pop_parent();ctx.parent_stack.auto_pop = 0; }
	if (ctx.fixed_x_stack.auto_pop) { gui_pop_fixed_x();ctx.fixed_x_stack.auto_pop = 0; }
	if (ctx.fixed_y_stack.auto_pop) { gui_pop_fixed_y();ctx.fixed_y_stack.auto_pop = 0; }
	if (ctx.fixed_width_stack.auto_pop) { gui_pop_fixed_width();ctx.fixed_width_stack.auto_pop = 0; }
	if (ctx.fixed_height_stack.auto_pop) { gui_pop_fixed_height();ctx.fixed_height_stack.auto_pop = 0; }
	if (ctx.pref_width_stack.auto_pop) { gui_pop_pref_width();ctx.pref_width_stack.auto_pop = 0; }
	if (ctx.pref_height_stack.auto_pop) { gui_pop_pref_height();ctx.pref_height_stack.auto_pop = 0; }
	if (ctx.bg_color_stack.auto_pop) { gui_pop_bg_color();ctx.bg_color_stack.auto_pop = 0; }
	if (ctx.text_color_stack.auto_pop) { gui_pop_text_color();ctx.text_color_stack.auto_pop = 0; }
	if (ctx.text_alignment_stack.auto_pop) { gui_pop_text_alignment();ctx.text_alignment_stack.auto_pop = 0; }
	if (ctx.font_scale_stack.auto_pop) { gui_pop_font_scale();ctx.font_scale_stack.auto_pop = 0; }
	if (ctx.child_layout_axis_stack.auto_pop) { gui_pop_child_layout_axis();ctx.child_layout_axis_stack.auto_pop = 0; }
}
#endif

