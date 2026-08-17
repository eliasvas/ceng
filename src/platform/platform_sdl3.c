#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>

// dlopen/dlclose
#include <dlfcn.h>

#include <SDL3/SDL.h>
#include "gl_loader.h"

#define STR_IMPLEMENTATION
#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#include "base/base_inc.h"
#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#define OGL_IMPLEMENTATION
#include "rend/rend_inc.h"

#define INPUT_IMPLEMENTATION
#include "core/input.h"

// TODO: Stop the asset manager single module bullshit
#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"
#include "game.h"


// Currently we just export this to the game layer, there should be better way
void platform_play_sound(const char *sound) {
  // TODO: SDL3_sound?
}

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

  static const u8 atlas_data[] = {
#embed "../../data/microgue.png"
  };
  gs.atlas = am_load_from_data(STR8L("atlas.png"), STR8((char*)atlas_data, sizeof(atlas_data)));
  gs.atlas_sprites_per_dim = v2m(16,10);

  gs.font = bfont_load_default_atlas(gs.persistent_arena, gs.frame_arena, 32, 1024, 1024);

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
      platform_try_reload_gamelib(&gs, &game_api, true);
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
