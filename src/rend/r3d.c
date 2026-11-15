#include "asset/asset_mgr.h"
// FIXME: Too much code duplication here..
// FIXME: these globals should be in a shared place, maybe asset system ok?

// Maybe asset management should happen somewhere..
static Ogl_Render_Bundle fullscreen_bundle = {};
static Ogl_Render_Bundle postproc_bundle = {};
static Ogl_Render_Bundle uber_bundle = {};
static Ogl_RT offscreen_rt;

static Ogl_Render_Bundle uber_shadow_bundle = {};
static Ogl_RT shadow_rt;

static Ogl_Render_Bundle uber_transparent_bundle = {};
static Ogl_RT transparent_rt;

static Ogl_Compute_Bundle depth_range_compute_bundle;
static Ogl_Compute_Bundle bounds_compute_bundle;

Ogl_Render_Bundle* r3d_bundle_from_render_mode(R3D_Render_Mode rmode) {
  switch (rmode) {
    case R3D_FWD: return &uber_bundle;
    case R3D_OPQ: return &uber_bundle;
    case R3D_TRN: return &uber_transparent_bundle;
    case R3D_SHD: return &uber_shadow_bundle;
    default: return &uber_bundle;
  }
}

Ogl_RT* r3d_rt_from_render_mode(R3D_Render_Mode rmode) {
  switch (rmode) {
    case R3D_FWD: return nullptr;
    case R3D_OPQ: return &offscreen_rt;
    case R3D_TRN: return &transparent_rt;
    case R3D_SHD: return &shadow_rt;
    default: return nullptr;
  }
}



  static const u8 DEPTH_RANGE_CS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/depth_range.comp.glsl"
  };

  static const u8 CASCADE_BOUNDS_CS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/cascade_bounds.comp.glsl"
  };

  static const u8 TRANSPARENT_FS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/transparent.frag.glsl"
  };

  static const u8 SHADOW_FS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/shadow.frag.glsl"
  };

  static const u8 UBER_VS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/uber.vert.glsl"
  };

  static const u8 UBER_FS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/uber.frag.glsl"
  };

  static const u8 FULLSCREEN_VS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/fullscreen.vert.glsl"
  };

  static const u8 FULLSCREEN_FS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/fullscreen.frag.glsl"
  };

  static const u8 POSTPROC_FS[] = {
#embed "shaders/common.glsl" suffix(, '\n',)
#embed "shaders/postproc.frag.glsl"
  };

typedef struct {
  m4 lsms[CSM_MAX_CASCADES];
  m4 view;
  m4 proj;
  v3 cam_pos;
  f32 _padding;
  v3 light_dir;
  f32 _padding2;
  v4 cascade_bounds;
  int cascade_idx;
  f32 _padding3,_padding4, _padding5;
} PerFrameData_UBO;

typedef struct {
  v4 base_color_factor;
  v4 emissive_factor;

  float metallic_factor;
  float roughness_factor;
  int base_tc_idx;
  int normal_tc_idx;

  int metallic_roughness_tc_idx;
  int emissive_tc_idx;
  int occlusion_tc_idx;
  int is_unlit;

  m4 model_matrix;
} Material_UBO;

