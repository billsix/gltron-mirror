add_library(gltron_filesystem STATIC path.c dirsetup.c)
target_link_libraries(gltron_filesystem
  PUBLIC gltron_includes nebu_includes
  PRIVATE gltron_paths gltron_warnings)
