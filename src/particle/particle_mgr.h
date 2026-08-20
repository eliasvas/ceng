#ifndef PARTICLE_MGR_H__
#define PARTICLE_MGR_H__

#include "base/base_inc.h"
#include "game.h"

// TODO: Maybe best candidate for multithreading support ok? (With like, pools)
// Also for profiling since we fill a giant vbo (at least thats the plan)


// This is the particle bible: https://alextardif.com/Particles.html


typedef struct {
  v3 pos;
  v3 vel;
  v4 col;
  v3 hdim;
  f32 lifespan; // in seconds

  u32 next_free_idx;
} Particle;

typedef struct Particle_Emitter Particle_Emitter;
struct Particle_Emitter {
  f32 sec_per_particle;
  f32 sec_counter;

  f32 particle_life_min;
  f32 particle_life_max;

  v3 pos;
  v3 vel;
  v4 col;
  v3 hdim;

  Particle_Emitter *next;
  Particle_Emitter *prev;
};

typedef struct Particle_Mgr Particle_Mgr;
struct Particle_Mgr {
  Arena *arena;

  // Regular Array
  Particle *particles;
  u64 max_particles;
  s64 first_free_particle_idx;
  s64 particle_next_idx;

  // Singly-Linked lists
  Particle_Emitter *emitter_first;
  Particle_Emitter *emitter_last;
  Particle_Emitter *free_emitter_nodes;

};

void particle_mgr_init(Particle_Mgr *pmgr, Arena *arena);
b32 particle_active(Particle *particle);
Particle *particle_mgr_new_particle(Particle_Mgr *pmgr);
void particle_mgr_kill_particle(Particle_Mgr *pmgr, Particle *particle);
Particle_Emitter *particle_mgr_new_emitter(Particle_Mgr *pmgr);
void particle_mgr_kill_emitter(Particle_Mgr *pmgr, Particle_Emitter *emitter);
void particle_mgr_update(Game_State *gs, Particle_Mgr *pmgr, f32 dt);
void particle_mgr_render(Game_State *gs, Particle_Mgr *pmgr);

#endif
