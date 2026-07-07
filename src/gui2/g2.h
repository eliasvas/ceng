#ifndef GUI2_H_
#define GUI2_H_
#include "core/core_inc.h"

typedef u64 Gui_ID;

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
  GUI_AXIS_X,
  GUI_AXIS_Y,
} Gui_Axis;

typedef enum {
  GUI_BOX_FLAG_CLICKABLE    = (0x1 << 0),
  GUI_BOX_FLAG_FIXED_X      = (0x1 << 1),
  GUI_BOX_FLAG_FIXED_Y      = (0x1 << 2),
  GUI_BOX_FLAG_FIXED_WIDTH  = (0x1 << 3),
  GUI_BOX_FLAG_FIXED_HEIGHT = (0x1 << 4),
  GUI_BOX_FLAG_DRAW_BOX     = (0x1 << 5),
  GUI_BOX_FLAG_DRAW_TEXT    = (0x1 << 6),
} Gui_Box_Flags;

typedef struct Gui_Box Gui_Box;
struct Gui_Box {
  // For layout
  Gui_Box *first;
  Gui_Box *last;
  Gui_Box *next;
  Gui_Box *prev;
  Gui_Box *parent;
  // For hashmap 
  Gui_Box *hash_next;
  Gui_Box *hash_prev;

  // TODO: Maybe add anim_coords????
  //rect local_rect;
  rect final_rect;

  v2 fixed_pos;
  v2 fixed_size;
  Gui_Size pref_size[2];

  // misc
  Gui_ID id;
  buf label;
  Gui_Box_Flags flags;
  Gui_Axis major_layout_axis;

  s64 last_frame_used;
  f32 color_mod;
};

typedef struct Gui_Box_Hash_Slot Gui_Box_Hash_Slot;
struct Gui_Box_Hash_Slot {
  Gui_Box *box;

  // Embedded hash links
  Gui_Box *hash_first;
  Gui_Box *hash_last;
};


// TODO: We should implement style stacks, this is temporary..
typedef struct {
  Gui_Box *parent;

  Gui_Axis major_layout_axis;
  Gui_Box_Flags flags;
} Gui_Layout_Params;

// Interface that we should provide (for now)
void bfont_draw_text(Font_Info *font_info, Arena *arena, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box);

void gui_init(Arena *tarena, Font_Info *font, Input *input);


b32 gui_button(buf label, Gui_Layout_Params params);
 
void gui_begin(rect viewport);
void gui_end();

#endif
