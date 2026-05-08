#include "audio/nebu_audio_system.h"

#include <stdio.h>

#include <SDL3/SDL.h>

void nebu_Audio_Init(void) {
  if (!SDL_Init(SDL_INIT_AUDIO)) {
    fprintf(stderr, "Couldn't initialize SDL audio: %s\n", SDL_GetError());
    /* FIXME: disable sound system */
  }
}
