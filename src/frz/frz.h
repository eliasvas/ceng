#ifndef FRZ_H__
#define FRZ_H__

/////////////////////////////////////////////
// Fast Rasterizer: A fast 3D rasterizer (!!)
/////////////////////////////////////////////
#include "base/base_inc.h"

////////////////////////////////////////////////
// Good References:
// https://immersivemath.com/ila/index.html
// https://foundationsofgameenginedev.com
// https://haqr.eu/tinyrenderer
////////////////////////////////////////////////

// TODO: Make this proper single header lib
// TODO: Multithreading
// TODO: Options for face removal / and stuff
// TODO: Right handed cube (w/ Index buffer too?)

// You need to provide this !! 
typedef struct {
  u32 *backbuffer;
  f32 *zbuf;
  v2 dim;
  // List of stuff -- in the future ok?!
} FRZ_Ctx;

// Should we just set the context at the beginning of the frame?
static FRZ_Ctx g_ctx;

static FRZ_Ctx* frz_get_gctx() {
  return &g_ctx;
}

static void frz_begin_frame(u32 *backbuffer, v2 dim) {
  g_ctx.backbuffer = backbuffer;
  g_ctx.dim = dim;
}

static void frz_end_frame() {

}

static void frz_clear() {
  FRZ_Ctx *ctx = frz_get_gctx();

  for (s32 y = 0; y < (s32)ctx->dim.y; y+=1) {
    for (s32 x = 0; x < (s32)ctx->dim.x; x+=1) {
      //u8 red = (u8)((x / (f32)ctx->dim.x) * 255.0);
      //ctx->backbuffer[x + y * (s32)ctx->dim.x] = (0xff << 24) | (0x00 << 16) | (0x00 << 8) | (red << 0);
      ctx->backbuffer[x + y * (s32)ctx->dim.x] = (0xff << 24) | (0x3f << 16) | (0x3f << 8) | (0x3f<< 0);
    }
  }
}

static void frz_imm_px(s32 x, s32 y, color c) {
  FRZ_Ctx *ctx = frz_get_gctx();
  if (x>=0 && x < ctx->dim.x && y >= 0 && y < ctx->dim.y) {
    //ctx->backbuffer[x + y * (s32)ctx->dim.x] = 0xffffffff;
    ctx->backbuffer[x + y * (s32)ctx->dim.x] = u32_from_color(c);
  }
}

#define SWAP(T, a, b) do { T temp = a; a = b; b = temp; }while(0);

static void frz_imm_linex(v2 start, v2 end, color c) {
  if (start.x > end.x) {
    SWAP(v2, start, end);
  }

  for (s32 x = start.x; x <= end.x; x+=1) {
    f32 perc = (x - start.x) / (end.x - start.x);
    f32 y = start.y + (end.y - start.y) * perc;
    frz_imm_px(x, (s32)y, c);
  }
}

static void frz_imm_liney(v2 start, v2 end, color c) {
  if (start.y > end.y) {
    SWAP(v2, start, end);
  }

  for (s32 y = start.y; y <= end.y; y+=1) {
    f32 perc = (y - start.y) / (end.y- start.y);
    f32 x = start.x + (end.x - start.x) * perc;
    frz_imm_px(x, (s32)y, c);
  }
}

static void frz_imm_line(v2 start, v2 end, color c) {
  if (end.x == start.x || end.y == start.y) return;

  f32 slopex = (end.y - start.y) / (end.x - start.x);
  f32 slopey = (end.x - start.x) / (end.y - start.y);
  if (slopex < slopey) {
    frz_imm_linex(start, end, c);
  }else {
    frz_imm_liney(start, end, c);
  }
}


#endif
