#ifndef _FONT_UTIL_H__
#define _FONT_UTIL_H__
#include "base/base_inc.h"
#include "core/ogl.h" // TODO: This should GO
#include "r2d.h"

// TODO: LOD stuff and our own lookup data structure (Glyph_Cache?)
// TODO: SDF font support
typedef struct {
  rect r;
  rect tc;
  v2 off;
  f32 xadvance;
}Glyph_Info;

// TODO: Maybe we can use a stack allocator for the permanent arena, so the Font_Info's glyphs array can be allocated there (in the end)
typedef struct {
  Glyph_Info glyphs[200];
  u32 first_codepoint;
  u32 last_codepoint;
  u32 glyph_count;

  Ogl_Tex atlas;
}Font_Info;

Font_Info font_util_load_default_atlas(Arena *arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height);
void font_util_flip_bitmap(u8 *bitmap, u32 width, u32 height);
rect font_util_calc_text_rect(Font_Info *font_info, buf text, v2 baseline_pos, f32 scale);
f32 font_util_measure_text_width(Font_Info *font_info, buf text, f32 scale);
f32 font_util_measure_text_height(Font_Info *font_info, buf text, f32 scale);
s64 font_util_count_glyphs_until_width(Font_Info *font_info, buf text, f32 scale, f32 target_width);

// TODO: This should be elsewhere ok?
void font_util_debug_draw_text(Font_Info *font_info, Arena *arena, R2D_Cmd_Chunk_List *cmd_list, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box);

#endif
