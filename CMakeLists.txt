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

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
# GNU/POSIX extensions enabled to match the autotools default (no -std flag),
# which is what code like nebu/filesystem/findpath.c relies on for realpath();
# with the C23 pin the effective flag is gnu23. C11-or-later is also needed for
# HandmadeMath's _Generic shorthand (see docs/plans/handmademath-port.md). The
# CXX standard is intentionally left unpinned here (C-only pin).
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(GLTRON_WARN "Enable the first-party warning gate" ON)
option(GLTRON_WERROR "Treat first-party warnings as errors" ON)
option(GLTRON_NETWORK "Enable experimental networking (requires SDL3_net)" OFF)
set(GLTRON_SNAPSHOT_DIR "~" CACHE STRING "Screenshot directory (compiled in)")
set(GLTRON_PREFERENCES_DIR "~" CACHE STRING "Preferences directory (compiled in)")

include(GNUInstallDirs)
set(GLTRON_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/gltron"
    CACHE STRING "Data directory baked into the binary")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3>=3.2.0)
pkg_check_modules(SDL3_SOUND REQUIRED IMPORTED_TARGET sdl3-sound)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Lua 5.4 REQUIRED)

# CMake's FindLua exposes variables, not a target. Wrap them so existing
# `target_link_libraries(... lua)` lines keep working.
add_library(lua INTERFACE)
# SYSTEM: Lua is an external dependency; don't apply our raised warnings to its headers.
target_include_directories(lua SYSTEM INTERFACE ${LUA_INCLUDE_DIR})
target_link_libraries(lua INTERFACE ${LUA_LIBRARIES})

# Vendored single-header math library. See third_party/HandmadeMath/ and
# docs/plans/handmademath-port.md.
add_library(handmademath INTERFACE)
# SYSTEM: vendored single-header; consuming it must not trip our raised warnings
# (e.g. gcc -Wpedantic on its GNU extensions). We never modify third_party/.
target_include_directories(handmademath SYSTEM INTERFACE
  ${CMAKE_SOURCE_DIR}/third_party/HandmadeMath)

if(GLTRON_NETWORK)
  pkg_check_modules(SDL3_NET REQUIRED IMPORTED_TARGET sdl3-net)
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

# The first-party warning gate, applied to first-party code only (not vendored
# libs — they come in as PUBLIC deps, never through this target). On by default;
# -DGLTRON_WARN=OFF gives a quiet quick build. -Werror is behind GLTRON_WERROR,
# flipped on once the tree is warning-clean (the C modernization sweep).
#
# Deliberately NOT the -Wconversion umbrella: the sweep keeps sign/narrowing
# conversions but drops int->float (intentional int-coord->float graphics/audio
# math). -Wno-unused-parameter: callback signatures (Lua/libpng/SDL) are
# externally dictated. -Wshorten-64-to-32 / -Wimplicit-int-conversion are
# clang-only; -Wstrict-prototypes / -Wold-style-definition are C-only.
add_library(gltron_warnings INTERFACE)
if(GLTRON_WARN)
  target_compile_options(gltron_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wvla;-Wwrite-strings;-Wformat=2;-Wsign-conversion;-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes;-Wold-style-definition>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:Clang>>:-Wshorten-64-to-32;-Wimplicit-int-conversion>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wshorten-64-to-32;-Wimplicit-int-conversion>)
  if(GLTRON_WERROR)
    target_compile_options(gltron_warnings INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-Werror>)
  endif()
endif()

add_subdirectory(nebu)
add_subdirectory(src)

install(DIRECTORY data/        DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/data    FILES_MATCHING REGEX "\\.(ftx|txt|ogg|wav|obj|mtl|fbmp)$")
install(DIRECTORY art/default/ DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/art/default FILES_MATCHING REGEX "\\.(lua|png)$")
install(DIRECTORY music/       DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/music   FILES_MATCHING REGEX "\\.it$")
install(DIRECTORY scripts/     DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/scripts FILES_MATCHING REGEX "\\.lua$")
install(DIRECTORY sounds/      DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/sounds  FILES_MATCHING REGEX "\\.wav$")
install(DIRECTORY levels/      DESTINATION ${CMAKE_INSTALL_DATADIR}/gltron/levels  FILES_MATCHING REGEX "\\.lua$")

set_property(TARGET gltron PROPERTY APPIMAGE_DESKTOP_FILE
    "${CMAKE_SOURCE_DIR}/packaging/gltron.desktop")
set_property(TARGET gltron PROPERTY APPIMAGE_ICON_FILE
    "${CMAKE_SOURCE_DIR}/art/default/gltron.png")
set_property(TARGET gltron PROPERTY APPIMAGE_APPRUN
    "${CMAKE_SOURCE_DIR}/packaging/AppRun")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_GENERATOR "External" CACHE STRING "")
endif()
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/CMake/Packaging-config.cmake")
include(CMake/Packaging.cmake)

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
