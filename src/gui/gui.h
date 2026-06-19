#ifndef GUI_H__
#define GUI_H__

#include "base/base_inc.h"
#include "core/core_inc.h"

typedef enum {
  GUI_AXIS_X,
  GUI_AXIS_Y,
} Gui_Axis;

static Gui_Axis gui_axis_flip(Gui_Axis axis) {
  return (axis == GUI_AXIS_X) ? GUI_AXIS_Y : GUI_AXIS_X;
}

typedef enum {
  GUI_TEXT_ALIGNMENT_LEFT,
  GUI_TEXT_ALIGNMENT_RIGHT,
  GUI_TEXT_ALIGNMENT_CENTER,
} Gui_Text_Alignment;

typedef enum {
  GUI_SIZE_KIND_NULL,
  GUI_SIZE_KIND_PIXELS,
  GUI_SIZE_KIND_TEXT_CONTENT,
  GUI_SIZE_KIND_PARENT_PCT,
  GUI_SIZE_KIND_CHILDREN_SUM,
} Gui_Size_Kind;

typedef struct {
  Gui_Size_Kind kind;
  f32 value;
  f32 strictness;
} Gui_Size;

typedef enum {
  GB_FLAG_CLICKABLE             = (1 << 0),
  GB_FLAG_FOCUS_HOT             = (1 << 1),
  GB_FLAG_FOCUS_ACTIVE          = (1 << 2),
  GB_FLAG_FIXED_WIDTH           = (1 << 3),
  GB_FLAG_FIXED_HEIGHT          = (1 << 4),
  GB_FLAG_FIXED_X               = (1 << 5),
  GB_FLAG_FIXED_Y               = (1 << 6),
  GB_FLAG_DRAW_HOT_ANIMATION    = (1 << 7),
  GB_FLAG_DRAW_ACTIVE_ANIMATION = (1 << 8),
  GB_FLAG_DRAW_BACKGROUND       = (1 << 9),
  GB_FLAG_DRAW_TEXT             = (1 << 10),
  GB_FLAG_CLIP                  = (1 << 11),
  GB_FLAG_HOVERING              = (1 << 12),
  GB_FLAG_OVERFLOW_X            = (1 << 13),
  GB_FLAG_OVERFLOW_Y            = (1 << 14),
  GB_FLAG_VIEW_SCROLL_X         = (1 << 15),
  GB_FLAG_VIEW_SCROLL_Y         = (1 << 16),
} Gui_Box_Flags;

typedef u64 Gui_Key;

typedef struct Gui_Box Gui_Box;
struct Gui_Box {
  // tree links
  Gui_Box* first;
  Gui_Box* last;
  Gui_Box* next;
  Gui_Box* prev;
  Gui_Box* parent;

  // hash links
  Gui_Box* hash_next;
  Gui_Box* hash_prev;

  // keying
  Gui_Key key;
  buf s;

  // layouting state
  v2 fixed_pos;
  v2 fixed_size;
  Gui_Size pref_size[2];
  Gui_Axis child_layout_axis;
  rect r; // used for immediate mode calculation AND for final drawing

  Gui_Box_Flags flags;
  f32 transparency;
  u64 last_used_frame_idx;
  v4 color;

  v4 text_color;
  Gui_Text_Alignment text_alignment;
  f32 font_scale;
  //v4 roundness;

  f32 hot_t;
	f32 active_t;
	u32 child_count;
	v2 view_off;
	v2 view_off_target; // for when we do better animations!
};

typedef struct Gui_Box_Hash_Slot Gui_Box_Hash_Slot;
struct Gui_Box_Hash_Slot {
  Gui_Box *hash_first;
  Gui_Box *hash_last;
};

typedef struct Gui_Panel Gui_Panel;
struct Gui_Panel {
  Gui_Panel *first;
  Gui_Panel *last;
  Gui_Panel *next;
  Gui_Panel *prev;
  Gui_Panel *parent;

  f32 parent_pct;
  Gui_Axis split_axis;
  buf label;
};

typedef struct Gui_Panel_Itr Gui_Panel_Itr;
typedef struct Gui_Panel_Itr_Node Gui_Panel_Itr_Node;
struct Gui_Panel_Itr { Gui_Panel *child; Gui_Panel *parent; };

