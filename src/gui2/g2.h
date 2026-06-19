#ifndef GUI2_H__
#define GUI2_H__
#include "base/base_inc.h"
#include "core/core_inc.h"

typedef u64 g2ID;

typedef struct {
  rect viewport;
  f32 g_scale;

  Arena *arena;
  Arena *temp_arena;

  Font_Info *font;
  Input *input;

  g2ID hot_id;
  g2ID active_id;

} g2_ctx;

void g2_init(Arena *tarena, Font_Info *font, Input *input);
b32 g2_button(buf label, v2 coords);

void g2_begin(rect viewport);
void g2_end();

#endif