void r3d_try_load_shaders(rect viewport) {
  if (depth_range_compute_bundle.sp.impl_state == 0) {
    depth_range_compute_bundle = (Ogl_Compute_Bundle) {
      .sp = ogl_shader_make((char*)DEPTH_RANGE_CS, nullptr),
      .images = {
        [0] = { .name = "drange", .img = ogl_tex_make(nullptr, 2, 1, OGL_TEX_FORMAT_R32UI, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT}),},
      },
    };
  }
  if (bounds_compute_bundle.sp.impl_state == 0) {
    bounds_compute_bundle = (Ogl_Compute_Bundle) {
      .sp = ogl_shader_make((char*)CASCADE_BOUNDS_CS, nullptr),
      .images = {
        [0] = { .name = "bounds", .img = ogl_tex_make(nullptr, 6, 1, OGL_TEX_FORMAT_R32UI, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT}),},
      },
    };
  }
  if (postproc_bundle.sp.impl_state == 0) {
    ogl_rt_init(&offscreen_rt, viewport.w, viewport.h, OGL_RT_A0, OGL_TEX_FORMAT_RGBA8U, 1);

    postproc_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make((char*)FULLSCREEN_VS, (char*)POSTPROC_FS),
      .depth_state = (Ogl_Depth_State) {
        .dwrite = OGL_DEPTH_WRITE_DISABLED,
        .dfunc  = OGL_DFUNC_ALWAYS,
      },
      .dyn_state = (Ogl_Dyn_State){
        //.viewport = viewport,
        //.scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_SCISSOR,
        //.flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR, 
      },
    };
  }

  if (fullscreen_bundle.sp.impl_state == 0) {
    fullscreen_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make((char*)FULLSCREEN_VS, (char*)FULLSCREEN_FS),
      .depth_state = (Ogl_Depth_State) {
        .dwrite = OGL_DEPTH_WRITE_DISABLED,
        .dfunc  = OGL_DFUNC_ALWAYS,
      },
      .dyn_state = (Ogl_Dyn_State){
        //.viewport = viewport,
        //.scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_SCISSOR,
        //.flags    = OGL_DYN_STATE_FLAG_BLEND | OGL_DYN_STATE_FLAG_SCISSOR, 
      },
    };
  }

  if (uber_bundle.sp.impl_state == 0) {
    ogl_rt_init(&shadow_rt, SHADOWMAP_DIM, SHADOWMAP_DIM, OGL_RT_A0, OGL_TEX_FORMAT_RGBA32F, true);

    uber_bundle = (Ogl_Render_Bundle){
      .sp = ogl_shader_make((char*)UBER_VS, (char*)UBER_FS),

      .textures[0] = (Ogl_Tex_Slot){ .name = "base_color_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[1] = (Ogl_Tex_Slot){ .name = "normal_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[2] = (Ogl_Tex_Slot){ .name = "metallic_roughness_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[3] = (Ogl_Tex_Slot){ .name = "emissive_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[4] = (Ogl_Tex_Slot){ .name = "occlusion_tex", .tex = *AM_GET(asset_id_from_path(STR8L("white.png")), tex)},
      .textures[5] = (Ogl_Tex_Slot){ .name = "shadow_map", .tex = shadow_rt.attachments[0]},
      .vbos = {
        [0] = {
          .buffer = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, nullptr, REND_MAX_INSTANCES, sizeof(Uber_Vertex)),
          .vattribs = {
            [0] = { .location = 0,   .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Uber_Vertex, pos),      .stride = sizeof(Uber_Vertex), .instanced = false, },
            [1] =  { .location = 1,  .type = OGL_DATA_TYPE_VEC3,  .offset = offsetof(Uber_Vertex, norm),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [2] =  { .location = 2,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, tangent),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [3] =  { .location = 3,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_0),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [4] =  { .location = 4,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_1),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [5] =  { .location = 5,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_2),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [6] =  { .location = 6,  .type = OGL_DATA_TYPE_VEC2,  .offset = offsetof(Uber_Vertex, tc_3),     .stride = sizeof(Uber_Vertex), .instanced = false, },
            [7] =  { .location = 7,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_0),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [8] =  { .location = 8,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_1),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [9] =  { .location = 9,  .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_2),    .stride = sizeof(Uber_Vertex), .instanced = false, },
            [10] = { .location = 10, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, col_3),    .stride = sizeof(Uber_Vertex), .instanced = false, },

            [11] = { .location = 11, .type = OGL_DATA_TYPE_IVEC4,  .offset = offsetof(Uber_Vertex, joint_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [12] = { .location = 12, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            // FIXME: If we get only 2 joints and 2 weights max, 11 + 4 = 15 < 16 (Most OpenGL implementation MAX attribs) 
#if 0
            [11] = { .location = 11, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_0),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [12] = { .location = 12, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_1),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [13] = { .location = 13, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_2),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [14] = { .location = 14, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, joint_3),  .stride = sizeof(Uber_Vertex), .instanced = false, },
            [15] = { .location = 15, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_0), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [16] = { .location = 16, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_1), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [17] = { .location = 17, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_2), .stride = sizeof(Uber_Vertex), .instanced = false, },
            [18] = { .location = 18, .type = OGL_DATA_TYPE_VEC4,  .offset = offsetof(Uber_Vertex, weight_3), .stride = sizeof(Uber_Vertex), .instanced = false, },
#endif
          },
        },
      },
      .ubos = {
        [0] = { .name = "PerFrameData", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(PerFrameData_UBO)), .start_offset = 0, .size = sizeof(PerFrameData_UBO) }, 
        [1] = { .name = "Material", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(Material_UBO)), .start_offset = 0, .size = sizeof(Material_UBO) },
        [2] = { .name = "JointMatrices", .buffer = ogl_buf_make(OGL_BUF_KIND_UNIFORM, OGL_BUF_HINT_DYNAMIC, nullptr, 1, sizeof(m4)*32), .start_offset = 0, .size = sizeof(m4)*32 },
      },
      .rt = offscreen_rt,
      .dyn_state = (Ogl_Dyn_State){
        //.viewport = viewport,
        //.scissor  = scissor,
        .flags    = OGL_DYN_STATE_FLAG_SCISSOR, 
      },
      .depth_state = (Ogl_Depth_State) {
        .dwrite = OGL_DEPTH_WRITE_ENABLED,
        .dfunc  = OGL_DFUNC_LESS,
      },
      .blend_state[0] = (Ogl_Blend_State) {
        .enabled = true,
        .sfactor = OGL_SRC_ALPHA,
        .dfactor = OGL_ONE_MINUS_SRC_ALPHA,
      },
    };

    uber_shadow_bundle = uber_bundle;
    uber_shadow_bundle.sp = ogl_shader_make((char*)UBER_VS, (char*)SHADOW_FS);
    uber_shadow_bundle.rt = shadow_rt;
    uber_shadow_bundle.dyn_state.flags = OGL_DYN_STATE_FLAG_SCISSOR; 
    M_ZERO_ARRAY(uber_shadow_bundle.blend_state);
  }

  if (uber_transparent_bundle.sp.impl_state == 0) {
    ogl_rt_init(&transparent_rt, viewport.w, viewport.h, OGL_RT_A0 | OGL_RT_A1, OGL_TEX_FORMAT_RGBA16F, true);
    uber_transparent_bundle = uber_bundle;
    uber_transparent_bundle.sp = ogl_shader_make((char*)UBER_VS, (char*)TRANSPARENT_FS);
    uber_transparent_bundle.rt = transparent_rt;
    uber_transparent_bundle.dyn_state.flags = OGL_DYN_STATE_FLAG_SCISSOR; 

    uber_transparent_bundle.depth_state = (Ogl_Depth_State) {
      .dwrite = OGL_DEPTH_WRITE_DISABLED,
      .dfunc  = OGL_DFUNC_LESS,
    };
    uber_transparent_bundle.blend_state[0] = (Ogl_Blend_State) {
      .enabled = true,
      .sfactor = OGL_ONE,
      .dfactor = OGL_ONE,
    };
    uber_transparent_bundle.blend_state[1] = (Ogl_Blend_State) {
      .enabled = true,
      .sfactor = OGL_ZERO,
      .dfactor = OGL_ONE_MINUS_SRC_COLOR,
    };
  }

}