typedef struct Gui_Parent_Node Gui_Parent_Node; struct Gui_Parent_Node{Gui_Parent_Node *next; Gui_Box *v; };
typedef struct Gui_Pref_Width_Node Gui_Pref_Width_Node; struct Gui_Pref_Width_Node {Gui_Pref_Width_Node *next; Gui_Size v;};
typedef struct Gui_Pref_Height_Node Gui_Pref_Height_Node; struct Gui_Pref_Height_Node {Gui_Pref_Height_Node *next; Gui_Size v;};
typedef struct Gui_Fixed_X_Node Gui_Fixed_X_Node; struct Gui_Fixed_X_Node {Gui_Fixed_X_Node *next; f32 v;};
typedef struct Gui_Fixed_Y_Node Gui_Fixed_Y_Node; struct Gui_Fixed_Y_Node {Gui_Fixed_Y_Node *next; f32 v;};
typedef struct Gui_Fixed_Width_Node Gui_Fixed_Width_Node; struct Gui_Fixed_Width_Node {Gui_Fixed_Width_Node *next; f32 v;};
typedef struct Gui_Fixed_Height_Node Gui_Fixed_Height_Node; struct Gui_Fixed_Height_Node {Gui_Fixed_Height_Node *next; f32 v;};
typedef struct Gui_Bg_Color_Node Gui_Bg_Color_Node; struct Gui_Bg_Color_Node {Gui_Bg_Color_Node *next; v4 v;};
typedef struct Gui_Text_Color_Node Gui_Text_Color_Node; struct Gui_Text_Color_Node {Gui_Text_Color_Node *next; v4 v;};
typedef struct Gui_Text_Alignment_Node Gui_Text_Alignment_Node; struct Gui_Text_Alignment_Node {Gui_Text_Alignment_Node *next; Gui_Text_Alignment v;};
typedef struct Gui_Font_Scale_Node Gui_Font_Scale_Node; struct Gui_Font_Scale_Node {Gui_Font_Scale_Node *next; f32 v;};
typedef struct Gui_Child_Layout_Axis_Node Gui_Child_Layout_Axis_Node; struct Gui_Child_Layout_Axis_Node {Gui_Child_Layout_Axis_Node *next; Gui_Axis v;};
typedef struct Gui_Panel_Itr_Node Gui_Panel_Itr_Node; struct Gui_Panel_Itr_Node{Gui_Panel_Itr_Node *next; Gui_Panel_Itr v; };

typedef enum {
	GUI_SIGNAL_FLAG_LMB_PRESSED  = (1<<0),
	GUI_SIGNAL_FLAG_MMB_PRESSED  = (1<<1),
	GUI_SIGNAL_FLAG_RMB_PRESSED  = (1<<2),
	GUI_SIGNAL_FLAG_LMB_RELEASED = (1<<3),
	GUI_SIGNAL_FLAG_MMB_RELEASED = (1<<4),
	GUI_SIGNAL_FLAG_RMB_RELEASED = (1<<5),
	GUI_SIGNAL_FLAG_MOUSE_HOVER  = (1<<7),
	GUI_SIGNAL_FLAG_SCROLLED     = (1<<7),
	// TODO -- maybe we need one dragging for each mouse key
	GUI_SIGNAL_FLAG_DRAGGING     = (1<<8),
	// ...
} Gui_Signal_Flag;

typedef struct {
	Gui_Box *box;
	v2 mouse;
	v2 drag_delta;
	Gui_Signal_Flag flags;
} Gui_Signal;

Gui_Signal gui_get_signal_for_box(Gui_Box *box);

typedef struct {
  Arena *temp_arena;
  Arena *persistent_arena;
  Font_Info *font;
  Input *input_ref;

  v2 screen_dim;
  f64 dt;

  Gui_Key hot_box_key;
  Gui_Key active_box_keys[INPUT_MOUSE_COUNT];

  u64 frame_idx;

  Gui_Box* root;
  Gui_Box* box_freelist;

  Gui_Panel* root_panel;
  Gui_Panel* panel_freelist;

#define GUI_SLOT_COUNT 64
	Gui_Box_Hash_Slot *slots;
  u32 slot_count;
  
  // The Stacks!
	Gui_Parent_Node parent_nil_stack_top;
	struct { Gui_Parent_Node *top; Gui_Box * bottom_val; Gui_Parent_Node *free; b32 auto_pop; } parent_stack;
	Gui_Fixed_X_Node fixed_x_nil_stack_top;
	struct { Gui_Fixed_X_Node *top; f32 bottom_val; Gui_Fixed_X_Node *free; b32 auto_pop; } fixed_x_stack;
	Gui_Fixed_Y_Node fixed_y_nil_stack_top;
	struct { Gui_Fixed_Y_Node *top; f32 bottom_val; Gui_Fixed_Y_Node *free; b32 auto_pop; } fixed_y_stack;
	Gui_Fixed_Width_Node fixed_width_nil_stack_top;
	struct { Gui_Fixed_Width_Node *top; f32 bottom_val; Gui_Fixed_Width_Node *free; b32 auto_pop; } fixed_width_stack;
	Gui_Fixed_Height_Node fixed_height_nil_stack_top;
	struct { Gui_Fixed_Height_Node *top; f32 bottom_val; Gui_Fixed_Height_Node *free; b32 auto_pop; } fixed_height_stack;
	Gui_Pref_Width_Node pref_width_nil_stack_top;
	struct { Gui_Pref_Width_Node *top; Gui_Size bottom_val; Gui_Pref_Width_Node *free; b32 auto_pop; } pref_width_stack;
	Gui_Pref_Height_Node pref_height_nil_stack_top;
	struct { Gui_Pref_Height_Node *top; Gui_Size bottom_val; Gui_Pref_Height_Node *free; b32 auto_pop; } pref_height_stack;
	Gui_Bg_Color_Node bg_color_nil_stack_top;
	struct { Gui_Bg_Color_Node *top; v4 bottom_val; Gui_Bg_Color_Node *free; b32 auto_pop; } bg_color_stack;
	Gui_Text_Color_Node text_color_nil_stack_top;
	struct { Gui_Text_Color_Node *top; v4 bottom_val; Gui_Text_Color_Node *free; b32 auto_pop; } text_color_stack;
	Gui_Font_Scale_Node font_scale_nil_stack_top;
	struct { Gui_Font_Scale_Node *top; f32 bottom_val; Gui_Font_Scale_Node *free; b32 auto_pop; } font_scale_stack;
	Gui_Text_Alignment_Node text_alignment_nil_stack_top;
	struct { Gui_Text_Alignment_Node *top; Gui_Text_Alignment bottom_val; Gui_Text_Alignment_Node *free; b32 auto_pop; } text_alignment_stack;
	Gui_Child_Layout_Axis_Node child_layout_axis_nil_stack_top;
	struct { Gui_Child_Layout_Axis_Node *top; Gui_Axis bottom_val; Gui_Child_Layout_Axis_Node *free; b32 auto_pop; } child_layout_axis_stack;
  Gui_Panel_Itr_Node panel_itr_nil_stack_top;
	struct { Gui_Panel_Itr_Node *top; Gui_Panel_Itr bottom_val; Gui_Panel_Itr_Node *free; b32 auto_pop; } panel_itr_stack;
	
} Gui_Context;



