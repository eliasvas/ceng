#include "particle_mgr.h"
#include "game.h"

void particle_mgr_init(Particle_Mgr *pmgr, Arena *arena) {
  pmgr->arena = arena;
  pmgr->max_particles = 2048;
  pmgr->particles = arena_push_array(pmgr->arena, Particle, pmgr->max_particles);
  pmgr->first_free_particle_idx = 0;
  pmgr->particle_next_idx = 1;
}

b32 particle_active(Particle *particle) {
  return (particle->next_free_idx == 0);
}

Particle *particle_mgr_new_particle(Particle_Mgr *pmgr) {
  if (pmgr->first_free_particle_idx) {
    s64 free_idx = pmgr->first_free_particle_idx;
    pmgr->first_free_particle_idx = pmgr->particles[free_idx].next_free_idx;
    M_ZERO_STRUCT(&pmgr->particles[free_idx]);
    return &pmgr->particles[free_idx];
  }
  return &pmgr->particles[pmgr->particle_next_idx++];
}

void particle_mgr_kill_particle(Particle_Mgr *pmgr, Particle *particle) {
  s64 free_idx = pmgr->first_free_particle_idx;
  particle->next_free_idx = free_idx;
  pmgr->first_free_particle_idx = (UINT_FROM_PTR(particle) - UINT_FROM_PTR(pmgr->particles)) / sizeof(Particle);
}

Particle_Emitter *particle_mgr_new_emitter(Particle_Mgr *pmgr) {
  Particle_Emitter *emitter = arena_push_array(pmgr->arena, Particle_Emitter, 1);
  dll_push_back(pmgr->emitter_first, pmgr->emitter_last, emitter);
  // Default values for pos,vel, col, hdim (you can override them)
  emitter->pos = v3m(0,0,0);
  emitter->vel = v3m(0,10,0);
  emitter->col = col(1,0.6,0.3,1);
  emitter->hdim = v3m(0.2,0.2,0.2);
  emitter->particle_life_min = 0.4;
  emitter->particle_life_max = 3.4;
  emitter->sec_per_particle = 0.1;
  
  return emitter;
}
void particle_mgr_kill_emitter(Particle_Mgr *pmgr, Particle_Emitter *emitter) {
  dll_remove(pmgr->emitter_first, pmgr->emitter_last, emitter);
  M_ZERO_STRUCT(emitter);
  sll_stack_push(pmgr->free_emitter_nodes, emitter);
}

void particle_mgr_update(Game_State *gs, Particle_Mgr *pmgr, f32 dt) {
  // Spawn the new particles from the emitters
  for (Particle_Emitter *emitter = pmgr->emitter_first; emitter != nullptr; emitter=emitter->next) {
    emitter->sec_counter += dt;
    if (emitter->sec_counter > emitter->sec_per_particle) {
      emitter->sec_counter -= emitter->sec_per_particle;
      Particle *p = particle_mgr_new_particle(pmgr);

      // Naive heterogeneity
      p->pos = emitter->pos;
      p->vel = v3_multf(v3_add(emitter->vel, v3m(2*(brand_f01()-0.5), 0, 2*(brand_f01()-0.5))), 0.5*brand_f01()+0.5);
      p->col = v4_multf(emitter->col, 0.5*brand_f01()+0.5);
      p->hdim = v3_multf(emitter->hdim, 0.8*brand_f01()+0.5);
      p->lifespan = brand_frange(emitter->particle_life_min, emitter->particle_life_max);
    }
  }

  for (s64 particle_idx = 0; particle_idx < pmgr->particle_next_idx; particle_idx+=1) {
    Particle *p = &pmgr->particles[particle_idx];
    if (particle_active(p)) {
      p->pos = v3_add(p->pos, v3_multf(p->vel,dt));
      p->lifespan -= dt;
      if (p->lifespan <= 0) {
        particle_mgr_kill_particle(pmgr, p);
      }
    }
  }
}

// FIXME: slowest code in the universe
void particle_mgr_render(Game_State *gs, Particle_Mgr *pmgr) {
  for (s64 particle_idx = 0; particle_idx < pmgr->particle_next_idx; particle_idx+=1) {
    Particle *p = &pmgr->particles[particle_idx];
    if (particle_active(p)) {
      m4 model = m4_mult(m4_translate(p->pos),m4_scale(v3_multf(p->hdim, 2.0f)));
      m4 mvp = m4_mult(m4_mult(gs->proj, gs->view), model);
      r3d_imm_xy_face(gs->game_viewport, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp, p->col);
    }
  }

}