////////////////////////////////////
// Context API! (More compact!)
////////////////////////////////////

R3D_Ctx* r3dc_begin(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Render_Mode rmode, R3D_Ctx_Flags flags) {
  R3D_Ctx *rctx = arena_push_array(arena, R3D_Ctx, 1);
  *rctx = (R3D_Ctx) {
    .arena = arena,
    .viewport = viewport,
    .cam = *c,
    .opt = *copt,
    .rmode = rmode,
  };
  for (s32 cascade_idx = 0; cascade_idx < CSM_MAX_CASCADES; cascade_idx+=1) {
    rctx->lsms[cascade_idx] = csm_calc_lsm_i_sdsm(rctx->viewport, &rctx->cam, &rctx->opt, cascade_idx);
  }

  return rctx;
}

R3D_Ctx* r3dc_begin_forward(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags) {
  return r3dc_begin(arena, viewport, c, copt, R3D_FWD, flags);
}


R3D_Ctx* r3dc_begin_depth_i(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, s32 cascade_idx, R3D_Ctx_Flags flags) {

  // HACK: we change the cmask of the global rt to write to correct component
  r3d_rt_from_render_mode(R3D_SHD)->cmask = (OGL_RT_CMASK_R << cascade_idx);

  R3D_Ctx *pass = r3dc_begin(arena, viewport, c, copt, R3D_SHD, flags);

  //FIXME: Hack, because we only use (proj * view)
  m4 proj = csm_calc_lsm_i(pass->viewport, &pass->cam, &pass->opt, cascade_idx);
  pass->cam.dirty_view = m4d(1.0);
  pass->cam.dirty_proj = proj;

  // FIXME: HACK, We set the viewport later because the original viewport (the real one) is used for calc_lsm_i, maybe pass it as argument instead
  pass->viewport = rec(0,0,SHADOWMAP_DIM,SHADOWMAP_DIM);

  pass->cascade_idx = cascade_idx;
  return pass;
}

R3D_Ctx* r3dc_begin_offscreen(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags) {
  R3D_Ctx *pass = r3dc_begin(arena, viewport, c, copt, R3D_OPQ, flags);
  return pass;
}

void r3d_clear() {
  // 0. Clear offscreen RT
  ogl_clear(&offscreen_rt);

  // 1. Clear transparent accum to v4m(0) and reveal to v4m(1)
  ogl_clear_color(&transparent_rt, 0, (float[]){0, 0, 0, 0});
  ogl_clear_color(&transparent_rt, 1, (float[]){1, 1, 1, 1});

  // 2. Clear cascaded shadow map RT
  ogl_clear_color(&shadow_rt, 0, (float[]){1,1,1,1});
  ogl_clear_depth(&shadow_rt);
}

