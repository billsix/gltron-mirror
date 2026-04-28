add_library(nebu_video STATIC
  2d.c camera.c console.c font.c mesh.c mesh_3ds.c pixels.c
  video_system.c video_utility.c)
target_include_directories(nebu_video PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(nebu_video
  PUBLIC nebu_includes lib3ds OpenGL::GL GLEW::GLEW PkgConfig::SDL2
  PRIVATE gltron_warnings)
