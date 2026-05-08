#include "audio/nebu_SourceMusic.h"

#include <stdio.h>
#include <string.h>

#include "base/nebu_debug_memory.h"

namespace Sound {

SourceMusic::SourceMusic(System* system) {
  _system = system;

  _sample = NULL;

  _sample_buffersize = 8192;
  _buffersize = 20 * _sample_buffersize;
  _buffer = new Uint8[_buffersize];
  memset(_buffer, 0, _buffersize);

  _decoded = 0;
  _read = 0;

  _filename = NULL;
}

SourceMusic::~SourceMusic() {
  SDL_WaitSemaphore(_sem);
  if (_buffer) delete _buffer;

  if (_sample) {
    Sound_FreeSample(_sample);
    _sample = NULL;
  }

  if (_filename) delete _filename;

  SDL_SignalSemaphore(_sem);
}

/*!
        \fn void SourceMusic::CreateSample(void)

        call this function only between semaphores
*/

void SourceMusic::CreateSample(void) {
  /* SDL3_sound's Sound_NewSampleFromFile detects the extension itself. */
  _sample =
      Sound_NewSampleFromFile(_filename, _system->GetAudioInfo(),
                              _sample_buffersize);

  if (_sample == NULL) {
    fprintf(stderr, "[error] failed loading sample from %s: %s\n",
            _filename, Sound_GetError());
    return;
  }

  _read = 0;
  _decoded = 0;
}

void SourceMusic::Load(char* filename) {
  int n = strlen(filename);
  _filename = new char[n + 1];
  memcpy(_filename, filename, n + 1);
  CreateSample();
}

void SourceMusic::CleanUp(void) {
  _read = 0;
  _decoded = 0;

  if (_sample != NULL) {
    Sound_FreeSample(_sample);
    _sample = NULL;
  }
}

int SourceMusic::Mix(Uint8* data, int len) {
  if (_sample == NULL) return 0;
  /* SDL3 SDL_TryWaitSemaphore returns true if it acquired the semaphore. */
  if (!SDL_TryWaitSemaphore(_sem)) {
    fprintf(stderr, "semaphore locked, skipping mix\n");
    return 0;
  }

  if (len < (_decoded - _read + _buffersize) % _buffersize) {
    if (_read + len <= _buffersize) {
      SDL_MixAudio(data, _buffer + _read, SDL_AUDIO_S16, len, _volume);
      _read = (_read + len) % _buffersize;
    } else {
      fprintf(stderr, "wrap around in buffer (%d, %d, %d)\n", len, _read,
              _buffersize);

      SDL_MixAudio(data, _buffer + _read, SDL_AUDIO_S16,
                   _buffersize - _read, _volume);
      len -= _buffersize - _read;
      SDL_MixAudio(data + _buffersize - _read, _buffer, SDL_AUDIO_S16,
                   len, _volume);
      _read = len;
    }
  } else {
    fprintf(stderr, "buffer underrun!\n");
  }

  SDL_SignalSemaphore(_sem);
  return 1;
}

void SourceMusic::Idle(void) {
  if (_sample == NULL) return;

  // printf("idling\n");
  while (_isPlaying &&
         (_read == _decoded || (_read - _decoded + _buffersize) % _buffersize >
                                   _sample_buffersize)) {
    // if(_read == _decoded)	printf("_read == _decoded == %d\n", _read);
    // fill the buffer
    int count = Sound_Decode(_sample);
    // printf("adding %d bytes to buffer\n", count);
    if (count <= _buffersize - _decoded) {
      memcpy(_buffer + _decoded, _sample->buffer, count);
    } else {
      // wrapping around end of buffer (usually doesn't happen when
      // _buffersize is a multiple of _sample_buffersize)
      // printf("wrapping around end of buffer\n");
      memcpy(_buffer + _decoded, _sample->buffer, _buffersize - _decoded);
      memcpy(_buffer, (Uint8*)_sample->buffer + _buffersize - _decoded,
             count - (_buffersize - _decoded));
    }
    _decoded = (_decoded + count) % _buffersize;

    // check for end of sample, loop
    if ((_sample->flags & SOUND_SAMPLEFLAG_ERROR) ||
        (_sample->flags & SOUND_SAMPLEFLAG_EOF)) {
      SDL_WaitSemaphore(_sem);
      CleanUp();
      if (_loop) {
        if (_loop != 255) _loop--;
        CreateSample();
      } else {
        _isPlaying = 0;
      }
      SDL_SignalSemaphore(_sem);
    }
  }  // buffer has been filled
}
}  // namespace Sound