R3D_Ctx* r3dc_begin_transparent(Arena *arena, rect viewport, Camera *c, CSM_Opt *copt, R3D_Ctx_Flags flags) {
  // Copy the offscreen (pass) RT's depth buffer to the transparent RT
  ogl_copy_depth_buffer(&offscreen_rt, &transparent_rt, viewport);
  //transparent_rt.depth_attachment = offscreen_rt.depth_attachment;

  R3D_Ctx *pass = r3dc_begin(arena, viewport, c, copt, R3D_TRN, flags);
  return pass;
}

void r3dc_end(R3D_Ctx *rctx) {
#if 1
  Ogl_RT* rt = r3d_rt_from_render_mode(rctx->rmode);
  if (rt) rt->cmask = OGL_RT_CMASK_ALL;
#endif
}

void r3dc_set_per_frame_ubo(R3D_Ctx *rctx, Ogl_Buf *buf) {
  PerFrameData_UBO pf_ubo = (PerFrameData_UBO) {
    .lsms[0] = rctx->lsms[0],
    .lsms[1] = rctx->lsms[1],
    .lsms[2] = rctx->lsms[2],
    .lsms[3] = rctx->lsms[3],
    .view = rctx->cam.dirty_view,
    .proj = rctx->cam.dirty_proj,
    .cam_pos = cam_get_pos(&rctx->cam),
    .light_dir = rctx->opt.light_dir,
    .cascade_idx = rctx->cascade_idx,
  };
  for (s32 cascade_idx = 0; cascade_idx < CSM_MAX_CASCADES; cascade_idx+=1) {
    pf_ubo.cascade_bounds.raw[cascade_idx] = csm_get_cascade_split_far(&rctx->cam, &rctx->opt, cascade_idx);
  }
  ogl_buf_update(buf, 0, &pf_ubo, 1, sizeof(pf_ubo));
}

void r3d_set_def_material(m4 model, b32 is_unlit) {
  Ogl_Tex *texture = AM_GET((Asset_Id){}, tex);
  uber_bundle.textures[0] = (Ogl_Tex_Slot){.name = ("base_color_tex"), .tex = *(texture),};
  uber_bundle.textures[1] = (Ogl_Tex_Slot){.name = ("normal_tex"), .tex = *(texture),};
  uber_bundle.textures[2] = (Ogl_Tex_Slot){.name = ("metallic_roughness_tex"), .tex = *(texture),};
  uber_bundle.textures[3] = (Ogl_Tex_Slot){.name = ("emissive_tex"), .tex = *(texture),};
  uber_bundle.textures[4] = (Ogl_Tex_Slot){.name = ("occlusion_tex"), .tex = *(texture),};

  Material_UBO material_ubo = (Material_UBO) {
    .base_color_factor = v4m(1,1,1,1),
    .metallic_factor = 1.0,
    .roughness_factor = 1.0,
    .emissive_factor = v4_zero,
    .base_tc_idx = 0,
    .metallic_roughness_tc_idx = 0,
    .normal_tc_idx = 0,
    .occlusion_tc_idx = 0,
    .model_matrix = model,
    .is_unlit = (is_unlit),
  };
  ogl_buf_update(&uber_bundle.ubos[1].buffer, 0, &material_ubo, 1, sizeof(Material_UBO));
}



void r3d_set_material(Mesh_Primitive_Info *info, m4 model, color tint) {
  Material_Info *material = &info->material;
  Ogl_Tex *texture = AM_GET(material->base_tex.tex_asset_id, tex);
  uber_bundle.textures[0] = (Ogl_Tex_Slot){.name = ("base_color_tex"), .tex = *(texture),};

  texture = AM_GET(material->normal_tex.tex_asset_id, tex);
  uber_bundle.textures[1] = (Ogl_Tex_Slot){.name = ("normal_tex"), .tex = *(texture),};

  texture = AM_GET(material->metallic_roughness_tex.tex_asset_id, tex);
  uber_bundle.textures[2] = (Ogl_Tex_Slot){.name = ("metallic_roughness_tex"), .tex = *(texture),};

  texture = AM_GET(material->emissive_tex.tex_asset_id, tex);
  uber_bundle.textures[3] = (Ogl_Tex_Slot){.name = ("emissive_tex"), .tex = *(texture),};

  texture = AM_GET(material->occlusion_tex.tex_asset_id, tex);
  uber_bundle.textures[4] = (Ogl_Tex_Slot){.name = ("occlusion_tex"), .tex = *(texture),};

  Material_UBO material_ubo = (Material_UBO) {
    .base_color_factor = v4_mult(material->base_color_factor, tint),
    .metallic_factor = material->metallic_factor,
    .roughness_factor = material->roughness_factor,
    .emissive_factor = v4_from_v3(material->emissive_factor, 1.0),
    .base_tc_idx = material->base_tex.tc_idx,
    .metallic_roughness_tc_idx = material->metallic_roughness_tex.tc_idx,
    .normal_tc_idx = material->normal_tex.tc_idx,
    .occlusion_tc_idx = material->occlusion_tex.tc_idx,
    .model_matrix = model,
  };
  ogl_buf_update(&uber_bundle.ubos[1].buffer, 0, &material_ubo, 1, sizeof(Material_UBO));
}

