#include "camera.h"
#include "core/input.h"

v3 cam_get_pos(Camera *c) {
  v3 cpos = c->xform.t;
  return cpos;
}

m4 cam_get_view(Camera *c) {
  v3 f_up = v3m(0,1,0);
  m4 view = m4_look_at(c->xform.t, v3m(0,0,0), f_up);
  return view;
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
  };
  return c;
}

void cam_update(Camera *c, struct Input *input, f32 dt) {
  if (input_key_down(input, KEY_SCANCODE_Q)) { c->xform.t.y -= dt * c->speed; }
  if (input_key_down(input, KEY_SCANCODE_E)) { c->xform.t.y += dt * c->speed; }
  if (input_key_down(input, KEY_SCANCODE_W)) { c->xform.t.z -= dt * c->speed; }
  if (input_key_down(input, KEY_SCANCODE_S)) { c->xform.t.z += dt * c->speed; }
  if (input_key_down(input, KEY_SCANCODE_A)) { c->xform.t.x -= dt * c->speed; }
  if (input_key_down(input, KEY_SCANCODE_D)) { c->xform.t.x += dt * c->speed; }

  c->dirty_view = cam_get_view(c);
  c->dirty_proj = cam_get_proj(c);
  c->dirty_vp = cam_get_vp(c);
}
