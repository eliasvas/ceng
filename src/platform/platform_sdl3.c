// Desc:
// This is the 'entry point' of the program
// Here we initialize all state (SDL/opengl/single-header libs)
// for each backend (e.g SDL/GLFW etc..) there should be one platform_xxx.c file

#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

#include <stdio.h>
#include <assert.h>

#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#include "base/base_inc.h"

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <GL/glew.h>
#define OGL_IMPLEMENTATION
#include "core/ogl.h"

#if !(ARCH_WASM64 || ARCH_WASM32)
#define INPUT_IMPLEMENTATION
#endif

#include "core/input.h"

#include "game.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

// Hack :)
#if OS_WINDOWS
  #define GAME_DLL "game.dll"
#else
  #define GAME_DLL "game.so"
#endif

static Game_Api game_api = {};
//BRAND_SEED_DEF();

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

typedef struct {
  SDL_AudioStream *stream;
  SDL_AudioFormat format;

  s32 current_sine_sample; // not needed
  s32 sample_rate;
  s32 channel_count;
}SDL_Audio_Output_Buffer;

typedef struct {
  SDL_Window *window;
  SDL_GLContext context;

  SDL_Audio_Output_Buffer audio_output;

  f64 dt;
  u64 frame_start;

  Game_State gs;
} SDL_State;


u64 platform_read_cpu_timer() {
  return SDL_GetPerformanceCounter();
}

u64 platform_read_cpu_freq() {
  return SDL_GetPerformanceFrequency();
}

u64 platform_get_ticks_since_epoch() {
  SDL_Time ticks = {};
  SDL_GetCurrentTime(&ticks);
  return (u64)ticks;
}

f64 platform_get_time() {
  return (f64)SDL_GetTicksNS() / 1000000000.0;
}

bool platform_try_load_game_api(Game_Api *prev) {
  // 1. Find dynlib path 
  char dll_name[1024];
  sprintf(dll_name, "%s/%s", SDL_GetBasePath(), GAME_DLL);

  // 2. Get its last_modified time 
  SDL_PathInfo pinfo = {};
  if (!SDL_GetPathInfo(dll_name, &pinfo)) return false;

  // 3. Copy library to game_{ver} for hot-reload to work
  char dll_ver_name[1024];
  game_api.api_version += 1;
  sprintf(dll_ver_name, "%s/%s%lu%s", SDL_GetBasePath(), "game_",game_api.api_version,".so");
  if (!SDL_CopyFile(dll_name, dll_ver_name)) return false;

  // 4. Load the library
  SDL_SharedObject *game_dll = SDL_LoadObject(dll_ver_name);
  if (!game_dll) return false;

  // 5. Fill the Game_Api struct
  game_api.init = (game_init_fn)SDL_LoadFunction(game_dll, "game_init");
  game_api.update = (game_update_fn)SDL_LoadFunction(game_dll, "game_update");
  game_api.render = (game_render_fn)SDL_LoadFunction(game_dll, "game_render");
  game_api.shutdown = (game_shutdown_fn)SDL_LoadFunction(game_dll, "game_shutdown");
  game_api.lib = game_dll;
  game_api.last_modified = pinfo.modify_time;
  if (!game_api.init || !game_api.update || !game_api.render || !game_api.shutdown) return false;

  return true;
}
void platform_unload_game_api(Game_Api *api) {
  // 1. delete the Library
  if (api->lib) {
    SDL_UnloadObject(api->lib);
    api->lib = nullptr;
  }
  // 2. delete the game_{ver}.so file (optional)
 
}

Ogl_Tex g_backbuffer;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  TIME_FUNC;

  profiler_begin();

  BRAND_SEED(platform_get_ticks_since_epoch());

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
    SDL_Log("Could not initialize SDL");
    return SDL_APP_FAILURE;
  }

  // FIXME: Why can't we do ES 3.0 on desktop mode? We should be able to
#if (ARCH_WASM64 || ARCH_WASM32)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_State *sdl_state= M_ALLOC(sizeof(SDL_State));
  M_ZERO_STRUCT(sdl_state);
  *appstate = sdl_state;

  // TODO: Maybe there should be some configuration file for stuff like this and profiling and stuff
