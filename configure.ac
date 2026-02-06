dnl Process this file with autoconf to produce a configure script.

dnl initialisation
AC_INIT([gltron], [0.71-beta-6])
AC_CONFIG_SRCDIR([src/gltron.c])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_PREFIX_DEFAULT("/usr/local")

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG

dnl CFLAGS set-up
CFLAGS="-DRC_NAME='\".gltronrc\"' -DPATH_SEPARATOR=\"'/'\" -DSEPARATOR=\"'/'\" $CFLAGS"

AC_ARG_ENABLE(warn,
AS_HELP_STRING([--enable-warn],[Enable compiler warnings @<:@default=off@:>@]),
	enable_warn=$enableval, enable_warn=no)
if test "x$enable_warn" = xyes; then
	CFLAGS="$CFLAGS -Wall -Werror"
fi

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],[Enable debugging @<:@default=off@:>@]),
	enable_debug=$enableval, enable_debug=off)
if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -g3"
	CXXFLAGS="$CXXFLAGS -g3"
fi

AC_ARG_ENABLE(profile,
AS_HELP_STRING([--enable-profile],[Enable profiling @<:@default=off@:>@]),
	enable_profile=$enableval, enable_profile=off)
if test "x$enable_profile" = xyes; then
	if test "x$GCC" = xyes; then
		CFLAGS="$CFLAGS -pg"
	else
		CFLAGS="$CFLAGS -p"
	fi
fi

AC_ARG_ENABLE(optimize,
AS_HELP_STRING([--enable-optimize=@<:@level@:>@],[Enable optimization @<:@default=s@:>@]),
	enable_optimize=$enableval, enable_optimize=s)
if test "x$enable_optimize" ; then
	CFLAGS="$CFLAGS -O$enable_optimize"
	CXXFLAGS="$CXXFLAGS -O$enable_optimize"
fi

dnl library checks
PKG_CHECK_MODULES([SDL2], [sdl2 >= 2.0.0], [
    CFLAGS="$CFLAGS $SDL2_CFLAGS"
    CXXFLAGS="$CXXFLAGS $SDL2_CFLAGS"
    LIBS="$LIBS $SDL2_LIBS"
], [AC_MSG_ERROR([SDL2 is not installed])])


PKG_CHECK_MODULES([SDL2_sound], [sdl2_sound >= 2.0.0], [
    CFLAGS="$CFLAGS $SDL2_CFLAGS"
    CXXFLAGS="$CXXFLAGS $SDL2_SOUND_CFLAGS"
    LIBS="$LIBS $SDL2_SOUND_LIBS"
], [AC_MSG_ERROR([SDL2 is not installed])])


dnl OpenGL check - handle macOS frameworks
case "${host_os}" in
    darwin*)
        LIBS="$LIBS -framework OpenGL"
        AC_MSG_NOTICE([Using OpenGL framework on macOS])
        ;;
    *)
        AC_CHECK_LIB(GL, glBegin,, AC_MSG_ERROR(OpenGL is not installed))
        ;;
esac

AC_CHECK_LIB(GLEW, glewInit,, AC_MSG_ERROR(GLEW is not installed))
AC_CHECK_LIB(z, gzopen,, AC_MSG_ERROR(libz is not installed))
AC_CHECK_LIB(png, png_read_info,, AC_MSG_ERROR(libpng is not installed))

dnl local data dir setup
AC_ARG_ENABLE(localdata,
AS_HELP_STRING([--enable-localdata],[Use local data directory @<:@default=no@:>@]),
	CPPFLAGSA="$CPPFLAGSA -DLOCAL_DATA",
	CPPFLAGSA="$CPPFLAGSA -DDATA_DIR=\\\"\$(datadir)/\$(PACKAGE)\\\"")

dnl snapshot dir setup
AC_ARG_WITH(snapshot-dir,
AS_HELP_STRING([--with-snapshot-dir=@<:@path@:>@],[Snapshot directory @<:@default=~@:>@]),
	CPPFLAGSA="$CPPFLAGSA -DSNAP_DIR=\\\"$withval\\\"",
	CPPFLAGSA="$CPPFLAGSA -DSNAP_DIR=\\\"~\\\"")

dnl preferences dir setup
AC_ARG_WITH(preferences-dir,
AS_HELP_STRING([--with-preferences-dir=@<:@path@:>@],[Preferences directory @<:@default=~@:>@]),
	CPPFLAGSA="$CPPFLAGSA -DPREF_DIR=\\\"$withval\\\"",
	CPPFLAGSA="$CPPFLAGSA -DPREF_DIR=\\\"~\\\"")

