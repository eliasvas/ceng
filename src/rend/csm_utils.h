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

// TODO: Fit within cascade's bvh stuff!
static m4 lsm_from_vp(m4 view, m4 proj, bbox scene, v3 ld, f32 zmult) {
  m4 vp = m4_mult(proj, view);

  // 0. Calculate the view matrix for the LSM by finding the world frustum center
  frustum f = frustum_transform_points(frustum_make_ndc(), m4_inv(vp));
  v3 f_center = frustum_calc_center(&f);
  m4 lsm_view = m4_look_at(v3_add(f_center, ld), f_center, v3m(0,1,0));

  // 1. Calculate the ortho matrix by finding the view-space bounds of the frustum  
  frustum fv = frustum_transform_points(f, lsm_view);
  bbox bounds = bbox_from_frustum(fv);

  // 2. Calculate the actual ortho matrix (+zMult)
  //bounds.min.z = (bounds.min.z <= 0) ? (bounds.min.z * zmult) : (bounds.min.z / zmult);
  //bounds.max.z = (bounds.max.z <= 0) ? (bounds.max.z / zmult) : (bounds.max.z * zmult);

  if (zmult > 1.0) {
    f32 z_padding = 10.0f;
    bounds.min.z -= z_padding;
    bounds.max.z += z_padding;
  }

  bbox s = bbox_transform(scene, lsm_view);
  bounds.min.x = MAX(bounds.min.x, s.min.x);
  bounds.max.x = MIN(bounds.max.x, s.max.x);

  bounds.min.y = MAX(bounds.min.y, s.min.y);
  bounds.max.y = MIN(bounds.max.y, s.max.y);

  // TODO: could we calculate also tight z bounds here?

  m4 lsm_proj = m4_ortho(bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y, -bounds.max.z, -bounds.min.z);
  m4 lsm = m4_mult(lsm_proj, lsm_view);

  return lsm;
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
