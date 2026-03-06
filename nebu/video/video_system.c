#include "video/nebu_renderer_gl.h"
#include "video/nebu_video_system.h"
#include "base/nebu_system.h"

#include "SDL.h"
#include <assert.h>
#include "base/nebu_debug_memory.h"

/* SDL2 Change: We no longer use a Surface for the screen.
   We need a Window handle and a GL Context handle. */
static SDL_Window* gWindow = NULL;
static SDL_GLContext gContext = NULL;

static int width = 0;
static int height = 0;
static int bitdepth = 0;
static int flags = 0;
static int video_initialized = 0;
static int window_id = 0;

void nebu_Video_Init(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Couldn't initialize SDL video: %s\n", SDL_GetError());
    exit(1);
  } else
    video_initialized = 1;
}

void nebu_Video_SetWindowMode(int x, int y, int w, int h) {
  /* SDL2 actually implements this easily, but keeping your logic as requested
   */
  fprintf(stderr, "ignoring (%d,%d) initial window position\n", x, y);
  width = w;
  height = h;
}

void nebu_Video_GetDimension(int* x, int* y) {
  *x = width;
  *y = height;
}

void nebu_Video_SetDisplayMode(int f) {
  int zdepth;
  flags = f;

  if (!video_initialized) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "[system] can't initialize Video: %s\n", SDL_GetError());
      exit(1);
    }
  }

  /* SDL2 attributes remain similar but are applied to the window context later
   */
  if (flags & SYSTEM_DOUBLE) SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  if (flags & SYSTEM_32_BIT) {
    zdepth = 24;
    bitdepth = 32;
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  } else {
    zdepth = 16;
    bitdepth = 0;
  }

  if (flags & SYSTEM_ALPHA) SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  if (flags & SYSTEM_DEPTH) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zdepth);

  if (flags & SYSTEM_STENCIL)
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  else
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

  video_initialized = 1;
}

void printOpenGLDebugInfo(void) {
  int r, g, b, a;
  fprintf(stderr, "GL vendor: %s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "GL renderer: %s\n", glGetString(GL_RENDERER));
  fprintf(stderr, "GL version: %s\n", glGetString(GL_VERSION));

  nebu_Video_GetDisplayDepth(&r, &g, &b, &a);
  fprintf(stderr, "Bitdepth: R:%d G:%d B:%d A:%d\n", r, g, b, a);
}

void SystemSetGamma(float red, float green, float blue) {
  /* SDL_SetGamma is deprecated in SDL2, but you can use SDL_SetWindowGammaRamp
     or just skip it as most modern OSes block hardware gamma changes. */
  Uint16 r[256], g[256], b[256];
  SDL_CalculateGammaRamp(red, r);
  SDL_CalculateGammaRamp(green, g);
  SDL_CalculateGammaRamp(blue, b);
  if (gWindow) SDL_SetWindowGammaRamp(gWindow, r, g, b);
}

void createWindow(void) {
  /* SDL2 Change: Replace SDL_SetVideoMode with SDL_CreateWindow and
   * SDL_GL_CreateContext */
  Uint32 window_flags = SDL_WINDOW_OPENGL;
  if (flags & SYSTEM_FULLSCREEN) window_flags |= SDL_WINDOW_FULLSCREEN;

  gWindow =
      SDL_CreateWindow("GLTron", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, width, height, window_flags);

  if (gWindow == NULL) {
    fprintf(stderr, "[system] Couldn't create window: %s\n", SDL_GetError());
    exit(1);
  }

  gContext = SDL_GL_CreateContext(gWindow);
  if (gContext == NULL) {
    fprintf(stderr, "[system] Couldn't create GL context: %s\n",
            SDL_GetError());
    exit(1);
  }

  window_id = 1;
}

void nebu_Video_GetDisplayDepth(int* r, int* g, int* b, int* a) {
  SDL_GL_GetAttribute(SDL_GL_RED_SIZE, r);
  SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, g);
  SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, b);
  SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, a);
}

int nebu_Video_Create(char* name) {
  assert(window_id == 0);
  assert(width != 0 && height != 0);

  createWindow();
  glewInit();

  /* Multi-texture check and fallback logic */
  if (!GLEW_ARB_multitexture) {
    printOpenGLDebugInfo();
    nebu_Video_Destroy(window_id);
    nebu_Video_Init();
    nebu_Video_SetDisplayMode(flags);
    fprintf(stderr, "trying without destination alpha\n");
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    createWindow();
    glewInit();
    if (!GLEW_ARB_multitexture) {
      fprintf(stderr, "multitexturing is not available\n");
      exit(1);
    }
  }

  /* SDL2 Change: SDL_WM_SetCaption is now SDL_SetWindowTitle */
  SDL_SetWindowTitle(gWindow, name);
  printOpenGLDebugInfo();

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  /* SDL2 Change: Swapping is now done via the window */
  SDL_GL_SwapWindow(gWindow);

  return window_id;
}

void nebu_Video_Destroy(int id) {
  if (id == window_id) {
    if (gContext) SDL_GL_DeleteContext(gContext);
    if (gWindow) SDL_DestroyWindow(gWindow);
    gContext = NULL;
    gWindow = NULL;
  } else {
    assert(0);
  }
  video_initialized = 0;
  window_id = 0;
}

void SystemReshapeFunc(void (*reshape)(int w, int h)) {
  fprintf(stderr, "reshape feature not supported\n");
}

void nebu_Video_WarpPointer(int x, int y) {
  /* SDL2 Change: SDL_WarpMouse is now window-relative */
  if (gWindow) SDL_WarpMouseInWindow(gWindow, x, y);
}

void nebu_Video_CheckErrors(const char* where) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) printf("[glError: %s] - %d\n", where, error);
}
