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
      ctx->backbuffer[x + y * (s32)ctx->dim.x] = (0xff << 24) | (0x2f << 16) | (0x2f << 8) | (0x2f<< 0);
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

#define FRZ_SWAP(T, a, b) do { T temp = a; a = b; b = temp; }while(0);

// TODO: This is very very slow, too many branches @OPTIIMIZE
static void frz_imm_line(v2 a, v2 b, color c) {
  // if line has bigger slope on the y-axis we invert it.. and write to [y,x] pixels
  b32 inverted = (abs_f32(b.x - a.x) < abs_f32(b.y - a.y));
  if (inverted) {
    FRZ_SWAP(f32, a.x, a.y); 
    FRZ_SWAP(f32, b.x, b.y); 
  }
  // a.x is always smaller
  if (a.x > b.x) {
    FRZ_SWAP(v2, a, b);
  }
  for (f32 x = a.x; x <= b.x; x += 1) {
    f32 t = (x - a.x) / (b.x - a.x); 
    f32 y = a.y + t * (b.y - a.y); 
    if (inverted) {
      frz_imm_px((s32)y, (s32)x, c);
    } else {
      frz_imm_px((s32)x, (s32)y, c);
    }
  }
}

static void frz_imm_tri_sweep(v2 a, v2 b, v2 c, color col) {
  if (a.y > b.y) { FRZ_SWAP(v2, a, b); }
  if (b.y > c.y) { FRZ_SWAP(v2, b, c); }
  if (a.y > b.y) { FRZ_SWAP(v2, a, b); }

#if 0
  frz_imm_line(a, b, col);
  frz_imm_line(b, c, col);
  frz_imm_line(c, a, col);
#endif

  if (a.y != b.y) {
    f32 delta_x1 = (b.x - a.x) / (b.y - a.y); 
    f32 delta_x2 = (c.x - a.x) / (c.y - a.y); 
    for (s32 y = a.y; y <= b.y; y+=1) {
      f32 x1 = a.x + delta_x1 * (y-a.y);
      f32 x2 = a.x + delta_x2 * (y-a.y);
      for (s32 x = minimum(x1,x2); x < maximum(x1,x2); x+=1) {
        frz_imm_px(x, y, col);
      }
    }
  }

  if (b.y != c.y) {
    f32 delta_x1 = (b.x - c.x) / (b.y - c.y);
    f32 delta_x2 = (a.x - c.x) / (a.y - c.y);
    for (s32 y = c.y; y >= b.y; y-=1) {
      f32 x1 = c.x - delta_x1 * (c.y-y);
      f32 x2 = c.x - delta_x2 * (c.y-y);
      for (s32 x = minimum(x1,x2); x < maximum(x1,x2); x+=1) {
        frz_imm_px(x, y, col);
      }
    }
  }
}

static f32 tri_area_sgn(v2 a, v2 b, v2 c) {
    return 0.5f*((b.y-a.y)*(b.x+a.x) + (c.y-b.y)*(c.x+b.x) + (a.y-c.y)*(a.x+c.x));
}

static void frz_imm_tri_bbox(v2 a, v2 b, v2 c, color col) {
  rect bbox = {
    .x = minimum(a.x, minimum(b.x, c.x)),
    .y = minimum(a.y, minimum(b.y, c.y)),
  };
  bbox.w = maximum(a.x, maximum(b.x, c.x)) - bbox.x;
  bbox.h = maximum(a.y, maximum(b.y, c.y)) - bbox.y;
  f32 A = tri_area_sgn(a,b,c);

  for (s32 x = 0; x < bbox.w; x+=1) {
    for (s32 y = 0; y < bbox.h; y+=1) {
      f32 x_coord = x+bbox.x;
      f32 y_coord = y+bbox.y;

      f32 alpha = tri_area_sgn(v2m(x_coord, y_coord), b, c) / A;
      f32 beta  = tri_area_sgn(a, v2m(x_coord, y_coord), c) / A;
      f32 gamma = tri_area_sgn(a, b, v2m(x_coord, y_coord)) / A;

      if (alpha < 0 || beta < 0 || gamma < 0) continue;
      frz_imm_px(x_coord, y_coord, col);
    }
  }

}


#endif
