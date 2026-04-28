add_library(gltron_audio STATIC sound.c sound_glue.cpp)
target_link_libraries(gltron_audio
  PUBLIC gltron_includes nebu_includes lua PkgConfig::SDL2 PkgConfig::SDL2_SOUND
  PRIVATE gltron_paths gltron_warnings)
