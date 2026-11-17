#ifndef CSM_UTILS_H__
#define CSM_UTILS_H__

#include "base/base_inc.h"
#include "camera.h"

#include "rend/r3dbg.h"

#define SHADOWMAP_DIM 1024
#define CSM_MAX_CASCADES 4

typedef enum {
  CSM_SPLIT_UNIFORM,
  CSM_SPLIT_LOGARITHMIC,
  CSM_SPLIT_PRACTICAL,
  CSM_SPLIT_MANUAL,
  CSM_SPLIT_SDSM,
} CSM_Split_Mode;

typedef enum {
  CSM_FIT_VIEW, // Fits tightly to the view frustum's sub-frusta
  CSM_FIT_SDSM, // Fits tightly to the depth sampled sub-frusta
} CSM_Fit_Mode;

typedef struct {

  f32 zadd;
  CSM_Fit_Mode fit_mode;
  CSM_Split_Mode split_mode;


  union {
    f32 lambda;
    v4 bounds;
  };
  v2 drange;

  bbox scene_bbox;
  b32 isect_bvh;


  b32 vis_flags; // ????

  f32 splits[CSM_MAX_CASCADES];
  frustum cascade_frustum[CSM_MAX_CASCADES];
  
  s32 cascade_count;
  v3 light_dir;
} CSM_Opt;

// TODO: Make this configurable
static CSM_Opt csm_opt_default() {
  CSM_Opt default_options = (CSM_Opt) {
    .zadd = 0,
    //.split_policy = CSM_SPLIT_MANUAL,
    //.split_policy = CSM_SPLIT_LOGARITHMIC,
    //.split_policy = CSM_SPLIT_UNIFORM,
    //.split_policy = CSM_SPLIT_PRACTICAL,
    .lambda = 0.5,
    .split_mode = CSM_SPLIT_SDSM,
    .fit_mode = CSM_FIT_SDSM,
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

  switch(opt->split_mode) {
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

static m4 lsm_from_frustum(bbox scene, v3 ld, f32 zadd, frustum fw) {
  // 0. Calculate the view matrix for the LSM by finding the world frustum center
  v3 f_center = frustum_calc_center(&fw);
  m4 lsm_view = m4_look_at(v3_add(f_center, ld), f_center, v3m(0,1,0));

  //r3dbg_cube(m4_mult(m4_translate(f_center), m4_scale(v3m(0.1,0.1,0.1))), CLR_ALPHA(CLR_WHITE, 1));

  // 1. Calculate the ortho matrix by finding the view-space bounds of the frustum  
  frustum fv = frustum_transform_points(fw, lsm_view);
  bbox bounds = bbox_from_frustum(fv);

  // 2. Apply zadd
  bounds.min.z -= zadd;
  bounds.max.z += zadd;

#if 0
  // 3. fit with scene aabb (from bvh) 
  bbox s = bbox_transform(scene, lsm_view);
  bounds.min.x = MAX(bounds.min.x, s.min.x);
  bounds.max.x = MIN(bounds.max.x, s.max.x);
  bounds.min.y = MAX(bounds.min.y, s.min.y);
  bounds.max.y = MIN(bounds.max.y, s.max.y);
#endif

  // 4. Calculate the actual ortho matrix
  m4 lsm_proj = m4_ortho(bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y, -bounds.max.z, -bounds.min.z);
  m4 lsm = m4_mult(lsm_proj, lsm_view);

  //r3dbg_cube(m4_mult(m4_inv(lsm), m4_scale(v3m(2,2,2))), CLR_ALPHA(CLR_BLUE, 0.4));

  return lsm;
}

static m4 csm_calc_lsm_i(rect viewport, Camera *camera, CSM_Opt *csm_opt, s32 index) {
  frustum f = (frustum){};

  // 0. For SDSM, frustum is the depth buffer per-cascade tight frustum
  if (csm_opt->fit_mode == CSM_FIT_SDSM) {
    f = csm_opt->cascade_frustum[index];
  } else {
    // 0. In general we calculate with view-frustum's per-cascade tight frustum
    f32 near_bound = csm_get_cascade_split_near(camera, csm_opt, index);
    f32 far_bound = csm_get_cascade_split_far(camera, csm_opt, index);

    f32 aspect = viewport.w/(f32)viewport.h;
    m4 persp_mat = m4_persp(camera->fovx, aspect, near_bound, far_bound);
    m4 view = camera->dirty_view;

    m4 vp = m4_mult(persp_mat, view);
    f = frustum_transform_points(frustum_make_ndc(), m4_inv(vp));
  }

  // 1. We calculate the LSM with all the options passed
  m4 lsm = lsm_from_frustum(
    (csm_opt->isect_bvh) ? csm_opt->scene_bbox : bbox_inf(), 
    csm_opt->light_dir, 
    csm_opt->zadd, f
  );

  return lsm;
}

static v2 csm_convert_drange_to_viewspace(Camera *cam, v2 drange) {
  v4 ndc_min = v4m(0,0,drange.x*2.0 - 1.0, 1);
  v4 coords_min = unproject(ndc_min, m4_inv(cam->dirty_proj));
  v4 ndc_max = v4m(0,0,drange.y*2.0 - 1.0, 1);
  v4 coords_max = unproject(ndc_max, m4_inv(cam->dirty_proj));
  v2 drange_viewspace = v2m(-coords_min.z, -coords_max.z);
  return drange_viewspace;
}

#endif
