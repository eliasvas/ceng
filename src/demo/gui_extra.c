#include "gui_extra.h"

extern Gui_Ctx ctx;

Gui_Signal gui_slider01(str8 label, f32 *value, f32 button_px, Gui_Axis axis) {
  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZEKIND_PIXELS, button_px, 1.0});
  gui_set_next_pref_size((axis), (Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 1.0});
  gui_set_next_child_layout_axis(axis);
  str8 scroll_bar_text = str8_sprintf(ctx.temp_arena, "%.*s_bar", (int)label.count, label.data);
  Gui_Box *scroll_bar = gui_box_make(scroll_bar_text, GUI_BOX_FLAG_DRAW_BOX);

  gui_push_parent(scroll_bar);

  gui_spacer(axis, (Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, *value, 0.0});

  f32 scroll_button_dim = button_px*2;
  gui_set_next_box_corner_radius(4.0);
  gui_set_next_box_softness(2.0);
  gui_set_next_pref_size((axis), (Gui_Size){.kind = GUI_SIZEKIND_PIXELS, scroll_button_dim, 1.0});
  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 0.0});
  gui_set_next_bg_color(v4_multf(clr(0.3,0.3,0.9,1.0), 0.9));
  //gui_set_next_bg_color(sdata->scroll_button_color);
  str8 scroll_button_text = str8_sprintf(ctx.temp_arena, "%.*s_sbutton", (int)label.count, label.data);
  Gui_Box *scroll_button = gui_box_make(scroll_button_text, GUI_BOX_FLAG_DRAW_BOX|GUI_BOX_FLAG_CLICKABLE);
  Gui_Signal scroll_button_sig = gui_signal_from_box(scroll_button);

  gui_spacer(axis, (Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0 - *(value), 0.0});

  //printf("value: %f\n", *value);
  if (gui_id_eq(scroll_button_sig.box->id, ctx.active_id)) {
    *(value) += 10.0 * input_get_mouse_delta(ctx.input).raw[axis] * ctx.dt;
    *(value) = CLAMP(*(value), 0, 1);
  }

  gui_pop_parent();

  return scroll_button_sig;
}