m4 calc_transform(Model_Info *info, s32 node_idx) {
  Transform_Node *node = &info->nodes[node_idx];
  m4 trans = m4_from_transform(node->xform);

  while (node->parent_idx != node_idx) {
    node_idx = node->parent_idx;
    node = &info->nodes[node->parent_idx];
    trans = m4_mult(m4_from_transform(node->xform), trans);
  }

  return trans;
}

// TODO: mesh_idx refers to specific skeleton, probably
// FIXME: Doing all the operations in one step really hurts performance.. generally..
#define JOINT_MAT_COUNT 32
m4 *calc_joint_mats_for_animation(Arena *arena, struct Model_Info *info, s32 mesh_idx, s32 anim_idx, f32 time_sec) {
  if (anim_idx < info->animation_count) {
    Animation *animation = &info->animations[anim_idx];
    for (s32 node_anim_idx = 0; node_anim_idx < animation->node_anim_count; node_anim_idx+=1) {
      Node_Anim *anim = &animation->node_anims[node_anim_idx];
      // 0. Calculate animation percent ( e.g we are 0.3 through )
      f32 anim_time = fmodf(time_sec, anim->max_duration);
      s32 prev_kf_idx = 0;
      s32 next_kf_idx = anim->kf_count-1;
      for (s32 kf_idx = 0; kf_idx < anim->kf_count; kf_idx+=1) {
        f32 timestamp = anim->kf_timestamps[kf_idx];
        if (timestamp < anim_time) prev_kf_idx = kf_idx;
        if (timestamp > anim_time) {
          next_kf_idx = kf_idx;
          break;
        }
      }

      f32 prev_timestamp = anim->kf_timestamps[prev_kf_idx];
      f32 next_timestamp = anim->kf_timestamps[next_kf_idx];
      f32 percent = 0;
      if (next_timestamp - prev_timestamp != 0.0) percent = (anim_time - prev_timestamp) / (next_timestamp - prev_timestamp);

      // 1. Update local transforms for all joints (maybe pull transforms to separate memory block?)

      // FIXME: Interpolate based on Interp_Type, dont do always linear!
      Transform_Node *tn = &info->nodes[anim->node_idx];
      //printf("timestamps: %f - %f - %f\n", prev_timestamp, anim_time, next_timestamp);
      v3 prev, next, interp;
      quat prev4, next4, interp4;
      switch(anim->kind) {
        case NODE_ANIM_KIND_TRANSLATION:
          prev = ((v3*)(anim->values))[prev_kf_idx];
          next = ((v3*)(anim->values))[next_kf_idx];
          interp = v3_lerp(prev, next, percent);
          tn->xform.t = interp;
          break;
        case NODE_ANIM_KIND_ROTATION:
          prev4 = ((quat*)(anim->values))[prev_kf_idx];
          next4 = ((quat*)(anim->values))[next_kf_idx];
          interp4 = quat_nlerp(prev4, next4, percent);
          tn->xform.r = interp4;
          break;
        case NODE_ANIM_KIND_SCALE:
          prev = ((v3*)(anim->values))[prev_kf_idx];
          next = ((v3*)(anim->values))[next_kf_idx];
          interp = v3_lerp(prev, next, percent);
          tn->xform.s = interp;
          break;
        default:
          break;
      }
    }
  }

  // 2. Calculate the actual joint matrices
  Mesh_Info *mesh = &info->meshes[mesh_idx];
  if (mesh->joint_hierarchy.joint_count > 0) {
    m4 mesh_global = calc_transform(info, mesh->node_idx);
    m4 inv_mesh_global = m4_inv(mesh_global);

    m4 *joint_matrices = arena_push_array(arena, m4, JOINT_MAT_COUNT); 
    for (s32 joint_idx = 0; joint_idx < mesh->joint_hierarchy.joint_count; joint_idx+=1) {
      s32 node_idx = mesh->joint_hierarchy.joints[joint_idx].node_id;

      m4 joint_global = calc_transform(info, node_idx);
      m4 ibm = mesh->joint_hierarchy.joints[joint_idx].ibn;
      joint_matrices[joint_idx] = m4_mult(inv_mesh_global, m4_mult(joint_global, ibm));
    }

    return joint_matrices;
  } else {
    m4 *joint_matrices = arena_push_array(arena, m4, JOINT_MAT_COUNT); 
    for (s32 joint_idx = 0; joint_idx < JOINT_MAT_COUNT; joint_idx+=1) {
      joint_matrices[joint_idx] = m4d(1.0f);
    }
    return joint_matrices;
  }

}

