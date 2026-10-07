//#define INPUT_IMPLEMENTATION
#include "core/input.h"
#include "base/base_inc.h"

#include "game.h"
#include "world.h"
#include "gui/gui.h"
#include "gui_extra.h"
#include "world_serializer.h"

#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"

extern f32 _blend_factor;

extern void platform_play_sound(const char *sound);

void game_init(struct Game_State *gs) {
  gs->world = arena_push_array(gs->persistent_arena, World, 1);
  world_init(gs->world);

  // Make the hero
  Entity *hero = setup_hero(
      world_add(gs->world), 
      transform_make(
        v3m(0,2,1), 
        quat_from_axis_angle((axis_angle){v3m(0,1,0), 0.1}), 
        v3m(1,1,1)
      )
  );
  //hero->tint = CLR_RED_RICH;
  hero->has_asset = true;
  hero->asset_id = gs->hero_model_asset_id;
  hero->anim_idx = 1;
  gs->hero_id = hero->id;

  assert(hero);

#if 1
  // Make the hero WEAPONS
  transform lgun_xform = transform_make(v3m(0.2, 0.25,-0.3), QUAT_ONE, v3m(0.1,0.1,0.1));
  Entity *hero_lgun = setup_none(world_add(gs->world), lgun_xform);
  hero_lgun->box = (Phys_Box) {
    // Dont thino we need this MOFO
    .col_off = v3m(10000000,0,0),
    .col_hdim = v3m(0.1,0.1,0.1),
  };
  hero_lgun->tint = CLR_WHITE;
  assert(hero_lgun);
  // Add hero head to hero
  entity_add_child(hero, hero_lgun);

  transform rgun_xform = transform_make(v3m(-0.2, 0.25,-0.3), QUAT_ONE, v3m(0.1,0.1,0.1));
  Entity *hero_rgun = setup_none(world_add(gs->world), rgun_xform);
  hero_rgun->box = (Phys_Box) {
    // Dont thino we need this MOFO
    .col_off = v3m(1000000,0,0),
    .col_hdim = v3m(0.1,0.1,0.1),
  };
  hero_rgun->tint = CLR_WHITE;
  assert(hero_rgun);
  // Add hero head to hero
  entity_add_child(hero, hero_rgun);
#endif

#if 1 
  // Make the ground
  Entity *ground = setup_wall(world_add(gs->world), transform_make(v3m(0,-1.01,0), QUAT_ONE, v3m(8,2,8)));
  ground->tint = CLR_BLUE_DAMSELFLY;

  // Make the pillars
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      transform pillar_xform = transform_make(v3m(width, height+0.5, 1), QUAT_ONE, v3_one);
      Entity *pillar = setup_wall(world_add(gs->world), pillar_xform);
      pillar->tint = CLR_BLUE_HIPPIE;
    }
  }
#endif

  gui_init(gs->frame_arena, gs->def_font_id, &gs->input);

#if 0 
  // Base64 test.. no reason
  base_64_test(gs->frame_arena);
#endif

#if 0
  // str8 test..
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
#endif

  // serializer test
  //wserializer_test(gs->persistent_arena);
}

void game_update(struct Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
  m4 vp = m4_mult(gs->proj, gs->view);


  // Push score over da hero

  Entity *hero = world_get_entity(gs->world, gs->hero_id);
  v3 hero_pos = hero->world.t;
  f32 font_scale = 1.5;
  str8 score_str = str8_sprintf(gs->frame_arena, "score: %ld", gs->score);
  Font_Info *fi = AM_GET(gs->def_font_id, font);
  f32 tw = bfont_measure_text_width(fi, score_str, font_scale);
  f32 th = bfont_measure_text_height(fi, score_str, font_scale);
  v2 hero_screen_pos = v2_sub(screen_from_world(v3_add(hero_pos,
          v3m(0,3.0*hero->box.col_hdim.y, 0)), gs->wdim, vp), v2m(tw/2.0, th/2.0)); 
  r2d_push_text(r2d_pass_front(), gs->def_font_id, gs->game_viewport, 
      gs->game_viewport, score_str, hero_screen_pos, font_scale, CLR_WHITE);

  // Make a test coin if none exists
  if (world_count_entities(gs->world, ENTITY_KIND_COIN) == 0) {
    gs->score += 1;
    transform coin_xform = transform_make(v3m(4*brand_f01()-2.0,0.0,4*brand_f01()-2.0), QUAT_ONE, v3_multf(v3_one, 1));
    Entity *test_coin = setup_coin(world_add(gs->world), coin_xform);

    test_coin->has_asset = true;
    test_coin->asset_id = gs->coin_model_asset_id;
    assert(test_coin);
  }

}

