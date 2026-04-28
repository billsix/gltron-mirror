add_library(gltron_base STATIC util.c)
target_link_libraries(gltron_base
  PUBLIC gltron_includes nebu_includes lua
  PRIVATE gltron_paths gltron_warnings)
