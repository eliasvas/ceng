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


  // Movement stuff
  static v3 cam_pos = v3m(0,0,6);
  f32 cam_speed = 3.0;

  if (input_mkey_down(&gs->input, INPUT_MOUSE_RMB)) {
    cam_pos.x += dt*cam_speed;
    //printf("campos: %f %f %f\n", cam_pos.x, cam_pos.y, cam_pos.z);
  }

  if (input_mkey_down(&gs->input, INPUT_MOUSE_LMB)) {
    cam_pos.x -= dt*cam_speed;
    //printf("campos: %f %f %f\n", cam_pos.x, cam_pos.y, cam_pos.z);
  }

  f32 rot = gs->time_sec*2.0;
  for (u32 cube_idx_triplet= 0; cube_idx_triplet < array_count(frz_cube_indices); cube_idx_triplet+=3) {
    FRZ_Vertex *vt0 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+0]];
    FRZ_Vertex *vt1 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+1]];
    FRZ_Vertex *vt2 = &frz_cube_verts[frz_cube_indices[cube_idx_triplet+2]];

    m4 clip_from_view  = m4_persp(45, gs->wdim.x/(f32)gs->wdim.y, 0.1, 100);
    m4 view_from_world = m4_view(v3m(0,0,6), v3m(0,0,0), v3m(0,1,0));

    v3 v0_world  = v3_multf(v3_rot_y(vt0->pos, rot), 2); 
    v4 v0_view   = m4_multv(view_from_world, v4_from_v3(v0_world, 1.0));
    v4 v0_clip   = m4_multv(clip_from_view, v0_view);
    v4 v0_ndc    = v4_divf(v0_clip, v0_clip.w);
    v4 v0_screen = frz_apply_viewport_transform(v0_ndc, gs->wdim);

    v3 v1_world  = v3_multf(v3_rot_y(vt1->pos, rot), 2); 
    v4 v1_view   = m4_multv(view_from_world, v4_from_v3(v1_world, 1.0));
    v4 v1_clip   = m4_multv(clip_from_view, v1_view);
    v4 v1_ndc    = v4_divf(v1_clip, v1_clip.w);
    v4 v1_screen = frz_apply_viewport_transform(v1_ndc, gs->wdim);

    v3 v2_world  = v3_multf(v3_rot_y(vt2->pos, rot), 2); 
    v4 v2_view   = m4_multv(view_from_world, v4_from_v3(v2_world, 1.0));
    v4 v2_clip   = m4_multv(clip_from_view, v2_view);
    v4 v2_ndc    = v4_divf(v2_clip, v2_clip.w);
    v4 v2_screen = frz_apply_viewport_transform(v2_ndc, gs->wdim);

    //------
    frz_imm_tri_bbox(v0_screen, v1_screen, v2_screen, vt0->uv, vt1->uv, vt2->uv, vt0->color, vt1->color, vt2->color);
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

