#ifndef GUI_EXTRA_H__
#define GUI_EXTRA_H__

#include "game.h"
#include "base/base_inc.h"
#include "core/core_inc.h"
#include "gui/gui.h"

Gui_Signal gui_slider01(str8 label, f32 *value, f32 button_px, Gui_Axis axis);

// gui_flag_selection(options)
// ([]str labels later)
Gui_Signal gui_flag_selection(str8 label, f32 button_px, u32 *value, s32 count);

#endif
