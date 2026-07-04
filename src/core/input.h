#ifndef INPUT_H__
#define INPUT_H__
#include "base/base_inc.h"
typedef enum {
  INPUT_EVENT_KIND_NONE = 0,
  INPUT_EVENT_KIND_KEEB,
  INPUT_EVENT_KIND_MOUSE,
  INPUT_EVENT_KIND_MOUSEMOTION,
  INPUT_EVENT_KIND_MOUSEWHEEL,
  INPUT_EVENT_KIND_GAMEPAD,
  INPUT_EVENT_KIND_RESIZE,
} Input_Event_Kind;

typedef enum {
  INPUT_MOUSE_LMB,
  INPUT_MOUSE_MMB,
  INPUT_MOUSE_RMB,
  INPUT_MOUSE_COUNT,
} Input_Mouse_Button;

typedef struct {
  Input_Mouse_Button button;
  b32 is_down;
} Input_Mouse_Event;

typedef struct {
  v2 mouse_pos;
} Input_MouseMotion_Event;

typedef struct {
  v2 wheel_delta;
} Input_MouseWheel_Event;

typedef enum {
    KEY_SCANCODE_UNKNOWN = 0,
    KEY_SCANCODE_A = 4,
    KEY_SCANCODE_B = 5,
    KEY_SCANCODE_C = 6,
    KEY_SCANCODE_D = 7,
    KEY_SCANCODE_E = 8,
    KEY_SCANCODE_F = 9,
    KEY_SCANCODE_G = 10,
    KEY_SCANCODE_H = 11,
    KEY_SCANCODE_I = 12,
    KEY_SCANCODE_J = 13,
    KEY_SCANCODE_K = 14,
    KEY_SCANCODE_L = 15,
    KEY_SCANCODE_M = 16,
    KEY_SCANCODE_N = 17,
    KEY_SCANCODE_O = 18,
    KEY_SCANCODE_P = 19,
    KEY_SCANCODE_Q = 20,
    KEY_SCANCODE_R = 21,
    KEY_SCANCODE_S = 22,
    KEY_SCANCODE_T = 23,
    KEY_SCANCODE_U = 24,
    KEY_SCANCODE_V = 25,
    KEY_SCANCODE_W = 26,
    KEY_SCANCODE_X = 27,
    KEY_SCANCODE_Y = 28,
    KEY_SCANCODE_Z = 29,

    KEY_SCANCODE_1 = 30,
    KEY_SCANCODE_2 = 31,
    KEY_SCANCODE_3 = 32,
    KEY_SCANCODE_4 = 33,
    KEY_SCANCODE_5 = 34,
    KEY_SCANCODE_6 = 35,
    KEY_SCANCODE_7 = 36,
    KEY_SCANCODE_8 = 37,
    KEY_SCANCODE_9 = 38,
    KEY_SCANCODE_0 = 39,

    KEY_SCANCODE_RETURN = 40,
    KEY_SCANCODE_ESCAPE = 41,
    KEY_SCANCODE_BACKSPACE = 42,
    KEY_SCANCODE_TAB = 43,
    KEY_SCANCODE_SPACE = 44,

    KEY_SCANCODE_MINUS = 45,
    KEY_SCANCODE_EQUALS = 46,
    KEY_SCANCODE_LEFTBRACKET = 47,
    KEY_SCANCODE_RIGHTBRACKET = 48,
    KEY_SCANCODE_BACKSLASH = 49,
    KEY_SCANCODE_NONUSHASH = 50,
    KEY_SCANCODE_SEMICOLON = 51,
    KEY_SCANCODE_APOSTROPHE = 52,
    KEY_SCANCODE_GRAVE = 53,
    KEY_SCANCODE_COMMA = 54,
    KEY_SCANCODE_PERIOD = 55,
    KEY_SCANCODE_SLASH = 56,
    KEY_SCANCODE_CAPSLOCK = 57,

    KEY_SCANCODE_F1 = 58,
    KEY_SCANCODE_F2 = 59,
    KEY_SCANCODE_F3 = 60,
    KEY_SCANCODE_F4 = 61,
    KEY_SCANCODE_F5 = 62,
    KEY_SCANCODE_F6 = 63,
    KEY_SCANCODE_F7 = 64,
    KEY_SCANCODE_F8 = 65,
    KEY_SCANCODE_F9 = 66,
    KEY_SCANCODE_F10 = 67,
    KEY_SCANCODE_F11 = 68,
    KEY_SCANCODE_F12 = 69,

    KEY_SCANCODE_PRINTSCREEN = 70,
    KEY_SCANCODE_SCROLLLOCK = 71,
    KEY_SCANCODE_PAUSE = 72,
    KEY_SCANCODE_INSERT = 73,
    KEY_SCANCODE_HOME = 74,
    KEY_SCANCODE_PAGEUP = 75,
    KEY_SCANCODE_DELETE = 76,
    KEY_SCANCODE_END = 77,
    KEY_SCANCODE_PAGEDOWN = 78,
    KEY_SCANCODE_RIGHT = 79,
    KEY_SCANCODE_LEFT = 80,
    KEY_SCANCODE_DOWN = 81,
    KEY_SCANCODE_UP = 82,

    KEY_SCANCODE_LCTRL = 224,
    KEY_SCANCODE_LSHIFT = 225,
    KEY_SCANCODE_LALT = 226,
    KEY_SCANCODE_LGUI = 227,
    KEY_SCANCODE_RCTRL = 228,
    KEY_SCANCODE_RSHIFT = 229,
    KEY_SCANCODE_RALT = 230,
    KEY_SCANCODE_RGUI = 231,

    KEY_SCANCODE_COUNT,
} Key_Scancode;

