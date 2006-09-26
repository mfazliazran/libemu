AC_PREREQ(2.50)
AC_INIT(libemu, 0.0.1)
AM_INIT_AUTOMAKE(libemu,0.0.1)
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_LIBTOOL_WIN32_DLL
#AC_DISABLE_STATIC
#AM_PROG_LIBTOOL
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_CC(gcc)
AC_PROG_MAKE_SET

# Arguments
AC_ARG_ENABLE(efence,
        [  --enable-efence       enable 'efence' memory debug support (default disabled)],
        [with_efence=$enableval],
        [with_efence='no'])

# Checks for GTK
PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.2) 
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION, :, AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

# Check for efence
if test "$with_efence" = 'yes'
then
  LIB_EFENCE='-lefence'
  LIBS="$LIB_EFENCE $LIBS"
fi

# Check compiler caracteristics
AC_C_CONST
AC_C_INLINE

# Checks for header files.
AC_HEADER_STDC

#AC_CHECK_LIB(iconv, libiconv_open, [EXTRA_LDFLAGS="$EXTRA_LDFLAGS -liconv"])

#AC_CHECK_SIZEOF(char, 1)
#    if test x$ac_cv_sizeof_char = x1; then
#        AC_DEFINE(int8_t, signed char)
#        AC_DEFINE(uint8_t, unsigned char)
#    fi

#    AC_TRY_COMPILE([
#     #include <windows.h>
#    ],[
#    ],[
#    have_win32_gcc=yes
#    ])


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
