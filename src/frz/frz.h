#ifndef FRZ_H__
#define FRZ_H__
#include "base/base_inc.h"

/////////////////////////////////////////////
// frz : A fast 3D rasterizer (!!)
// Most generic math functions can be found at src/base/bmath.h
// along with a list of references for more math study.
//
// Next Steps:
// https://fgiesen.wordpress.com/2011/07/09/a-trip-through-the-graphics-pipeline-2011-index/
// https://fgiesen.wordpress.com/2013/02/17/optimizing-sw-occlusion-culling-index/
/////////////////////////////////////////////

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

#ifndef FRZ_IMPLEMENTATION
void frz_begin_frame(u32 *backbuffer, v2 dim, Arena *talloc);
void frz_end_frame();
void frz_imm_tri_bbox(v4 a, v4 b, v4 c, v2 uva, v2 uvb, v2 uvc, color ca, color cb, color cc);
void frz_clear();
v4 frz_apply_viewport_transform(v4 p_ndc, v2 wdim);
#else

// TODO: Maybe make a 24-vertex VBO, to have correct UVs everywhere..
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
  // front (+Z)
  0, 1, 2, 0, 2, 3,
  // right (+X)
  1, 4, 7, 1, 7, 2,
  // back (-Z)
  4, 5, 6, 4, 6, 7,
  // left (-X)
  5, 0, 3, 5, 3, 6,
  // bottom (-Y)
  5, 4, 1, 5, 1, 0,
  // top (+Y)
  3, 2, 7, 3, 7, 6,
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

void frz_begin_frame(u32 *backbuffer, v2 dim, Arena *talloc) {
  g_ctx.backbuffer = backbuffer;
  g_ctx.dim = dim;
  g_ctx.talloc = talloc;
  g_ctx.zbuf = arena_push_array(g_ctx.talloc, f32, g_ctx.dim.x * g_ctx.dim.y);
  for (s32 i = 0; i < g_ctx.dim.x*g_ctx.dim.y; i+=1) {
    g_ctx.zbuf[i] = F32_MAX;
  }

}

void frz_end_frame() {

}

void frz_clear() {
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
  if (inverted) { FRZ_SWAP(f32, a.x, a.y); FRZ_SWAP(f32, b.x, b.y); }
  // a.x is always smaller
  if (a.x > b.x) { FRZ_SWAP(v2, a, b); }

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

static f64 frz_edge(v2 a, v2 b, v2 c) { return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x); }
v4 frz_apply_viewport_transform(v4 p_ndc, v2 wdim) { return v4m(((p_ndc.x+1) / 2) * wdim.x, ((p_ndc.y+1)/2)*wdim.y, p_ndc.z, p_ndc.w); }
void frz_imm_tri_bbox(v4 a, v4 b, v4 c, v2 uva, v2 uvb, v2 uvc, color ca, color cb, color cc) {
  FRZ_Ctx *ctx = frz_get_gctx();
  rect bbox = {
    .x = minimum(a.x, minimum(b.x, c.x)),
    .y = minimum(a.y, minimum(b.y, c.y)),
  };
  bbox.w = maximum(a.x, maximum(b.x, c.x)) - bbox.x;
  bbox.h = maximum(a.y, maximum(b.y, c.y)) - bbox.y;
  //printf("bbox: (%f %f %f %f)\n", bbox.x, bbox.y, bbox.w, bbox.h);
  f64 area = frz_edge(v2_from_v4(a), v2_from_v4(b), v2_from_v4(c));

  // Non CCW face removal right now! in 2D of course
  if (area < 0) return;

  for (s32 y = bbox.y; y <= bbox.y+bbox.h; y+=1) {
    for (s32 x = bbox.x; x <= bbox.x+bbox.w; x+=1) {
      v2 p = v2m(x, y);

      f32 alpha = frz_edge(v2m(p.x, p.y), v2_from_v4(b), v2_from_v4(c)) / area;
      f32 beta  = frz_edge(v2_from_v4(a), v2m(p.x, p.y), v2_from_v4(c)) / area;
      f32 gamma = frz_edge(v2_from_v4(a), v2_from_v4(b), v2m(p.x, p.y)) / area;

      if (alpha < 0 || beta < 0 || gamma < 0) continue;

      f32 interpolated_depth = alpha * a.z + beta * b.z + gamma * c.z;
      if (interpolated_depth < ctx->zbuf[(s32)((s32)p.x + (s32)p.y * ctx->dim.x)]) {
        f32 ia = 1.0f / a.w;
        f32 ib = 1.0f / b.w;
        f32 ic = 1.0f / c.w;
        v2 interp_uv = v2_divf( v2_add( v2_multf(uva, alpha * ia), v2_add( v2_multf(uvb, beta * ib), v2_multf(uvc, gamma * ic))), alpha * ia + beta * ib + gamma * ic);

        f32 tex_color = arrow_tex[15 - (s32)(interp_uv.y * 15.99)][(s32)(interp_uv.x * 15.99)] / 255;
        //f32 tex_color= 1;

        v4 interp_color = v4_divf( v4_add( v4_multf(ca, alpha * ia), v4_add( v4_multf(cb, beta * ib), v4_multf(cc, gamma * ic))), alpha * ia + beta * ib + gamma * ic);
        interp_color = v4_multf(interp_color, tex_color);

        ctx->zbuf[(s32)((s32)p.x + (s32)p.y * ctx->dim.x)] = interpolated_depth;
        frz_imm_px(p.x, p.y, interp_color);
      }
    }
  }
}
#endif

