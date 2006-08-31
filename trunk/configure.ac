AC_PREREQ(2.50)
AC_INIT(libemu, 0.0.1)
AM_INIT_AUTOMAKE(libemu,0.0.1)
AC_CONFIG_SRCDIR(src/lib/libemu.c)
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.2 glib-2.0 >= 2.2)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION, :, AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
case $OSTYPE in
  *msys*) MMS_BITFIELDS='-mms-bitfields' ;;
  *)      MMS_BITFIELDS='' ;;
esac
AC_SUBST([MMS_BITFIELDS])

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/compiler/Makefile
		 src/compiler/templates/Makefile
                 src/lib/Makefile
                 examples/Makefile
		 examples/tutorial/Makefile
		 examples/atari2600/Makefile
		 src/compiler/emutouch:src/compiler/emutouch.in
		 src/compiler/emucc:src/compiler/emucc.in])
AC_OUTPUT
