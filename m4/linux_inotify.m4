AC_DEFUN([ARCHON_CHECK_LINUX_INOTIFY], [

  ARCHON_CHECK_LIB([inotify], [support for Linux inotify file system event reporting], [Linux inotify], [ARCHON_HAVE_LINUX_INOTIFY], [], [
    AC_CHECK_HEADER([sys/inotify.h], [
      AC_CHECK_FUNC([inotify_init], [], [archon_not_found=yes])
    ], [archon_not_found=yes])
  ])

])
