#include "entity.h"
#include "game.h"

// Forward declaration from World..
Entity *entity_collides(World *world, Entity_ID id, v3 candidate_pos);

////////////////////////////////////////////
// Hero Entity
////////////////////////////////////////////

void update_hero(Game_State *gs, Entity *e, f32 dt) {
  World *world = gs->world;
  // Movement dir
  v3 move_dir = v3m(0,0,0);
  if (input_key_down(&gs->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
  if (input_key_down(&gs->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }
  f32 speed = 5.0;
  e->box.vel.x = move_dir.x * speed;
  e->box.vel.z = move_dir.z * speed;

  // For reload testing
  //e->col = col(1,0,0,1);

  // Dash logic
  if (e->dash_timer <= 0 && input_key_pressed(&gs->input, KEY_SCANCODE_LSHIFT)) {
    e->dash_timer = 0.1;
    e->dash_dir = move_dir;
  }
  if (e->dash_timer > 0) {
    f32 dash_scale = 30;

    e->box.vel.x = e->dash_dir.x * dash_scale; 
    e->box.vel.z = e->dash_dir.z * dash_scale; 

    e->dash_timer -= dt;
  } else {
    e->dash_timer = 0;
  }

  // Jump logic
  {
    // Jump logic
    f32 jump_scale = 5;
    if (input_key_pressed(&gs->input, KEY_SCANCODE_SPACE)) { 
      e->box.vel.y = jump_scale;
    }
    f32 le_G = -9.8;
    e->box.vel.y = lerp(e->box.vel.y, le_G, dt);
  }
  // Simple axis separated movement
  e->move_dir = v3_norm(e->box.vel);
  for (s32 axis = 0; axis < 3; axis += 1) {
    v3 candidate_pos_axis = e->box.pos;
    candidate_pos_axis.raw[axis] += e->box.vel.raw[axis] * dt;
    Entity *collides_with = entity_collides(world, e->id, candidate_pos_axis);
    if (!collides_with) e->box.pos = candidate_pos_axis;
    else if (collides_with->kind == ENTITY_KIND_COIN) {
      // Remove the coin
      color obj_color = collides_with->col;
      v3 obj_pos = collides_with->box.pos;
      world_remove(world, collides_with->id);
      // Spawn a short emitter
      Particle_Emitter *death_coin_particles = particle_mgr_new_emitter(gs->pmgr);
      death_coin_particles->lifespan = 0.1;
      death_coin_particles->col = obj_color;
      death_coin_particles->pos = obj_pos;
      death_coin_particles->sec_per_particle = 0.003;
      death_coin_particles->particle_life_min = 0.1;
      death_coin_particles->particle_life_max = 0.8;
    }

  }
}

void draw_hero(Game_State *gs, Entity *e) {
  // Render le cube
  m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
  m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

  // Render le collider
  m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
      m4_mult(m4_translate(e->box.pos),
      m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
  );
  m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
      v4m(e->dynamic,e->dynamic,e->dynamic,1));
}

Entity *setup_hero(Entity *e, v3 pos) {
  e->update_fn = update_hero;
  e->draw_fn = draw_hero;
  e->kind = ENTITY_KIND_HERO;
  e->dynamic = true;
  e->box = (Phys_Box) {
    .pos = pos,
    .col_off = v3m(0,0,0),
    .hdim = v3m(0.3, 0.5, 0.3),
    .col_hdim = v3m(0.5,0.5,0.5),
  };
  e->col = v4m(0.9,0.4,0.3,1.0);
  return e;
}


////////////////////////////////////////////
// Wall Entity
////////////////////////////////////////////

void update_wall(Game_State *gs, Entity *e, f32 dt) {
  // TBA
}

void draw_wall(Game_State *gs, Entity *e) {
  // Render le cube
  m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
  m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

  // Render le collider
  m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
      m4_mult(m4_translate(e->box.pos),
      m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
  );
  m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
      v4m(e->dynamic,e->dynamic,e->dynamic,1));
}

Entity *setup_wall(Entity *e, v3 pos) {
  e->update_fn = update_wall;
  e->draw_fn = draw_wall;
  e->kind = ENTITY_KIND_WALL;
  e->dynamic = false;
  e->box = (Phys_Box) {
    .pos = pos,
    .col_off = v3m(0,0,0),
    .col_hdim = v3m(0.5,0.5,0.5),
    .hdim = v3m(0.5,0.5,0.5),
  };
  e->col = v4m(0.2,0.4,0.9,1.0);
  return e;
}


////////////////////////////////////////////
// Coin Entity
////////////////////////////////////////////

void update_coin(Game_State *gs, Entity *e, f32 dt) {

}

void draw_coin(Game_State *gs, Entity *e) {
  // Render le cube
  m4 model = m4_mult(m4_translate(e->box.pos),m4_scale(v3_multf(e->box.hdim, 2.0f)));
  m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, e->col);

  // Render le collider
  m4 model_collider = m4_mult(m4_translate(e->box.col_off), 
      m4_mult(m4_translate(e->box.pos),
      m4_scale(v3_multf(e->box.col_hdim, 2.0f)))
  );
  m4 mvp_collider = m4_mult(m4_mult(gs->proj, gs->view), model_collider);
  r3d_imm_cube(gs->game_viewport, OGL_PRIM_TYPE_LINE_LOOP, (m4*)&mvp_collider,
      v4m(e->dynamic,e->dynamic,e->dynamic,1));
}

Entity *setup_coin(Entity *e, v3 pos) {
  e->update_fn = update_coin;
  e->draw_fn = draw_coin;
  e->kind = ENTITY_KIND_COIN;
  e->dynamic = true;
  e->box = (Phys_Box) {
    .pos = pos,
    .col_off = v3m(0,0,0),
    .hdim = v3m(0.2, 0.1, 0.2),
    .col_hdim = v3m(0.2,0.1,0.2),
  };
  e->col = v4m(0.95,0.9,0.0,1.0);
  return e;
}
