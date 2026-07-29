#include "input/nebu_input_system.h"
#include "input/nebu_system_keynames.h"
#include "base/nebu_system.h"
#include "video/nebu_video_system.h"
#include "scripting/nebu_scripting.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/nebu_debug_memory.h"

static float joystick_threshold = 0;
static int mouse_x = -1;
static int mouse_y = -1;

enum { eMaxKeyState = 1024 };
static int keyState[eMaxKeyState];

/* Map SDL_JoystickID -> dense slot index (0..MAX_JOY-1). SDL3's
 * event.jaxis.which and event.jbutton.which carry instance IDs rather than 0..N
 * indices, but the rest of the input layer (SYSTEM_JOY_OFFSET arithmetic,
 * joy_axis_state[] in the event handler) still wants a small index. */
enum { MAX_JOY = 4 };
static SDL_JoystickID joystick_ids[MAX_JOY];
static int n_joysticks;

static int joystick_slot(SDL_JoystickID which) {
  for (int i = 0; i < n_joysticks; i++) {
    if (joystick_ids[i] == which) return i;
  }
  return -1;
}

static void setKeyState(int key, int state) {
  if (key < eMaxKeyState) keyState[key] = state;
}

void nebu_Input_Init(void) {
  int i;

  /* joystick */
  if (SDL_Init(SDL_INIT_JOYSTICK)) {
    int sdl_joystick_count = 0;
    SDL_JoystickID* sdl_joysticks = SDL_GetJoysticks(&sdl_joystick_count);

    int max_joy = 2; /* default... override by setting NEBU_MAX_JOY */
    char* NEBU_MAX_JOY = getenv("NEBU_MAX_JOY");

    if (NEBU_MAX_JOY) {
      int n;
      char* endptr;
      errno = 0;
      n = strtol(NEBU_MAX_JOY, &endptr, 10);
      if (n < 0) n = 0;
      if (n > MAX_JOY) n = MAX_JOY;
      if (!*endptr && !errno) max_joy = n;
    }

    int to_open = sdl_joystick_count;
    if (to_open > max_joy) to_open = max_joy;

    n_joysticks = 0;
    for (i = 0; i < to_open; i++) {
      if (SDL_OpenJoystick(sdl_joysticks[i])) {
        joystick_ids[n_joysticks++] = sdl_joysticks[i];
      }
    }
    SDL_free(sdl_joysticks);

    if (n_joysticks) SDL_SetJoystickEventsEnabled(true);
  } else {
    const char* s = SDL_GetError();
    fprintf(stderr, "[init] couldn't initialize joysticks: %s\n", s);
  }
  for (i = 0; i < eMaxKeyState; i++) {
    keyState[i] = NEBU_INPUT_KEYSTATE_UP;
  }
}

void nebu_Input_Grab(void) {
  SDL_Window* window = nebu_Video_GetWindow();
  if (window) {
    SDL_SetWindowMouseGrab(window, true);
  }
}

void nebu_Input_Ungrab(void) {
  SDL_Window* window = nebu_Video_GetWindow();
  if (window) {
    SDL_SetWindowMouseGrab(window, false);
  }
}

void nebu_Input_HidePointer(void) { SDL_HideCursor(); }

void nebu_Input_UnhidePointer(void) { SDL_ShowCursor(); }

void SystemMouse(int buttons, int state, int x, int y) {
  if (current && current->mouse) current->mouse(buttons, state, x, y);
}

int nebu_Input_GetKeyState(int key) {
  if (key > eMaxKeyState)
    return NEBU_INPUT_KEYSTATE_UP;
  else
    return keyState[key];
}

void nebu_Input_Mouse_GetDelta(int* x, int* y) {
  int wx, wy;

  if (mouse_x == -1 || mouse_y == -1) {
    // mouse coordinates not yet initialized
    *x = 0;
    *y = 0;
  }

  nebu_Video_GetDimension(&wx, &wy);
  *x = mouse_x - wx / 2;
  *y = mouse_y - wy / 2;

  // printf("[input] returned delta %d,%d\n", *x, *y);
}

void nebu_Input_Mouse_WarpToOrigin(void) {
  int wx, wy;
  nebu_Video_GetDimension(&wx, &wy);
  nebu_Video_WarpPointer(wx / 2, wy / 2);
  // printf("[input] warped to %d,%d\n", wx / 2, wy /2);
}

void SystemMouseMotion(int x, int y) {
  // save mouse position
  // printf("[input] mouse motion to %d, %d\n", x, y);
  mouse_x = x;
  mouse_y = y;
  if (current && current->mouseMotion) current->mouseMotion(x, y);
}