f32 _blend_factor = 0.0;
void r3dc_imm_model(R3D_Ctx *rctx, struct Model_Info *info, m4 model, f32 time_sec, s32 anim_idx, color tint) {
  Ogl_Render_Bundle *bundle = r3d_bundle_from_render_mode(rctx->rmode);
  Ogl_RT *rt = r3d_rt_from_render_mode(rctx->rmode);
  if (rt) bundle->rt = *rt;

  for (s64 mesh_idx = 0; mesh_idx < info->mesh_count; mesh_idx+=1) {
    Mesh_Info *mesh = &info->meshes[mesh_idx];
    m4 mesh_global = calc_transform(info, mesh->node_idx);
    s32 joint_count = mesh->joint_hierarchy.joint_count;

    // Skeletal animation part..
    Temp_Arena temp = get_scratch(0,0);

#if 0
    //f32 _blend_factor = 0.5;
    m4 *joint_matrices_a = calc_joint_mats_for_animation(temp.arena, info, mesh_idx, 0, time_sec);
    m4 *joint_matrices_b = calc_joint_mats_for_animation(temp.arena, info, mesh_idx, 1, time_sec);

    m4 *joint_matrices = arena_push_array(temp.arena, m4, JOINT_MAT_COUNT);
    for (s32 joint_idx = 0; joint_idx < JOINT_MAT_COUNT; joint_idx+=1) {
      transform t_a = transform_from_m4(joint_matrices_a[joint_idx]);
      transform t_b = transform_from_m4(joint_matrices_b[joint_idx]);
      transform blended = (transform) {
        .t = v3_lerp(t_a.t, t_b.t, _blend_factor),
        .r = quat_nlerp(t_a.r, t_b.r, _blend_factor),
        .s = v3_lerp(t_a.s, t_b.s, _blend_factor),
      };
      joint_matrices[joint_idx] = m4_from_transform(blended);
    }
#else
    // FIXME why we need to calculate 0 before 1 for matrices to be ok
    calc_joint_mats_for_animation(temp.arena, info, mesh_idx, 0, time_sec);
    m4 *joint_matrices = calc_joint_mats_for_animation(temp.arena, info, mesh_idx, anim_idx, time_sec);
#endif

    ogl_buf_update(&bundle->ubos[2].buffer, 0, joint_matrices, 1, sizeof(m4)*JOINT_MAT_COUNT);
    release_scratch(temp);

    for (s64 primitive_idx = 0; primitive_idx < mesh->prim_count; primitive_idx+=1) {
      Mesh_Primitive_Info *prim =  &mesh->prims[primitive_idx];
      m4 model_matrix = m4_mult(model, (joint_count) ? m4d(1.0) : mesh_global);

      bundle->vbos[0].buffer = prim->vbo;
      bundle->index_buffer = prim->ibo;
      bundle->dyn_state.viewport = *(Ogl_rect *)&rctx->viewport;
      bundle->dyn_state.scissor = *(Ogl_rect *)&rctx->viewport;

      r3dc_set_per_frame_ubo(rctx, &bundle->ubos[0].buffer);
      r3d_set_material(prim, model_matrix, tint);

      if (prim->ibo.count) {
        ogl_render_bundle_draw_indexed(bundle, prim->type, prim->ibo.count);
      } else {
        ogl_render_bundle_draw(bundle, prim->type, prim->vbo.count, 1);
      }
    }
  }

}

void r3d_do_fullscreen(rect viewport, Ogl_Tex *tex) {
  fullscreen_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
  fullscreen_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;
  fullscreen_bundle.textures[0] = (Ogl_Tex_Slot){ .name = "u_tex", .tex = *tex };
  ogl_render_bundle_draw(&fullscreen_bundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, 1);
}

void r3d_do_postproc(rect viewport) {
  postproc_bundle.dyn_state.viewport = *(Ogl_rect *)&viewport;
  postproc_bundle.dyn_state.scissor = *(Ogl_rect *)&viewport;
  postproc_bundle.textures[0] = (Ogl_Tex_Slot){ .name = "u_tex", .tex = offscreen_rt.attachments[0] };
  postproc_bundle.textures[1] = (Ogl_Tex_Slot){ .name = "accum", .tex = transparent_rt.attachments[0] };
  postproc_bundle.textures[2] = (Ogl_Tex_Slot){ .name = "reveal", .tex = transparent_rt.attachments[1] };
  ogl_render_bundle_draw(&postproc_bundle, OGL_PRIM_TYPE_TRIANGLE_FAN, 4, 1);

  rect r = rec(1300,0,300,300);
  r3d_do_fullscreen(r, &shadow_rt.depth_attachment);
}

