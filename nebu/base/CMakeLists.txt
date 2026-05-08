add_library(nebu_base STATIC
  nebu_geom.c
  random.c util.c system.c surface.c png_texture.c
  debug_memory.c)
target_link_libraries(nebu_base
  PUBLIC nebu_includes PNG::PNG ZLIB::ZLIB PkgConfig::SDL3
  PRIVATE gltron_warnings)
