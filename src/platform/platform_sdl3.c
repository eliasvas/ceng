#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdlib.h>
// dlopen/dlclose
#include <dlfcn.h>

#include <SDL3/SDL.h>
#define USE_SDL3_IO 1
#include "gl_loader.h"

#define STR_IMPLEMENTATION
#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#define ARENA_IMPLEMENTATION 
#include "base/base_inc.h"

#define OGL_IMPLEMENTATION
#include "rend/rend_inc.h"

#define INPUT_IMPLEMENTATION
#include "core/input.h"

#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"

#include "game.h"


// @MustImplement
#if USE_SDL3_IO
#include <SDL3_image/SDL_image.h>
u8* platform_img_to_raw(Arena *arena, str8 image_data, v2 *out_dim) {
  SDL_IOStream* io = SDL_IOFromConstMem(image_data.data, image_data.count);
  SDL_Surface* surface = IMG_Load_IO(io, true);
  if (!surface) {
    printf("IMG_Load_IO failed: %s\n", SDL_GetError());
    // FIXME: Why? -> look at bfont.. when we make a font atlas, we have only CPU image data, not-png but raw.
    // In case the surface couldn't be created we assume the image is raw and square..
    out_dim->x = sqrt_f64(image_data.count / (sizeof(u32))); 
    out_dim->y = out_dim->x; 
    return image_data.data;
  } else {
    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    out_dim->x = rgba->w;
    out_dim->y = rgba->h;
    SDL_FlipSurface(rgba, SDL_FLIP_VERTICAL);
    //return rgba->pixels;
    u8 *px = arena_push_array_nz(arena, u8, sizeof(u8) * 4 * rgba->w * rgba->h);
    M_COPY(px, rgba->pixels, sizeof(u8) * 4 * rgba->w * rgba->h);
    SDL_DestroySurface(rgba);
    // Is this needed too?
    SDL_DestroySurface(surface);
    return px;
  }
}
#else // stb implementation (for porting and stuff)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
u8* platform_img_to_raw(Arena *arena, str8 image_data, v2 *out_dim) {
  stbi_set_flip_vertically_on_load(true);
  s32 width = 0;
  s32 height = 0;
  s32 nr_channels = 0;
  u8 *px_data = stbi_load_from_memory((u8*)image_data.data, image_data.count, &width, &height, &nr_channels, STBI_rgb_alpha);
  if (width > 0 && height > 0) {
    printf("dim: %d %d\n\n", width, height);
    *out_dim = v2m(width, height);
  } else {
    out_dim->x = sqrt_f64(image_data.count / (sizeof(u32))); 
    out_dim->y = out_dim->x; 
    px_data = image_data.data;
  } 
  return px_data;
}
#endif


#if USE_SDL3_IO
// FIXME: Should we destroy surfaces and stuff? what about the font??

