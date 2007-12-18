# Checks for presence of <sys/ioctl.h>, and that <sys/ioctl.h> or
# <termios.h> defines the type 'winsize' and the constant
# 'TIOCGWINSZ'.
#
# Assumes
#  #include <termios.h>
#  #include <sys/ioctl.h>
#
#
AC_DEFUN([ARCHON_CHECK_GET_TERM_SIZE], [

  AC_MSG_CHECKING(for availability of terminal size)
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([
#include <termios.h>
#include <sys/ioctl.h>
#if !(defined(TIOCGWINSZ))
#error
#endif
  ])], [
    AC_MSG_RESULT([yes])
    AC_CHECK_TYPE([struct winsize], [
      AC_DEFINE([ARCHON_HAVE_TERM_SIZE], [1], [Define to 1 if you have access to terminal size via ioctl(fd, TIOCGWINSZ, ...) and want to use it.])
    ], [], [
#include <termios.h>
#include <sys/ioctl.h>
    ])
  ], [
    AC_MSG_FAILURE([no])
  ])
])
