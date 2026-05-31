#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include "font_util.h"

#include "base/base_inc.h"
#include "ogl.h"
#include "r2d.h"

// By default we just embed ProggyClean - Ugly AF but for now it'll do!
static const u8 default_font_data[] = {
#embed "../../data/ProggyClean.ttf"
};

Font_Info font_util_load_default_atlas(Arena *arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height) {
  Font_Info font = {};

  font.first_codepoint = 32; // ' ' 
  font.last_codepoint = 127; // '~'
  font.glyph_count = font.last_codepoint - font.first_codepoint + 1; 

  u8 *font_bitmap = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height);

  stbtt_packedchar *packed_chars = arena_push_array(arena, stbtt_packedchar, font.glyph_count);
  stbtt_aligned_quad *aligned_quads = arena_push_array(arena, stbtt_aligned_quad, font.glyph_count);

  // Pack all the needed glyphs to the bitmap and get their metrics (packedchar / aligned_quad)
  stbtt_pack_context pctx = {};
  stbtt_PackBegin(&pctx, font_bitmap, atlas_width, atlas_height, 0, 1, nullptr);
  stbtt_PackFontRange(&pctx, default_font_data, 0, glyph_height_in_px, font.first_codepoint, font.glyph_count, packed_chars);
  stbtt_PackEnd(&pctx);

  for (u32 glyph_idx = 0; glyph_idx < font.glyph_count; ++glyph_idx) {
    f32 trash_x, trash_y;
    stbtt_GetPackedQuad(packed_chars, atlas_width, atlas_height, glyph_idx, &trash_x, &trash_y, &aligned_quads[glyph_idx], 1);
  }

  // Calculate our internal font metrics, which we will use in-engine for font rendering
  for (u32 glyph_idx = 0; glyph_idx < font.glyph_count; ++glyph_idx) {
    stbtt_packedchar pc = packed_chars[glyph_idx];
    stbtt_aligned_quad ac = aligned_quads[glyph_idx];
    font.glyphs[glyph_idx].r = (rect){
      .x = pc.x0,
      .y = pc.y0,
      .w = pc.x1 - pc.x0,
      .h = pc.y1 - pc.y0,
    };

    // w/h = atlas_width atlas_height
    // NOTE: Not sure if this one is needed..
    font.glyphs[glyph_idx].tc = (rect){
      .x = ac.s0,
      .y = ac.t0,
      .w = ac.s1 - ac.s0,
      .h = ac.t1 - ac.t0,
    };
    font.glyphs[glyph_idx].off = v2m(pc.xoff, pc.yoff);
    font.glyphs[glyph_idx].xadvance = pc.xadvance;
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
 

  // Transform to OpenGL-style texture (mainly by convention, I like the upright view on renderdoc) + make the actual texture
  font_util_flip_bitmap(font_bitmap, atlas_width, atlas_height);

  // Transform to RGBA
  u8 *font_bitmap_rgba = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height*4);
  for (u32 i = 0; i < atlas_width*atlas_height; ++i) {
    font_bitmap_rgba[4*i + 0] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 1] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 2] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 3] = font_bitmap[i + 0];
  }

  // Finally make the texture
  font.atlas = ogl_tex_make(font_bitmap_rgba, atlas_width, atlas_height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});

  return font;
}

void font_util_flip_bitmap(u8 *bitmap, u32 width, u32 height) {
  for (u32 y = 0; y < height/2; ++y) {
    for (u32 x = 0; x < width; ++x) {
      u8 temp = bitmap[x + y * width];
      bitmap[x+y*width] = bitmap[x+(height-y)*width];
      bitmap[x+(height-y)*width] = temp;
    }
  }
}

