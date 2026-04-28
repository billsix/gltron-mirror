# CPack configuration. Linux uses the External generator to invoke
# appimagetool directly on the cpack-staged install tree.
#
# We deliberately do NOT use linuxdeploy or any patchelf-based dependency
# bundler. Every Fedora 43 lib that linuxdeploy bundled (libSDL2, libSDL2_sound,
# libpng16) crashed at the same offset 0x2cc in dl_init on Bill's host —
# both with the pinned 1-alpha-20240109-1 release and with the much newer
# `continuous` build. The pattern is consistent enough that something about
# the bundling itself corrupts the libs in a way that survives every
# linuxdeploy version we tried. Skipping bundling sidesteps the issue entirely.
#
# The resulting AppImage is a thin wrapper that depends on the host having
# compatible shared libs (Fedora 43-class). That matches the stated scope —
# this AppImage is a build-reliability check, not a portable distribution.
# If wider portability is wanted later, build SDL2/SDL_sound/etc. from source
# in the Dockerfile and bundle those source-built libs (the system F43 libs
# are off-limits because of the corruption issue).

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}"
    CACHE STRING "The resulting package name")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "3D lightcycle game (Tron-style), fixed-function OpenGL"
    CACHE STRING "Package description for the package metadata")

set(CPACK_PACKAGE_VENDOR "gltron")
set(CPACK_PACKAGE_CONTACT "billsix@gmail.com")
set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION
    "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_OUTPUT_FILE_PREFIX "${PROJECT_BINARY_DIR}/_packages")

set(CPACK_EXTERNAL_ENABLE_STAGING YES)
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${PROJECT_BINARY_DIR}/appimage-generate.cmake")

# Generated at configure time; runs at `cpack -G External` time. Downloads
# appimagetool on demand (cached under the build dir), stages AppRun /
# desktop / icon at the AppDir root alongside cpack's installed tree, then
# invokes appimagetool to package the directory.
file(GENERATE
  OUTPUT "${PROJECT_BINARY_DIR}/appimage-generate.cmake"
  CONTENT [[
include(CMakePrintHelpers)
cmake_print_variables(CPACK_TEMPORARY_DIRECTORY)
cmake_print_variables(CPACK_PACKAGE_FILE_NAME)

find_program(APPIMAGETOOL_EXECUTABLE
  NAMES appimagetool appimagetool-x86_64.AppImage
  PATHS ${CPACK_PACKAGE_DIRECTORY}/appimagetool)

if(NOT APPIMAGETOOL_EXECUTABLE)
  message(STATUS "Downloading appimagetool")
  set(APPIMAGETOOL_EXECUTABLE ${CPACK_PACKAGE_DIRECTORY}/appimagetool/appimagetool)
  file(DOWNLOAD
      https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
      ${APPIMAGETOOL_EXECUTABLE}
      INACTIVITY_TIMEOUT 30
      LOG ${CPACK_PACKAGE_DIRECTORY}/appimagetool/download.log
      STATUS APPIMAGETOOL_DOWNLOAD)
  list(GET APPIMAGETOOL_DOWNLOAD 0 AI_STATUS_CODE)
  if(NOT AI_STATUS_CODE EQUAL 0)
    message(FATAL_ERROR "appimagetool download failed: ${APPIMAGETOOL_DOWNLOAD}")
  endif()
  execute_process(COMMAND chmod +x ${APPIMAGETOOL_EXECUTABLE} COMMAND_ECHO STDOUT)
endif()

set(APPDIR "${CPACK_TEMPORARY_DIRECTORY}")

# AppRun at AppDir root.
file(COPY_FILE
  "$<TARGET_PROPERTY:gltron,APPIMAGE_APPRUN>"
  "${APPDIR}/AppRun")
execute_process(COMMAND chmod +x "${APPDIR}/AppRun")

# Desktop file at root and at the standard freedesktop location.
file(COPY_FILE
  "$<TARGET_PROPERTY:gltron,APPIMAGE_DESKTOP_FILE>"
  "${APPDIR}/gltron.desktop")
file(MAKE_DIRECTORY "${APPDIR}/usr/share/applications")
file(COPY_FILE
  "$<TARGET_PROPERTY:gltron,APPIMAGE_DESKTOP_FILE>"
  "${APPDIR}/usr/share/applications/gltron.desktop")

# Icon at root and under hicolor/256x256.
file(COPY_FILE
  "$<TARGET_PROPERTY:gltron,APPIMAGE_ICON_FILE>"
  "${APPDIR}/gltron.png")
file(MAKE_DIRECTORY "${APPDIR}/usr/share/icons/hicolor/256x256/apps")
file(COPY_FILE
  "$<TARGET_PROPERTY:gltron,APPIMAGE_ICON_FILE>"
  "${APPDIR}/usr/share/icons/hicolor/256x256/apps/gltron.png")

execute_process(
  COMMAND
    ${APPIMAGETOOL_EXECUTABLE}
    --appimage-extract-and-run
    --no-appstream
    "${APPDIR}"
    "${CPACK_PACKAGE_FILE_NAME}.appimage"
  RESULT_VARIABLE AI_RESULT)
if(NOT AI_RESULT EQUAL 0)
  message(FATAL_ERROR "appimagetool failed (exit ${AI_RESULT})")
endif()
]])

include(CPack)
