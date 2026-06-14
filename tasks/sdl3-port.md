# Port from SDL2 to SDL3

**Status:** **shipped 2026-05-08** on Linux. macOS path not retested.

## What landed

- `CMakeLists.txt` and per-subdir CMakeLists swapped from
  `pkg_check_modules(... sdl2/SDL2_sound)` to `sdl3/sdl3-sound` and
  every `PkgConfig::SDL2*` target was renamed to `PkgConfig::SDL3*`.
- `Dockerfile` package list now installs `SDL3 SDL3-devel SDL3_sound
  SDL3_sound-devel` (Fedora 44 ships SDL3 3.4.8 and SDL3_sound).
- All `#include "SDL.h"` etc. were rewritten to the SDL3 paths
  (`<SDL3/SDL.h>`, `<SDL3/SDL_opengl.h>`, `<SDL3_sound/SDL_sound.h>`).
- `nebu/include/base/nebu_system.h` now explicitly pulls in `<stdio.h>`
  and `<stdlib.h>` because SDL3's `<SDL.h>` no longer transitively
  exposes them; many consumers in `src/game/` had relied on the SDL2
  behaviour.

### Subsystem-level changes

- **Video** (`nebu/video/video_system.c`):
  - `SDL_CreateWindow` lost its `(x, y)` arguments.
  - `SDL_GL_DeleteContext` → `SDL_GL_DestroyContext`.
  - `SDL_WarpMouseInWindow` takes floats now.
  - Bool-returning `SDL_Init` / `SDL_InitSubSystem` checked with `!`
    instead of `< 0`.
  - `SystemSetGamma` is now a no-op: SDL3 dropped
    `SDL_CalculateGammaRamp` and `SDL_SetWindowGammaRamp`, and modern
    compositors don't honour per-window hardware gamma anyway.
- **Input** (`nebu/input/input_system.c`):
  - `SDL_NumJoysticks` / `SDL_JoystickOpen(index)` →
    `SDL_GetJoysticks(&count)` returning instance IDs +
    `SDL_OpenJoystick(instance_id)`.
  - Joystick events now carry `SDL_JoystickID` instance IDs in
    `event.jaxis.which` / `event.jbutton.which`; we keep a small
    `joystick_ids[MAX_JOY]` table to map instance IDs back to a dense
    0..N-1 slot the `SYSTEM_JOY_OFFSET` arithmetic still expects.
  - `event.key.keysym.sym` → `event.key.key`. Matching event-type rename
    `SDL_KEYDOWN` → `SDL_EVENT_KEY_DOWN`, etc.
  - `SDL_SetWindowGrab` → `SDL_SetWindowMouseGrab` (bool param).
  - `SDL_ShowCursor(state)` split into `SDL_ShowCursor()` /
    `SDL_HideCursor()`.
  - Mouse coords (`event.button.x/y`, `event.motion.x/y`) are floats now;
    we cast back to int at the call site.
  - `SDL_PRESSED` / `SDL_RELEASED` are gone; mouse buttons expose
    `down: bool`. The `SYSTEM_MOUSEPRESSED` / `SYSTEM_MOUSERELEASED`
    macros in `nebu_input_system.h` are now plain `1`/`0` literals so
    callers in `src/game/game.c` keep compiling unchanged.
  - `SDLK_ENTER` is not in SDL3; `SYSTEM_KEY_ENTER` aliases `SDLK_RETURN`.
- **Base** (`nebu/base/system.c`, `nebu/base/surface.c`):
  - `SDL_GetTicks()` returns `Uint64` in SDL3; cast to `unsigned int` at
    `nebu_Time_GetElapsed` to keep the legacy interface.
  - Event-type renames in the main loop's switch.
  - `SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24, R/G/B/Amask)` →
    `SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGB24)`.
  - `SDL_FreeSurface` → `SDL_DestroySurface`.
  - **New:** `nebu_System_Shutdown()` calls `SDL_Quit()`. The previous
    behaviour — `nebu_System_Exit()` calling `SDL_Quit()` from inside
    the main loop's `SDL_QUIT` handler — caused a use-after-free on
    SDL3 because the audio stream and video objects were still being
    destroyed in `exitSubsystems()` afterwards. `SDL_Quit` is now
    deferred to the very end of `exitSubsystems()` in
    `src/game/init.c`.
