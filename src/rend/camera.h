#ifndef CAMERA_H__
#define CAMERA_H__
#include "base/base_inc.h"

struct Input;

typedef struct {
  // Maybe we need a v3 pos here..
  transform xform;

  f32 near, far;
  f32 aspect;
  f32 fovx;

  f32 speed;

  m4 dirty_view;
  m4 dirty_proj;
  m4 dirty_vp;
} Camera;

v3 cam_get_pos(Camera *c);
m4 cam_get_view(Camera *c);
m4 cam_get_proj(Camera *c);
m4 cam_get_vp(Camera *c);
Camera cam_make_default();
void cam_update(Camera *c, struct Input *input, f32 dt);

#endif