typedef struct {
  Key_Scancode scancode;
  b32 is_down;
} Input_Keeb_Event;

// TODO: this
typedef struct {
  u32 TBH;
} Input_Gamepad_Event;

typedef struct {
  union {
    Input_Mouse_Event         me;
    Input_MouseMotion_Event   mme;
    Input_MouseWheel_Event    mwe;
    Input_Keeb_Event          ke;
    Input_Gamepad_Event       ge;
  } data;
  Input_Event_Kind kind;
} Input_Event;


typedef struct Input_Event_Node Input_Event_Node;
struct Input_Event_Node {
  Input_Event_Node *next;
  Input_Event evt;
};

typedef struct {
  b32 was_down;
  b32 is_down;
  u64 transition_count;
}Input_Key_State;

// FIXME: probably should be allocated on the Heap!
typedef struct {
    Input_Key_State keeb_state[KEY_SCANCODE_COUNT];
    Input_Key_State mouse_state[INPUT_MOUSE_COUNT];

    Input_Event_Node *first;
    Input_Event_Node *last;

    v2 wheel_delta;
    v2 mouse_pos;
    v2 prev_mouse_pos;
    b32 win_resized;
} Input;

#ifndef INPUT_IMPLEMENTATION
  void input_process_events(Input *input);
  void input_end_frame(Input *input);
  void input_push_event(Input *input, Arena *arena, Input_Event *evt);

  b32 input_key_pressed(Input *input, Key_Scancode key);
  b32 input_key_released(Input *input, Key_Scancode key);
  b32 input_key_up(Input *input, Key_Scancode key);
  b32 input_key_down(Input *input, Key_Scancode key);

  b32 input_mkey_pressed(Input *input, Input_Mouse_Button button);
  b32 input_mkey_released(Input *input, Input_Mouse_Button button);
  b32 input_mkey_up(Input * input, Input_Mouse_Button button);
  b32 input_mkey_down(Input *input, Input_Mouse_Button button);

  v2 input_get_mouse_pos(Input *input);
  v2 input_get_mouse_delta(Input *input);
  v2 input_get_scroll_delta(Input *input);

  b32 input_win_resized(Input *input);