#define DEFAULT_WIN_DIM_X 800
#define DEFAULT_WIN_DIM_Y 600

  sdl_state->window = SDL_CreateWindow("g0", DEFAULT_WIN_DIM_X, DEFAULT_WIN_DIM_Y, SDL_WINDOW_OPENGL);
  if (!sdl_state->window) {
    SDL_Log("Could not create window");
  }

  sdl_state->context = SDL_GL_CreateContext(sdl_state->window);
  if (!sdl_state->context) {
    SDL_Log("Could not create OpenGL context");
  }

  if (!SDL_GL_MakeCurrent(sdl_state->window, sdl_state->context)) {
    SDL_Log("Could not make OpenGL context current");
  }
  glewInit();

  SDL_Log("OpenGL version: %s\n", glGetString(GL_VERSION));
  SDL_Log("Renderer: %s\n", glGetString(GL_RENDERER));
  SDL_Log("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  SDL_SetWindowResizable(sdl_state->window, true);
  SDL_GL_SetSwapInterval(1); // Set this to 0 to disable VSync

  sdl_state->gs.persistent_arena = arena_make(GB(1));
  sdl_state->gs.frame_arena = arena_make(MB(256));
  sdl_state->gs.wdim = v2m(DEFAULT_WIN_DIM_X, DEFAULT_WIN_DIM_Y);

  // @Initialize the Audio output buffer..
  sdl_state->audio_output = (SDL_Audio_Output_Buffer) {
    .stream = nullptr,
    .current_sine_sample = 0,
    .channel_count = 1,
    .format = SDL_AUDIO_F32,
    .sample_rate = 8000, // Hz
  };
  SDL_AudioSpec spec = (SDL_AudioSpec) {
    .channels = sdl_state->audio_output.channel_count,
    .format = sdl_state->audio_output.format,
    .freq = sdl_state->audio_output.sample_rate,
  };
  sdl_state->audio_output.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
  if (sdl_state->audio_output.stream == nullptr) {
    SDL_Log("Audio stream creation Failed");
    return SDL_APP_FAILURE;
  }
  SDL_ResumeAudioStreamDevice(sdl_state->audio_output.stream);
  sdl_state->gs.audio_out = (Game_Audio_Output_Buffer) {
    .sample_rate = sdl_state->audio_output.sample_rate,
    .current_sine_sample= sdl_state->audio_output.current_sine_sample,
    .channel_count = sdl_state->audio_output.channel_count,
    // not really needed..
    .samples = nullptr,
    .samples_requested = 0,
  };

  r_clear_cmds(&sdl_state->gs.cmd_list);

  // IMPORTANT:
  // These are used by the game, loaded in platform code for now.
  // When we implement actual Asset/Resource system this will change.. I hope!
  ogl_init(); // To create the bullshit empty VAO opengl side, nothing else
  Game_State *gs = &sdl_state->gs;
  gs->red = ogl_tex_make((u8[]){250,90,72,255}, 1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  Platform_Image_Data image = platform_load_image_bytes_as_rgba("data/microgue.png");
  assert(image.width > 0);
  assert(image.height > 0);
  assert(image.data != nullptr);
  gs->atlas = ogl_tex_make(image.data, image.width, image.height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});
  gs->atlas_sprites_per_dim = v2m(16,10);
  gs->font = bfont_load_default_atlas(gs->persistent_arena, 64, 1024, 1024);
  stbi_image_free(image.data);

  //---------------------------------------------------------------------------
  // software renderer WIP
  g_backbuffer = ogl_tex_make(nullptr,0,0,OGL_TEX_FORMAT_RGBA8U,(Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  //---------------------------------------------------------------------------

#if (ARCH_WASM64 || ARCH_WASM32)
  // On WASM we just load the functions..
  game_api.init = game_init;
  game_api.update = game_update;
  game_api.render = game_render;
  game_api.shutdown = game_shutdown;
#else
  if (!platform_try_load_game_api(&game_api)){
    printf("libgame failed to load err:[%s] - exiting!", SDL_GetError());
    return SDL_APP_SUCCESS;
  }
#endif

  game_api.init(&sdl_state->gs);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  TIME_FUNC;
  SDL_State *sdl_state = (SDL_State*)appstate;

  Input_Event_Node input_event = {};
  if (event->type == SDL_EVENT_QUIT) {
      return SDL_APP_SUCCESS;
  } else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
    // Update Game_State with new window dimensions
    sdl_state->gs.wdim = v2m(event->window.data1, event->window.data2);
    input_event.evt = (Input_Event){
      .kind = INPUT_EVENT_KIND_RESIZE,
    };
  } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
    v2 mouse_pos = v2m(event->motion.x, event->motion.y);
    input_event.evt = (Input_Event){
      .data.mme = (Input_MouseMotion_Event) { .mouse_pos = mouse_pos },
      .kind = INPUT_EVENT_KIND_MOUSEMOTION,
    };
  } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
    v2 wheel_delta = v2m(event->wheel.x, event->wheel.y);
    input_event.evt = (Input_Event){
      .data.mwe = (Input_MouseWheel_Event) { .wheel_delta = wheel_delta },
      .kind = INPUT_EVENT_KIND_MOUSEWHEEL,
    };
  } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    input_event.evt = (Input_Event){
      .data.me = (Input_Mouse_Event) {
        .button = (Input_Mouse_Button)(event->button.button - 1),
        .is_down = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN),
      },
      .kind = INPUT_EVENT_KIND_MOUSE,
    };
  } else if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
    input_event.evt = (Input_Event){
      .data.ke = (Input_Keeb_Event) {
        .scancode = (Key_Scancode)event->key.scancode,
        .is_down = (event->type == SDL_EVENT_KEY_DOWN),
      },
      .kind = INPUT_EVENT_KIND_KEEB,
    };
  }
  input_push_event(&sdl_state->gs.input, sdl_state->gs.frame_arena, &input_event.evt);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  TIME_FUNC;
  SDL_State *sdl_state = (SDL_State*)appstate;

  // Process the events for the current frame of execution
  input_process_events(&sdl_state->gs.input);
  // Clear the per-frame arena
  arena_clear(sdl_state->gs.frame_arena);

  // Perform update and render
  ogl_clear(col(0.0,0.0,0.0,1.0));
  sdl_state->gs.time_sec = platform_get_time();

  // Allocate needed audio samples for game to fill
  // Note: Currently we only output samples when there is less than a second of samples remaining, which is a gigantic delay..
  u64 samples_for_half_second = sdl_state->audio_output.sample_rate * sdl_state->audio_output.channel_count / 2;
  u64 queued_samples_count = SDL_GetAudioStreamQueued(sdl_state->audio_output.stream);
  f32 *samples_to_write = nullptr;
  u64 needed_samples = 0;
  if (queued_samples_count < samples_for_half_second) {
    u64 SAMPLE_COUNT = 512;
    samples_to_write = arena_push_array(sdl_state->gs.frame_arena, f32, SAMPLE_COUNT);
    needed_samples = SAMPLE_COUNT;
  }
  sdl_state->gs.audio_out.samples_requested = needed_samples;
  sdl_state->gs.audio_out.samples = samples_to_write;


  sdl_state->gs.pixels = arena_push(sdl_state->gs.frame_arena, sizeof(u32)*sdl_state->gs.wdim.x*sdl_state->gs.wdim.y);
  // Do actual game's update and render
  game_api.update(&sdl_state->gs, sdl_state->dt);
  game_api.render(&sdl_state->gs, sdl_state->dt);

  //---------------------------------------------------------------------------
  // software renderer WIP
  //if (input_win_resized(&sdl_state->gs.input)) {
    ogl_tex_update(&g_backbuffer, (u8*)sdl_state->gs.pixels, sdl_state->gs.wdim.x, sdl_state->gs.wdim.y, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  //}
  Game_State *gs = &sdl_state->gs; 
  R_Cmd cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_CAMERA, .c = (R_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_CAMERA, .c = (R_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  R_Quad quad = (R_Quad) {
      .src_rect = rec(0,0,gs->wdim.x,gs->wdim.y),
      .dst_rect = rec(0,0,gs->wdim.x,gs->wdim.y),
      .c = col(1,1,1,0.5),
      .tex = g_backbuffer,
      .rot_deg = 0,
  };
  cmd = (R_Cmd){ .kind = R_CMD_KIND_ADD_QUAD, .q = quad};
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //---------------------------------------------------------------------------

  // Do platform-side rendering of given commands, with r_begin/end + ogl
  r_render_cmds(sdl_state->gs.frame_arena, &sdl_state->gs.cmd_list);

  // Swap the window
  {
    TIME_BLOCK("Swap Window");
    SDL_GL_SwapWindow(sdl_state->window);
  }

  // Actually put the samples..
  if (queued_samples_count < samples_for_half_second) {
    sdl_state->audio_output.current_sine_sample = sdl_state->gs.audio_out.current_sine_sample;
    SDL_PutAudioStreamData(sdl_state->audio_output.stream, &samples_to_write[0], sizeof(f32) * needed_samples);
  }

#if !(ARCH_WASM64 || ARCH_WASM32)
  // Perform a reload if necessary
  char dll_name[1024];
  sprintf(dll_name, "%s/%s", SDL_GetBasePath(), GAME_DLL);
  SDL_PathInfo pinfo = {};
  SDL_GetPathInfo(dll_name, &pinfo);
  if (input_key_pressed(&sdl_state->gs.input, KEY_SCANCODE_R) || game_api.last_modified != pinfo.modify_time) {
    SDL_Delay(500);
    SDL_Log("Performing Reload");
    game_api.shutdown(&sdl_state->gs);
    platform_unload_game_api(&game_api);
    assert(platform_try_load_game_api(&game_api));
    //arena_clear(sdl_state->gs.persistent_arena); // optional 
    // TODO: should we have a case where we do game.init(..) too after loading
    //game_api.init(&sdl_state->gs); 
  }
#endif

 
  // Perform timings (Should this happen before swap window maybe?)
  u64 frame_end = SDL_GetTicksNS();
  sdl_state->dt = (frame_end - sdl_state->frame_start) / 1000000000.0;
  //printf("fps=%f begin=%f end=%f\n", 1.0/sdl_state->dt, (f32)sdl_state->frame_start, (f32)frame_end);
  sdl_state->frame_start = SDL_GetTicksNS();

  input_end_frame(&sdl_state->gs.input);
  return (sdl_state->gs.should_close) ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

// TODO: Here we could serialize everything right??
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  platform_unload_game_api(&game_api); // Is this needed though? maybe for a game_api.shutdown(..) save?
  profiler_end_and_print();

  SDL_State *sdl_state = (SDL_State*)appstate;
  SDL_Log("Quitting");
  SDL_GL_DestroyContext(sdl_state->context);
  SDL_DestroyWindow(sdl_state->window);
  SDL_Quit();
}