rect font_util_calc_text_rect(Font_Info *font_info, buf text, v2 baseline_pos, f32 scale) {
  u32 glyph_count = text.count;
  if (glyph_count == 0) return (rect){};

  Glyph_Info first_glyph = font_info->glyphs[text.data[0] - font_info->first_codepoint];
  rect r = (rect) {
    .x = baseline_pos.x,// + first_glyph.off.x*scale,
    .y = baseline_pos.y,// + first_glyph.off.y*scale,
    .w = first_glyph.r.w*scale,
    .h = first_glyph.r.h*scale,
  };

  for (u32 glyph_idx = 0; glyph_idx < glyph_count; ++glyph_idx) {
    Glyph_Info glyph = font_info->glyphs[text.data[glyph_idx] - font_info->first_codepoint];
    rect r1 = (rect) {
      .x = baseline_pos.x,// + glyph.off.x*scale,
      .y = baseline_pos.y,// + glyph.off.y*scale,
      .w = glyph.r.w*scale,
      .h = glyph.r.h*scale,
    };
    baseline_pos.x += glyph.xadvance*scale;
    r = rect_calc_bounding_rect(r, r1);
  }

  return r;
}

f32 font_util_measure_text_width(Font_Info *font_info, buf text, f32 scale) {
  return font_util_calc_text_rect(font_info, text, v2m(0,0), scale).w;
}

f32 font_util_measure_text_height(Font_Info *font_info, buf text, f32 scale) {
  return font_util_calc_text_rect(font_info, text, v2m(0,0), scale).h;
}

s64 font_util_count_glyphs_until_width(Font_Info *font_info, buf text, f32 scale, f32 target_width) {
  s64 glyph_count = 0;
  while (glyph_count < text.count) {
    f32 text_w = font_util_measure_text_width(font_info, buf_make(text.data, glyph_count), scale);
    if (text_w >= target_width) {
      if (glyph_count > 0) glyph_count -= 1;
      break;
    } else {
      glyph_count+=1;
    }
  }
  return glyph_count;
}

void font_util_debug_draw_text(Font_Info *font_info, Arena *arena, R2D_Cmd_Chunk_List *cmd_list, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box) {

  // set viewport 
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = viewport };
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // set scissor
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = clip_rect};
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // set camera
  R2D_Cam cam = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0.0, };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = cam };
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // push quad


  //R2D* text_rend = r2d_begin(arena, &(R2D_Cam){ .offset = v2m(0, 0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0.0, }, viewport, clip_rect);

  rect tr = font_util_calc_text_rect(font_info, text, baseline_pos, scale);
  if (draw_box) {
    R2D_Quad quad = (R2D_Quad) {
        .dst_rect = tr,
        .c = col(0.9,0.4,0.4,1.0),
        .tex = {0},
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(arena, cmd_list, cmd, 256);
  }

  for (s32 i = 0; i < text.count; ++i) {
    u8 c = text.data[i];
    Glyph_Info metrics = font_info->glyphs[c - font_info->first_codepoint];
    //r2d_push_quad(text_rend, (R2D_Quad) { .dst_rect = rec(baseline_pos.x+metrics.off.x*scale, baseline_pos.y+metrics.off.y*scale, metrics.r.w*scale, metrics.r.h*scale), .src_rect = rec(metrics.r.x, metrics.r.y, metrics.r.w, metrics.r.h), .c = col, .tex = font_info->atlas, });
    f32 atlas_height = font_info->atlas.height;
    R2D_Quad quad = (R2D_Quad) {
        .dst_rect = rec(baseline_pos.x,//+metrics.off.x*scale,
                        baseline_pos.y,//+metrics.off.y*scale, 
                        metrics.r.w*scale,
                        metrics.r.h*scale
        ),
        .src_rect = rec(metrics.r.x,
            atlas_height - metrics.r.y - metrics.r.h,
            metrics.r.w,
            metrics.r.h
        ),
        .c = col,
        .tex = font_info->atlas,
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(arena, cmd_list, cmd, 256);

    baseline_pos.x += metrics.xadvance*scale;
  }

  //r2d_end(text_rend);
}