// TODO: Shouldn't this be inside csm_utils or no?
// Maybe we just refactor
v2 r3d_compute_depth_range(R3D_Ctx *rctx) {
  // 0. update drange tex to have [-1,0] range
  uint initial_data[2] = { 0xFFFFFFFFu, 0u };
  ogl_tex_update(&depth_range_compute_bundle.images[0].img, (u8*)initial_data, 2, 1, OGL_TEX_FORMAT_R32UI, (Ogl_Tex_Params){});

  // 1. execute the compute shader to find actual drange(min,max)
  depth_range_compute_bundle.textures[0] = 
    (Ogl_Tex_Slot){ .name = "depth_buffer", .tex = offscreen_rt.depth_attachment };
  ogl_compute_bundle_dispatch(&depth_range_compute_bundle, 
      offscreen_rt.depth_attachment.width, offscreen_rt.depth_attachment.height, 1);


  // 2. read back the texture and unproject min/max from ndc to view space
  u32 drange[2];
  ogl_tex_read(&depth_range_compute_bundle.images[0].img, (u8*)drange);
  // Converting to view space??
  f32 min_z = *(f32*)&(drange[0]);
  f32 max_z = *(f32*)&(drange[1]);

  return v2m(min_z, max_z);
}

// For each of the cascades we compute a tight frustum for all the fragments 
frustum r3d_compute_cascade_frustum(R3D_Ctx *rctx, v2 drange, v2 drange_viewspace, s32 cascade_idx) {
  // -1. find the ndc space needed near far for the cascade_idx
  //f32 cascade_near = csm_get_cascade_split_near(&rctx->cam, &rctx->opt, 0);
  //cascade_near = project(v3m(0,0,cascade_near), rctx->cam.dirty_proj).z;
  //f32 cascade_far = csm_get_cascade_split_far(&rctx->cam, &rctx->opt, CSM_MAX_CASCADES-1);
  //cascade_far = project(v3m(0,0,cascade_far), rctx->cam.dirty_proj).z;

#if 0
  f64 step = (drange.y - drange.x) / (f64)CSM_MAX_CASCADES;
  f32 cascade_near = drange.x - step * (cascade_idx);
  //cascade_near = project(v3m(0,0,cascade_near), rctx->cam.dirty_proj).z;
  f32 cascade_far = drange.y - step * (cascade_idx+1);
  //cascade_far = project(v3m(0,0,cascade_far), rctx->cam.dirty_proj).z;
  //printf("%f %f\n", cascade_near, cascade_far);
#endif

  //static f32 n[CSM_MAX_CASCADES],f[CSM_MAX_CASCADES];

  f64 step = ((f64)drange.y - (f64)drange.x) / (f64)CSM_MAX_CASCADES;
  f32 cascade_near = drange.x + step * cascade_idx;
  f32 cascade_far  = drange.x + step * (cascade_idx + 1);


  // 0. update tex to have [-1,0] range for xy and drange for z
#if 0
  u32 near_as_u32 = *((u32*)&n[cascade_idx]);
  u32 far_as_u32 = *((u32*)&f[cascade_idx]);
#endif

  u32 near_as_u32 = *((u32*)&cascade_near);
  u32 far_as_u32 = *((u32*)&cascade_far);

  uint initial_data[6] = { 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0u, near_as_u32, far_as_u32};
  //uint initial_data[6] = { 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0u};
  ogl_tex_update(&bounds_compute_bundle.images[0].img, (u8*)initial_data, 6, 1, OGL_TEX_FORMAT_R32UI, (Ogl_Tex_Params){});

  // 1. execute the compute shader to find actual drange(min,max)
  bounds_compute_bundle.textures[0] = 
    (Ogl_Tex_Slot){ .name = "depth_buffer", .tex = offscreen_rt.depth_attachment };
  ogl_compute_bundle_dispatch(&bounds_compute_bundle, 
      offscreen_rt.depth_attachment.width, offscreen_rt.depth_attachment.height, 1);

  // 2. read back the texture and produce NDC frustum
  u32 bounds[6];
  ogl_tex_read(&bounds_compute_bundle.images[0].img, (u8*)bounds);
  f32 bounds_min_x = *(f32*)&(bounds[0]);
  f32 bounds_max_x = *(f32*)&(bounds[1]);

  f32 bounds_min_y = *(f32*)&(bounds[2]);
  f32 bounds_max_y = *(f32*)&(bounds[3]);

  f32 bounds_min_z = *(f32*)&(bounds[4]);
  f32 bounds_max_z = *(f32*)&(bounds[5]);

  v3 ndc_min = v3m(bounds_min_x*2.0-1.0, bounds_min_y*2.0-1.0, bounds_min_z*2.0-1.0);
  v3 ndc_max = v3m(bounds_max_x*2.0-1.0, bounds_max_y*2.0-1.0, bounds_max_z*2.0-1.0);

#if 0
  printf("(viewport) bounds z: [%f:%f]\n", bounds_min_z, bounds_max_z);
  n[cascade_idx] = -project(ndc_min, m4_inv(rctx->cam.dirty_proj)).z;
  f[cascade_idx] = -project(ndc_max, m4_inv(rctx->cam.dirty_proj)).z;
  printf("(view) bounds z: [%f:%f]\n", n, f);
#endif

  // 3. Compute the tight frustum

  // Get the scene frustum in NDC
  frustum scene = frustum_from_ndc_points(ndc_min, ndc_max);
  // Go to world space????
  scene = frustum_transform_points(scene, m4_inv(rctx->cam.dirty_vp)); 

  // Optional bullshit
#if 0
  // Compute world-space bbox for debug drawing
  bbox bbox_bounds = bbox_from_frustum(scene);

  rctx->opt.scene_bbox2 = bbox_normalize(bbox_bounds);
  rctx->opt.scene_bbox2_frustum = scene;


  /*
  printf("coords: [%f][%f][%f][%f][%f][%f]\n",
    bounds_min_x, bounds_max_x, bounds_min_y,
    bounds_max_y, bounds_min_z, bounds_max_z
  );
  */
#endif

  return scene;


}

