#ifndef GUI2_H_
#define GUI2_H_
#include "core/core_inc.h"

typedef u64 Gui_ID;


typedef enum {
  G2_BOX_FLAG_CLICKABLE = (0x1 << 0),
  G2_BOX_FLAG_FIXED_X = (0x1 << 1),
  G2_BOX_FLAG_FIXED_Y = (0x1 << 2),
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
#define G2_AXIS_COUNT 2
  f32 local_coords[G2_AXIS_COUNT];
  f32 coords[G2_AXIS_COUNT];

  // misc
  Gui_ID id;
  buf label;
  Gui_Box_Flags flags;
  s64 frame_idx;
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

  f32 fixed_x;
  f32 fixed_y;
  Gui_Box_Flags flags;
} Gui_Layout_Params;

// Interface that we should provide (for now)
void bfont_draw_text(Font_Info *font_info, Arena *arena, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box);

void gui_init(Arena *tarena, Font_Info *font, Input *input);


b32 gui_button(buf label, Gui_Layout_Params params);
 
void gui_begin(rect viewport);
void gui_end();

#endif
