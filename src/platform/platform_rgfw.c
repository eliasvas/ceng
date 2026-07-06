#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>

#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#include "base/base_inc.h"
#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#if (ARCH_WASM64 || ARCH_WASM32)
#include <GLES3/gl3.h>
#else
#include "gl_loader.h"
#endif

#define OGL_IMPLEMENTATION
#include "core/ogl.h"

#define INPUT_IMPLEMENTATION
#include "core/input.h"

#define ASSET_MGR_IMPLEMENTATION
#include "core/asset_mgr.h"

// DESKTOP: Because miniaudio has TOO MANY warnings when building
// I have opted to put it in its own SILENT compilation unit (no -Wall)
#if (ARCH_WASM64 || ARCH_WASM32)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#else
#include "miniaudio/miniaudio.h"
#endif

#include "game.h"

#define RGFW_DEBUG
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#define RGFW_ALLOC_DROPFILES
#define RGFW_PRINT_ERRORS
#define RGFW_DEBUG
#define GL_SILENCE_DEPRECATION
#include <RGFW/RGFW.h>

ma_engine ma_eng;
// Currently we just export this to the game layer, there should be better way
void platform_play_sound(const char *sound) {
  ma_engine_play_sound(&ma_eng, sound, nullptr);
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
  // 0. RGFW initialization (window + OpenGL)
  /////////////////////////////////////////////////////
  RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
#if (ARCH_WASM64 || ARCH_WASM32)
  hints->major = 3;
  hints->minor = 0;
  hints->profile = RGFW_glES;
#else
  hints->major = 4;
  hints->minor = 3;
  //hints->profile = RGFW_glCompatibility;
  hints->profile = RGFW_glCore;
#endif // (ARCH_WASM64 || ARCH_WASM32)

  RGFW_setGlobalHints_OpenGL(hints);

  RGFW_window* win = RGFW_createWindow("window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize | RGFW_windowHide);
  RGFW_window_createContext_OpenGL(win, hints);
  RGFW_window_show(win);
  RGFW_window_setExitKey(win, RGFW_keyEscape);
  RGFW_window_swapInterval_OpenGL(win, 1);
  const GLubyte *version = glGetString(GL_VERSION);
  printf("OpenGL Version: %s\n", version);

#if !(ARCH_WASM64 || ARCH_WASM32)
  if (GL_loadGL((GLloadfunc)RGFW_getProcAddress_OpenGL)) {
      printf("Failed to load OpenGL functions\n");
      return -1;
  }
#endif // !(ARCH_WASM64 || ARCH_WASM32)
  /////////////////////////////////////////////////////
  // 1. miniaudio initialization
  /////////////////////////////////////////////////////
  ma_result result;
  ma_engine_config engineConfig;
  engineConfig = ma_engine_config_init();
  result = ma_engine_init(&engineConfig, &ma_eng);
  if (result != MA_SUCCESS) {
      return result;
  }
  ma_engine_set_volume(&ma_eng, 0.05);
  printf("miniaudio engine OK\n");

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
  gs.atlas = am_load_from_data(MAKE_STR("atlas.png"), (buf){(char*)atlas_data, sizeof(atlas_data)});
  gs.atlas_sprites_per_dim = v2m(16,10);
  gs.font = bfont_load_default_atlas(gs.persistent_arena, gs.frame_arena, 64, 1024, 1024);


  f64 dt = 1.0/60.0;
  u64 frame_count = 0;
  platform_try_reload_gamelib(&gs, &game_api, true);

  /////////////////////////////////////////////////////
  // 3. Game Loop
  /////////////////////////////////////////////////////
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
    frame_count+=1;
    u64 frame_start = platform_read_cpu_timer();
#if !(ARCH_WASM64 || ARCH_WASM32)
    ogl_clear();
#endif // !(ARCH_WASM64 || ARCH_WASM32)
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
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {
      Input_Event_Node input_event = {};
      switch(event.type) {
        case RGFW_windowResized:
          gs.wdim = v2m(event.update.w, event.update.h);
          input_event.evt = (Input_Event){
            .kind = INPUT_EVENT_KIND_RESIZE,
          };
          break;
        case RGFW_mousePosChanged:
          v2 new_mp = v2m(event.mouse.x, event.mouse.y);
          input_event.evt = (Input_Event){
            .data.mme = (Input_MouseMotion_Event) { .mouse_pos = new_mp },
            .kind = INPUT_EVENT_KIND_MOUSEMOTION,
          };
          break;
        case RGFW_keyPressed:
        case RGFW_keyReleased:
          if (event.key.repeat == 1) continue;
          s32 value = event.key.value;
          s32 scancode = 0;
          // @TODO: More keys mapped needed here please
          if (value >= 'A' && value <= 'Z') scancode = KEY_SCANCODE_A + (value-'A');
          else if (value >= 'a' && value <= 'z') scancode = KEY_SCANCODE_A + (value-'a');
          else if (value >= '0' && value <= '9') scancode = (value == '0') ? KEY_SCANCODE_0 : KEY_SCANCODE_1 + (value - '1');
          else if (value == RGFW_keyUp) scancode = KEY_SCANCODE_UP;
          else if (value == RGFW_keyDown) scancode = KEY_SCANCODE_DOWN;
          else if (value == RGFW_keyLeft) scancode = KEY_SCANCODE_LEFT;
          else if (value == RGFW_keyRight) scancode = KEY_SCANCODE_RIGHT;
          else if (value == RGFW_keyTab) scancode = KEY_SCANCODE_TAB;
          else if (value == RGFW_keyShiftL) scancode = KEY_SCANCODE_LSHIFT;
          else if (value == RGFW_keyShiftR) scancode = KEY_SCANCODE_LSHIFT;
          else if (value == RGFW_keyControlL) scancode = KEY_SCANCODE_LCTRL;
          else if (value == RGFW_keyControlR) scancode = KEY_SCANCODE_RCTRL;
          else if (value == RGFW_keyAltL) scancode = KEY_SCANCODE_LALT;
          else if (value == RGFW_keyAltR) scancode = KEY_SCANCODE_RALT;
          else if (value == RGFW_keySpace) scancode = KEY_SCANCODE_SPACE;

          input_event.evt = (Input_Event){
            .data.ke = (Input_Keeb_Event) {
              .scancode = (Key_Scancode)scancode,
              .is_down = (event.type == RGFW_keyPressed),
            },
            .kind = INPUT_EVENT_KIND_KEEB,
          };
          break;
        case RGFW_mouseButtonPressed:
        case RGFW_mouseButtonReleased:
          b32 button_idx = event.button.value;
          if (button_idx >= INPUT_MOUSE_COUNT) continue; // no handling
          input_event.evt = (Input_Event){
            .data.me = (Input_Mouse_Event) {
              .button = (Input_Mouse_Button)(button_idx),
              .is_down = (event.type == RGFW_mouseButtonPressed),
            },
            .kind = INPUT_EVENT_KIND_MOUSE,
          };
          break;
        default:
          continue;
          break;
      }
      input_push_event(&gs.input, gs.frame_arena, &input_event.evt);
    }

    input_process_events(&gs.input);

    /////////////////////////////////////////////////////
    // 3.3 Perform update + render calling the game lib
    /////////////////////////////////////////////////////
    {
      TIME_BLOCK("GAME_UPDATERENDER");
      rn_begin(gs.frame_arena, gs.game_viewport);
      game_api.update(&gs, dt);
      game_api.render(&gs, dt);
#ifdef SOFT_REND
      ogl_tex_update(&gs.g_backbuffer, (u8*)gs.pixels, gs.wdim.x, gs.wdim.y, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){});
#endif
    }

    /////////////////////////////////////////////////////
    // 3.4 Render all the stuff (captured in game_render(..))
    /////////////////////////////////////////////////////
    rn_flush_all();

    /////////////////////////////////////////////////////
    // 3.5 Swap the window (Desktop mode only)
    /////////////////////////////////////////////////////
#if !(ARCH_WASM64 || ARCH_WASM32)
  // Swap the window 
  {
    TIME_BLOCK("Swap Window");
    RGFW_window_swapBuffers_OpenGL(win);
  }
#endif // !(ARCH_WASM64 || ARCH_WASM32)

    /////////////////////////////////////////////////////
    // 3.6 EOF Timing stuff (dt/sleep/timecalc)
    /////////////////////////////////////////////////////
    input_end_frame(&gs.input);
    u64 frame_end = platform_read_cpu_timer();
#if (ARCH_WASM64 || ARCH_WASM32)
    dt = (frame_end - frame_start) / (f64)get_nano_freq();
    f64 wasm_target_ms = 16.66;
    f64 sleep_ms = wasm_target_ms - (dt * 1000);
    dt = wasm_target_ms / 1000.0;
    gs.time_sec += wasm_target_ms / 1000.0;
    emscripten_sleep((u32)sleep_ms);
#else 
    dt = (frame_end - frame_start) / (f64)get_nano_freq();
    gs.time_sec += platform_get_time() - frame_start / (f64) get_nano_freq();
    //printf("fps=%f begin=%f end=%f\n", 1.0/dt, (f32)frame_start, (f32)frame_end);
    //printf("sec: %f\n", gs.time_sec);
#endif // (ARCH_WASM64 || ARCH_WASM32)

  }

  /////////////////////////////////////////////////////
  // 4. Print profiler info + cleanup (optional)
  /////////////////////////////////////////////////////
  profiler_end_and_print();
  RGFW_window_close(win);
  return 0;
}
