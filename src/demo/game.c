//#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"
#include "gui2/g2.h"
#include "entity.h"

#define ASSET_MGR_IMPLEMENTATION
#include "core/asset_mgr.h"

extern void platform_play_sound(const char *sound);

void game_init(Game_State *gs) {
  entity_store_init();

  // Make the hero
  Entity *hero = entity_store_add();
  hero->kind = ENTITY_KIND_HERO;
  hero->pos = v3m(1,0,1);
  hero->dynamic = true;
  hero->size = v3m(1,1,1);

  gui_init(gs->frame_arena, &gs->font, &gs->input);
}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
}

void game_draw_origin_grid(Game_State *gs, s32 cell_count) {
  assert(cell_count%2 == 0 && "Cell count has to be divisible by 2 because its centered to origin dummy");
  color c1 = v4m(0.7,0.7,0.7,1);
  color c2 = v4m(0.8,0.4,0.4,1);
  color c3 = v4m(0.4,0.8,0.4,1);
  Tri_Vertex quad_verts[4] = {
    (Tri_Vertex) {.pos = v3m(0,0,1), .color = c1}, 
    (Tri_Vertex) {.pos = v3m(0,0,0), .color = c1}, 
    (Tri_Vertex) {.pos = v3m(1,0,0), .color = c1}, 
    (Tri_Vertex) {.pos = v3m(1,0,1), .color = c1}, 
  };

  // Regular grid
  for (s32 z_coord = - cell_count/2; z_coord < cell_count/2; z_coord += 1) {
    for (s32 x_coord = - cell_count/2; x_coord < cell_count/2; x_coord += 1) {
      if (z_coord == 0 && x_coord == 0) continue;

      // This is the logic..
      m4 model = m4_translate(v3m(x_coord,0,z_coord));
      rn_imm_verts(gs->game_viewport, quad_verts, array_count(quad_verts), OGL_PRIM_TYPE_LINE_LOOP, gs->view_mat, model);
    }
  } 

  // X-axis red highlight
  Tri_Vertex line_x[4] = {
    (Tri_Vertex) {.pos = v3m(-cell_count/2,0,0), .color = c2}, 
    (Tri_Vertex) {.pos = v3m(cell_count/2,0,0), .color = c2}, 
  };
  rn_imm_verts(gs->game_viewport, line_x, array_count(line_x), OGL_PRIM_TYPE_LINE_LOOP, gs->view_mat, m4d(1.0));

  // Z-axis green highlight
  Tri_Vertex line_z[4] = {
    (Tri_Vertex) {.pos = v3m(0,0,-cell_count/2), .color = c3}, 
    (Tri_Vertex) {.pos = v3m(0,0,cell_count/2), .color = c3}, 
  };
  rn_imm_verts(gs->game_viewport, line_z, array_count(line_z), OGL_PRIM_TYPE_LINE_LOOP, gs->view_mat, m4d(1.0));

}

void game_render(Game_State *gs, float dt) {

  m4 model = m4_mult(m4_translate(v3m(0,0,0)), m4_rotate(gs->time_sec*3.14, v3m(0,1,0)));
  gs->view_mat = m4_view(v3m(0,8,10), v3m(0,0,0), v3m(0,1,0));

  // 0. Draw grid
  game_draw_origin_grid(gs, 10);

  // 2. Draw the cube
  rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, gs->view_mat, m4d(1.0), v4m(1,0.3,0.3,1));
  rn_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, gs->view_mat, m4d(1.0), v4m(0,0,0,1));

  // 3. Draw a spinning quad
  Tri_Vertex my_verts[] = {
    (Tri_Vertex) {.pos = v3m(-1,+1,0), .color = v4m(1,0,1,0.5)}, 
    (Tri_Vertex) {.pos = v3m(-1,-1,0), .color = v4m(1,1,0,0.5)}, 
    (Tri_Vertex) {.pos = v3m(+1,-1,0), .color = v4m(0,1,1,0.5)}, 
    (Tri_Vertex) {.pos = v3m(+1,+1,0), .color = v4m(0,0,1,0.5)}, 
  };
  rn_imm_verts(gs->game_viewport, my_verts, array_count(my_verts), OGL_PRIM_TYPE_TRIANGLE_FAN, gs->view_mat, model);
  entity_store_update_render(gs, dt);


  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport);

  Gui_Layout_Params params = (Gui_Layout_Params) {
    .flags = (GUI_BOX_FLAG_CLICKABLE | GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT),
    .major_layout_axis = GUI_AXIS_X,
  };

  // Small GUI test (mainly for Box reuse test right now)
  static bool other_enabled = false;
  if (gui_button(MAKE_STR("Click Secret"), params)) {
    other_enabled = !other_enabled;
    printf("Reset\n");
  }

  if (other_enabled) {
    if (gui_button(MAKE_STR("WOW"), params)) {
      printf("WOW\n");
    }
    //gs->request_reload = true;
  }
  gui_end();
#endif

}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

