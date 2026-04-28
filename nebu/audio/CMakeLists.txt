add_library(nebu_audio STATIC
  SoundSystem.cpp Source.cpp Source3D.cpp SourceCopy.cpp
  SourceEngine.cpp SourceMusic.cpp SourceSample.cpp
  audio_system.c)
target_link_libraries(nebu_audio
  PUBLIC nebu_includes PkgConfig::SDL2 PkgConfig::SDL2_SOUND
  PRIVATE gltron_warnings)
