add_library(gltron_includes INTERFACE)
target_include_directories(gltron_includes INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(base)
add_subdirectory(filesystem)
add_subdirectory(configuration)
add_subdirectory(audio)
add_subdirectory(video)
add_subdirectory(input)
add_subdirectory(game)

add_executable(gltron gltron.c)
set_target_properties(gltron PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(gltron PRIVATE
  gltron_game
  gltron_input
  gltron_audio
  gltron_video
  gltron_configuration
  gltron_base
  gltron_filesystem
  nebu_input
  nebu_audio
  nebu_video
  nebu_scripting
  nebu_filesystem
  nebu_base
  lua
  OpenGL::GL
  GLEW::GLEW
  PNG::PNG
  ZLIB::ZLIB
  PkgConfig::SDL2
  PkgConfig::SDL2_SOUND
  gltron_paths
  gltron_warnings)

if(GLTRON_NETWORK)
  target_compile_definitions(gltron PRIVATE NETWORK)
  target_link_libraries(gltron PRIVATE PkgConfig::SDL2_NET)
endif()

install(TARGETS gltron RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
