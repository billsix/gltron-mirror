add_library(gltron_video STATIC
  artpack.c explosion.c fonts.c gamegraphics.c
  graphics_fx.c graphics_hud.c graphics_lights.c
  graphics_utility.c graphics_world.c level.c
  load_texture.c material.c model.c recognizer.c
  screenshot.c skybox.c texture.c trail.c trail_geometry.c
  trail_render.c video.c visuals_2d.c)
target_link_libraries(gltron_video
  PUBLIC gltron_includes nebu_includes lua OpenGL::GL GLEW::GLEW PkgConfig::SDL2
  PRIVATE gltron_paths gltron_warnings)
