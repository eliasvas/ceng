
#include "bfont.h"

#include "base/base_inc.h"
#include "rend/rend_inc.h"


// By default we just embed ProggyClean - Ugly AF but for now it'll do!
static const u8 default_font_data[] = {
#embed "../../data/ProggyClean.ttf"
};

extern Font_Info platform_load_font(Arena *arena, u8 *font_data, u32 font_byte_count, u32 atlas_width, u32 atlas_height, u32 glyph_height_in_px, u8 **bitmap);

// FIXME: persistent arena can be used for temporary allocations you know!
Font_Info bfont_load_default_atlas(Arena *arena, Arena *temp_arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height) {

  u8 *font_bitmap = nullptr;
  Font_Info font = platform_load_font(arena, (u8*)default_font_data, sizeof(default_font_data), atlas_width, atlas_height, glyph_height_in_px, &font_bitmap);

  // Transform to RGBA
  u64 temp_arena_pos_start = arena_get_current_pos(temp_arena);
  u8 *font_bitmap_rgba = (u8*)arena_push_array(temp_arena, u8, sizeof(u32)*atlas_width*atlas_height);
  for (u32 i = 0; i < atlas_width; i+=1) {
    for (u32 j = 0; j < atlas_height; j+=1) {
      for (u32 comp = 0; comp < 4; comp+=1) {
        font_bitmap_rgba[4*(i + (atlas_height - 1 - j) * atlas_width) + comp] = font_bitmap[i + j *atlas_width];
      }
    }
  }

  font.tex_dim = v2m(atlas_width, atlas_height);
  font.tex_id = am_load_from_data(STR8L("fa.png"), STR8(font_bitmap_rgba, sizeof(u32)*atlas_width*atlas_height));
  //font.tex_id = am_load_from_data(STR8L("fa.png"), STR8(font_bitmap_rgba, sizeof(u32)*atlas_width*atlas_height));
  arena_reset_to_pos(temp_arena, temp_arena_pos_start);

  return font;
}

rect bfont_calc_text_rect(Font_Info *font_info, str8 text, v2 pos, f32 scale) {
  u32 glyph_count = text.count;
  if (glyph_count == 0) return (rect){};

  Glyph_Info first_glyph = font_info->glyphs[text.data[0] - font_info->first_codepoint];
  rect r = (rect) {
    .x = pos.x,
    .y = pos.y,// + first_glyph.off.y*scale,
    .w = first_glyph.dim.x*scale,
    .h = first_glyph.dim.y*scale,
  };

  for (u32 glyph_idx = 0; glyph_idx < glyph_count; ++glyph_idx) {
    Glyph_Info glyph = font_info->glyphs[text.data[glyph_idx] - font_info->first_codepoint];
    rect r1 = (rect) {
      .x = pos.x + glyph.off.x*scale,
      .y = pos.y,// + glyph.off.y*scale,
      .w = glyph.dim.x*scale,
      //.h = glyph.dim.y*scale,
      .h = font_info->glyph_height_in_px,
    };
    pos.x += glyph.xadvance*scale;
    r = rect_calc_bounding_rect(r, r1);
  }

  return r;
}

f32 bfont_measure_text_width(Font_Info *font_info, str8 text, f32 scale) {
  return bfont_calc_text_rect(font_info, text, v2m(0,0), scale).w;
}

f32 bfont_measure_text_height(Font_Info *font_info, str8 text, f32 scale) {
  return bfont_calc_text_rect(font_info, text, v2m(0,0), scale).h;
}

s64 bfont_count_glyphs_until_width(Font_Info *font_info, str8 text, f32 scale, f32 target_width) {
  s64 glyph_count = 0;
  while (glyph_count < text.count) {
    f32 text_w = bfont_measure_text_width(font_info, STR8(text.data, glyph_count), scale);
    if (text_w >= target_width) {
      if (glyph_count > 0) glyph_count -= 1;
      break;
    } else {
      glyph_count+=1;
    }
  }
  return glyph_count;
}

void bfont_draw_text(Font_Info *font_info, Arena *arena, rect viewport, rect clip_rect, str8 text, v2 pos, f32 scale, color col, bool draw_bounding_box) {
  rect tr = bfont_calc_text_rect(font_info, text, pos, scale);
  Ogl_Tex *font_tex = (Ogl_Tex*)am_get(font_info->tex_id);

  if (draw_bounding_box) {
    R_Quad quad = (R_Quad) {
        .dst_rect = tr,
        .c = col(0.9,0.4,0.4,1.0),
    };
    r2d_push_quad(r2d_pass_front(), quad);
  }

  v2 baseline_pos = pos;
  baseline_pos.y -= font_info->descent_px * scale;
  for (s32 i = 0; i < text.count; i+=1) {
    u8 c = text.data[i];
    Glyph_Info metrics = font_info->glyphs[c - font_info->first_codepoint];
    f32 atlas_height = font_info->tex_dim.y;
    R_Quad quad = (R_Quad) {
        .dst_rect = 
          rect_clip_against(
            rec(baseline_pos.x + ((i==0)?0:metrics.off.x*scale), 
              baseline_pos.y - (metrics.off.y*scale + metrics.r.h*scale), 
              metrics.r.w*scale, metrics.r.h*scale),
            clip_rect
        ),
        .src_rect = rec(metrics.r.x,
            atlas_height - metrics.r.y - metrics.r.h,
            metrics.r.w,
            metrics.r.h
        ),
        .c = col,
        .tex = font_tex,
    };
    r2d_push_quad(r2d_pass_front(), quad);
    baseline_pos.x += metrics.xadvance*scale;
  }
}