// Small working example
#if 0
  frz_begin_frame(gs->pixels, gs->wdim, gs->frame_arena);
  frz_clear();
  v2 mp = input_get_mouse_pos(&gs->input);
  mp.y = gs->wdim.y - mp.y;
  frz_imm_line(v2m(0,0), mp, col(1,1,1,1));


  m4 world_from_model = m4_rotate(gs->time_sec*2.0, v3m(0,1,0));
  m4 view_from_world  = m4_view(v3m(0,0,6), v3m(0,0,0), v3m(0,1,0));
  m4 clip_from_view   = m4_persp(45, gs->wdim.x/(f32)gs->wdim.y, 0.1, 100);

  for (u32 cube_idx_triplet= 0; cube_idx_triplet < array_count(frz_cube_indices); cube_idx_triplet+=3) {
    FRZ_Vertex *vt0 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+0]];
    FRZ_Vertex *vt1 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+1]];
    FRZ_Vertex *vt2 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+2]];


    v3 v0_world  = v3_from_v4(m4_multv(world_from_model, v4_from_v3(vt0->pos,1))); 
    v4 v0_view   = m4_multv(view_from_world, v4_from_v3(v0_world, 1.0));
    v4 v0_clip   = m4_multv(clip_from_view, v0_view);
    v4 v0_ndc    = v4_div(v0_clip, v4m(v0_clip.w, v0_clip.w, v0_clip.w, 1));
    v4 v0_screen = frz_apply_viewport_transform(v0_ndc, gs->wdim);

    v3 v1_world  = v3_from_v4(m4_multv(world_from_model, v4_from_v3(vt1->pos,1))); 
    v4 v1_view   = m4_multv(view_from_world, v4_from_v3(v1_world, 1.0));
    v4 v1_clip   = m4_multv(clip_from_view, v1_view);
    v4 v1_ndc    = v4_div(v1_clip, v4m(v1_clip.w, v1_clip.w, v1_clip.w, 1));
    v4 v1_screen = frz_apply_viewport_transform(v1_ndc, gs->wdim);

    v3 v2_world  = v3_from_v4(m4_multv(world_from_model, v4_from_v3(vt2->pos,1))); 
    v4 v2_view   = m4_multv(view_from_world, v4_from_v3(v2_world, 1.0));
    v4 v2_clip   = m4_multv(clip_from_view, v2_view);
    v4 v2_ndc    = v4_div(v2_clip, v4m(v2_clip.w, v2_clip.w, v2_clip.w, 1));
    v4 v2_screen = frz_apply_viewport_transform(v2_ndc, gs->wdim);

    frz_imm_tri_bbox(v0_screen, v1_screen, v2_screen, vt0->uv, vt1->uv, vt2->uv, vt0->color, vt1->color, vt2->color);
  }

  frz_end_frame();
#endif

#endif
