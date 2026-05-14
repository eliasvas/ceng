//#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"
#include "gui/gui.h"
#include "frz/frz.h"

// TODO: Move to a better 2D renderer that can do arbitrary polygons not just AABBS rotated?
// TODO: lookup a good fzf pipeline to be able to search
// TODO: Maybe this should be just a test-bed and have repos reference this?? idk, I dont want many assets inside this repo maybe
// TODO: make-prg for building via build.sh and make an argument to only build, also run, and also just export the object files

void game_init(Game_State *gs) { }

void game_update(Game_State *gs, float dt) {
  static bool gui_initialized = false;
  if (!gui_initialized) {
    gui_context_init(gs->frame_arena, &gs->font);
    gui_initialized = true;
  }
  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);

  //if (input_win_resized(&gs->input)) { printf("Screen resize!\n"); }

  // TODO: screen_dim should become window_dim right?
  frz_begin_frame(gs->pixels, gs->screen_dim);
  frz_clear();
  v2 mp = input_get_mouse_pos(&gs->input);
  mp.y = gs->screen_dim.y - mp.y;
  frz_imm_line(v2m(0,0), mp, col(1,1,1,1));


  v2 midpoint = v2_multf(gs->screen_dim, 0.5);
  v2 tri_dim = v2m(100,100); 
#define POINT_COUNT 3
  v2 points[POINT_COUNT] = {
    v2m(tri_dim.x,-tri_dim.y),
    v2m(-tri_dim.x,-tri_dim.y),
    v2m(0,tri_dim.y),
  };
  f32 rot = gs->time_sec;
  for (s32 pidx = 0; pidx < POINT_COUNT; pidx+=1) {
    points[pidx] = v2_rot(points[pidx], rot);
    points[pidx] = v2_add(points[pidx], midpoint);
  }

#if 0
  frz_imm_line(points[0], points[1], col(1,0,0,1));
  frz_imm_line(points[1], points[2], col(0,1,0,1));
  frz_imm_line(points[2], points[0], col(0,0,1,1));
#endif

  for (u32 cube_idx_triplet= 0; cube_idx_triplet < array_count(frz_cube_indices); cube_idx_triplet+=3) {
    FRZ_Vertex *vt0 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+0]];
    FRZ_Vertex *vt1 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+1]];
    FRZ_Vertex *vt2 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+2]];

    v2 v0_ss = v2_mult(v2_add(v2_divf(v2_rot(v2m(vt0->pos.x, vt0->pos.y), rot), 2), v2m(0.5,0.5)), gs->screen_dim);
    v2 v1_ss = v2_mult(v2_add(v2_divf(v2_rot(v2m(vt1->pos.x, vt1->pos.y), rot), 2), v2m(0.5,0.5)), gs->screen_dim);
    v2 v2_ss = v2_mult(v2_add(v2_divf(v2_rot(v2m(vt2->pos.x, vt2->pos.y), rot), 2), v2m(0.5,0.5)), gs->screen_dim);

    //------
    frz_imm_tri_bbox(v0_ss, v1_ss, v2_ss, vt0->color, vt1->color, vt2->color);
  }

  frz_end_frame();
}


void game_render(Game_State *gs, float dt) {
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);


  f32 scale_factor = mod_f32(gs->time_sec, 1.0);
  scale_factor = ease_in_quad(scale_factor);

  v2 screen_mp = v2_multf(gs->screen_dim, 0.5);
  f32 hero_w = 300;
  hero_w = hero_w * scale_factor;


  R2D_Quad quad = (R2D_Quad) {
      //.src_rect = rec(9*8,0*8,8,8),
      .src_rect = rec(8*8,1*8,8,8),
      .dst_rect = rec(screen_mp.x - hero_w*0.5, screen_mp.y - hero_w*0.5, hero_w, hero_w),
      .c = col(1,1,1,1),
      .tex = gs->atlas,
      .rot_deg = 0,
  };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
  // Uncomment this to see da flame
  //r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);

  // ..
  // ..
  // In the end, perform a UI pass (TBA)
  // Right now: Print debug hero info stuff
  static u32 squish_count = 0;
  if (input_mkey_pressed(&gs->input, INPUT_MOUSE_RMB)) {
    squish_count+=1;
    printf("RMB\n");
    ogl_clear(col(0.5,0.5,0.0,1.0));
  }

  gui_frame_begin(gs->screen_dim, &gs->input, &gs->cmd_list, dt);

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane = gui_pane(MAKE_STR("Main_Pane"));
  gui_push_parent(debug_pane.box);
  {
    gui_set_next_bg_color(col(0.1, 0.2, 0.4, 0.5));
    gui_set_next_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    buf hero_info = arena_sprintf(gs->frame_arena, "Press RMB for a nice squish");
    gui_label(hero_info);
  }
  gui_pop_parent();

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane2 = gui_pane(MAKE_STR("Main_Pane2"));
  {
    gui_push_parent(debug_pane2.box);
    gui_set_next_bg_color(col(0.3, 0.1, 0.7, 0.5));
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    buf high_entity_info = arena_sprintf(gs->frame_arena, "squish count: %d", squish_count);
    gui_label(high_entity_info);

    /*
    buf choices[4] = {
      [0] = arena_sprintf(gs->frame_arena, "ease_in_quad"),
      [1] = arena_sprintf(gs->frame_arena, "ease_out_quad"),
      [2] = arena_sprintf(gs->frame_arena, "ease_in_qubic"),
      [3] = arena_sprintf(gs->frame_arena, "ease_out_qubic"),
    };
    gui_choice_box(buf_make("ease func", 6), choices, 4);
    */
  }
  gui_pop_parent();
  //--------------
  gui_frame_end();
}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

