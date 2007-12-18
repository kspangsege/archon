AC_DEFUN([ARCHON_CHECK_SCRIPT_LIBS], [

  ARCHON_SPIDERMONKEY_CFLAGS=
  ARCHON_SPIDERMONKEY_LIBS=

  AC_ARG_VAR([SPIDERMONKEY_CONFIG], [script distributed with SpiderMonkey for determining compiler and linker flags])

  ARCHON_CHECK_LIB([spidermonkey], [support for SpiderMonkey ECMA script engine], [the SpiderMonkey ECMA script engine], [ARCHON_HAVE_SPIDERMONKEY],  [ARCHON_SPIDERMONKEY_], [
    AC_PATH_PROG([SPIDERMONKEY_CONFIG], [js-config], [no])
    AS_IF([test "x$SPIDERMONKEY_CONFIG" != xno], [
      archon_cflags="`$SPIDERMONKEY_CONFIG --cflags`"
      archon_libs="`$SPIDERMONKEY_CONFIG --libs`"
    ], [archon_not_found=yes])
  ])

  AC_SUBST([ARCHON_SPIDERMONKEY_CFLAGS])
  AC_SUBST([ARCHON_SPIDERMONKEY_LIBS])

])