- **Audio** (`nebu/audio/*.{cpp,c}`, `src/audio/sound_glue.cpp`):
  - SDL3's `SDL_AudioSpec` is `{format, channels, freq}` — no
    `samples`/`silence`/`size`/`callback`/`userdata` fields. The
    callback is wired through `SDL_OpenAudioDeviceStream` instead.
  - `c_callback` was rewritten as an `SDL_AudioStreamCallback`
    `(userdata, stream, additional_amount, total_amount)`. It chunks
    the request through a stack buffer, runs the existing
    `Sound::System::Callback` mixer, and feeds the result to the
    stream via `SDL_PutAudioStreamData`.
  - `Audio_Init` opens the device with
    `SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, ...)`.
    `Audio_Start` / `Audio_Quit` use
    `SDL_ResumeAudioStreamDevice` / `SDL_PauseAudioStreamDevice` /
    `SDL_DestroyAudioStream`. `SDL_OpenAudio` / `SDL_PauseAudio` /
    `SDL_CloseAudio` were removed in SDL3.
  - `AUDIO_S16SYS` → `SDL_AUDIO_S16`.
  - `SDL_MixAudio(dst, src, len, int_volume)` →
    `SDL_MixAudio(dst, src, format, len, float_volume)`. We pass
    `_volume` (already 0..1) directly and stop multiplying by the
    deleted `SDL_MIX_MAXVOLUME`.
  - `SDL_mutex` → `SDL_Mutex`, `SDL_sem` → `SDL_Semaphore`. Function
    renames: `SDL_SemWait/Post/TryWait` →
    `SDL_WaitSemaphore/SignalSemaphore/TryWaitSemaphore`.
    `SDL_TryWaitSemaphore` returns `true` when it acquires the
    semaphore (inverted from `SDL_SemTryWait`'s 0-success).
- **SDL_sound → SDL3_sound** (`nebu/audio/SourceMusic.cpp`,
  `SourceSample.cpp`, headers):
  - `Sound_AudioInfo` was replaced with `SDL_AudioSpec`. The `_info`
    member of `Sound::System` switched type accordingly; `_info.rate`
    became `_info.freq`.
  - `Sound_NewSample(SDL_RWops*, ext, info, size)` calls were rewritten
    to `Sound_NewSampleFromFile(filename, spec, size)`, which lets
    SDL3_sound handle the `SDL_IOStream` and extension detection
    internally. The `_rwops` member was dropped from `SourceMusic`.
  - The `#ifndef macintosh` `SDL_LockAudio` / `SDL_UnlockAudio`
    fallbacks were removed (those APIs are gone in SDL3); the
    semaphore branch is now unconditional.

### Dead code left in place

- `nebu/audio/music_rwop.c` is not built by any CMakeLists and still
  references SDL2 `SDL_RWops`. Trim it as part of the broader
  "reduce nebu surface" task in
  [modernization-survey.md](modernization-survey.md), not here.

## Smoke test result

The Fedora 44 container build configures, compiles, links against
`libSDL3.so.0` + `libSDL3_sound.so.0`, and runs:

- The window opens, the GL 4.5 compatibility-profile context comes up
  via Mesa llvmpipe.
- Texture, font, level, art-pack, and lua loading all complete.
- Music decoder loads `song_revenge_of_cats.it`; some "buffer
  underrun!" lines print during startup while the music decoder fills
  its buffer, which matches the existing pre-port behaviour.
- The menu reaches `StartGame` and the game asks for video reset.
- On exit, the new `nebu_System_Shutdown()` ordering eliminates the
  previous `corrupted size vs. prev_size` heap abort.

The container does not have an interactive display attached for full
gameplay, so the "30s of AI match" verification from
`modernization-survey.md` was not exercised. Run that on the host to
finalise the smoke test.

## Follow-ups not in scope here

- macOS rebuild + the SDL2-sound hack referenced in commit `21113968`
  may need an SDL3 equivalent.
- The "buffer underrun" prints from `SourceMusic::Mix` are not new but
  worth a closer look — the message fires whenever the decoder hasn't
  filled enough buffer for the next mixer chunk. They appear during
  music load and may also appear under audio-thread pressure.
- AppImage packaging (`tasks/archive/2026/04/27/appimage-target.md`) was previously
  bundling the host's SDL2-compat-shim; with a real SDL3 binary the
  bundling logic should be revisited.
