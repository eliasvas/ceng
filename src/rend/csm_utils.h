#ifndef CSM_UTILS_H__
#define CSM_UTILS_H__

#include "base/base_inc.h"
#include "camera.h"

#define SHADOWMAP_DIM 1024
#define CSM_MAX_CASCADES 4

typedef enum {
  CSM_SPLIT_UNIFORM,
  CSM_SPLIT_LOGARITHMIC,
  CSM_SPLIT_PRACTICAL,
  CSM_SPLIT_MANUAL,
  CSM_SPLIT_SDSM,
} CSM_Split_Policy;

typedef enum {
  CSM_FIT_FLAG_ZMULT = 0x1,
  CSM_FIT_FLAG_ISECT_BVH = 0x2,
} CSM_Fit_Flags;

typedef struct {

  // Fitting strategy
  f32 zmult;
  CSM_Fit_Flags fit_flags;

  // Split strategy (lambda can be used for practical, bounds idk where..)
  CSM_Split_Policy split_policy;
  union {
    f32 lambda;
    v4 bounds;
  };
  v2 drange;

  bbox scene_bbox;
  
  s32 cascade_count;
  v3 light_dir;
} CSM_Opt;

// TODO: Make this configurable
static CSM_Opt csm_opt_default() {
  CSM_Opt default_options = (CSM_Opt) {
    .zmult = 2.0,
    //.split_policy = CSM_SPLIT_MANUAL,
    //.split_policy = CSM_SPLIT_LOGARITHMIC,
    //.split_policy = CSM_SPLIT_UNIFORM,
    //.split_policy = CSM_SPLIT_PRACTICAL,
    .split_policy = CSM_SPLIT_SDSM,
    .lambda = 0.5,
    //.fit_flags = CSM_FIT_FLAG_ZMULT,
    .cascade_count = CSM_MAX_CASCADES,
    .light_dir = v3_norm(v3m(0.2,1,1)),
    .scene_bbox = (bbox){},
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

static m4 csm_calc_lsm_i(rect viewport, Camera *camera, CSM_Opt *csm_opt, s32 index) {
  f32 near_bound = csm_get_cascade_split_near(camera, csm_opt, index);
  f32 far_bound = csm_get_cascade_split_far(camera, csm_opt, index);

  f32 aspect = viewport.w/(f32)viewport.h;
  m4 persp_mat = m4_persp(camera->fovx, aspect, near_bound, far_bound);
  m4 view = camera->dirty_view;
  m4 lsm = lsm_from_vp(view, persp_mat, 
      (csm_opt->fit_flags & CSM_FIT_FLAG_ISECT_BVH) ? csm_opt->scene_bbox : bbox_inf(), 
      csm_opt->light_dir, 
      (csm_opt->fit_flags == CSM_FIT_FLAG_ZMULT) ? csm_opt->zmult : 1.0
);

  return lsm;
}


#endif
