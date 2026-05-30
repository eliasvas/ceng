#ifndef EASING_H__
#define EASING_H__

#include "bmath.h"
// For x in [0,1] these functions will produce a y in [0,1]
// with the correct easing behavior

static f32 ease_in_linear(f32 x) {
  return x;
}

static f32 ease_in_quad(f32 x) {
  return x * x;
}

static f32 ease_out_quad(f32 x) {
  return 1.0 - pow_f32(1.0-x, 2);
}

static f32 ease_in_qubic(f32 x) {
  return x * x * x;
}

static f32 ease_out_qubic(f32 x) {
  return 1.0 - pow_f32(1.0-x, 3);
}


#endif
