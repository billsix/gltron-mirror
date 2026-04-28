add_library(gltron_configuration STATIC settings.c)
target_link_libraries(gltron_configuration
  PUBLIC gltron_includes nebu_includes lua
  PRIVATE gltron_paths gltron_warnings)
