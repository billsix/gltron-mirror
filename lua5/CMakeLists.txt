add_library(lua STATIC
  lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c
  lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltests.c
  ltm.c lundump.c lvm.c lzio.c)
target_include_directories(lua
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lua PUBLIC m)

add_library(lualib STATIC
  lib/lauxlib.c lib/lbaselib.c lib/ldblib.c lib/liolib.c lib/lmathlib.c
  lib/loadlib.c lib/lstrlib.c lib/ltablib.c)
target_include_directories(lualib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lualib PUBLIC lua ${CMAKE_DL_LIBS})
