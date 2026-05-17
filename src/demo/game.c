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
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);

  //if (input_win_resized(&gs->input)) { printf("Screen resize!\n"); }

  frz_begin_frame(gs->pixels, gs->wdim, gs->frame_arena);
  frz_clear();
  v2 mp = input_get_mouse_pos(&gs->input);
  mp.y = gs->wdim.y - mp.y;
  frz_imm_line(v2m(0,0), mp, col(1,1,1,1));


  v2 midpoint = v2_multf(gs->wdim, 0.5);
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

  for (u32 cube_idx_triplet= 0; cube_idx_triplet < array_count(frz_cube_indices); cube_idx_triplet+=3) {
    FRZ_Vertex *vt0 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+0]];
    FRZ_Vertex *vt1 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+1]];
    FRZ_Vertex *vt2 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+2]];

    f32 near_plane = 0.1;

    v3 cam_pos = v3m(0,0,5);
    f32 r = 1;
    f32 l = -1;
    f32 t = 1;
    f32 b = -1;

    v3 v0_world  = v3_multf(v3_rot_y(vt0->pos, rot), 5); 
    v3 v0_cam    = v3_sub(v0_world, cam_pos);
    v2 v0_ss     = v2_divf(v2_multf(v2m(v0_cam.x, v0_cam.y), near_plane), -v0_cam.z);
    v2 v0_ndc    = v2m(2 * v0_ss.x / (r-l) - (r+l) / (r-l), 2 * v0_ss.y / (t - b) - (t + b) / (t - b));
    v2 v0_raster = v2m(((v0_ndc.x+1) / 2) * gs->wdim.x, ((1 - v0_ndc.y)/2)*gs->wdim.y); 


    v3 v1_world  = v3_multf(v3_rot_y(vt1->pos, rot), 5); 
    v3 v1_cam    = v3_sub(v1_world, cam_pos);
    v2 v1_ss     = v2_divf(v2_multf(v2m(v1_cam.x, v1_cam.y), near_plane), -v1_cam.z);
    v2 v1_ndc    = v2m(2 * v1_ss.x / (r-l) - (r+l) / (r-l), 2 * v1_ss.y / (t - b) - (t + b) / (t - b));
    v2 v1_raster = v2m(((v1_ndc.x+1) / 2) * gs->wdim.x, ((1 - v1_ndc.y)/2)*gs->wdim.y); 

    v3 v2_world  = v3_multf(v3_rot_y(vt2->pos, rot), 5); 
    v3 v2_cam    = v3_sub(v2_world, cam_pos);
    v2 v2_ss     = v2_divf(v2_multf(v2m(v2_cam.x, v2_cam.y), near_plane), -v2_cam.z);
    v2 v2_ndc    = v2m(2 * v2_ss.x / (r-l) - (r+l) / (r-l), 2 * v2_ss.y / (t - b) - (t + b) / (t - b));
    v2 v2_raster = v2m(((v2_ndc.x+1) / 2) * gs->wdim.x, ((1 - v2_ndc.y)/2)*gs->wdim.y); 


#if 0
    v3 wv0 = v3_multf(v3_rot_y(vt0->pos, rot), 0.5); // rotate and scale in 'ws'
    v2 v0_ss = v2_mult(v2_add(v2m(wv0.x,wv0.y), v2m(0.5,0.5)), gs->wdim); // apply a viewport transform, sort of

    v3 wv1 = v3_multf(v3_rot_y(vt1->pos, rot), 0.5); // rotate and scale in 'ws'
    v2 v1_ss = v2_mult(v2_add(v2m(wv1.x,wv1.y), v2m(0.5,0.5)), gs->wdim); // apply a viewport transform, sort of

    v3 wv2 = v3_multf(v3_rot_y(vt2->pos, rot), 0.5); // rotate and scale in 'ws'
    v2 v2_ss = v2_mult(v2_add(v2m(wv2.x,wv2.y), v2m(0.5,0.5)), gs->wdim); // apply a viewport transform, sort of
#endif

    //------
    frz_imm_tri_bbox(v0_raster, v1_raster, v2_raster, vt0->uv, vt1->uv, vt2->uv, vt0->color, vt1->color, vt2->color);
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

  v2 screen_mp = v2_multf(gs->wdim, 0.5);
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

  gui_frame_begin(gs->wdim, &gs->input, &gs->cmd_list, dt);

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

