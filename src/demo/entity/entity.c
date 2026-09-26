#include "entity/entity.h"
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

void collide_hero(World *world, Entity *e, Entity *other) {
    if (other->kind == ENTITY_KIND_COIN) {
      world_remove(world, other->id);
    }
}

void update_hero(World *world, Entity *e, f32 dt) {
  // Invoke state machine's update func
  e->hero_sm.funcs[e->hero_sm.state].on_update(world, e, dt);

  // Perform simple axis separated movement
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
  e->kind = ENTITY_KIND_HERO;
  entity_setup_const_data(e, e->kind);
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


static void hero_transition_to(World *world, Entity *e, Hero_State target) {
  e->hero_sm.funcs[e->hero_sm.state].on_exit(world, e);
  e->hero_sm.state = target;
  e->hero_sm.funcs[e->hero_sm.state].on_enter(world, e);
}

static void hero_walk_enter(struct World *world, struct Entity *entity) { }
static void hero_walk_exit(struct World *world, struct Entity *entity) { }
static void hero_walk_update(struct World *world, struct Entity *entity, f32 dt) {
  // TODO: maybe this should be a common helper (move_dir)
  v3 move_dir = v3m(0,0,0);
  if (input_key_down(world->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
  if (input_key_down(world->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }

  f32 speed = 5.0;
  entity->box.vel.x = move_dir.x * speed;
  entity->box.vel.z = move_dir.z * speed;

  // Jump logic
  {
    f32 jump_scale = 5;
    if (input_key_pressed(world->input, KEY_SCANCODE_SPACE)) { 
      entity->box.vel.y = jump_scale;
    }
    f32 le_G = -9.8;
    entity->box.vel.y = LERP(entity->box.vel.y, le_G, dt);
  }

  if (input_key_pressed(world->input, KEY_SCANCODE_LSHIFT) && v3_len(move_dir) > 0) {
    hero_transition_to(world, entity, HERO_STATE_DASH);
  }
}

static void hero_dash_enter(struct World *world, struct Entity *entity) {
  entity->dash_timer = 0.1;

  v3 move_dir = v3m(0,0,0);
  if (input_key_down(world->input, KEY_SCANCODE_RIGHT)) { move_dir.x+=1; }
  if (input_key_down(world->input, KEY_SCANCODE_LEFT)) { move_dir.x-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_UP)) { move_dir.z-=1; }
  if (input_key_down(world->input, KEY_SCANCODE_DOWN)) { move_dir.z+=1; }
  entity->dash_dir = move_dir;
}
static void hero_dash_exit(struct World *world, struct Entity *entity) {
  entity->dash_dir = v3m(0,0,0);
  entity->dash_timer = 0.0;
}
static void hero_dash_update(struct World *world, struct Entity *entity, f32 dt) {
  if (entity->dash_timer > 0) {
    f32 dash_scale = 30;
    entity->box.vel.x = entity->dash_dir.x * dash_scale; 
    entity->box.vel.z = entity->dash_dir.z * dash_scale; 
    entity->dash_timer -= dt;
  } else {
    hero_transition_to(world, entity, HERO_STATE_WALK);
  }
}

static Hero_State_Machine hero_sm_make(struct Entity *e) {
  Hero_State_Machine m = {};

  m.state = HERO_STATE_WALK;
  m.funcs[HERO_STATE_WALK] = (State_Func){
    .on_enter = hero_walk_enter,
    .on_exit = hero_walk_exit,
    .on_update = hero_walk_update,
  };
  m.funcs[HERO_STATE_DASH] = (State_Func){
    .on_enter = hero_dash_enter,
    .on_exit = hero_dash_exit,
    .on_update = hero_dash_update,
  };

  return m;
}


////////////////////////////////////////////
// Wall Entity
////////////////////////////////////////////

void collide_wall(World *world, Entity *e, Entity *other) {}

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
  e->kind = ENTITY_KIND_WALL;
  entity_setup_const_data(e, e->kind);
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

void collide_coin(World *world, Entity *e, Entity *other) {
}

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
  e->kind = ENTITY_KIND_COIN;
  entity_setup_const_data(e, e->kind);
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


// TODO: Should we break this up as well, only place we switch on entity_kind
void entity_setup_const_data(Entity *e, Entity_Kind kind) {
  switch (e->kind) {
    case ENTITY_KIND_HERO:
      e->update_fn = update_hero;
      e->draw_fn = draw_hero;
      e->kill_fn = kill_hero;
      e->collide_fn = collide_hero;
      e->hero_sm = hero_sm_make(e);
      break;
    case ENTITY_KIND_WALL:
      e->update_fn = update_wall;
      e->draw_fn = draw_wall;
      e->kill_fn = kill_wall;
      e->collide_fn = collide_wall;
      break;
    case ENTITY_KIND_COIN:
      e->update_fn = update_coin;
      e->draw_fn = draw_coin;
      e->kill_fn = kill_coin;
      e->collide_fn = collide_coin;
      break;
    case ENTITY_KIND_ENEMY:
      //e->update_fn = update_enemy;
      //e->draw_fn = draw_enemy;
      //e->kill_fn = kill_enemy;
      //e->collide_fn = collide_enemy;
      break;
    case ENTITY_KIND_BULLET:
      //e->update_fn = update_bullet;
      //e->draw_fn = draw_bullet;
      //e->kill_fn = kill_bullet;
      //e->collide_fn = collide_bullet;
    case ENTITY_KIND_NONE:
    default:
      break;
  }
}
