#include "entity.h"
#include "game.h"

// Forward declaration from World..
b32 world_entity_collides(World *world, Entity_ID id, v3 candidate_pos);

bbox bbox_from_phys_box(Phys_Box *box) {
  v3 collider_center = v3_add(box->pos, box->col_off);
  return bbox_normalize(bbox_from_center_hdim(collider_center, box->col_hdim));
}

bbox entity_get_collider_bbox(Entity *entity) {
  return bbox_from_phys_box(&entity->box);
}

void entity_common_draw(World *world, Entity *e) {
  transform xform = {
    .t = e->box.pos,
    .r = qu(0,0,0,1),
    .s = v3_multf(e->box.hdim, 2.0f),
  };
  transform collider_xform = {
    .t = v3_add(e->box.col_off, e->box.pos),
    .r = qu(0,0,0,1),
    .s = v3_multf(e->box.col_hdim, 2.0f),
  };

  Entity_Render_Command cmd = (Entity_Render_Command) {
    .xform = xform,
    .asset_id = (Asset_Id){},
    .col = e->col,

    .collider_xform = collider_xform, 
    .collider_col = (e->dynamic) ? clr(1,1,1,1) : clr(0,0,0,1),
  };

  world->rcommands[world->rcommand_count++] = cmd;
}


////////////////////////////////////////////
// Hero Entity
////////////////////////////////////////////

void update_hero(World *world, Entity *e, f32 dt) {
  // Movement dir
  v3 move_dir = v3m(0,0,0);
  if (input_key_down(world->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
  if (input_key_down(world->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }
  f32 speed = 5.0;
  e->box.vel.x = move_dir.x * speed;
  e->box.vel.z = move_dir.z * speed;

  // For reload testing
  //e->col = col(1,0,0,1);

  // Dash logic
  if (e->dash_timer <= 0 && input_key_pressed(world->input, KEY_SCANCODE_LSHIFT)) {
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
    if (input_key_pressed(world->input, KEY_SCANCODE_SPACE)) { 
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
    b32 collides = world_entity_collides(world, e->id, candidate_pos_axis);
    if (!collides) e->box.pos = candidate_pos_axis;

  }
}

void kill_hero(World *world, Entity *e) {}

void draw_hero(World *world, Entity *e) {
  entity_common_draw(world, e);
}

Entity *setup_hero(Entity *e, v3 pos) {
  e->update_fn = update_hero;
  e->draw_fn = draw_hero;
  e->kill_fn = kill_hero;
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

void update_wall(World *world, Entity *e, f32 dt) {
  // TBA
}

void kill_wall(World *world, Entity *e) {
  printf("WALL killed!\n");
}

void draw_wall(World *world, Entity *e) {
  entity_common_draw(world, e);
}

Entity *setup_wall(Entity *e, v3 pos) {
  e->update_fn = update_wall;
  e->draw_fn = draw_wall;
  e->kill_fn = kill_wall;
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
void update_coin(World *world, Entity *e, f32 dt) {

}

void kill_coin(struct World *world, Entity *e) {
  printf("COIN killed!\n");

  //color obj_color = e->col;
  v3 obj_pos = e->box.pos;
  // Spawn a short emitter
  Particle_Emitter *death_coin_particles = particle_mgr_new_emitter(world->pmgr);
  death_coin_particles->lifespan = 0.1;
  //death_coin_particles->col = obj_color;
  death_coin_particles->pos = obj_pos;
  death_coin_particles->sec_per_particle = 0.003;
  death_coin_particles->particle_life_min = 0.1;
  death_coin_particles->particle_life_max = 0.8;
}

void draw_coin(World *world, Entity *e) {
  entity_common_draw(world, e);
}

Entity *setup_coin(Entity *e, v3 pos) {
  e->update_fn = update_coin;
  e->draw_fn = draw_coin;
  e->kill_fn = kill_coin;
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
