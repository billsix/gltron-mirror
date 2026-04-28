add_library(gltron_game STATIC
  camera.c computer.c computer_utilities.c credits.c
  engine.c event.c game.c globals.c gui.c
  init.c level.c pause.c
  timedemo.c switchCallbacks.c scripting_interface.c
  resource.c)
target_link_libraries(gltron_game
  PUBLIC gltron_includes nebu_includes lua
  PRIVATE gltron_paths gltron_warnings)
