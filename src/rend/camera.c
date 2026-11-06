#include "camera.h"
#include "core/input.h"

v3 cam_get_pos(Camera *c) {
  v3 cpos = c->xform.t;
  return cpos;
}

m4 cam_get_view(Camera *c) {
  m4 xform = m4_from_transform(c->xform);
  return m4_inv(xform);
}

m4 cam_get_proj(Camera *c) {
  m4 proj = m4_persp(c->fovx, c->aspect, c->near, c->far);
  return proj;
}

m4 cam_get_vp(Camera *c) {
  m4 view = cam_get_view(c);
  m4 proj = cam_get_proj(c);
  return m4_mult(proj, view);
}

Camera cam_make_default() {
  Camera c = (Camera) {
    .near = 0.1,
    .far = 100,
    .aspect = 1.0,
    .fovx = 45.0,
    .speed = 10.0,
    .xform = transform_make(v3m(0,0,20), QUAT_ONE, v3m(1,1,1)),
  };
  return c;
}

void cam_update(Camera *c, struct Input *input, f32 dt) {
  f32 dist = v3_len(c->xform.t);
  if (input_key_down(input, KEY_SCANCODE_Q)) { dist += dt*c->speed;}
  if (input_key_down(input, KEY_SCANCODE_E)) { dist -= dt*c->speed;}

  quat r_x = QUAT_ONE;
  {
    f32 dangle = 0;
    if (input_key_down(input, KEY_SCANCODE_A)) { dangle -= dt * c->speed; }
    if (input_key_down(input, KEY_SCANCODE_D)) { dangle += dt * c->speed; }
    r_x = quat_from_axis_angle((axis_angle){v3m(0,1,0), dangle});
  }
  quat r_y = QUAT_ONE;
  {
    f32 dangle = 0;
    if (input_key_down(input, KEY_SCANCODE_W)) { dangle -= dt * c->speed; }
    if (input_key_down(input, KEY_SCANCODE_S)) { dangle += dt * c->speed; }
    r_y = quat_from_axis_angle((axis_angle){v3m(1,0,0), dangle});
  }
  quat rot = quat_mult(quat_mult(r_x, c->xform.r), r_y);

  c->xform.t = v3_from_v4(m4_multv(m4_from_quat(rot), v4m(0,0,dist,1)));
  c->xform.r = rot;

  c->dirty_view = cam_get_view(c);
  c->dirty_proj = cam_get_proj(c);
  c->dirty_vp = cam_get_vp(c);
}
