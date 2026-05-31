//#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"
#include "gui/gui.h"
#include "gui2/g2.h"
#include "entity.h"

// TODO: Move to a better 2D renderer that can do arbitrary polygons not just AABBS rotated?
// TODO: lookup a good fzf pipeline to be able to search
// TODO: Maybe this should be just a test-bed and have repos reference this?? idk, I dont want many assets inside this repo maybe
// TODO: make-prg for building via build.sh and make an argument to only build, also run, and also just export the object files


const char* entity_map= R"(@@@@@@@@@@
@$#######@
@##$$####@
@##$#####@
@##$$####@
@#$####$$@
@@@@@@@@@@
)";

void game_init(Game_State *gs) {
  entity_store_init();
  // Add the map
  entity_store_add_map(entity_map);
  // Make the hero
  Entity *hero = entity_store_add();
  hero->kind = ENTITY_KIND_HERO;
  hero->tex_coords = rec(4*8,9*8,8,8);

  hero->start_coords = v3m(1,1,0);
  hero->target_coords = v3m(1,1,0);
  entity_set_coords_imm(hero, v3m(1,1,0));

  hero->layer = 1;

  g2_init(gs->frame_arena, &gs->font, &gs->cmd_list, &gs->input);
}

void game_update(Game_State *gs, float dt) {
  static bool gui_initialized = false;
  if (!gui_initialized) {
    gui_context_init(gs->frame_arena, &gs->font);
    gui_initialized = true;
  }
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
}


void game_render(Game_State *gs, float dt) {
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 10.0, .rot_deg = 0} };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);

  entity_store_update_render(gs, dt);

  // Perform a reload if reset button is clicked
  g2_begin(gs->game_viewport);
  if (g2_button(MAKE_STR("reset"), v2m(20,20))) {
    gs->request_reload = true;
  }
  g2_end();
}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

