AC_DEFUN([ARCHON_CHECK_LIBMAGIC], [

  ARCHON_LIBMAGIC_CFLAGS=
  ARCHON_LIBMAGIC_LIBS=

  ARCHON_CHECK_LIB([magic], [support for MIME magic], [the Magic library], [ARCHON_HAVE_LIBMAGIC], [ARCHON_LIBMAGIC_], [
    AC_CHECK_HEADER([magic.h], [
      AC_CHECK_LIB([magic], [magic_open], [archon_libs="-lmagic"], [archon_not_found=yes])
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_LIBMAGIC_CFLAGS])
  AC_SUBST([ARCHON_LIBMAGIC_LIBS])

])
