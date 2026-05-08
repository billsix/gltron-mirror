#ifndef NEBU_Sound_System_H
#define NEBU_Sound_System_H

extern "C" {
#include "base/nebu_util.h"
}

#include "audio/nebu_Source.h"
#include <HandmadeMath.h>

#include <SDL3_sound/SDL_sound.h>

namespace Sound {
extern "C" {
/* SDL3 audio-stream callback shape: SDL hands us the stream and asks for at
 * least `additional_amount` more bytes. We render via System::Callback into a
 * scratch buffer and feed it via SDL_PutAudioStreamData. */
void c_callback(void* userdata, SDL_AudioStream* stream,
                int additional_amount, int total_amount);
}

class Listener {
 public:
  Listener() {};
  HMM_Vec3 _location;
  HMM_Vec3 _velocity;
  HMM_Vec3 _direction;
  HMM_Vec3 _up;
};

enum { eUninitialized, eInitialized };

class System {
 public:
  System(SDL_AudioSpec* spec);
  ~System();
  typedef SDL_AudioStreamCallback Audio_Callback;
  Audio_Callback GetCallback() { return c_callback; };
  void Callback(Uint8* data, int len);
  void Idle(); /* remove dead sound sources */
  void AddSource(Source* source);
  /* SDL3_sound replaced Sound_AudioInfo with SDL_AudioSpec. */
  SDL_AudioSpec* GetAudioInfo() { return &_info; };
  Listener& GetListener() { return _listener; };
  void SetMixMusic(int value) { _mix_music = value; };
  void SetMixFX(int value) { _mix_fx = value; };
  void SetStatus(int eStatus) { _status = eStatus; };

 protected:
  SDL_AudioSpec* _spec;
  SDL_AudioSpec _info;
  Listener _listener;
  nebu_List _sources;
  int _mix_music;
  int _mix_fx;
  int _status;
};

}  // namespace Sound

#endif