void r3dc_imm_verts(R3D_Ctx *rctx, Uber_Vertex *verts, s32 vert_count, Ogl_Prim_Type prim, m4 model) {
  Ogl_Render_Bundle *bundle = r3d_bundle_from_render_mode(rctx->rmode);
  Ogl_RT *rt = r3d_rt_from_render_mode(rctx->rmode);
  if (rt) bundle->rt = *rt;

  Ogl_Buf vbo = ogl_buf_make(OGL_BUF_KIND_VERTEX, OGL_BUF_HINT_DYNAMIC, verts, 1, sizeof(Uber_Vertex)*vert_count);
  bundle->vbos[0].buffer = vbo;

  r3dc_set_per_frame_ubo(rctx, &bundle->ubos[0].buffer);
  r3d_set_def_material(model, true);

  // Set dynamically before drawcall currently
  bundle->dyn_state.viewport = *(Ogl_rect *)&rctx->viewport;
  bundle->dyn_state.scissor = *(Ogl_rect *)&rctx->viewport;

  ogl_render_bundle_draw(bundle, prim, vert_count, 1);

  // FIXME: This is retarded.. doing cleanup each invocation.. uhm.. what
  ogl_buf_deinit(&vbo);
}


void r3dc_imm_cube(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model) {
  Uber_Vertex cube_verts[36] = {
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f,-0.5f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)}
  };
  // Just put the fries in the bag bro
  for (s32 i = 0; i < 36; i += 3) {
    v3 a = cube_verts[i+0].pos;
    v3 b = cube_verts[i+1].pos;
    v3 c = cube_verts[i+2].pos;
    v3 n = v3_norm(v3_cross(v3_sub(b,a), v3_sub(c,a)));
    cube_verts[i+0].norm = n; 
    cube_verts[i+1].norm = n; 
    cube_verts[i+2].norm = n; 
  }

  r3dc_imm_verts(rctx, cube_verts, ARRAY_COUNT(cube_verts), prim, model);
}

void r3dc_imm_xy_face(R3D_Ctx *rctx, Ogl_Prim_Type prim, color c, m4 model) {
  Uber_Vertex cube_verts[6] = {
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f,-0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f,-0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m( 0.5f, 0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = v3m(-0.5f, 0.5f, 0.0f), .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
  };
  r3dc_imm_verts(rctx, cube_verts, ARRAY_COUNT(cube_verts), prim, model);
}

void r3dc_imm_frustum(R3D_Ctx *rctx, frustum f, color c, Ogl_Prim_Type prim) {
  Uber_Vertex frustum[8] = {
    (Uber_Vertex) {.pos = f.points[0], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[1], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[2], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[3], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},

    (Uber_Vertex) {.pos = f.points[4], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[5], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[6], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
    (Uber_Vertex) {.pos = f.points[7], .col_0 = c, .joint_0 = iv4m(1,0,0,0), .weight_0 = v4m(1,0,0,0)},
  };

  Uber_Vertex verts[] = {
    frustum[0], frustum[1], frustum[2], frustum[3], frustum[0],
    frustum[1], frustum[5], frustum[6], frustum[2], frustum[1],
    frustum[2], frustum[6], frustum[7], frustum[3], frustum[2],
    frustum[3], frustum[7], frustum[4], frustum[0], frustum[3],
    frustum[0], frustum[4], frustum[5], frustum[1], frustum[0],
    frustum[4], frustum[7], frustum[6], frustum[5], frustum[4],
  };

  r3dc_imm_verts(rctx, verts, ARRAY_COUNT(verts), prim, m4_translate(v3m(0,0,0)));
}

