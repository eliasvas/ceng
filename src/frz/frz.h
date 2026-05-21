#ifndef FRZ_H__
#define FRZ_H__

/////////////////////////////////////////////
// frz : A fast 3D rasterizer (!!)
// Most generic math functions can be found at src/base/bmath.h
// along with a list of references for more math study.
/////////////////////////////////////////////

#include "base/base_inc.h"

// TODO: Make this proper single header lib
// TODO: Multithreading
// TODO: Options for face removal / and stuff
// TODO: Right handed cube (w/ Index buffer too?)

typedef struct {
  v3 pos;
  v3 norm;
  v2 uv;
  v4 color;
} FRZ_Vertex;

#define FRZ_WHITE color_from_rgba8(255,255,255,255)
#define FRZ_RED color_from_rgba8(255, 0, 0, 255)
#define FRZ_GREEN color_from_rgba8(0, 255, 0, 255)
#define FRZ_BLUE color_from_rgba8(0, 0, 255, 255)
#define FRZ_BLACK color_from_rgba8(0, 0, 0, 255)

// Just the first face done for now, should also do the others..
FRZ_Vertex frz_cube_verts[] = {
  (FRZ_Vertex) {.pos = v3m(-0.5, -0.5, +0.5), .uv = v2m(0,0), .color = FRZ_RED,},
  (FRZ_Vertex) {.pos = v3m(+0.5, -0.5, +0.5), .uv = v2m(1,0), .color = FRZ_GREEN,},
  (FRZ_Vertex) {.pos = v3m(+0.5, +0.5, +0.5), .uv = v2m(1,1), .color = FRZ_WHITE,},
  (FRZ_Vertex) {.pos = v3m(-0.5, +0.5, +0.5), .uv = v2m(0,1), .color = FRZ_BLUE,},

  (FRZ_Vertex) {.pos = v3m(+0.5, -0.5, -0.5), .uv = v2m(0,0), .color = FRZ_RED,},
  (FRZ_Vertex) {.pos = v3m(-0.5, -0.5, -0.5), .uv = v2m(1,0), .color = FRZ_GREEN,},
  (FRZ_Vertex) {.pos = v3m(-0.5, +0.5, -0.5), .uv = v2m(1,1), .color = FRZ_WHITE,},
  (FRZ_Vertex) {.pos = v3m(+0.5, +0.5, -0.5), .uv = v2m(0,1), .color = FRZ_BLUE,},

};

s32 frz_cube_indices[] = {
  0,1,2, 0,2,3,
  1,4,7, 1,7,2,
  4,5,6, 4,6,7,
  5,0,3, 5,3,6,
  0,1,4, 0,4,5,
  3,2,7, 3,7,6,
};

const uint8_t arrow_tex[16][16] = {
    {  0,  0,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,255,255,255,255,255,255,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,255,255,255,255,255,255,255,255,  0,  0,  0,  0},
    {  0,  0,  0,255,255,255,255,255,255,255,255,255,255,  0,  0,  0},
    {  0,  0,255,255,255,255,255,255,255,255,255,255,255,255,  0,  0},
    {  0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  0},
    {255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,  0,  0}
};


// You need to provide this !! 
typedef struct {
  u32 *backbuffer;
  f32 *zbuf;
  v2 dim;
  Arena *talloc;
  // List of stuff -- in the future ok?!
} FRZ_Ctx;

// Should we just set the context at the beginning of the frame?
static FRZ_Ctx g_ctx;

static FRZ_Ctx* frz_get_gctx() {
  return &g_ctx;
}

static void frz_begin_frame(u32 *backbuffer, v2 dim, Arena *talloc) {
  g_ctx.backbuffer = backbuffer;
  g_ctx.dim = dim;
  g_ctx.talloc = talloc;
  g_ctx.zbuf = arena_push_array(g_ctx.talloc, f32, g_ctx.dim.x * g_ctx.dim.y);
  for (s32 i = 0; i < g_ctx.dim.x*g_ctx.dim.y; i+=1) {
    g_ctx.zbuf[i] = F32_MAX;
  }

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

// Barycentric coord calculation
static f64 frz_edge(v2 a, v2 b, v2 c) {
    return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}

static v4 frz_apply_viewport_transform(v4 p_ndc, v2 wdim) {
    return v4m(((p_ndc.x+1) / 2) * wdim.x, ((1 + p_ndc.y)/2)*wdim.y, p_ndc.z, p_ndc.w); 
}

static void frz_imm_tri_bbox(v4 a, v4 b, v4 c, v2 uva, v2 uvb, v2 uvc, color ca, color cb, color cc) {
  FRZ_Ctx *ctx = frz_get_gctx();
  rect bbox = {
    .x = minimum(a.x, minimum(b.x, c.x)),
    .y = minimum(a.y, minimum(b.y, c.y)),
  };
  bbox.w = maximum(a.x, maximum(b.x, c.x)) - bbox.x;
  bbox.h = maximum(a.y, maximum(b.y, c.y)) - bbox.y;
  f64 area = frz_edge(v2_from_v4(a), v2_from_v4(b), v2_from_v4(c));

  // Non CCW face removal right now! in 2D of course
  //if (area < 0) return;

  for (s32 y = bbox.y; y <= bbox.y+bbox.h; y+=1) {
    for (s32 x = bbox.x; x <= bbox.x+bbox.w; x+=1) {
      v2 p = v2m(x, y);

      f32 alpha = frz_edge(v2m(p.x, p.y), v2_from_v4(b), v2_from_v4(c)) / area;
      f32 beta  = frz_edge(v2_from_v4(a), v2m(p.x, p.y), v2_from_v4(c)) / area;
      f32 gamma = frz_edge(v2_from_v4(a), v2_from_v4(b), v2m(p.x, p.y)) / area;

      v2 interp_uv = v2_add(v2_multf(uva, alpha), v2_add(v2_multf(uvb, beta), v2_multf(uvc, gamma)));
      f32 tex_color = arrow_tex[15 - (s32)(interp_uv.y * 15.99)][(s32)(interp_uv.x * 15.99)] / 255;
      //f32 tex_color= 1;

      v4 interpolated_color = v4_add(v4_multf(ca, alpha), v4_add(v4_multf(cb, beta), v4_multf(cc, gamma)));
      interpolated_color = v4_multf(interpolated_color, tex_color);


      if (alpha < 0 || beta < 0 || gamma < 0) continue;

      // TODO: depth policy, less/lequal/greater/none whatever
      f32 interpolated_depth = alpha * a.z + beta * b.z + gamma * c.z;
      if (interpolated_depth < ctx->zbuf[(s32)((s32)p.x + (s32)p.y * ctx->dim.x)]) {
        ctx->zbuf[(s32)((s32)p.x + (s32)p.y * ctx->dim.x)] = interpolated_depth;
        frz_imm_px(p.x, p.y, interpolated_color);
      }
    }
  }
}

#endif
