#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>

#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#include "base/base_inc.h"

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#if (ARCH_WASM64 || ARCH_WASM32)
#include <GLES3/gl3.h>
#else
#include "gl_loader.h"
#endif

#define OGL_IMPLEMENTATION
#include "core/ogl.h"

#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"

#define RGFW_DEBUG
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#define RGFW_ALLOC_DROPFILES
#define RGFW_PRINT_ERRORS
#define RGFW_DEBUG
#define GL_SILENCE_DEPRECATION
#include <RGFW/RGFW.h>

u64 platform_read_cpu_timer() {
  return get_time_ns();
}

u64 platform_read_cpu_freq() {
  return get_nano_freq();
}

f64 platform_get_time() {
  return (f64)get_time_ns() / (f64)get_nano_freq();
}

typedef struct {
  u8 *data;
  u64 width;
  u64 height;
} Platform_Image_Data;
Platform_Image_Data platform_load_image_bytes_as_rgba(const char *filepath);

Platform_Image_Data platform_load_image_bytes_as_rgba(const char *filepath) {
  Platform_Image_Data img_data = {};

  stbi_set_flip_vertically_on_load(true);
  int width, height, nrChannels;
  u8 *px_data = stbi_load(filepath, &width, &height, &nrChannels, STBI_rgb_alpha);

  img_data.width = width;
  img_data.height = height;
  img_data.data = px_data;

  return img_data;
}

