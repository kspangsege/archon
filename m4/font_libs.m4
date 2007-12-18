AC_DEFUN([ARCHON_CHECK_FONT_LIBS], [

  ARCHON_FONT_CFLAGS=
  ARCHON_FONT_LIBS=

  AC_ARG_VAR([FREETYPE_CONFIG], [script distributed with FreeType for determining compiler and linker flags])

  ARCHON_CHECK_LIB([freetype], [support for FreeType font loader], [the FreeType library], [ARCHON_HAVE_FREETYPE],  [ARCHON_FONT_], [
    AC_PATH_PROG([FREETYPE_CONFIG], [freetype-config], [no])
    AS_IF([test "x$FREETYPE_CONFIG" != xno], [
      archon_cflags="`$FREETYPE_CONFIG --cflags` -I`$FREETYPE_CONFIG --prefix`/include"
      archon_libs="`$FREETYPE_CONFIG --libs`"
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_FONT_CFLAGS])
  AC_SUBST([ARCHON_FONT_LIBS])

])