#include <SDL3_ttf/SDL_ttf.h>
Font_Info platform_load_font(Arena *arena, u8 *font_data, u32 font_byte_count, u32 atlas_width, u32 atlas_height, u32 glyph_height_in_px, u8 **bitmap) {
  Font_Info font = {};

  font.first_codepoint = 32; // ' ' 
  font.last_codepoint = 127; // '~'
  font.glyph_count = font.last_codepoint - font.first_codepoint+1; 
  font.glyph_height_in_px = glyph_height_in_px;

  u8 *font_bitmap = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height);
  assert(font_bitmap);

  TTF_Font *f = NULL;

  f = TTF_OpenFontIO(SDL_IOFromConstMem((char*)font_data, font_byte_count), true, glyph_height_in_px);
  if (!f) {
      SDL_Log("Couldn't open font: %s\n", SDL_GetError());
  }

  font.ascent_px  = (f32)TTF_GetFontAscent(f);
  font.descent_px = (f32)TTF_GetFontDescent(f);
  s32 line_skip = TTF_GetFontLineSkip(f);
  font.line_gap_px = (f32)line_skip - (font.ascent_px - font.descent_px);

  s32 atlas_x = 0;
  s32 atlas_y = 0;
  s32 row_height = 0;

  for (s32 glyph_idx = 0; glyph_idx < font.glyph_count; glyph_idx++) {
    u32 codepoint = font.first_codepoint + glyph_idx;
    Glyph_Info *glyph = &font.glyphs[glyph_idx];

    int minx = 0;
    int maxx = 0;
    int miny = 0;
    int maxy = 0;
    int advance = 0;

    // 0. Get SDL_ttf glyph metrics and TTF image for each glyph
    bool valid = TTF_GetGlyphMetrics( f, codepoint, &minx, &maxx, &miny, &maxy, &advance);

    if (!valid) {
      // Glyph has no metrics
      continue;
    }
    TTF_ImageType image_type;
    SDL_Surface *surface = TTF_GetGlyphImage( f, codepoint, &image_type);
    if (!surface) {
      continue;
    }
    int glyph_w = surface->w;
    int glyph_h = surface->h;

    // 1. Shift to right position in atlas to get ready to write
    if (atlas_x + glyph_w > (s32)atlas_height) {
      atlas_y += row_height;
      atlas_x = 0;
      row_height = 0;
    }

    if (row_height < glyph_h) {
      row_height = glyph_h;
    }

    // 2. Pack the glyph inside the created texture
    for (s32 y = 0; y < glyph_h; y += 1) {
      u32 *src = (u32 *)((u8 *)surface->pixels + y * surface->pitch);
      for (s32 x = 0; x < glyph_w; x += 1) {
        u32 pixel = src[x];
        // Since tex is 32 bit monochrome, no problem here
        u8 alpha = (u8)(pixel >> 24); 

        font_bitmap[(atlas_y+y)*atlas_width + (atlas_x+x)] = alpha;
      }
    }

    // 3. Calc the metric infos
    glyph->r = rec(atlas_x, atlas_y, glyph_w, glyph_h);
    glyph->dim = v2m( (f32)glyph_w, (f32)glyph_h);
    glyph->xadvance = (f32)advance;
    glyph->off = v2m((f32)minx,(f32)-maxy);
    glyph->tc = rec((f32)atlas_x / (f32)atlas_width, (f32)atlas_y / (f32)atlas_height, (f32)glyph_w / (f32)atlas_width,(f32)glyph_h / (f32)atlas_height);

    atlas_x += glyph_w;
    atlas_x += 1;
  }
  *bitmap = font_bitmap;
  assert(bitmap);

  TTF_CloseFont(f);

  return font;
}
#else

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
Font_Info platform_load_font(Arena *arena, u8 *font_data, u32 font_byte_count, u32 atlas_width, u32 atlas_height, u32 glyph_height_in_px, u8 **bitmap) {
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
  stbtt_PackFontRange(&pctx, font_data, 0, glyph_height_in_px, font.first_codepoint, font.glyph_count, packed_chars);
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
  stbtt_InitFont(&font_info, font_data, stbtt_GetFontOffsetForIndex(font_data, 0));
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

  *bitmap = font_bitmap;
  assert(bitmap);

  return font;
}

#endif

#if USE_SDL3_IO
void platform_play_sound(const char *sound) {
  // TBA
}
#else
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
void platform_play_sound(const char *sound) {
  static ma_engine ma_eng;
  static b32 ma_initialized = false;
  if (!ma_initialized){
    ma_result result;
    ma_engine_config engineConfig;
    engineConfig = ma_engine_config_init();
    result = ma_engine_init(&engineConfig, &ma_eng);
    if (result != MA_SUCCESS) {
        return result;
    }
    ma_engine_set_volume(&ma_eng, 0.05);
    ma_initialized = true;
  }

  ma_engine_play_sound(&ma_eng, sound, nullptr);
}
#endif

u64 platform_read_cpu_timer() {
  return get_time_ns();
}

u64 platform_read_cpu_freq() {
  return get_nano_freq();
}

f64 platform_get_time() {
  return (f64)get_time_ns() / (f64)get_nano_freq();
}

