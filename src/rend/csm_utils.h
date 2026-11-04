#ifndef CSM_UTILS_H__
#define CSM_UTILS_H__

#include "base/base_inc.h"
#include "camera.h"

#define CSM_MAX_CASCADES 4

typedef enum {
  CSM_SPLIT_UNIFORM,
  CSM_SPLIT_LOGARITHMIC,
  CSM_SPLIT_PRACTICAL,
  CSM_SPLIT_MANUAL,
  CSM_SPLIT_SDSM, // SDSM
} CSM_Split_Policy;

typedef enum {
  CSM_FIT_TIGHT,
  CSM_FIT_APPLY_ZMULT,
  CSM_FIT_ISECT_BVH,
} CSM_Fitting_Flags;

typedef struct {

  // Fitting strategy
  f32 zmult;
  CSM_Fitting_Flags fflags;

  // Split strategy
  CSM_Split_Policy split_policy;
  union {
    f32 lambda;
    v4 bounds;
  };
  v2 drange;
  
  s32 cascade_count;
  v3 light_dir;
} CSM_Opt;

// TODO: Make this configurable
static CSM_Opt csm_opt_default() {
  CSM_Opt default_options = (CSM_Opt) {
    .zmult = 1.0,
    //.split_policy = CSM_SPLIT_MANUAL,
    //.split_policy = CSM_SPLIT_LOGARITHMIC,
    //.split_policy = CSM_SPLIT_UNIFORM,
    //.split_policy = CSM_SPLIT_PRACTICAL,
    .split_policy = CSM_SPLIT_SDSM,
    .lambda = 0.5,
    .fflags = CSM_FIT_APPLY_ZMULT | CSM_FIT_ISECT_BVH,
    .cascade_count = CSM_MAX_CASCADES,
    .light_dir = v3_norm(v3m(0.1,1,1)),
  };

  return default_options;
}

// Fixed bounds for now
static f32 csm_get_cascade_split_near(Camera *c, CSM_Opt *opt, s32 cascade_idx) {
  f32 bounds[CSM_MAX_CASCADES+1] = (f32[]){0.1,20,30,40,100};

  f32 t = (f32)cascade_idx / opt->cascade_count;
  f32 uniform_split = c->near + (c->far - c->near)*t;
  f32 logarithmic_split = c->near * pow_f32(c->far / c->near, t);
  f32 logarithmic_split_sdsm = opt->drange.x * pow_f32(opt->drange.y / opt->drange.x, t);
  //f32 linear_split_sdsm = opt->drange.x + (opt->drange.y - opt->drange.x)*t;

  switch(opt->split_policy) {
    case CSM_SPLIT_UNIFORM:
      return uniform_split;
    case CSM_SPLIT_LOGARITHMIC:
      return logarithmic_split;
    case CSM_SPLIT_PRACTICAL:
      return opt->lambda * logarithmic_split + (1.0 - opt->lambda) * uniform_split;
    case CSM_SPLIT_MANUAL:
      return bounds[cascade_idx];
    case CSM_SPLIT_SDSM:
      return logarithmic_split_sdsm;
      break;
    default:
      break;
  }
  assert(0);
  return 0;
}

static f32 csm_get_cascade_split_far(Camera *c, CSM_Opt *opt, s32 cascade_idx) {
  return csm_get_cascade_split_near(c, opt, cascade_idx+1);
}

static color csm_get_cascade_idx_color(s32 cascade_idx) {
  color c = CLR_WHITE;
  switch (cascade_idx) {
    case 0: 
      c = CLR_BLUE_HIPPIE;
      break; 
    case 1: 
      c = CLR_RED_RICH;
      break;
    case 2:
      c = CLR_GREEN_EVA; 
      break;
    case 3:
      c = CLR_PURPLE_C64; 
      break;
    default: 
      c = CLR_WHITE;
      break;
  }
  c.a = 0.5;
  return c;
}

#endif
