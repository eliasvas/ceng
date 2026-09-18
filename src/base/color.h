#ifndef COLOR_H__
#define COLOR_H__
#include "base/helper.h"
#include "base/bmath.h"

typedef v4 color; 
#define clr(x,y,z,w) (v4m(x,y,z,w))
#define clr_256(x,y,z,w) (clr(x/255.0, y/255.0, z/255.0, w/255.0)) 
#define clr_hex(h) clr_256(((h>>16)&0xff),((h>>8)&0xff),((h>>0)&0xff),255)
#define clr_hexa(h) clr_256(((h>>24)&0xff,((h>>16)&0xff),((h>>8)&0xff),((h>>0)&0xff))

#define CLR_VARG(c) (c).x, (c).y, (c).z, (c).w
#define CLR_VARG_256(c) COL_VARG(col((c).x*255, (c).y*255,(c).z*255,(c).w*255))

#define CLR_WHITE  (clr(1,1,1,1))
#define CLR_RED    (clr(1,0,0,1))
#define CLR_GREEN  (clr(0,1,0,1))
#define CLR_BLUE   (clr(0,0,1,1))
#define CLR_YELLOW (clr(1,1,0,1))

#define CLR_BLUE_DAMSELFLY (clr_hex(0x309BD9))
#define CLR_BLUE_HIPPIE    (clr_hex(0x49869C))
#define CLR_GREEN_CLASSIC  (clr_hex(0x3CB452))
#define CLR_GREEN_EVA      (clr_hex(0x3DFF8E))
#define CLR_RED_PINK       (clr_hex(0xFC2C5C))
#define CLR_RED_RICH       (clr_hex(0xFA0A42))
#define CLR_PURPLE_C64     (clr_hex(0x6F75D3))
#define CLR_PURPLE_RAIN    (clr_hex(0x7844C7))

// TODO: Color Spaces

static u32 u32_from_color(color c) {
  u32 res = 0;

  for (u32 component = 0; component < 4; component+=1) {
    res |= ((u32)(c.raw[component] * 255.0)) << (component*8);
  }

  return res;
}

#endif