#else

  // TODO: Gamepad Events
  // TODO: Scrolling Events
  // TODO: Multiple keyboards/mouses (Gonna be hard we have to map KB/M to logical indices I think)

  void input_process_events(Input *input) {
    for (Input_Event_Node *node = input->first; node != nullptr; node = node->next) {
      Input_Event evt = node->evt;
      switch (evt.kind) {
        case INPUT_EVENT_KIND_NONE:
          // We should never have empty events in the queue
          assert(0 && "Empty event cannot be processed");
          break;
        case INPUT_EVENT_KIND_RESIZE:
          input->win_resized = true;
          break;
        case INPUT_EVENT_KIND_KEEB:
          Input_Keeb_Event ke = evt.data.ke;
          if (input->keeb_state[ke.scancode].is_down != ke.is_down) {
            input->keeb_state[ke.scancode].is_down = ke.is_down;
            input->keeb_state[ke.scancode].transition_count += 1;
          }
          break;
        case INPUT_EVENT_KIND_MOUSE:
          Input_Mouse_Event me = evt.data.me;
          if (input->mouse_state[me.button].is_down != me.is_down) {
            input->mouse_state[me.button].is_down = me.is_down;
            input->mouse_state[me.button].transition_count += 1;
          }
          break;
        case INPUT_EVENT_KIND_MOUSEMOTION:
          input->mouse_pos = evt.data.mme.mouse_pos; 
          break;
        case INPUT_EVENT_KIND_MOUSEWHEEL:
          input->wheel_delta = v2_add(input->wheel_delta, evt.data.mwe.wheel_delta); 
          break;
        case INPUT_EVENT_KIND_GAMEPAD:
          // TBA
          break;
        default: 
          assert(0 && "Corrupted event cannot be processed");
          break;
      }
    }
    // clear the event queue at the end
    input->first = nullptr;
    input->last = nullptr;
  }

  void input_end_frame(Input *input) {
    // Reset transition counts and set 'was_down' to previous frame's is_down field
    for (u32 keeb_key_idx = 0; keeb_key_idx < KEY_SCANCODE_COUNT; ++keeb_key_idx) {
      input->keeb_state[keeb_key_idx].was_down = input->keeb_state[keeb_key_idx].is_down;
      input->keeb_state[keeb_key_idx].transition_count = 0;
    }
    for (u32 mouse_key_idx = 0; mouse_key_idx < INPUT_MOUSE_COUNT; ++mouse_key_idx) {
      input->mouse_state[mouse_key_idx].was_down = input->mouse_state[mouse_key_idx].is_down;
      input->mouse_state[mouse_key_idx].transition_count = 0;
    }
    input->prev_mouse_pos = input->mouse_pos;
    input->wheel_delta = v2m(0,0);
    input->win_resized = false;
  }

  void input_push_event(Input *input, Arena *arena, Input_Event *evt) {
    if (evt->kind != INPUT_EVENT_KIND_NONE) {
      Input_Event_Node *new_event = arena_push_array(arena, Input_Event_Node, 1);
      M_COPY(&new_event->evt, evt, sizeof(Input_Event));
      sll_queue_push(input->first, input->last, new_event);
    }
  }

  // TODO: Maybe we should also take transitions into account? (yes)

  b32 input_key_pressed(Input* input, Key_Scancode key) {
    return input->keeb_state[key].is_down && !input->keeb_state[key].was_down;
  }
  b32 input_key_released(Input* input, Key_Scancode key) {
    return !input->keeb_state[key].is_down && input->keeb_state[key].was_down;
  }
  b32 input_key_up(Input *input, Key_Scancode key) {
    return !input->keeb_state[key].is_down;
  }
  b32 input_key_down(Input *input, Key_Scancode key) {
    return input->keeb_state[key].is_down;
  }

  b32 input_mkey_pressed(Input *input, Input_Mouse_Button button) {
    return input->mouse_state[button].is_down && !input->mouse_state[button].was_down;
  }
  b32 input_mkey_released(Input *input, Input_Mouse_Button button) {
    return !input->mouse_state[button].is_down && input->mouse_state[button].was_down;
  }
  b32 input_mkey_up(Input *input, Input_Mouse_Button button) {
    return !input->mouse_state[button].is_down;
  }
  b32 input_mkey_down(Input *input, Input_Mouse_Button button) {
    return input->mouse_state[button].is_down;
  }

  v2 input_get_mouse_pos(Input *input) {
    return input->mouse_pos;
  }
  v2 input_get_mouse_delta(Input *input) {
    return v2_sub(input->mouse_pos, input->prev_mouse_pos);
  }

  v2 input_get_scroll_delta(Input *input) {
    return input->wheel_delta;
  }

  b32 input_win_resized(Input *input) {
    return input->win_resized;
  }
#endif

#endif
