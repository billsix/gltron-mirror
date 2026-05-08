#include "base/nebu_system.h"
#include "video/nebu_video_system.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

Callbacks* current = 0;
Callbacks default_callbacks;

static int return_code = -1;
static int redisplay = 0;
static int idle = 1;
static int fps_last = 0;
static int fps_dt = 1;

void nebu_Init(void) {
  memset(&default_callbacks, 0, sizeof(Callbacks));
  current = &default_callbacks;
}

void nebu_System_Exit() {
  /* Don't call SDL_Quit here: the audio/video subsystems still need their
     SDL handles during exitSubsystems(), which runs after the main loop
     returns. SDL_Quit happens later via nebu_System_Shutdown(). */
  fprintf(stderr, "[system] scheduling application exit\n");

  redisplay = 0;
  idle = 0;
}

void nebu_System_Shutdown(void) {
  fprintf(stderr, "[system] shutting down SDL now\n");
  SDL_Quit();
}

int nebu_Time_GetTimeForLastFrame() { return fps_dt; }

unsigned int nebu_Time_GetElapsed() {
  /* SDL3's SDL_GetTicks returns Uint64; truncate to keep the existing
     unsigned-int interface. Differences within a session still fit. */
  return (unsigned int)SDL_GetTicks();
}

static int lastFrame = 0;
void nebu_Time_SetCurrentFrameTime(unsigned t) { lastFrame = t; }

unsigned int nebu_Time_GetElapsedSinceLastFrame() {
  return nebu_Time_GetElapsed() - lastFrame;
}

void nebu_Time_FrameDelay(unsigned int delay) {
  if (nebu_Time_GetElapsedSinceLastFrame() < delay)
    nebu_System_Sleep(delay - nebu_Time_GetElapsedSinceLastFrame());
  // nebu_Time_SetCurrentFrameTime( nebu_Time_GetElapsed() );
}

int nebu_System_MainLoop() {
  SDL_Event event;

  return_code = -1;
  while (return_code == -1) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_MOTION:
          nebu_Intern_HandleInput(&event);
          break;
        case SDL_EVENT_QUIT:
          nebu_System_Exit();       // shut down
          nebu_System_ExitLoop(0);  // exit mainloop
          break;
        default:
          /* ignore event */
          break;
      }
    }
    if (current && current->display && redisplay) {
      current->display();
      redisplay = 0;
    }
    if (current && current->idle && idle) current->idle();
  }
  if (current && current->exit) (current->exit)();
  current = NULL;
  return return_code;
}

void nebu_System_SetCallbacks(Callbacks* cb) {
  if (current && current->exit) (current->exit)();

  current = cb;
  if (current && current->init) current->init();
}

void nebu_System_ExitLoop(int value) { return_code = value; }

void nebu_System_PostRedisplay() { redisplay = 1; }

void nebu_System_SwapBuffers() {
  int now = nebu_Time_GetElapsed();
  fps_dt = now - fps_last;
  fps_last = now;
  nebu_Time_SetCurrentFrameTime(now);
  SDL_Window* window = nebu_Video_GetWindow();
  if (window) {
    SDL_GL_SwapWindow(window);
  }
}

void nebu_System_SetCallback_Display(void (*display)(void)) {
  current->display = display;
}

void nebu_System_SetCallback_Key(void (*keyboard)(int, int, int, int)) {
  current->keyboard = keyboard;
}

void nebu_System_SetCallback_MouseMotion(void (*mouseMotion)(int, int)) {
  current->mouseMotion = mouseMotion;
}

void nebu_System_SetCallback_Idle(void (*idle)(void)) { current->idle = idle; }

void nebu_System_Sleep(int ms) { SDL_Delay(ms); }