void platform_try_reload_gamelib(Game_State *gs, Game_Api *game_api, b32 call_init) {
  struct stat glib_stat;
  if (stat("build/libgame.so", &glib_stat) == -1) {
    printf("couldn't stat libgame.so - loading statically (no action needed)\n");
    game_api->init     = game_init;
    game_api->update   = game_update;
    game_api->render   = game_render;
    game_api->shutdown = game_shutdown;
  } else {
    buf gamelib_path = MAKE_STR("build/libgame.so");
    s64 mod_time = glib_stat.st_mtim.tv_nsec;

    // If its a reload (not first time, copy the dll to another file first
    static int reload_count = 0;
    if (reload_count > 0) {
      if (mod_time != game_api->last_modified) {
        if (game_api->lib) {
          dlclose(game_api->lib);
        }            
#if OS_WINDOWS
        buf cp_cmd = arena_sprintf(gs->frame_arena, "copy build/libgame.so build/libgame_%d.so", reload_count);
#else
        buf cp_cmd = arena_sprintf(gs->frame_arena, "cp build/libgame.so build/libgame_%d.so", reload_count);
#endif
        system(cp_cmd.data);
        game_api->last_modified = mod_time;
        gamelib_path = arena_sprintf(gs->frame_arena, "build/libgame_%d.so", reload_count);
      } else {
        return;
      }
    }

    // Then load the function pointers as necessary
    void *game_lib = dlopen(gamelib_path.data, RTLD_LAZY | RTLD_DEEPBIND);
    assert(game_lib);
    game_api->init = (game_init_fn)dlsym(game_lib, "game_init");
    assert(game_api->init);
    game_api->update = (game_update_fn)dlsym(game_lib, "game_update");
    assert(game_api->update);
    game_api->render = (game_render_fn)dlsym(game_lib, "game_render");
    assert(game_api->render);
    game_api->shutdown = (game_shutdown_fn)dlsym(game_lib, "game_shutdown");
    assert(game_api->shutdown);
    game_api->lib = game_lib;
    assert(game_api->lib);
    game_api->last_modified = mod_time;

    reload_count+=1;
  }
  if (call_init) {
    game_api->init(gs);
  }
}

