add_library(lib3ds STATIC
  io.c float.c vector.c matrix.c quat.c tcb.c ease.c chunk.c file.c
  background.c atmosphere.c shadow.c viewport.c material.c mesh.c
  camera.c light.c tracks.c node.c)

# Public consumers do `#include <lib3ds/foo.h>`, so the source root must be
# on their include path. Private: the source dir itself, so the empty
# lib3ds/config.h satisfies `#include <config.h>` from these files.
target_include_directories(lib3ds
  PUBLIC ${CMAKE_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
