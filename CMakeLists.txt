cmake_minimum_required(VERSION 3.21)
project(gltron VERSION 0.71.6 LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-tree builds are not supported. Run cmake from a separate "
    "build directory, e.g.:\n  cmake -S . -B build")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
# GNU/POSIX extensions enabled to match the autotools default (no -std flag),
# which is what code like nebu/filesystem/findpath.c relies on for realpath().
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(GLTRON_WARN "Enable -Wall -Werror" OFF)
option(GLTRON_NETWORK "Enable experimental networking (requires SDL2_net)" OFF)
set(GLTRON_SNAPSHOT_DIR "~" CACHE STRING "Screenshot directory (compiled in)")
set(GLTRON_PREFERENCES_DIR "~" CACHE STRING "Preferences directory (compiled in)")

include(GNUInstallDirs)
set(GLTRON_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/gltron"
    CACHE STRING "Data directory baked into the binary" FORCE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=2.0.0)
pkg_check_modules(SDL2_SOUND REQUIRED IMPORTED_TARGET SDL2_sound)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

if(GLTRON_NETWORK)
  pkg_check_modules(SDL2_NET REQUIRED IMPORTED_TARGET SDL2_net)
endif()

# Common compile defs for code that touches paths.
add_library(gltron_paths INTERFACE)
target_compile_definitions(gltron_paths INTERFACE
  RC_NAME=".gltronrc"
  DATA_DIR="${GLTRON_DATA_DIR}"
  SNAP_DIR="${GLTRON_SNAPSHOT_DIR}"
  PREF_DIR="${GLTRON_PREFERENCES_DIR}"
  VERSION="${PROJECT_VERSION}")
if(WIN32)
  target_compile_definitions(gltron_paths INTERFACE
    PATH_SEPARATOR='\\\\' SEPARATOR='\\\\')
else()
  target_compile_definitions(gltron_paths INTERFACE
    PATH_SEPARATOR='/' SEPARATOR='/')
endif()

# Common warning flags applied to first-party code only (not vendored libs).
add_library(gltron_warnings INTERFACE)
if(GLTRON_WARN)
  target_compile_options(gltron_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wall -Werror>)
endif()

add_subdirectory(lib3ds)
add_subdirectory(lua5)
add_subdirectory(nebu)
add_subdirectory(src)

install(DIRECTORY data/        DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/data    FILES_MATCHING REGEX "\\.(ftx|txt|ogg|wav|obj|mtl|fbmp)$")
install(DIRECTORY art/default/ DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/art/default FILES_MATCHING REGEX "\\.(lua|png)$")
install(DIRECTORY music/       DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/music   FILES_MATCHING REGEX "\\.it$")
install(DIRECTORY scripts/     DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/scripts FILES_MATCHING REGEX "\\.lua$")
install(DIRECTORY sounds/      DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/sounds  FILES_MATCHING REGEX "\\.wav$")
install(DIRECTORY levels/      DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/levels  FILES_MATCHING REGEX "\\.lua$")

include(CTest)
if(BUILD_TESTING)
  # The game asserts(argc == 1) and otherwise needs a display to run, so a
  # functional smoke test isn't viable without an installed data tree and a
  # display. The most we can verify in CI is that the binary linked and is
  # executable.
  add_test(NAME gltron_binary_exists
           COMMAND ${CMAKE_COMMAND} -E true
           WORKING_DIRECTORY $<TARGET_FILE_DIR:gltron>)
  set_tests_properties(gltron_binary_exists PROPERTIES
    REQUIRED_FILES $<TARGET_FILE:gltron>)
endif()
