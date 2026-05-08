add_library(nebu_includes INTERFACE)
target_include_directories(nebu_includes INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Several public nebu headers transitively pull in third-party headers:
#   nebu/include/scripting/nebu_scripting.h → "lua.h"
#   nebu/include/base/nebu_system.h         → <SDL3/SDL.h>
# Propagate those include paths to every nebu consumer.
target_link_libraries(nebu_includes INTERFACE lua PkgConfig::SDL3 handmademath)

add_subdirectory(base)
add_subdirectory(filesystem)
add_subdirectory(scripting)
add_subdirectory(audio)
add_subdirectory(video)
add_subdirectory(input)
