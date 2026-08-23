#ifndef GUI2_H_
#define GUI2_H_
#include "core/core_inc.h"

// @IDEA: Have a gui_render layer where a command buffer API
// will be used and output just rectangles + texcoords for drawing,
// having clipped them correctly and applied alpha and everything,
// and make that agnostic to the actual rendering API, then these
// commands will be fed to an SDF renderer for the actual work,
// that way we dont have to hardware clip and can draw UI in 1 drawcall (probably)

/*
@HMMM: Maybe the whole 2d renderer should do the SDF thing but also have a view matrix
so we are FULLY generic and can render game graphics or UI with the same shader?
Think about this a bit not sure we can do that, look into the SDF more closely.
*/
typedef u64 Gui_ID;

typedef enum {
  GUI_TEXT_ALIGNMENT_LEFT,
  GUI_TEXT_ALIGNMENT_RIGHT,
  GUI_TEXT_ALIGNMENT_CENTER,
} Gui_Text_Alignment;

typedef enum {
  GUI_SIZEKIND_NONE,
  GUI_SIZEKIND_PIXELS,
  GUI_SIZEKIND_TEXT_CONTENT,
  GUI_SIZEKIND_PERCENT_OF_PARENT,
  GUI_SIZEKIND_SUM_OF_CHILDREN,
} Gui_SizeKind;

typedef struct {
  Gui_SizeKind kind;
  f32 value; // can encode multiple stuff (e.g perc_of_parent or pixels or padding)
  f32 strictness; // How big percentage box is willing to give up
} Gui_Size;

#define GUI_AXIS_COUNT 2
typedef enum {
  GUI_AXIS_X = 0,
  GUI_AXIS_Y = 1,
} Gui_Axis;

typedef enum {
  GUI_BOX_FLAG_CLICKABLE        = (0x1 <<  0),
  GUI_BOX_FLAG_SCROLLABLE       = (0x1 <<  1),
  GUI_BOX_FLAG_FIXED_X          = (0x1 <<  2),
  GUI_BOX_FLAG_FIXED_Y          = (0x1 <<  3),
  GUI_BOX_FLAG_FIXED_WIDTH      = (0x1 <<  4),
  GUI_BOX_FLAG_FIXED_HEIGHT     = (0x1 <<  5),
  GUI_BOX_FLAG_ALLOW_OVERFLOW_X = (0x1 <<  6),
  GUI_BOX_FLAG_ALLOW_OVERFLOW_Y = (0x1 <<  7),
  GUI_BOX_FLAG_DRAW_BOX         = (0x1 <<  8),
  GUI_BOX_FLAG_DRAW_TEXT        = (0x1 <<  9),
  GUI_BOX_FLAG_CLIP             = (0x1 << 10),
  GUI_BOX_FLAG_VIEW_CLAMP_X     = (0x1 << 11),
  GUI_BOX_FLAG_VIEW_CLAMP_Y     = (0x1 << 12),
} Gui_Box_Flags;

typedef struct Gui_Box Gui_Box;
struct Gui_Box {
  // For layout
  Gui_Box *first;
  Gui_Box *last;
  Gui_Box *next;
  Gui_Box *prev;
  Gui_Box *parent;
  s32 child_count;
  // For hashmap 
  Gui_Box *hash_next;
  Gui_Box *hash_prev;

  // TODO: Maybe add anim_coords????
  rect final_rect;

  v2 fixed_pos;
  v2 fixed_size;
  Gui_Size pref_size[2];
  v2 view_off;

  // max width/height of whole box (not just visible part
  v2 view_bounds;

  // misc
  Gui_ID id;
  str8 label;
  Gui_Box_Flags flags;
  Gui_Axis major_layout_axis;
  Gui_Text_Alignment text_align;
  v4 bg_color;
  v4 text_color;
 
  f32 corner_radius; // TODO: maybe corner_radii down the line?
  f32 softness;

  f32 hot_t;
  f32 active_t;

  s64 last_frame_used;
};

typedef struct Gui_Box_Hash_Slot Gui_Box_Hash_Slot;
struct Gui_Box_Hash_Slot {
  Gui_Box *box;

  // Embedded hash links
  Gui_Box *hash_first;
  Gui_Box *hash_last;
};


typedef enum {
  GUI_SIGNAL_FLAG_LMB_PRESSED  = (0x1 << 0),
  GUI_SIGNAL_FLAG_MMB_PRESSED  = (0x1 << 1),
  GUI_SIGNAL_FLAG_RMB_PRESSED  = (0x1 << 2),
  GUI_SIGNAL_FLAG_LMB_RELEASED = (0x1 << 3),
  GUI_SIGNAL_FLAG_MMB_RELEASED = (0x1 << 4),
  GUI_SIGNAL_FLAG_RMB_RELEASED = (0x1 << 5),
} Gui_Signal_Flags;

typedef struct {
  // TODO: Add a scroll value?
  Gui_Signal_Flags sflags;
  Gui_Box *box;
}Gui_Signal;

void gui_init(Arena *tarena, Font_Info *font, Input *input);
Gui_Box *gui_nil_box();
Gui_Signal gui_button(str8 label);
Gui_Signal gui_pane(str8 label);
Gui_Signal gui_spacer(Gui_Axis layout_axis, Gui_Size size);


typedef struct {
  f32 scroll_percent;
  f32 item_px;
  s32 item_count;

  u32 scroll_bar_px;
  u32 scroll_button_px;
  color scroll_button_color;
  f32 scroll_speed;
} Gui_Scroll_Data;
Gui_Signal gui_scroll_list_begin(str8 s, Gui_Axis axis, Gui_Scroll_Data* sdata);
void gui_scroll_list_end(str8 s);

void gui_begin(rect viewport, f32 dt);
void gui_end();


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
typedef struct Gui_Box_Corner_Radius_Node Gui_Box_Corner_Radius_Node; struct Gui_Box_Corner_Radius_Node {Gui_Box_Corner_Radius_Node *next; f32 v;};
typedef struct Gui_Box_Softness_Node Gui_Box_Softness_Node; struct Gui_Box_Softness_Node {Gui_Box_Softness_Node *next; f32 v;};
typedef struct Gui_Child_Layout_Axis_Node Gui_Child_Layout_Axis_Node; struct Gui_Child_Layout_Axis_Node {Gui_Child_Layout_Axis_Node *next; Gui_Axis v;};
#include "gui_stacks.h"

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
  f32 dt;

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
  Gui_Box_Corner_Radius_Node box_corner_radius_nil_stack_top;
  struct { Gui_Box_Corner_Radius_Node *top; f32 bottom_val; Gui_Box_Corner_Radius_Node *free; b32 auto_pop; } box_corner_radius_stack;
  Gui_Box_Softness_Node box_softness_nil_stack_top;
  struct { Gui_Box_Softness_Node *top; f32 bottom_val; Gui_Box_Softness_Node *free; b32 auto_pop; } box_softness_stack;
  Gui_Text_Alignment_Node text_alignment_nil_stack_top;
  struct { Gui_Text_Alignment_Node *top; Gui_Text_Alignment bottom_val; Gui_Text_Alignment_Node *free; b32 auto_pop; } text_alignment_stack;
  Gui_Child_Layout_Axis_Node child_layout_axis_nil_stack_top;
  struct { Gui_Child_Layout_Axis_Node *top; Gui_Axis bottom_val; Gui_Child_Layout_Axis_Node *free; b32 auto_pop; } child_layout_axis_stack;

} Gui_Ctx;


#endif
