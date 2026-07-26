//#define INPUT_IMPLEMENTATION
#include "core/input.h"
#include "base/base_inc.h"

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
  hero->dynamic = true;
  hero->box = (Phys_Box) {
    .pos = HMM_V3(1,4,1),
    .off = HMM_V3(0,0,0),
    .hdim = HMM_V3(0.5,0.5,0.5),
  };
  hero->col = v4m(0.9,0.4,0.3,1.0);
  
  // Make a test
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      Entity *test = entity_store_add();
      test->dynamic = false;
      test->box = (Phys_Box) {
        .pos = HMM_V3(width,height,1),
        .off = HMM_V3(0,0,0),
        .hdim = HMM_V3(0.5,0.5,0.5),
      };
      test->col = v4m(0.2,0.4,0.9,1.0);
    }
  }

  Entity *ground = entity_store_add();
  ground->dynamic = false;
  ground->box = (Phys_Box) {
    .pos = HMM_V3(0,-1.01,0),
    .off = HMM_V3(0,0,0),
    .hdim = HMM_V3(4,1,4),
  };
  ground->col = v4m(0.4,0.4,0.4,1.0);

  gs->view = HMM_LookAt_RH(HMM_V3(0,8,10), HMM_V3(0,0,0), HMM_V3(0,1,0));

  gui_init(gs->frame_arena, &gs->font, &gs->input);
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
      HMM_Mat4 model = HMM_Translate(HMM_V3(x_coord, 0, z_coord));
      HMM_Mat4 mvp = HMM_Mul(HMM_Mul(gs->proj, gs->view), model);

      rn_imm_verts(gs->game_viewport, quad_verts, array_count(quad_verts), OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp);
    }
  } 

  HMM_Mat4 mv = HMM_Mul(gs->proj, gs->view);

  // X-axis red highlight
  Tri_Vertex line_x[4] = {
    (Tri_Vertex) {.pos = v3m(-cell_count/2,0,0), .color = c2}, 
    (Tri_Vertex) {.pos = v3m(cell_count/2,0,0), .color = c2}, 
  };
  rn_imm_verts(gs->game_viewport, line_x, array_count(line_x), OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mv);

  // Z-axis green highlight
  Tri_Vertex line_z[4] = {
    (Tri_Vertex) {.pos = v3m(0,0,-cell_count/2), .color = c3}, 
    (Tri_Vertex) {.pos = v3m(0,0,cell_count/2), .color = c3}, 
  };
  rn_imm_verts(gs->game_viewport, line_z, array_count(line_z), OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mv);

}

void game_render(Game_State *gs, float dt) {

  //m4 model = m4_mult(m4_translate(v3m(0,0,0)), m4_rotate(gs->time_sec*3.14, v3m(0,1,0)));


  // 0. Draw grid
  game_draw_origin_grid(gs, 10);

#if 0
  HMM_Mat4 mvp = HMM_Mul(HMM_Mul(gs->proj, gs->view), HMM_QToM4(rot_q));
  // 3. Draw a spinning quad
  HMM_Quat rot_q = HMM_QFromAxisAngle_RH(HMM_V3(0,1,0), HMM_PI * gs->time_sec);
  Tri_Vertex my_verts[] = {
    (Tri_Vertex) {.pos = v3m(-1,+1,0), .color = v4m(1,0,1,0.9)}, 
    (Tri_Vertex) {.pos = v3m(-1,-1,0), .color = v4m(1,1,0,0.9)}, 
    (Tri_Vertex) {.pos = v3m(+1,-1,0), .color = v4m(0,1,1,0.9)}, 
    (Tri_Vertex) {.pos = v3m(+1,+1,0), .color = v4m(0,0,1,0.9)}, 
  };
  rn_imm_verts(gs->game_viewport, my_verts, array_count(my_verts), OGL_PRIM_TYPE_TRIANGLE_FAN, (m4*)&mvp);
#endif
  entity_store_update_render(gs, dt);

  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport, dt);
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);

#if 1
  // Small GUI test (mainly for Box reuse test right now)
  gui_set_next_pref_height((Gui_Size) {GUI_SIZEKIND_SUM_OF_CHILDREN, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size) {GUI_SIZEKIND_PERCENT_OF_PARENT, 0.65, 1.0});
  gui_set_next_bg_color(v4m(0.3,0.3,0.9,0.85));
  Gui_Signal pane = gui_pane(STR8L("Pane"));
  // HACK
  pane.box->flags |= GUI_BOX_FLAG_ALLOW_OVERFLOW_X;
  pane.box->flags |= GUI_BOX_FLAG_ALLOW_OVERFLOW_Y;
  gui_push_parent(pane.box);

  gui_push_pref_width((Gui_Size) {GUI_SIZEKIND_TEXT_CONTENT, 1.0, 1.0});
  gui_push_pref_height((Gui_Size) {GUI_SIZEKIND_TEXT_CONTENT, 1.0, 1.0});
  gui_set_next_bg_color(v4m(0.7,0.5,0.3,0.85));
  static bool other_enabled = false;
  if (gui_button(STR8L("Secret")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
    other_enabled = !other_enabled;
    printf("Reset\n");
  }

  gui_push_pref_width((Gui_Size) {GUI_SIZEKIND_TEXT_CONTENT, 1.0, 1.0});
  gui_push_pref_height((Gui_Size) {GUI_SIZEKIND_TEXT_CONTENT, 1.0, 0.0});
  gui_set_next_bg_color(v4m(0.1,0.9,0.1,0.85));
  gui_button(STR8L("WOWO"));

  gui_spacer((Gui_Size) {GUI_SIZEKIND_PIXELS, 4, 0.0});

  gui_set_next_bg_color(v4m(0.1,0.1,0.3,0.35));
  gui_set_next_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
  gui_set_next_pref_height((Gui_Size) {GUI_SIZEKIND_PIXELS, 200, 0.0});
  gui_set_next_pref_width((Gui_Size) {GUI_SIZEKIND_TEXT_CONTENT, 5.0, 0.0});
  if (other_enabled) {
    if (gui_button(STR8L("WOWO##2")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
      printf("WOWO##2\n");
    }
    //gs->request_reload = true;
  }

  gui_pop_parent(); // Pane
#endif

  gui_end();
#endif

}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

