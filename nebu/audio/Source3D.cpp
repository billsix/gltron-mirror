#include "audio/nebu_Source3D.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

Uint8 tmp[65536];

#define USOUND 50
#define EPSILON 0.1f
#define SOUND_VOL_THRESHOLD 0.1
#define VOLSCALE_BASE 1000

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

/*!
  \fn int fxShift(float shift, Uint8 *target, Uint8 *source, int len)

  \param shift   Shift of the frequency (new_frequency = shift * old_frequency)
  \param target  Target buffer to mix into; format: Sint16 LR stereo
  \param source  The buffer to mix from
  \param len     The amount of bytes to mix into the target buffer

  \return        The amount of samples read from the source
*/

int fxComputeShiftLen(float shift, int len) {
  return (int)((len - 1) * shift + 1);
}

int fxShift(float shift, Uint8* target, Uint8* source, int len) {
  int i, j;

  // amount of samples to mix/write
  len /= 4;

  for (i = 0; i < len; i++) {  // LR pairs
    for (j = 0; j < 2; j++) {  // channels
      Sint32 result = 0;
      int pa;
      float t;

      pa = (int)(i * shift);
      t = (i * shift) - pa;

      result = (Sint32)(*((Sint16*)target + j + 2 * i) * 1.0f +
                        *((Sint16*)source + j + 2 * (pa + 0)) * (1.0f - t) +
                        *((Sint16*)source + j + 2 * (pa + 1)) * (t));

#define MAX_SINT16 ((1 << 15) - 1)
#define MIN_SINT16 (-(1 << 15))
      if (result > MAX_SINT16) {
        result = MAX_SINT16;
        fprintf(stderr, "overflow\n");
      } else if (result < MIN_SINT16) {
        result = MIN_SINT16;
        fprintf(stderr, "underflow\n");
      }

      *((Sint16*)target + j + 2 * i) = (Sint16)result;
      /* *(Sint16*) (target + 2 * j + 4 * i) += (Sint16)
         ( *(Sint16*) (source + 2 * j + 4 * (k + 0) ) * ( 1 - l ) +
         *(Sint16*) (source + 2 * j + 4 * (k + 2) ) * ( l ) ); */
    }
  }
  return 4 * fxComputeShiftLen(shift, len);
}

/*!
  \fn void fxPan(float pan, float vol, Uint8 *buf, int len)

  \param vol   Volume (for distance attenuation), should be between 0 and 1
  \param pan   Panning, -1.0 is left, 1.0 is right, 0.0 is center
  \param buf   The sample to be modified in-place, format: Sint16 LR stereo
  \param len   Number of bytes
*/

void fxPan(float pan, float vol, Uint8* buf, int len) {
  int i;

  float left_vol = -vol * (-1.0f + pan) / 2.0f;
  float right_vol = vol * (1.0f + pan) / 2.0f;

  for (i = 0; i < len; i += 4) {
    *(Sint16*)(buf + i) =  // *= left_vol
        (Sint16)(left_vol * *(Sint16*)(buf + i));
    *(Sint16*)(buf + i + 2) =  // *= right_vol
        (Sint16)(right_vol * *(Sint16*)(buf + i + 2));
  }
}

namespace Sound {

void Source3D::GetModifiers(float& fPan, float& fVolume, float& fShift) {
  /* HMM port note: gltron's old Vector3 operator* was the dot product;
     HMM's HMM_Vec3 operator* is component-wise multiply. Every dot here
     is therefore spelled explicitly with HMM_DotV3. */

  HMM_Vec3 vSourceLocation = _location;
  HMM_Vec3 vSourceVelocity = _velocity;

  Listener listener = _system->GetListener();
  HMM_Vec3 vListenerLocation = listener._location;
  HMM_Vec3 vListenerVelocity = listener._velocity;
  HMM_Vec3 vListenerDirection = listener._direction;
  HMM_Vec3 vListenerUp = listener._up;

  if (HMM_LenV3(vSourceLocation - vListenerLocation) < EPSILON) {
    fPan = 0;
    fVolume = 1.0f;
    fShift = 1.0f;
    return;
  }

  vListenerDirection = HMM_NormV3(vListenerDirection);
  vListenerUp = HMM_NormV3(vListenerUp);
  HMM_Vec3 vListenerLeft =
      HMM_NormV3(HMM_Cross(vListenerDirection, vListenerUp));

  /* panning */
  HMM_Vec3 vTarget = vSourceLocation - vListenerLocation;
  HMM_Vec3 v1 = vListenerLeft * HMM_DotV3(vTarget, vListenerLeft);
  HMM_Vec3 v2 = vListenerDirection * HMM_DotV3(vTarget, vListenerDirection);
  HMM_Vec3 vTargetPlanar = v1 + v2;

  vTargetPlanar = HMM_NormV3(vTargetPlanar);
  float cosPhi = HMM_DotV3(vTargetPlanar, vListenerDirection);

  fPan = 1 - (float)fabs(cosPhi);

  if (HMM_DotV3(vTargetPlanar, vListenerLeft) < 0) fPan = -fPan;

  /* done panning */

  /* attenuation */
  float vTargetLen = HMM_LenV3(vTarget);
  float fallOff = (float)pow(vTargetLen, 1.8f);
  fVolume = (fallOff > VOLSCALE_BASE) ? (VOLSCALE_BASE / fallOff) : (1.0f);

  /* done attenuation */

  /* doppler */

  fShift = (USOUND + HMM_DotV3(vListenerVelocity, vTarget) / vTargetLen) /
           (USOUND + HMM_DotV3(vSourceVelocity, vTarget) / vTargetLen);
  if (fShift < 0.5) {
    printf("clamping fShift from %.2f to 0.5\n", fShift);
    fShift = 0.5f;
  }
  if (fShift > 1.5) {
    printf("clamping fShift from %.2f to 1.5\n", fShift);
    fShift = 1.5f;
  }

  /* done doppler */
}

int Source3D::Mix(Uint8* data, int len) {
  if (_source->_buffer == NULL) return 0;

  if (_source->IsPlaying()) {
    // TODO: find out if source volume is handled correctly
    // int volume = (int)(_source->GetVolume() * SDL_MIX_MAXVOLUME);
    float pan = 0, shift = 1.0f, vol = 1.0f;
    int clen, shifted_len;

    GetModifiers(pan, vol, shift);
    // printf("received: volume: %.4f, panning: %.4f, shift: %.4f\n", vol, pan,
    // shift);

    shifted_len = 4 * fxComputeShiftLen(shift, len / 4);
    clen = MAX(len, shifted_len) + 32;  // safety distance

    assert(clen < _source->_buffersize);

    if (vol > SOUND_VOL_THRESHOLD) {
      // copy clen bytes from the buffer to a temporary buffer
      if (clen <= _source->_buffersize - _position) {
        memcpy(tmp, _source->_buffer + _position, clen);
      } else {
        memcpy(tmp, _source->_buffer + _position,
               _source->_buffersize - _position);
        memcpy(tmp + _source->_buffersize - _position, _source->_buffer,
               clen - (_source->_buffersize - _position));
      }

      fxPan(pan, vol, tmp, clen);
      // fxshift mixes the data to the stream
      _position += fxShift(shift, data, tmp, len);

      if (_position > _source->_buffersize) _position -= _source->_buffersize;

      return 1;  // mixed something
    }
  }
  return 0;  // didn't mix anything to the stream
}
}  // namespace Sound
