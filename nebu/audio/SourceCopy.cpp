#include "audio/nebu_SourceCopy.h"

#include <assert.h>
#include <stdio.h>

namespace Sound {
int SourceCopy::Mix(Uint8* data, int len) {
  if (_source->_buffer == NULL) return 0;

  float volume = _source->GetVolume();
  int buffersize = _source->_buffersize;
  Uint8* buffer = (Uint8*)_source->_buffer;

  assert(len < buffersize);

  if (len < buffersize - _position) {
    SDL_MixAudio(data, buffer + _position, SDL_AUDIO_S16, len, volume);
    _position += len;
  } else {
    SDL_MixAudio(data, buffer + _position, SDL_AUDIO_S16,
                 buffersize - _position, volume);
    len -= buffersize - _position;

    printf("end of sample reached!\n");
    if (_loop) {
      if (_loop != 255) _loop--;

      _position = 0;
      SDL_MixAudio(data, buffer + _position, SDL_AUDIO_S16, len, volume);
      _position += len;
    } else {
      _isPlaying = 0;
    }
  }
  return 1;
}
}  // namespace Sound