dnl sound set-up
AC_ARG_ENABLE(sound,
AS_HELP_STRING([--disable-sound],[Don't compile in sound @<:@default=off@:>@]),
  use_sound=$enableval, use_sound=yes)

if test "x$use_sound" = xyes; then
      PKG_CHECK_MODULES([SDL2_SOUND], [SDL2_sound], [
        CFLAGS="$CFLAGS $SDL2_SOUND_CFLAGS"
        LIBS="-lSDL2_sound $SDL2_SOUND_LIBS $LIBS"
        SOUND_OBJS="sound.o sound_glue.o Source.o Source3D.o SourceMusic.o SourceSample.o SoundSystem.o SourceCopy.o SourceEngine.o"
    ], [
        AC_MSG_WARN([SDL2_sound not found - disabling sound])
        SOUND_OBJS="sound_stubs.o sound_glue_stubs.o"
    ])
    AC_SUBST(SOUND_OBJS)
else
    SOUND_OBJS="sound_stubs.o sound_glue_stubs.o"
    AC_SUBST(SOUND_OBJS)
fi

dnl network set-up
AC_ARG_ENABLE(network,
AS_HELP_STRING([--enable-network],[Experimental network mode @<:@default=off@:>@]),
	enable_network=$enableval, enable_network=off)
if test "x$enable_network" = xyes; then
	PKG_CHECK_MODULES([SDL2_NET], [SDL2_net], [
        CFLAGS="$CFLAGS -DNETWORK $SDL2_NET_CFLAGS"
        LIBS="$LIBS $SDL2_NET_LIBS"
        NETWORK_OBJS="network.o system_net.o"
    ], [
        AC_MSG_WARN([SDL2_net not found - disabling network])
    ])
	AC_SUBST(NETWORK_OBJS)
fi

dnl header checks
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP
AC_CHECK_HEADERS(unistd.h)


dnl OpenGL header check - handle macOS frameworks
have_gl_h=false
case "${host_os}" in
    darwin*)
        AC_CHECK_HEADERS(OpenGL/gl.h, have_gl_h=true, have_gl_h=false)
        if test "x$have_gl_h" = xtrue; then
            AC_MSG_NOTICE([Using OpenGL headers from framework on macOS])
        fi
        ;;
    *)
        AC_CHECK_HEADERS(GL/gl.h, have_gl_h=true, have_gl_h=false)
        if test "x$have_gl_h" = xfalse; then
            AC_CHECK_HEADERS(/usr/X11R6/include/GL/gl.h, includes_in_x=1 have_gl_h=true, have_gl_h=false)
        fi
        if test "x$have_gl_h" = xfalse; then
            AC_CHECK_HEADERS(/usr/local/include/GL/gl.h, includes_in_local=1 have_gl_h=true, have_gl_h=false)
        fi
        ;;
esac

if test "x$have_gl_h" = xfalse; then
    AC_MSG_ERROR(cant find GL headers)
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_CHECK_FUNCS(strstr mkstemp)

CPPFLAGS="$CPPFLAGS $CPPFLAGSA"

AC_CONFIG_FILES([Makefile \
	lib3ds/Makefile \
	lua5/Makefile \
	lua5/include/Makefile \
	lua5/lib/Makefile \
	nebu/Makefile \
	nebu/include/Makefile \
	nebu/include/audio/Makefile \
	nebu/include/base/Makefile \
	nebu/include/filesystem/Makefile \
	nebu/include/input/Makefile \
	nebu/include/scripting/Makefile \
	nebu/include/video/Makefile \
	nebu/base/Makefile \
	nebu/filesystem/Makefile \
	nebu/scripting/Makefile \
	nebu/input/Makefile \
	nebu/video/Makefile \
	nebu/audio/Makefile \
	src/Makefile \
	src/include/Makefile \
	src/include/audio/Makefile \
	src/include/base/Makefile \
	src/include/base/config.h \
	src/include/configuration/Makefile \
	src/include/filesystem/Makefile \
	src/include/game/Makefile \
	src/include/input/Makefile \
	src/include/scripting/Makefile \
	src/include/video/Makefile \
	src/base/Makefile \
	src/filesystem/Makefile \
	src/configuration/Makefile \
	src/audio/Makefile \
	src/video/Makefile \
	src/input/Makefile \
	src/game/Makefile \
	art/Makefile \
	art/default/Makefile \
	data/Makefile \
	music/Makefile \
	sounds/Makefile \
	scripts/Makefile \
	levels/Makefile \
	])
AC_OUTPUT
