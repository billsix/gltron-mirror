add_library(gltron_input STATIC input.c)
target_link_libraries(gltron_input
  PUBLIC gltron_includes nebu_includes lua PkgConfig::SDL3
  PRIVATE gltron_paths gltron_warnings)
