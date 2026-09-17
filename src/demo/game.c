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
  Entity *hero = setup_hero(world_add(gs->world), v3m(1,4,1));
  assert(hero);

  // Make the pillars
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      setup_wall(world_add(gs->world), v3m(width,height,1));
#if 0
      Particle_Emitter *emitter = particle_mgr_new_emitter(gs->pmgr);
      emitter->pos = v3m(width, height, 1);
      emitter->sec_per_particle = 0.1;
#endif
    }
  }

  // Make the ground
  Entity *ground = setup_wall(world_add(gs->world), v3m(0,-1.01,0));
  ground->box.col_hdim = v3m(4,1,4),
  ground->box.hdim = v3m(4,1,4),
  ground->col = v4m(0.4,0.4,0.4,1.0);

  gui_init(gs->frame_arena, &gs->font, &gs->input);

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

  // Make a test coin if none exists
  if (world_count_entities(gs->world, ENTITY_KIND_COIN) == 0) {
    Entity *test_coin = setup_coin(
        world_add(gs->world), v3m(4*brand_f01()-2.0,0.5,4*brand_f01()-2.0)
    );
    assert(test_coin);
  }

}

// FIXME: Make a VBO for this goddam it, or.. something
void game_draw_origin_grid(struct Game_State *gs, s32 cell_count) {
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

void game_render(struct Game_State *gs, float dt) {
  v3 cam_pos = v3m(0,8,10);
  gs->view = m4_look_at(cam_pos, v3m(0,0,0), v3m(0,1,0));
  gs->proj = m4_persp(45, gs->game_viewport.w/gs->game_viewport.h, 0.1, 100);
  // 0. Draw grid
  game_draw_origin_grid(gs, 10);
  // Draw the test model
  m4 vp = m4_mult(gs->proj, gs->view);

  m4 static_model_matrix = m4_mult(m4_translate(v3m(0,0,0)), m4_scale(v3m(0.2,0.2,0.2)));
  Model_Info *static_model = AM_GET(gs->static_model_asset_id, model);
  r3d_imm_model(gs->game_viewport, static_model, vp, static_model_matrix, cam_pos, gs->time_sec);

  m4 anim_model_matrix = m4_mult(
      m4_translate(v3m(1,0,0)),
      m4_mult(
        //m4_from_quat(quat_from_axis_angle(v3m(1,0,0), -M_PI/2)), 
        //m4_scale(v3m(2,2,2))
        m4_from_quat(quat_from_axis_angle(v3m(1,0,0), 0)), 
        m4_scale(v3m(0.05,0.05,0.05))
      )
  );
  Model_Info *anim_model = AM_GET(gs->anim_model_asset_id, model);
  r3d_imm_model(gs->game_viewport, anim_model, vp, anim_model_matrix, cam_pos, gs->time_sec);

  // Do world picking..
  b32 lmb_pressed = input_mkey_pressed(&gs->input, INPUT_MOUSE_LMB);
  if (lmb_pressed) {
    m4 inv_vp = m4_inv(vp);
    v2 mp = input_get_mouse_pos(&gs->input);

    v3 near = world_from_screen(v3m(mp.x, mp.y, -1.0), v2m(800,600), inv_vp);
    v3 far = world_from_screen(v3m(mp.x, mp.y, 1.0), v2m(800,600), inv_vp);
    v3 dir = v3_norm(v3_sub(far, near));

    ray r = (ray) {
      .orig = cam_pos,
      .dir = dir,
      .t = 0,
    };

    Entity *e = world_pick_entity(gs->world, r);
    if (e) {
      e->col = v4m(brand_frange(0,1),brand_range(0,1),brand_range(0,1),1);
    }
  }


  // Rest of the frame
  world_update_render(gs, dt);
  // Draw the entity render commands.. TODO: Add asset_ids to be possible, also make cube default mesh right? or make cube/col explicit
  for (s32 i = 0; i < gs->world->rcommand_count; i+=1) {
    Entity_Render_Command *cmd = &gs->world->rcommands[i];
    m4 world = m4_from_transform(cmd->xform);
    m4 mvp = m4_mult(vp, world);
    r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, cmd->col);
    // TODO: Render the collider as well.. We need more stuff in Entity_Render_Command
    m4 world_collider = m4_from_transform(cmd->collider_xform);
    m4 cmvp = m4_mult(vp, world_collider);
    r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&cmvp, cmd->collider_col);
  }

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
    .scroll_button_color = col(0.5,1,0.4,1),
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

