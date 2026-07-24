#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

// int stbi_write_png_compression_level = 0;
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "bfont.h"

#include "base/base_inc.h"
#include "ogl.h"
#include "rend.h"

// By default we just embed ProggyClean - Ugly AF but for now it'll do!
static const u8 default_font_data[] = {
#embed "../../data/ProggyClean.ttf"
};

void png_stbi_write_func(void *context, void *data, int size) {
  str8 *pinfo = (str8*)context;
  memcpy(pinfo->data+pinfo->count, data, size);
  pinfo->count += size;
}

// FIXME: persistent arena can be used for temporary allocations you know!
Font_Info bfont_load_default_atlas(Arena *arena, Arena *temp_arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height) {
  Font_Info font = {};

  font.first_codepoint = 32; // ' ' 
  font.last_codepoint = 127; // '~'
  font.glyph_count = font.last_codepoint - font.first_codepoint+1; 
  font.glyph_height_in_px = glyph_height_in_px;

  u8 *font_bitmap = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height);
  stbtt_packedchar *packed_chars = arena_push_array(arena, stbtt_packedchar, font.glyph_count);
  stbtt_aligned_quad *aligned_quads = arena_push_array(arena, stbtt_aligned_quad, font.glyph_count);

  // Pack all the needed glyphs to the bitmap and get their metrics (packedchar / aligned_quad)
  stbtt_pack_context pctx = {};
  stbtt_PackBegin(&pctx, font_bitmap, atlas_width, atlas_height, 0, 1, nullptr);
  stbtt_PackFontRange(&pctx, default_font_data, 0, glyph_height_in_px, font.first_codepoint, font.glyph_count, packed_chars);
  stbtt_PackEnd(&pctx);


  for (s32 glyph_idx = 0; glyph_idx < font.glyph_count; glyph_idx+=1) {
    f32 trash_x, trash_y;
    stbtt_GetPackedQuad(packed_chars, atlas_width, atlas_height, glyph_idx, &trash_x, &trash_y, &aligned_quads[glyph_idx], 1);
  }

  // Calculate our internal font metrics, which we will use in-engine for font rendering
  for (s32 glyph_idx = 0; glyph_idx < font.glyph_count; glyph_idx+=1) {
    Glyph_Info *font_glyph = &font.glyphs[glyph_idx];

    stbtt_packedchar pc = packed_chars[glyph_idx];
    font_glyph->r = (rect){
      .x = pc.x0,
      .y = pc.y0,
      .w = pc.x1 - pc.x0,
      .h = pc.y1 - pc.y0,
    };
    font_glyph->off = v2m(pc.xoff, pc.yoff);
    font_glyph->xadvance = pc.xadvance;

    // Is this needed?
    font_glyph->dim = v2m(pc.x1 - pc.x0, pc.y1 - pc.y0);

    // w/h = atlas_width atlas_height
    // NOTE: Not sure if this one is needed..
    stbtt_aligned_quad ac = aligned_quads[glyph_idx];
    font_glyph->tc = (rect){
      .x = ac.s0,
      .y = ac.t0,
      .w = ac.s1 - ac.s0,
      .h = ac.t1 - ac.t0,
    };
    //printf("Loaded glyph=[%c] off=(%f, %f) dim=(%f, %f) xadv=(%.1f)\n", ' ' + glyph_idx, font_glyph->off.x, font_glyph->off.y, font_glyph->dim.x, font_glyph->dim.y, font_glyph->xadvance);
  }

  // @HACK, This is because stbtt_Pack API is made to pack glyphs so the SPACE on has
  // no size, which means also no xadvance I think, for that reason we use the Font API to populate its xadvance..
  stbtt_fontinfo font_info;
  stbtt_InitFont(&font_info, default_font_data, stbtt_GetFontOffsetForIndex(default_font_data, 0));
  f32 scale = stbtt_ScaleForPixelHeight(&font_info, glyph_height_in_px);
  int advance, lsb;
  u32 space_glyph_idx = ' ' - font.first_codepoint;
  stbtt_GetCodepointHMetrics(&font_info, font.first_codepoint + space_glyph_idx, &advance, &lsb);
  font.glyphs[space_glyph_idx].xadvance = advance * scale;
 
  s32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
  font.ascent_px= (f32)ascent*scale;
  font.descent_px = (f32)descent*scale;
  font.line_gap_px = (f32)line_gap*scale;

  // Transform to RGBA
  u64 temp_arena_pos_start = arena_get_current_pos(temp_arena);
  u8 *font_bitmap_rgba = (u8*)arena_push_array(temp_arena, u8, sizeof(u8)*atlas_width*atlas_height*4);
  for (u32 i = 0; i < atlas_width*atlas_height; i+=1) {
    for (u32 j = 0; j < 4; j+=1) {
      font_bitmap_rgba[4*i + j] = font_bitmap[i];
    }
  }

  str8 ctx = (str8) {
    .data = arena_push_array(temp_arena, u8, sizeof(u32)*atlas_width *atlas_height + 1024),
    .count = 0,
  };
  stbi_write_png_to_func(png_stbi_write_func, &ctx, atlas_width, atlas_height, 4, font_bitmap_rgba, atlas_width*sizeof(u32));
  font.tex_dim = v2m(atlas_width, atlas_height);
  font.tex_id = am_load_from_data(STR8L("fa.png"), STR8(ctx.data, ctx.count));
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
    rn_push_quad(rn_pass_front(), quad);
  }

  v2 baseline_pos = pos;
  baseline_pos.y -= font_info->descent_px * scale;
  for (s32 i = 0; i < text.count; i+=1) {
    u8 c = text.data[i];
    Glyph_Info metrics = font_info->glyphs[c - font_info->first_codepoint];
    f32 atlas_height = font_info->tex_dim.y;
    R_Quad quad = (R_Quad) {
        .dst_rect = rec(baseline_pos.x + ((i==0)?0:metrics.off.x*scale),
                        baseline_pos.y - (metrics.off.y*scale + metrics.r.h*scale),//+metrics.off.y*scale, 
                        metrics.r.w*scale,
                        metrics.r.h*scale
        ),
        .src_rect = rec(metrics.r.x,
            atlas_height - metrics.r.y - metrics.r.h,
            metrics.r.w,
            metrics.r.h
        ),
        .c = col,
        .tex = font_tex,
    };
    rn_push_quad(rn_pass_front(), quad);
    baseline_pos.x += metrics.xadvance*scale;
  }
}