const char* nebu_Input_GetKeyname(int key) {
  if (key < SYSTEM_CUSTOM_KEYS)
    return SDL_GetKeyName(key);
  else {
    int i;

    for (i = 0; i < CUSTOM_KEY_COUNT; i++) {
      if (custom_keys.key[i].key == key) return custom_keys.key[i].name;
    }
    return "unknown custom key";
  }
}

void nebu_Intern_HandleInput(SDL_Event* event) {
  const char* keyname;
  int key, state;
  static int joy_axis_state[MAX_JOY] = {0};
  static int joy_lastaxis[MAX_JOY] = {0};
  int slot;

  switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      if (event->type == SDL_EVENT_KEY_DOWN) {
        state = NEBU_INPUT_KEYSTATE_DOWN;
      } else {
        state = NEBU_INPUT_KEYSTATE_UP;
      }

      keyname = SDL_GetKeyName(event->key.key);
      key = 0;
      switch (event->key.key) {
        case SDLK_SPACE:
          key = ' ';
          break;
        case SDLK_ESCAPE:
          key = 27;
          break;
        case SDLK_RETURN:
          key = 13;
          break;
        default:
          if (keyname[0] && keyname[1] == 0) key = keyname[0];
          break;
      }
      setKeyState(key, state);
      if (current && current->keyboard)
        current->keyboard(state, key ? key : (int)event->key.key, 0, 0);
      break;
    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
      slot = joystick_slot(event->jaxis.which);
      if (slot < 0) break;
      if (abs(event->jaxis.value) <= joystick_threshold * SYSTEM_JOY_AXIS_MAX) {
        // axis returned to origin, only generate event if it was set before
        if (joy_axis_state[slot] & (1 << event->jaxis.axis)) {
          joy_axis_state[slot] &= ~(1 << event->jaxis.axis);
          key = SYSTEM_JOY_LEFT + slot * SYSTEM_JOY_OFFSET;
          if (event->jaxis.axis == 1) {
            key += 2;
          }
          if (joy_lastaxis[slot] & (1 << event->jaxis.axis)) {
            key++;
          }
          setKeyState(key, NEBU_INPUT_KEYSTATE_UP);
          if (current && current->keyboard)
            current->keyboard(NEBU_INPUT_KEYSTATE_UP, key, 0, 0);
        }
      } else {
        // axis set, only generate event if it wasn't set before
        if (!(joy_axis_state[slot] & (1 << event->jaxis.axis))) {
          joy_axis_state[slot] |= (1 << event->jaxis.axis);
          key = SYSTEM_JOY_LEFT + slot * SYSTEM_JOY_OFFSET;
          if (event->jaxis.axis == 1) {
            key += 2;
          }
          if (event->jaxis.value > 0) {
            key++;
            joy_lastaxis[slot] |= (1 << event->jaxis.axis);
          } else {
            joy_lastaxis[slot] &= ~(1 << event->jaxis.axis);
          }
          setKeyState(key, NEBU_INPUT_KEYSTATE_DOWN);
          if (current && current->keyboard)
            current->keyboard(NEBU_INPUT_KEYSTATE_DOWN, key, 0, 0);
        }
      }
      break;

#if 0
		if (abs(event->jaxis.value) <= joystick_threshold * SYSTEM_JOY_AXIS_MAX) {
			skip_axis_event &= ~(1 << event->jaxis.axis);
			break;
		}
		if(skip_axis_event & (1 << event->jaxis.axis))
			break;
		skip_axis_event |= 1 << event->jaxis.axis;
		key = SYSTEM_JOY_LEFT + event->jaxis.which * SYSTEM_JOY_OFFSET;
		if(event->jaxis.axis == 1)
			key += 2;
		if(event->jaxis.value > 0)
			key++;
		setKeyState(key, NEBU_INPUT_KEYSTATE_DOWN);
		if(current && current->keyboard)
			current->keyboard(NEBU_INPUT_KEYSTATE_DOWN, key, 0, 0);
		break;
#endif
    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP:
      slot = joystick_slot(event->jbutton.which);
      if (slot < 0) break;
      state = event->jbutton.down ? NEBU_INPUT_KEYSTATE_DOWN
                                  : NEBU_INPUT_KEYSTATE_UP;
      key = SYSTEM_JOY_BUTTON_0 + event->jbutton.button +
            SYSTEM_JOY_OFFSET * slot;
      setKeyState(key, state);
      if (current && current->keyboard) current->keyboard(state, key, 0, 0);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      SystemMouse(
          event->button.button,
          event->button.down ? SYSTEM_MOUSEPRESSED : SYSTEM_MOUSERELEASED,
          (int)event->button.x, (int)event->button.y);
      break;
    case SDL_EVENT_MOUSE_MOTION:
      SystemMouseMotion((int)event->motion.x, (int)event->motion.y);
      break;
  }
}

void SystemSetJoyThreshold(float f) { joystick_threshold = f; }
