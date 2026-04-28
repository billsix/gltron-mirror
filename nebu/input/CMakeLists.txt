add_library(nebu_input STATIC
  system_keynames.c input_system.c)
target_link_libraries(nebu_input
  PUBLIC nebu_includes PkgConfig::SDL2
  PRIVATE gltron_warnings)
