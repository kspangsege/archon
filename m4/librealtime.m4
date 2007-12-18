AC_DEFUN([ARCHON_CHECK_LIBREALTIME], [

  ARCHON_LIBREALTIME_CFLAGS=
  ARCHON_LIBREALTIME_LIBS=

  ARCHON_CHECK_LIB([librt], [support for the POSIX realtime library], [the POSIX realtime library], [ARCHON_HAVE_LIBREALTIME], [ARCHON_LIBREALTIME_], [
    AC_CHECK_LIB([rt], [clock_gettime], [
      archon_libs="-lrt"
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_LIBREALTIME_CFLAGS])
  AC_SUBST([ARCHON_LIBREALTIME_LIBS])

])
