#ifndef COLOR_H__
#define COLOR_H__
#include "base/helper.h"
#include "base/bmath.h"

// TODO: Color Spaces

typedef v4 color; 
#define col(x,y,z,w) (v4m(x,y,z,w))

static u32 u32_from_color(color c) {
  u32 res = 0;

  for (u32 component = 0; component < 4; component+=1) {
    res |= ((u32)(c.raw[component] * 255.0)) << (component*8);
  }

  return res;
}

#endif