int main(void) {
  profiler_begin();
  BRAND_SEED(time(0));

  Game_State gs = {};
  Game_Api game_api = {};
  scratch_init(MB(100));

  /////////////////////////////////////////////////////
  // 0. SDL3 initialization (window + OpenGL)
  /////////////////////////////////////////////////////

  if (!SDL_Init(SDL_INIT_VIDEO)) {
      fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      return 1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  if(TTF_Init() == false) {
      fprintf(stderr, "SDL_ttf init failed: %s\n", SDL_GetError());
      return 1;
  }

  SDL_Window *window = SDL_CreateWindow("window", 800, 600, SDL_WINDOW_OPENGL);
  if (!window) {
      fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
      fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
  }
  // VSYNC I Think?
  //SDL_GL_SetSwapInterval(1);

  if (!SDL_GL_MakeCurrent(window, gl_context)) {
      fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
      SDL_GL_DestroyContext(gl_context);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
  }

  if (GL_loadGL((GLloadfunc)SDL_GL_GetProcAddress)) {
      printf("Failed to load OpenGL functions\n");
      return -1;
  }

  printf("OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
  printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
  printf("OpenGL version:  %s\n", glGetString(GL_VERSION));
  printf("GLSL version:    %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  /////////////////////////////////////////////////////
  // 2. Game_State initialization
  /////////////////////////////////////////////////////
  gs.persistent_arena = arena_make(GB(1));
  gs.frame_arena = arena_make(MB(256));
  gs.wdim = v2m(800, 600);

  ogl_init(); // To create the bullshit empty VAO opengl side, nothing else

  // Asset loading stuff
  am_init(gs.persistent_arena, gs.frame_arena);

  // Load a texture
  static const u8 atlas_data[] = {
#embed "../../data/microgue.png"
  };
  gs.atlas = am_load_from_data(STR8L("atlas.png"), STR8((char*)atlas_data, sizeof(atlas_data)));
  gs.atlas_sprites_per_dim = v2m(16,10);

  gs.model_asset_id = am_load_from_fullpath(STR8L("data/gltf-sample-models/2.0/Avocado/glTF/Avocado.gltf"));

  gs.font = bfont_load_default_atlas(gs.persistent_arena, 32, 256, 256);

  f64 dt = 1.0/60.0;
  u64 frame_count = 0;
  platform_try_reload_gamelib(&gs, &game_api, true);

  /////////////////////////////////////////////////////
  // 3. Game Loop
  /////////////////////////////////////////////////////

  while (true) {
    frame_count+=1;
    u64 frame_start = platform_read_cpu_timer();
    ogl_clear();
    arena_clear(gs.frame_arena);

    /////////////////////////////////////////////////////
    // 3.1 Reloading logic (happens once every second/target_frames)
    /////////////////////////////////////////////////////
    if (frame_count % 60 == 0) {
      platform_try_reload_gamelib(&gs, &game_api, false);
    }

    /////////////////////////////////////////////////////
    // 3.2 Handling incoming events for the frame
    /////////////////////////////////////////////////////

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        exit(1);
      }
      Input_Event_Node input_event = {};
      if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        // Update Game_State with new window dimensions
        gs.wdim = v2m(event.window.data1, event.window.data2);
        input_event.evt = (Input_Event){
          .kind = INPUT_EVENT_KIND_RESIZE,
        };
      } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        v2 mouse_pos = v2m(event.motion.x, gs.wdim.y - event.motion.y);
        input_event.evt = (Input_Event){
          .data.mme = (Input_MouseMotion_Event) { .mouse_pos = mouse_pos },
          .kind = INPUT_EVENT_KIND_MOUSEMOTION,
        };
      } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        v2 wheel_delta = v2m(event.wheel.x, event.wheel.y);
        input_event.evt = (Input_Event){
          .data.mwe = (Input_MouseWheel_Event) { .wheel_delta = wheel_delta },
          .kind = INPUT_EVENT_KIND_MOUSEWHEEL,
        };
      } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        input_event.evt = (Input_Event){
          .data.me = (Input_Mouse_Event) {
            .button = (Input_Mouse_Button)(event.button.button - 1),
            .is_down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN),
          },
          .kind = INPUT_EVENT_KIND_MOUSE,
        };
      } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        input_event.evt = (Input_Event){
          .data.ke = (Input_Keeb_Event) {
            .scancode = (Key_Scancode)event.key.scancode,
            .is_down = (event.type == SDL_EVENT_KEY_DOWN),
          },
          .kind = INPUT_EVENT_KIND_KEEB,
        };
      }
      input_push_event(&gs.input, gs.frame_arena, &input_event.evt);
    }
    input_process_events(&gs.input);

    /////////////////////////////////////////////////////
    // 3.3 Perform update + render calling the game lib
    /////////////////////////////////////////////////////
    {
      TIME_BLOCK("GAME_UPDATERENDER");
      r2d_begin(gs.frame_arena, gs.game_viewport);
      game_api.update(&gs, dt);
      game_api.render(&gs, dt);
#ifdef SOFT_REND
      ogl_tex_update(&gs.g_backbuffer, (u8*)gs.pixels, gs.wdim.x, gs.wdim.y, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){});
#endif
    }

    /////////////////////////////////////////////////////
    // 3.4 Render all the stuff (captured in game_render(..))
    /////////////////////////////////////////////////////
    r2d_flush_all();

    /////////////////////////////////////////////////////
    // 3.5 Swap the window (Desktop mode only)
    /////////////////////////////////////////////////////
#if !(ARCH_WASM64 || ARCH_WASM32)
  // Swap the window 
  {
    TIME_BLOCK("Swap Window");
    SDL_GL_SwapWindow(window);
  }
#endif // !(ARCH_WASM64 || ARCH_WASM32)

    /////////////////////////////////////////////////////
    // 3.6 EOF Timing stuff (dt/sleep/timecalc)
    /////////////////////////////////////////////////////
    input_end_frame(&gs.input);
    u64 frame_end = platform_read_cpu_timer();
    dt = (frame_end - frame_start) / (f64)get_nano_freq();
    gs.time_sec += platform_get_time() - frame_start / (f64) get_nano_freq();
    //printf("fps=%f begin=%f end=%f\n", 1.0/dt, (f32)frame_start, (f32)frame_end);
    //printf("sec: %f\n", gs.time_sec);
  }

  /////////////////////////////////////////////////////
  // 4. Print profiler info + cleanup (optional)
  /////////////////////////////////////////////////////
  profiler_end_and_print();
  return 0;
}
