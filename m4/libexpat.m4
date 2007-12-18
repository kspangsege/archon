AC_DEFUN([ARCHON_CHECK_LIBEXPAT], [

  ARCHON_LIBEXPAT_CFLAGS=
  ARCHON_LIBEXPAT_LIBS=

  ARCHON_CHECK_LIB([expat], [support for the eXpat XML parser], [the eXpat library], [ARCHON_HAVE_LIBEXPAT], [ARCHON_LIBEXPAT_], [
    AC_CHECK_HEADER([expat.h], [
      AC_CHECK_LIB([expatw], [XML_ParserCreate], [
        archon_cflags="-DXML_UNICODE"
        archon_libs="-lexpatw"
      ], [archon_not_found=yes])
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_LIBEXPAT_CFLAGS])
  AC_SUBST([ARCHON_LIBEXPAT_LIBS])

])