void game_draw_origin_grid(struct Game_State *gs, s32 cell_count) {
  s32 line_count_per_axis = cell_count + 1; 
  Tri_Vertex *points = arena_push_array(gs->frame_arena, Tri_Vertex, line_count_per_axis*4);
  color c1 = v4m(0.7,0.7,0.7,1);

  m4 model = m4_translate(v3m(0, 0, 0));

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

  // TODO: could move this to draw pass, or have draw_origin accept an rctx?
  R3D_Ctx* grid_pass = r3dc_begin(gs->frame_arena, gs->game_viewport, gs->view, gs->proj, gs->cam_pos, v3_zero, 0);
  r3dc_imm_verts(grid_pass, points, line_count_per_axis * 4, OGL_PRIM_TYPE_LINE, model);
  r3dc_end(grid_pass);
}

void game_render(struct Game_State *gs, float dt) {
  v3 cam_pos = v3m(0,8,10);
  gs->cam_pos = cam_pos;
  gs->view = m4_look_at(cam_pos, v3m(0,0,0), v3m(0,1,0));
  gs->proj = m4_persp(45, gs->game_viewport.w/gs->game_viewport.h, 0.1, 100);

  // 0. Draw grid
  game_draw_origin_grid(gs, 10);
  // Draw the test model
  m4 vp = m4_mult(gs->proj, gs->view);


#if 0
  // Draw the lantern (for testing)
  m4 lantern_model = m4_mult(m4_translate(v3m(0,0,0)), m4_scale(v3m(0.2,0.2,0.2)));
  Model_Info *lantern = AM_GET(gs->lantern_model_asset_id, model);
  r3d_imm_model(gs->game_viewport, lantern, vp, lantern_model, cam_pos, gs->time_sec, 0, CLR_WHITE);

  // Draw the fox (for testing)
  m4 fox_model= m4_mult(
      m4_translate(v3m(1,0,0)),
      m4_mult(
        m4_from_quat(quat_from_axis_angle((axis_angle){v3m(1,0,0), 0})), 
        m4_scale(v3m(0.05,0.05,0.05))
      )
  );
  Model_Info *fox = AM_GET(gs->fox_model_asset_id, model);
  r3d_imm_model(gs->game_viewport, fox, vp, fox_model, cam_pos, gs->time_sec, 0, CLR_WHITE);



  m4 hero_model_matrix = m4_scale(v3m(3,3,3));
  Model_Info *hero_model = AM_GET(gs->hero_model_asset_id, model);
  r3d_imm_model(gs->game_viewport, hero_model, vp, hero_model_matrix, cam_pos, gs->time_sec, 1, CLR_WHITE);
#endif

  // Do world picking..
  b32 lmb_pressed = input_mkey_pressed(&gs->input, INPUT_MOUSE_LMB);
  if (lmb_pressed) {
    m4 inv_vp = m4_inv(vp);
    v2 mp = input_get_mouse_pos(&gs->input);

    v3 near = world_from_screen(v3m(mp.x, mp.y, -1.0), v2m(gs->wdim.x,gs->wdim.y), inv_vp);
    v3 far = world_from_screen(v3m(mp.x, mp.y, 1.0), v2m(gs->wdim.x,gs->wdim.y), inv_vp);
    v3 dir = v3_norm(v3_sub(far, near));

    ray r = (ray) {
      .orig = cam_pos,
      .dir = dir,
      .t = 0,
    };

    Entity *e = world_pick_entity(gs->world, r);
    if (e) {
      e->tint = v4m(brand_frange(0,1),brand_range(0,1),brand_range(0,1),1);
    }
  }


  // Rest of the frame
  world_update_render(gs, dt);
  // Draw the entity render commands.. TODO: Add asset_ids to be possible, also make cube default mesh right? or make cube/col explicit

  v3 light_dir = v3_norm(v3m(0.2,1,-1));


  ///////////////////////
  // SHADOW PASS
  ///////////////////////
  R3D_Ctx* shadow_pass = r3dc_begin(gs->frame_arena, gs->game_viewport, m4d(1.0), m4d(1.0), 
      gs->cam_pos, light_dir, R3D_FLAG_IS_DEPTH_PASS | R3D_FLAG_CLEAR_ALL);
  for (s32 i = 0; i < gs->world->rcommand_count; i+=1) {
    Entity_Render_Command *cmd = &gs->world->rcommands[i];
    m4 world = m4_from_transform(cmd->xform);
    //m4 mvp = m4_mult(vp, world);
    if (cmd->has_asset) {
      Model_Info *model = AM_GET(cmd->asset_id, model);
      //m4 local_matrix = m4_mult(m4_scale(v3m(1, 1, 1)),m4_translate(v3m(0,-0.5,0)));
      m4 local_matrix = m4d(1.0); 
      world = m4_mult(world, local_matrix);
      r3dc_imm_model(shadow_pass, model, world, gs->time_sec, cmd->anim_idx, cmd->tint);
    } else {
      r3dc_imm_cube(shadow_pass, OGL_PRIM_TYPE_TRIANGLE, cmd->tint, world);
    }
  }
  r3dc_end(shadow_pass);

  ///////////////////////
  // LIGHTPASS
  ///////////////////////
  R3D_Ctx* light_pass = r3dc_begin(gs->frame_arena, gs->game_viewport, gs->view, gs->proj, 
      gs->cam_pos, light_dir, 0);
  for (s32 i = 0; i < gs->world->rcommand_count; i+=1) {
    Entity_Render_Command *cmd = &gs->world->rcommands[i];
    m4 world = m4_from_transform(cmd->xform);
    //m4 mvp = m4_mult(vp, world);
    if (cmd->has_asset) {
      Model_Info *model = AM_GET(cmd->asset_id, model);
      //m4 local_matrix = m4_mult(m4_scale(v3m(1, 1, 1)),m4_translate(v3m(0,-0.5,0)));
      m4 local_matrix = m4d(1.0); 
      world = m4_mult(world, local_matrix);
      r3dc_imm_model(light_pass, model, world, gs->time_sec, cmd->anim_idx, cmd->tint);
    } else {
      r3dc_imm_cube(light_pass, OGL_PRIM_TYPE_TRIANGLE, cmd->tint, world);
    }

    // TODO: Render the collider as well.. We need more stuff in Entity_Render_Command
    m4 world_collider = m4_from_transform(cmd->collider_xform);
    r3dc_imm_cube(light_pass, OGL_PRIM_TYPE_LINE_LOOP, cmd->collider_col, world_collider);
  }
  r3dc_end(light_pass);

  ///////////////////////
  // BVH PASS 
  ///////////////////////
  R3D_Ctx* bvh_pass = r3dc_begin(gs->frame_arena, gs->game_viewport, gs->view, gs->proj, 
      gs->cam_pos, light_dir, 0);
  // BVH vis
  BVH_Render_Config bvh_rc = {
    .colors = {
      [0] = v4_multf(CLR_GREEN_EVA, 0.5),
      [1] = v4_multf(CLR_RED_PINK, 0.5),
      [2] = v4_multf(CLR_PURPLE_C64, 0.5),
      [3] = v4_multf(CLR_BLUE_HIPPIE, 0.5),
      [4] = v4_multf(CLR_GREEN_CLASSIC, 0.5),
      [5] = v4_multf(CLR_BLUE_DAMSELFLY, 0.5),
      [6] = v4_multf(CLR_RED_RICH, 0.5),
      [7] = v4_multf(CLR_PURPLE_RAIN, 0.5),
    },
    .running_time_sec = gs->time_sec,
    // FIXME: currently the jetpack things confuse the BVH!!! 
#if 1
    .kind = BVH_RENDER_OFF,
#else
    .seconds_per_level = 0.5,
    .kind = BVH_RENDER_LEVEL_BY_LEVEL,
#endif
  };
  BVH_Node *root = gs->world->bvh_root;
  assert(root);
  world_render_bvh(bvh_pass, gs->world, gs->world->bvh_root, vp, gs->game_viewport, bvh_rc);
  r3dc_end(bvh_pass);


  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport, dt);
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);

  static Gui_Scroll_Data sdata = {
    .item_px = 30, // FIXME change this to 60 to see some weird stuff..
    .item_count = 7,
    .scroll_bar_px = 15,
    .scroll_button_px = 15,
    .scroll_button_color = clr(0.5,1,0.4,1),
    .scroll_speed = 1,
    .scroll_percent = 0,
  };

  Gui_Signal scroll_list = gui_scroll_list_begin(STR8L("MyScrollTest"), GUI_AXIS_Y, &sdata);
  assert(scroll_list.box);
  gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PIXELS, 400.0, 1.0});

    // Animation blending factor (for testing currently)
    gui_slider01(STR8L("blendFactor"), &_blend_factor, sdata.item_px, GUI_AXIS_X);

    // Good to have buttons
    if (gui_button(STR8L("Print")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) printf("AAAA\n");
    if (gui_button(STR8L("Serialize")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) world_serialize(gs);
    if (gui_button(STR8L("Deserialize")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) world_deserialize(gs);
    if (gui_button(STR8L("Reload")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) {
      printf("Reloading game dynamic lib (with init)\n");
      gs->request_reload = true;
    }

    // This is just for frame arena to have enough data to cause a spike in memory..
    f32 *random_yuge_alloc = arena_push_array(gs->frame_arena, char, MB(10)); 
    assert(random_yuge_alloc);

    // Usage percentages for persistent frame and scratch arenas
    f32 pers_pct = arena_get_pct_filled(gs->persistent_arena); 
    str8 pers_pct_label = str8_sprintf(gs->frame_arena, "persistent: %ld%%", (s32)(100.0*pers_pct));
    gui_label(pers_pct_label);
    f32 frame_pct = arena_get_pct_filled(gs->frame_arena); 
    str8 frame_pct_label = str8_sprintf(gs->frame_arena, "frame: %ld%%", (s32)(100.0*frame_pct));
    gui_label(frame_pct_label);
    f32 scratch_pct = arena_get_pct_filled(get_scratch(0,0).arena); 
    str8 scratch_pct_label = str8_sprintf(gs->frame_arena, "scratch: %ld%%", (s32)(100.0*scratch_pct));
    gui_label(scratch_pct_label);

    gui_pop_pref_width();
    gui_scroll_list_end(STR8L("MyScrollTest"));
  gui_end();


#endif

}

void game_shutdown(struct Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

