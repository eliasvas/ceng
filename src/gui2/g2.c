#include "g2.h"

void font_util_debug_draw_text(Font_Info *font_info, Arena *arena, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box);

static g2_ctx ctx;

void g2_init(Arena *tarena, Font_Info *font, Input *input) {
  ctx.arena = arena_make(MB(256));
  ctx.temp_arena = tarena;
  ctx.g_scale = 1.0;
  ctx.font = font;
  ctx.input = input;
}

b32 g2_button(buf label, v2 coords) {
  g2ID id = djb2_buf(label);
  b32 res = false;

  rect r = font_util_calc_text_rect(ctx.font, label, coords, ctx.g_scale);
  v2 bl_mp = input_get_mouse_pos(ctx.input);
  bl_mp.y = (ctx.viewport.h - bl_mp.y);
  b32 collides = rect_isect_point(r, bl_mp);
  b32 lmb_pressed = input_mkey_pressed(ctx.input, INPUT_MOUSE_LMB);
  b32 lmb_released = input_mkey_released(ctx.input, INPUT_MOUSE_LMB);


  if (collides) {
    ctx.hot_id = id;
    if (lmb_pressed) {
      ctx.active_id = id;
    }
  }
  if (lmb_released) {
    if (ctx.hot_id == id) {
      res = true;
    }
    ctx.active_id = 0;
  }

  b32 is_hot = (ctx.hot_id == id);
  b32 is_active = (ctx.active_id == id);

  // Some color modulation to see in which state we are in
  f32 color_mod = 0.4;
  if (is_hot) color_mod = 0.7;
  if (is_active) color_mod = 1.0;

  font_util_debug_draw_text(ctx.font, ctx.temp_arena, ctx.viewport , ctx.viewport, label, v2m(coords.x, coords.y), ctx.g_scale, col(color_mod, color_mod, color_mod,1), true);
  return res;
}

void g2_begin(rect viewport) {
  ctx.viewport = viewport;
}

void g2_end() {
  ctx.hot_id = 0;
}
