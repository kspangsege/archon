AC_DEFUN([ARCHON_APPEND], [
  AS_IF([test "x$$1" != x -a "x$2" != x], [$1="$$1 $2"], [$1="$$1$2"])
])


AC_DEFUN([ARCHON_PREPEND], [
  AS_IF([test "x$$1" != x -a "x$2" != x], [$1="$2 $$1"], [$1="$2$$1"])
])


AC_DEFUN([_ARCHON_CHECK_LIB_1], [
  AS_UNSET([archon_not_found])
  AS_UNSET([archon_cflags])
  AS_UNSET([archon_libs])
  $1
  AS_IF([test "x$archon_not_found" != xyes], [archon_with=yes], [test "x$archon_with" = xyes], [
    AC_MSG_ERROR([$2 was not available], [1])
  ], [
    AC_MSG_WARN([$2 was not available])
    archon_with=no
  ])
])


# Add support for optional library
#
# $1 - single word lower case feature name (png)
# $2 - help string (support for PNG images)
# $3 - qualified library name (PNG library)
# $4 - 'have' name (ARCHON_HAVE_LIBPNG)
# $5 - 'cflags' and 'libs' prefix (ARCHON_IMAGE_) (may be empty)
# $6 - checking code
# $7 - optional prerequisite shell test
# $8 - pass 'no' to suppress the definition of a 'have' C preprocessor macro.
# $9 - pass 'no' to disallow manual specification of compiler and linker flags.
#
# If the checking code does not detect the library, it must set
# 'archon_not_found' to 'yes', otherwise it must set 'archon_cflags',
# and 'archon_libs' to appropriate values.
#
# Assuming that the 'cflags' and 'libs' prefix is 'ARCHON_IMAGE_',
# this macro will append the necessary compiler to the shell variable
# 'ARCHON_IMAGE_CFLAGS', and linker flags to 'ARCHON_IMAGE_LIBS'. If
# the prefix is set to the empty string, then this part is disabled.
#
# Assuming that the single word lower case feature name is 'png', this
# macro will set the shell variable 'archon_with_png' to 'yes' if the
# library was available, otherwise it will set it to 'no'.
#
AC_DEFUN([ARCHON_CHECK_LIB], [
  AC_ARG_WITH([$1], [AS_HELP_STRING([--with-$1]m4_if($9, [no], [], [m4_if($5, [], [], [(=LIBS(,CFLAGS))])]),
                                    [include $2 [default=check]])],
              [archon_with="$with_$1"], [archon_with=check])
  m4_if([m4_normalize([$7])], [], [AS_IF([$7], [], [archon_with=no])])
  AS_IF([test "x$archon_with" != xno], [
    m4_if($9, [no], [
      _ARCHON_CHECK_LIB_1([$6], [$3])
    ], [
      AS_IF([test "x$archon_with" != xyes -a "x$archon_with" != xcheck], [
        AC_REQUIRE([AC_PROG_AWK])
        archon_cflags=`echo ",$archon_with," | $AWK 'BEGIN{FS=","}{print [$]3}'`
        archon_libs=`echo ",$archon_with," | $AWK 'BEGIN{FS=","}{print [$]2}'`
        archon_with=yes
      ], [
        _ARCHON_CHECK_LIB_1([$6], [$3])
      ])
    ])
    AS_IF([test "x$archon_with" = xyes], [
      m4_if($8, [no], [], [
        AC_DEFINE([$4], [1], [Define to 1 if you have $3 and want to use it.])
      ])
      m4_if($5, [], [], [
        ARCHON_PREPEND([$5CFLAGS], [$archon_cflags])
        ARCHON_PREPEND([$5LIBS],   [$archon_libs])
      ])
    ])
  ])
  AM_CONDITIONAL([$4], [test "x$archon_with" = xyes])
  archon_with_$1="$archon_with"
])
