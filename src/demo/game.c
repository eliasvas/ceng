//#define INPUT_IMPLEMENTATION
#include "core/input.h"
#include "base/base_inc.h"

#include "game.h"
#include "gui/gui.h"
#include "entity.h"

#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"

extern void platform_play_sound(const char *sound);

void game_init(Game_State *gs) {
  entity_store_init();

  // Make the hero
  Entity *hero = entity_store_add();
  hero->kind = ENTITY_KIND_HERO;
  hero->dynamic = true;
  hero->box = (Phys_Box) {
    .pos = HMM_V3(1,4,1),
    .col_off = HMM_V3(0,0,0),
    .hdim = HMM_V3(0.3, 0.5, 0.3),
    .col_hdim = HMM_V3(0.5,0.5,0.5),
  };
  hero->col = v4m(0.9,0.4,0.3,1.0);
  
  // Make a test
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      Entity *test = entity_store_add();
      test->dynamic = false;
      test->box = (Phys_Box) {
        .pos = HMM_V3(width,height,1),
        .col_off = HMM_V3(0,0,0),
        .col_hdim = HMM_V3(0.5,0.5,0.5),
        .hdim = HMM_V3(0.5,0.5,0.5),
      };
      test->col = v4m(0.2,0.4,0.9,1.0);
    }
  }

  Entity *ground = entity_store_add();
  ground->dynamic = false;
  ground->box = (Phys_Box) {
    .pos = HMM_V3(0,-1.01,0),
    .col_off = HMM_V3(0,0,0),
    .col_hdim = HMM_V3(4,1,4),
    .hdim = HMM_V3(4,1,4),
  };
  ground->col = v4m(0.4,0.4,0.4,1.0);

  gs->view = HMM_LookAt_RH(HMM_V3(0,8,10), HMM_V3(0,0,0), HMM_V3(0,1,0));

  gui_init(gs->frame_arena, &gs->font, &gs->input);

  Json_Element *root = json_parse(gs->frame_arena, test_str);
  Json_Element* elem = json_lookup(root, MAKE_STR("msg-bools"));
  assert(elem);

}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
  gs->proj = HMM_Perspective_RH_NO(45, gs->game_viewport.w/gs->game_viewport.h, 0.1, 100);


  // Camera stuff (This is wrong because we post-multiply.. FIXME)
  v2 mouse_delta = input_get_mouse_delta(&gs->input);
  if (input_mkey_down(&gs->input, INPUT_MOUSE_RMB)) { 
    HMM_Quat rot_q = HMM_QFromAxisAngle_RH(HMM_V3(0,1,0), mouse_delta.x*dt);
    gs->view = HMM_Mul(gs->view, HMM_QToM4(rot_q));
  }
  if (input_mkey_down(&gs->input, INPUT_MOUSE_RMB)) { 
    HMM_Quat rot_q = HMM_QFromAxisAngle_RH(HMM_V3(1,0,0), mouse_delta.y*dt);
    gs->view = HMM_Mul(gs->view, HMM_QToM4(rot_q));
  }
}

void game_draw_origin_grid(Game_State *gs, s32 cell_count) {
  s32 line_count_per_axis = cell_count + 1; 
  Tri_Vertex *points = arena_push_array(gs->frame_arena, Tri_Vertex, line_count_per_axis*4);
  color c1 = v4m(0.7,0.7,0.7,1);

  HMM_Mat4 model = HMM_Translate(HMM_V3(0, 0, 0));
  HMM_Mat4 mvp = HMM_Mul(HMM_Mul(gs->proj, gs->view), model);

  s32 point_idx = 0;

#if 1
  for (s32 line_x = 0; line_x < line_count_per_axis; line_x +=1) {
    v3 start = v3m(-cell_count/2.0,0, line_x - cell_count/2.0);
    v3 end   = v3m(+cell_count/2.0,0, line_x - cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }
#endif

  for (s32 line_z = 0; line_z < line_count_per_axis; line_z +=1) {
    v3 start = v3m(line_z - cell_count/2.0, 0,-cell_count/2.0);
    v3 end   = v3m(line_z - cell_count/2.0, 0,+cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }

  assert(point_idx == line_count_per_axis*4);
  rn_imm_verts(gs->game_viewport, points, line_count_per_axis * 4, OGL_PRIM_TYPE_LINE, (m4*)&mvp);

}

void game_render(Game_State *gs, float dt) {
  //m4 model = m4_mult(m4_translate(v3m(0,0,0)), m4_rotate(gs->time_sec*3.14, v3m(0,1,0)));

  // 0. Draw grid
  game_draw_origin_grid(gs, 10);
  entity_store_update_render(gs, dt);

  // Simple test for quad rendernig
#if 0
  // TODO: Maybe we should push/pop asset ids for textures??
  // Sample quad for renderer architecture
  R_Quad q = (R_Quad){
    .dst_rect = rec(300,200,500,500),
    .src_rect = rec(0,0,128,80),
    .c = col(1.0,1.0,1.0,1.0),
    .rot_deg = 9.0 * gs->time_sec,
    .corner_radius = 20.0,
    .softness = 4.0,
    .tex = (Ogl_Tex*)am_get(asset_id_from_path(STR8L("white.png"))),
  };
  r2d_push_quad(r2d_pass_front(), q);

  // For test
  //r2d_flush_all();

  q = (R_Quad){
    .dst_rect = rec(500,200,500,500),
    .src_rect = rec(0,0,128,80),
    .c = col(1.0,1.0,1.0,1.0),
    .rot_deg = 19.0 * gs->time_sec,
    .corner_radius = 20.0,
    .softness = 4.0,
    .tex = (Ogl_Tex*)am_get(asset_id_from_path(STR8L("atlas.png"))),
  };
  r2d_push_quad(r2d_pass_front(), q);

  //r2d_flush_all();

#endif


  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport, dt);
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);

  static Gui_Scroll_Data sdata = {
    .item_px = 60, // FIXME change this to 60 to see some weird stuff..
    .item_count = 8,
    .scroll_bar_px = 15,
    .scroll_button_px = 15,
    .scroll_button_color = col(0.5,1,0.4,1),
    .scroll_speed = 1,
    .scroll_percent = 0,
  };

  Gui_Signal scroll_list = gui_scroll_list_begin(STR8L("MyScrollTest"), GUI_AXIS_Y, &sdata);
  assert(scroll_list.box);
  //gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PIXELS, 100.0, 0.0});
  gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 0.0});
    if (gui_button(STR8L("AAAA")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) printf("AAAA\n");
    gui_button(STR8L("BBBB"));
    gui_button(STR8L("CCCC"));
    gui_button(STR8L("DDDD"));
    gui_button(STR8L("EEEE"));
    gui_button(STR8L("FFFF"));
    gui_button(STR8L("GGGG"));
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

