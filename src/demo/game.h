#ifndef _GAME_H__
#define _GAME_H__

#include "base/base_inc.h"
#include "core/core_inc.h"

// TODO: Input should be a pointer right??
// Allocated along with game state!

typedef struct {
  s32 current_sine_sample; // not needed
  s32 sample_rate;
  s32 channel_count;

  // Game must fill these every frame (!!)
  f32 *samples;
  u64 samples_requested;
} Game_Audio_Output_Buffer;

typedef struct {
  Arena *persistent_arena; // For persistent allocations
  Arena *frame_arena; // For per-frame allocations
  rect game_viewport;
  b32 should_close;
  
  // Interface between platform <-> game
  f32 time_sec;
  v2 wdim;
  Input input;
  R2D_Cmd_Chunk_List cmd_list;
  Game_Audio_Output_Buffer audio_out;

  Ogl_Tex g_backbuffer;
  u32* pixels;

  // Loaded Asset resources (TODO: Asset system)
  Ogl_Tex atlas;
  v2 atlas_sprites_per_dim;
  Ogl_Tex red;
  Font_Info font;

} Game_State;

void game_init(Game_State *gs);
void game_update(Game_State *gs, f32 dt);
void game_render(Game_State *gs, f32 dt);
void game_shutdown(Game_State *gs);

typedef void (*game_init_fn) (Game_State *gs);
typedef void (*game_update_fn) (Game_State *gs, f32 dt);
typedef void (*game_render_fn) (Game_State *gs, f32 dt);
typedef void (*game_shutdown_fn) (Game_State *gs);

typedef struct {
  game_init_fn init;
  game_update_fn update;
  game_render_fn render;
  game_shutdown_fn shutdown;

  void *lib;
  s64 last_modified;

 u64 api_version;
} Game_Api;

#endif
