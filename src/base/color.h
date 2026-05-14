#ifndef COLOR_H__
#define COLOR_H__
#include "base/helper.h"
#include "base/bmath.h"

// TODO: Color Spaces

typedef v4 color; 
#define col(x,y,z,w) (v4m(x,y,z,w))

#define color_from_rgba8(red, green, blue, alpha) ((color) { .r = (red/255.0f), .g = (green/255.0f), .b = (blue/255.0f), .a = (alpha/255.0f)})

static u32 u32_from_color(color c) {
  u32 res = 0;

  for (u32 component = 0; component < 4; component+=1) {
    res |= ((u32)(c.raw[component] * 255.0)) << (component*8);
  }

  return res;
}

#endif
