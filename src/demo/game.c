//#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"
#include "gui/gui.h"
#include "gui2/g2.h"
#include "entity.h"

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

  g2_init(gs->frame_arena, &gs->font, &gs->input);

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
#if 0
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R_Cmd cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_CAMERA, .c = (R_C2D){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R_Cmd){ .kind = R_CMD_KIND_SET_CAMERA_2D, .c = (R_C2D){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 10.0, .rot_deg = 0} };
  r_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
#endif

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

