AC_DEFUN([ARCHON_CHECK_LIBREADLINE], [

  ARCHON_LIBREADLINE_CFLAGS=
  ARCHON_LIBREADLINE_LIBS=

  ARCHON_CHECK_LIB([readline],
                   [support for advanced shell-like editing while reading lines from a text terminal],
                   [the Readline library], [ARCHON_HAVE_LIBREADLINE], [ARCHON_LIBREADLINE_], [
    OLD_LIBS="$LIBS"
    LIBS=""
    AX_LIB_READLINE
    AS_IF([test "x$ax_cv_lib_readline" = xno], [archon_not_found=yes], [
      archon_libs="$ax_cv_lib_readline"
    ])
    AC_LINK_IFELSE([AC_LANG_CALL([], [rl_bind_key])], [
      AC_DEFINE([HAVE_READLINE_BIND_KEY], [1], [Define if your readline library has `rl_bind_key'])
    ])
    LIBS="$OLD_LIBS"
  ], [], [no], [no])

  AC_SUBST([ARCHON_LIBREADLINE_CFLAGS])
  AC_SUBST([ARCHON_LIBREADLINE_LIBS])

])
