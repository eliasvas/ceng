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

Gui_Signal gui_flag_selection(str8 label, f32 button_px, u32 *value, s32 count) {
  // 0. Make a container (layout on x)
  Gui_Axis axis = GUI_AXIS_X;
  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZEKIND_PIXELS, button_px, 1.0});
  gui_set_next_pref_size((axis), (Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 1.0});
  gui_set_next_child_layout_axis(axis);
  str8 parento_text = str8_sprintf(ctx.temp_arena, "CONTAINER##%.*s", (int)label.count, label.data);
  Gui_Box *parento = gui_box_make(parento_text, GUI_BOX_FLAG_DRAW_BOX);
  assert(parento);

  gui_push_parent(parento);
  // 1. Make a label with the label text:w
  gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZEKIND_PIXELS, button_px, 1.0});
  gui_set_next_pref_size((axis), (Gui_Size){.kind = GUI_SIZEKIND_TEXT_CONTENT, 5.0, 1.0});
  gui_set_next_child_layout_axis(axis);
  str8 label_text = str8_sprintf(ctx.temp_arena, "%.*s##labelstr", (int)label.count, label.data);
  Gui_Box *labelbox = gui_box_make(label_text, GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT);
  assert(labelbox);

  // 2. Make count checkboxes
  for (s32 idx = 0; idx < count; idx+=1) {
    gui_set_next_pref_size(gui_axis_flip(axis), (Gui_Size){.kind = GUI_SIZEKIND_PIXELS, button_px, 1.0});
    gui_set_next_pref_size((axis), (Gui_Size){.kind = GUI_SIZEKIND_TEXT_CONTENT, 2.0, 1.0});
    gui_set_next_child_layout_axis(axis);
    if ((0x1 << idx) & *value) gui_set_next_bg_color(v4m(1,0.3,0.3,1.0));
    str8 button_text = str8_sprintf(ctx.temp_arena, "%d##%.*slabelforthis%d", idx, (int)label.count, label.data, idx);
    Gui_Box *buttonbox = gui_box_make(button_text, GUI_BOX_FLAG_DRAW_BOX | GUI_BOX_FLAG_DRAW_TEXT);
    assert(buttonbox);

    b32 mouse_over = rect_isect_point(buttonbox->final_rect, input_get_mouse_pos(ctx.input));
    b32 mb_pressed = input_mkey_pressed(ctx.input, INPUT_MOUSE_LMB);
    if (mouse_over && mb_pressed) {
      *value ^= (0x1 << idx); 
    }
  }
  gui_pop_parent();


  return (Gui_Signal){};
}

