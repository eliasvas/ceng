//#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"
#include "gui2/g2.h"
#include "entity.h"

extern void platform_play_sound(const char *sound);

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

  gui_init(gs->frame_arena, &gs->font, &gs->input);

}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
}


void game_render(Game_State *gs, float dt) {
#if 0
  entity_store_update_render(gs, dt);
#else
  //rn_imm_tri(gs->game_viewport, v3m(-1,+1,0), v3m(-1,-1,0), v3m(+1,-1,0), gs->time_sec*3.14);
  //rn_imm_tri(gs->game_viewport, v3m(+1,-1,0), v3m(+1,+1,0), v3m(-1,+1,0),  gs->time_sec*3.14);
  m4 model = m4_mult(m4_translate(v3m(0,0,-10)), m4_rotate(gs->time_sec*3.14, v3m(0,1,0)));

  Tri_Vertex my_verts[] = {
    (Tri_Vertex) {.pos = v3m(-1,+1,0), .color = v4m(1,0,1,1)}, 
    (Tri_Vertex) {.pos = v3m(-1,-1,0), .color = v4m(1,1,0,1)}, 
    (Tri_Vertex) {.pos = v3m(+1,-1,0), .color = v4m(0,1,1,1)}, 
    (Tri_Vertex) {.pos = v3m(+1,+1,0), .color = v4m(0,0,1,1)}, 
  };
  rn_imm_tri(gs->game_viewport, my_verts, array_count(my_verts), OGL_PRIM_TYPE_TRIANGLE_FAN, model);

  Tri_Vertex my_verts2[] = {
    (Tri_Vertex) {.pos = v3m(-1,+1,0), .color = v4m(1,0,0,1)}, 
    (Tri_Vertex) {.pos = v3m(-1,-1,0), .color = v4m(1,0,0,1)}, 
    (Tri_Vertex) {.pos = v3m(+1,-1,0), .color = v4m(1,0,0,1)}, 
    (Tri_Vertex) {.pos = v3m(+1,+1,0), .color = v4m(1,0,0,1)}, 
    (Tri_Vertex) {.pos = v3m(-1,+1,0), .color = v4m(1,0,0,1)}, 
  };
  rn_imm_tri(gs->game_viewport, my_verts2, array_count(my_verts2), OGL_PRIM_TYPE_LINE_STRIP, model);

#endif
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport);

  Gui_Layout_Params params = (Gui_Layout_Params) {
    .flags = (G2_BOX_FLAG_FIXED_X | G2_BOX_FLAG_FIXED_Y),
    .fixed_x = 100,
    .fixed_y = 100,
  };


  // Small GUI test (mainly for Box reuse test right now)
  static bool other_enabled = false;
  if (gui_button(MAKE_STR("Click Secret"), params)) {
    other_enabled = !other_enabled;
    printf("Reset");
  }

  if (other_enabled) {
    params.fixed_y += 100;
    if (gui_button(MAKE_STR("WOW"), params)) {
      printf("WOW\n");
    }
    //gs->request_reload = true;
  }

  gui_end();
}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