int main(void) {
  profiler_begin();
  BRAND_SEED(time(0));

  Game_State gs = {};

  // TODO: Try to do the reload thingy
#if (ARCH_WASM64 || ARCH_WASM32 || 1)
  Game_Api game_api = (Game_Api){
    .init     = game_init,
    .update   = game_update,
    .render   = game_render,
    .shutdown = game_shutdown,
  };
#else
  Game_Api game_api = {};
  void *game_lib = dlopen("./build/libgame.so", RTLD_LAZY | RTLD_DEEPBIND);
  assert(game_lib);
  game_api.lib = game_lib;
  game_api.init = (game_init_fn)dlsym(game_lib, "game_init");
  game_api.update = (game_update_fn)dlsym(game_lib, "game_update");
  game_api.render = (game_render_fn)dlsym(game_lib, "game_render");
  game_api.shutdown = (game_shutdown_fn)dlsym(game_lib, "game_shutdown");
#endif // (ARCH_WASM64 || ARCH_WASM32 || 1)

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
  // 1. Game_State initialization
  /////////////////////////////////////////////////////
  gs.persistent_arena = arena_make(GB(1));
  gs.frame_arena = arena_make(MB(256));
  gs.wdim = v2m(800, 600);

  ogl_init(); // To create the bullshit empty VAO opengl side, nothing else
  gs.red = ogl_tex_make((u8[]){250,90,72,255}, 1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  Platform_Image_Data image = platform_load_image_bytes_as_rgba("data/microgue.png");
  assert(image.width > 0);
  assert(image.height > 0);
  assert(image.data != nullptr);
  //gs.atlas = ogl_tex_make(image.data, image.width, image.height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});
  gs.atlas = ogl_tex_make(image.data, image.width, image.height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){});
  gs.atlas_sprites_per_dim = v2m(16,10);
  gs.font = font_util_load_default_atlas(gs.persistent_arena, 64, 1024, 1024);
  stbi_image_free(image.data);
  gs.g_backbuffer = ogl_tex_make(nullptr,0,0,OGL_TEX_FORMAT_RGBA8U,(Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  f64 dt = 1.0/60.0;
  u64 frame_count = 0;
  game_api.init(&gs);

  /////////////////////////////////////////////////////
  // 2. Game Loop
  /////////////////////////////////////////////////////
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
    frame_count+=1;
    u64 frame_start = platform_read_cpu_timer();
#if !(ARCH_WASM64 || ARCH_WASM32)
    ogl_clear(col(0,0,0.0,1.0));
#endif // !(ARCH_WASM64 || ARCH_WASM32)
    arena_clear(gs.frame_arena);


    /////////////////////////////////////////////////////
    // 2.1 Reloading logic (happens once every second/target_frames)
    /////////////////////////////////////////////////////
#if !(ARCH_WASM64 || ARCH_WASM32) 
    if (frame_count % 60 == 0) {
      struct stat glib_stat;
      if (stat("build/libgame.so", &glib_stat) == -1) {
        printf("couldn't stat libgame.so\n");
      } else {
        s64 mod_time = glib_stat.st_mtim.tv_nsec;
        if (game_api.last_modified != mod_time) {
          static int reload_count = 0;
          reload_count+=1;
          // This is ugly as hell
#if OS_WINDOWS
          buf cp_cmd = arena_sprintf(gs.frame_arena, "copy build/libgame.so build/libgame_%d.so", reload_count);
#else
          buf cp_cmd = arena_sprintf(gs.frame_arena, "cp build/libgame.so build/libgame_%d.so", reload_count);
#endif
          system(cp_cmd.data);
          buf glib_newpath = arena_sprintf(gs.frame_arena, "build/libgame_%d.so", reload_count);
          if (game_api.lib) {
            dlclose(game_api.lib);
          }            
          void *game_lib = dlopen(glib_newpath.data, RTLD_LAZY | RTLD_DEEPBIND);
          assert(game_lib);
          game_api.init = (game_init_fn)dlsym(game_lib, "game_init");
          game_api.update = (game_update_fn)dlsym(game_lib, "game_update");
          game_api.render = (game_render_fn)dlsym(game_lib, "game_render");
          game_api.shutdown = (game_shutdown_fn)dlsym(game_lib, "game_shutdown");
          game_api.lib = game_lib;
          game_api.last_modified = mod_time;
          printf("reload mod time: %ld\n", mod_time);
        }
      }
    }
#endif // !(ARCH_WASM64 || ARCH_WASM32) 

    /////////////////////////////////////////////////////
    // 2.2 Handling incoming events for the frame
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
          else if (value >= '0' && value < '9') scancode = (value == '0') ? KEY_SCANCODE_0 : KEY_SCANCODE_1 + (value - '1');
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
    // 2.3 Perform update + render calling the game lib
    /////////////////////////////////////////////////////
    gs.pixels = arena_push(gs.frame_arena, sizeof(u32)*gs.wdim.x*gs.wdim.y);
    game_api.update(&gs, dt);
    game_api.render(&gs, dt);
    ogl_tex_update(&gs.g_backbuffer, (u8*)gs.pixels, gs.wdim.x, gs.wdim.y, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){});

    /////////////////////////////////////////////////////
    // 2.4 Render the software rendered texture
    /////////////////////////////////////////////////////
    R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs.game_viewport };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs.game_viewport };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs.game_viewport.w/2.0, gs.game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs.zoom, .rot_deg = 0} };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    R2D_Quad quad = (R2D_Quad) {
        .src_rect = rec(0,0,gs.wdim.x,gs.wdim.y),
        .dst_rect = rec(0,0,gs.wdim.x,gs.wdim.y),
        .c = col(1,1,1,1),
        .tex = gs.g_backbuffer,
        .rot_deg = 0,
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);

    /////////////////////////////////////////////////////
    // 2.5 Render all the quads (captured in game_render(..))
    /////////////////////////////////////////////////////
    r2d_render_cmds(gs.frame_arena, &gs.cmd_list);

    /////////////////////////////////////////////////////
    // 2.6 Swap the window (Desktop mode only)
    /////////////////////////////////////////////////////
#if !(ARCH_WASM64 || ARCH_WASM32)
  // Swap the window 
  {
    TIME_BLOCK("Swap Window");
    RGFW_window_swapBuffers_OpenGL(win);
  }
#endif // !(ARCH_WASM64 || ARCH_WASM32)

    /////////////////////////////////////////////////////
    // 2.7 EOF Timing stuff (dt/sleep/timecalc)
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
  // 3. Print profiler info + cleanup (optional)
  /////////////////////////////////////////////////////
  profiler_end_and_print();
  RGFW_window_close(win);
  return 0;
}
