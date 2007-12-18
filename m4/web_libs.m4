AC_DEFUN([ARCHON_CHECK_WEB_LIBS], [

  ARCHON_LIBWWW_CFLAGS=
  ARCHON_LIBWWW_LIBS=

  AC_ARG_VAR([LIBWWW_CONFIG], [script distributed with W3C Libwww for determining compiler and linker flags])

  ARCHON_CHECK_LIB([libwww], [support for W3C Libwww], [the W3C Libwww], [ARCHON_HAVE_LIBWWW], [ARCHON_LIBWWW_], [
    AC_PATH_PROG([LIBWWW_CONFIG], [libwww-config], [no])
    AS_IF([test "x$LIBWWW_CONFIG" != xno], [
      archon_cflags="`$LIBWWW_CONFIG --cflags`"
      archon_libs="`$LIBWWW_CONFIG --libs`"
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_LIBWWW_CFLAGS])
  AC_SUBST([ARCHON_LIBWWW_LIBS])

])