void gui_context_init(Arena *temp_arena, Font_Info *font);
Arena* gui_get_build_arena();
void gui_frame_begin(v2 screen_dim, Input *input, f64 dt);
void gui_frame_end();
void gui_render_hierarchy(Gui_Box *box);

Gui_Key gui_key_zero(void);
Gui_Key gui_key_from_str(buf s);
b32 gui_key_match(Gui_Key a, Gui_Key b);

Gui_Context* gui_get_ctx();
Gui_Box *gui_box_nil_id();
b32 gui_box_is_nil(Gui_Box *box);
Gui_Box *gui_box_make(Gui_Box_Flags flags, buf s);
Gui_Box *gui_box_lookup_from_key(Gui_Box_Flags flags, Gui_Key key);
Gui_Box *gui_box_build_from_str(Gui_Box_Flags flags, buf s);
Gui_Box *gui_box_build_from_key(Gui_Box_Flags flags, Gui_Key key, buf s);

void gui_autopop_all_stacks();
void gui_init_stacks();

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


void gui_layout_root(Gui_Box *root, Gui_Axis axis);
Gui_Axis gui_top_child_layout_axis(void);
Gui_Axis gui_set_next_child_layout_axis(Gui_Axis v);
Gui_Axis gui_push_child_layout_axis(Gui_Axis v);
Gui_Axis gui_pop_child_layout_axis(void);

Gui_Panel_Itr gui_top_panel_itr(void);
Gui_Panel_Itr gui_set_next_panel_itr(Gui_Panel_Itr itr);
Gui_Panel_Itr gui_push_panel_itr(Gui_Panel_Itr itr);
Gui_Panel_Itr gui_pop_panel_itr(void);
bool gui_empty_panel_itr(void);
void gui_panel_layout_panels_and_boundaries(Gui_Panel *root_panel, rect root_rect);

// pushes fixed widths heights (TODO -- i should probably add all the lower level stack functions in future)
void gui_push_rect(rect r);
void gui_set_next_rect(rect r);
void gui_pop_rect(void);

Gui_Size gui_push_pref_size(Gui_Axis axis, Gui_Size v);
Gui_Size gui_set_next_pref_size(Gui_Axis axis, Gui_Size v);
Gui_Size gui_pop_pref_size(Gui_Axis axis);

// widgets
Gui_Signal gui_button(buf s);
Gui_Signal gui_pane(buf s);
Gui_Signal gui_label(buf s);
Gui_Signal gui_spacer(Gui_Size size);

typedef struct {
  f32 scroll_percent;
  f32 item_px;
  s32 item_count;

  u32 scroll_bar_px;
  u32 scroll_button_px;
  color scroll_button_color;
  f32 scroll_speed;
} Gui_Scroll_Data;

Gui_Signal gui_scroll_list_begin(buf s, Gui_Axis axis, Gui_Scroll_Data* sdata);
void gui_scroll_list_end(buf s);

Gui_Signal gui_multi_line_text(buf s, buf text);

typedef enum {
  GUI_DIALOG_STATE_NOTHING_HAPPENED = 0,
  GUI_DIALOG_STATE_NEXT_PRESSED = 1,
  GUI_DIALOG_STATE_PREV_PRESSED = 2,
}Gui_Dialog_State;
Gui_Dialog_State gui_dialog(buf id, buf person_name, buf prompt);
int gui_choice_box(buf id, buf *choices, int count);

typedef struct {
  b32 exit_btn_pressed;
  b32 start_btn_pressed;

  b32 fullscreen;
  b32 turbo;

  f32 music_volume;
  f32 fx_volume;
} Simple_Game_Options;

void gui_simple_game_options_menu(buf s, Simple_Game_Options *opt);

#endif
