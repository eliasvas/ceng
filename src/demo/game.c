//#define INPUT_IMPLEMENTATION
#include "core/input.h"
#include "base/base_inc.h"

#include "game.h"
#include "entity.h"
#include "gui/gui.h"

#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"

extern void platform_play_sound(const char *sound);

void game_init(Game_State *gs) {
  gs->entity_store = arena_push_array(gs->persistent_arena, Entity_Store, 1);
  entity_store_init(gs->entity_store);
  gs->pmgr = arena_push_array(gs->persistent_arena, Particle_Mgr, 1);
  particle_mgr_init(gs->pmgr, gs->persistent_arena);

  // Make the hero
  Entity *hero = setup_hero(entity_store_add(gs->entity_store), v3m(1,4,1));
  assert(hero);

  // Make the pillars
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      setup_wall(entity_store_add(gs->entity_store), v3m(width,height,1));
#if 0
      Particle_Emitter *emitter = particle_mgr_new_emitter(gs->pmgr);
      emitter->pos = v3m(width, height, 1);
      emitter->sec_per_particle = 0.1;
#endif
    }
  }

  // Make the ground
  Entity *ground = setup_wall(entity_store_add(gs->entity_store), v3m(0,-1.01,0));
  ground->box.col_hdim = v3m(4,1,4),
  ground->box.hdim = v3m(4,1,4),
  ground->col = v4m(0.4,0.4,0.4,1.0);

  gui_init(gs->frame_arena, &gs->font, &gs->input);

  // Base64 test.. no reason
  base_64_test(gs->frame_arena);

  str8_list list = {};
  str8_list_push_back(gs->persistent_arena, &list, STR8L("One"));
  str8_list_push_back(gs->persistent_arena, &list, STR8L("Two"));
  str8_list_push_back(gs->persistent_arena, &list, STR8L("Three"));
  str8_list_push_back(gs->persistent_arena, &list, STR8L("Four"));
  str8_list_pop_front(&list);
  str8_list_pop_back(&list);
  str8 joined = str8_list_join(gs->persistent_arena, &list);
  assert(str8_eq(joined, STR8L("TwoThree")));
  str8_list_print(&list);
}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
  gs->proj = m4_persp(45, gs->game_viewport.w/gs->game_viewport.h, 0.1, 100);
  particle_mgr_update(gs, gs->pmgr, dt);

  // Make a test coin if none exists
  if (entity_store_count_entities(gs->entity_store, ENTITY_KIND_COIN) == 0) {
    Entity *test_coin = setup_coin(
        entity_store_add(gs->entity_store), v3m(4*brand_f01()-2.0,0.5,4*brand_f01()-2.0)
    );
    assert(test_coin);
  }

}

// FIXME: Make a VBO for this goddam it, or.. something
void game_draw_origin_grid(Game_State *gs, s32 cell_count) {
  s32 line_count_per_axis = cell_count + 1; 
  Tri_Vertex *points = arena_push_array(gs->frame_arena, Tri_Vertex, line_count_per_axis*4);
  color c1 = v4m(0.7,0.7,0.7,1);

  m4 model = m4_translate(v3m(0, 0, 0));
  m4 vp = m4_mult(gs->proj, gs->view);
  m4 mvp = m4_mult(vp, model);

  s32 point_idx = 0;

  for (s32 line_x = 0; line_x < line_count_per_axis; line_x +=1) {
    v3 start = v3m(-cell_count/2.0,0, line_x - cell_count/2.0);
    v3 end   = v3m(+cell_count/2.0,0, line_x - cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }

  for (s32 line_z = 0; line_z < line_count_per_axis; line_z +=1) {
    v3 start = v3m(line_z - cell_count/2.0, 0,-cell_count/2.0);
    v3 end   = v3m(line_z - cell_count/2.0, 0,+cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }

  assert(point_idx == line_count_per_axis*4);
  r3d_imm_verts(gs->game_viewport, points, line_count_per_axis * 4, OGL_PRIM_TYPE_LINE, (m4*)&mvp);
}

void game_render(Game_State *gs, float dt) {
  v3 cam_pos = v3m(0,8,10);
  gs->view = m4_look_at(cam_pos, v3m(0,0,0), v3m(0,1,0));
  // 0. Draw grid
  game_draw_origin_grid(gs, 10);
  // Draw the test model
  m4 vp = m4_mult(gs->proj, gs->view);
  Model_Info *mi = AM_GET(asset_id_from_path(STR8L("Lantern.gltf")), model);
  m4 model_for_mesh = m4_mult(m4_rotate(gs->time_sec * M_PI, v3m(0,1,0)), m4_scale(v3m(0.3,0.3,0.3)));
  //Model_Info *mi = AM_GET(asset_id_from_path(STR8L("Avocado.gltf")), model);
  //m4 model_for_mesh = m4_mult(m4_from_quat(quat_from_axis_angle(v3m(0,1,0), gs->time_sec*3.14)), m4_scale(v3m(50,50,50)));
  r3d_imm_model(gs->game_viewport, mi, vp, model_for_mesh, cam_pos);

  // Rest of the frame
  entity_store_update_render(gs, dt);
  particle_mgr_render(gs, gs->pmgr);

  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport, dt);
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);

  static Gui_Scroll_Data sdata = {
    .item_px = 30, // FIXME change this to 60 to see some weird stuff..
    .item_count = 8,
    .scroll_bar_px = 15,
    .scroll_button_px = 15,
    .scroll_button_color = col(0.5,1,0.4,1),
    .scroll_speed = 1,
    .scroll_percent = 0,
  };

  Gui_Signal scroll_list = gui_scroll_list_begin(STR8L("MyScrollTest"), GUI_AXIS_Y, &sdata);
  assert(scroll_list.box);
  gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PIXELS, 400.0, 1.0});
    if (gui_button(STR8L("AAAA")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) printf("AAAA\n");
    gui_button(STR8L("BBBB"));
    gui_button(STR8L("CCCC"));
    gui_button(STR8L("DDDD"));
    gui_button(STR8L("EEEE"));
    gui_button(STR8L("FFFF"));
    gui_button(STR8L("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"));
    gui_button(STR8L("HHHH"));
    gui_pop_pref_width();
  gui_scroll_list_end(STR8L("MyScrollTest"));
  gui_end();
#endif

}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

