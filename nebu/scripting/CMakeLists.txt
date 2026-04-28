add_library(nebu_scripting STATIC scripting.c)
target_link_libraries(nebu_scripting
  PUBLIC nebu_includes lua lualib
  PRIVATE gltron_warnings)
