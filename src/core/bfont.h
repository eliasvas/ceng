#ifndef BFONT_H__
#define BFONT_H__
#include "base/base_inc.h"
#include "core/asset_mgr.h"
#include "rend.h"

// TODO: LOD stuff and our own lookup data structure (Glyph_Cache?)
// TODO: SDF font support

typedef struct {
  rect r;
  rect tc;
  v2 off;
  v2 dim;
  f32 xadvance;
} Glyph_Info;

// TODO: Maybe we can use a stack allocator for the permanent arena, so the Font_Info's glyphs array can be allocated there (in the end)
typedef struct {
  Glyph_Info glyphs[200];
  u32 first_codepoint;
  u32 last_codepoint;
  s32 glyph_count;

  f32 glyph_height_in_px;
  f32 ascent_px; // distance from baseline to hightest glyph extent
  f32 descent_px; // distance from baseline to lowest glyph extent
  f32 line_gap_px; // extra spacing between lines

  Asset_Id tex_id;
  v2 tex_dim;
}Font_Info;

Font_Info bfont_load_default_atlas(Arena *arena, Arena *temp_arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height);
void bfont_flip_bitmap(u8 *bitmap, s32 width, s32 height);
f32 bfont_measure_text_width(Font_Info *font_info, str8 text, f32 scale);
f32 bfont_measure_text_height(Font_Info *font_info, str8 text, f32 scale);
rect bfont_calc_text_rect(Font_Info *font_info, str8 text, v2 baseline_pos, f32 scale);
s64 bfont_count_glyphs_until_width(Font_Info *font_info, str8 text, f32 scale, f32 target_width);

// TODO: This should be elsewhere ok?
void bfont_draw_text(Font_Info *font_info, Arena *arena, rect viewport, rect clip_rect, str8 text, v2 baseline_pos, f32 scale, color col, bool draw_box);
#endif
