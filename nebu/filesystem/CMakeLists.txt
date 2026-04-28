add_library(nebu_filesystem STATIC
  filesystem.c file_io.c directory.c findpath.c)
target_link_libraries(nebu_filesystem
  PUBLIC nebu_includes
  PRIVATE gltron_paths gltron_warnings)
