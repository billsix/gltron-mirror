#ifndef NEBU_Sound_Source3D_H
#define NEBU_Sound_Source3D_H

#include "nebu_Sound.h"
#include "nebu_SoundSystem.h"
#include "nebu_SourceSample.h"
#include <HandmadeMath.h>

#define USOUND 50
#define EPSILON 0.1f
#define SOUND_VOL_THRESHOLD 0.1
#define VOLSCALE_BASE 1000

namespace Sound {
class Source3D : public Source {
 public:
  Source3D(System* system, SourceSample* source) {
    _system = system;
    _source = source;

    _location = HMM_V3(0, 0, 0);
    _velocity = HMM_V3(0, 0, 0);

    _position = 0;
  };
  HMM_Vec3 _location;
  HMM_Vec3 _velocity;
  SourceSample* _source;

  virtual int Mix(Uint8* data, int len);
  virtual void GetModifiers(float& fPan, float& fVolume, float& fShift);
  //  protected:
  int _position;

 protected:
  Source3D() {
    _location = HMM_V3(0, 0, 0);
    _velocity = HMM_V3(0, 0, 0);

    _position = 0;
  };
};
}  // namespace Sound

#endif
