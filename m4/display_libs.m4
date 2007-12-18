AC_DEFUN([ARCHON_CHECK_DISPLAY_LIBS], [

  ARCHON_DISPLAY_CFLAGS=""
  ARCHON_DISPLAY_LIBS=""


  # Check for X
  AC_REQUIRE([AC_PATH_XTRA])
  AS_IF([test "x$no_x" != xyes], [
    ARCHON_PREPEND([ARCHON_DISPLAY_CFLAGS], [$X_CFLAGS])
    ARCHON_PREPEND([ARCHON_DISPLAY_LIBS],   [$X_EXTRA_LIBS])
    ARCHON_PREPEND([ARCHON_DISPLAY_LIBS],   [-lX11])
    ARCHON_PREPEND([ARCHON_DISPLAY_LIBS],   [$X_LIBS])
    ARCHON_PREPEND([ARCHON_DISPLAY_LIBS],   [$X_PRE_LIBS])
    AC_DEFINE([ARCHON_HAVE_XLIB], [1], [Define to 1 if you have the X11 Library and want to use it.])
    archon_with_xlib=yes
  ], [archon_with_xlib=no])
  AS_IF([test "x$no_x" = xyes], [AC_MSG_WARN([X11 Library was not available])])
  AM_CONDITIONAL([ARCHON_HAVE_XLIB], [test "x$no_x" != xyes])


  # Check for OpenGL
  ARCHON_CHECK_LIB([opengl], [support for OpenGL rendering], [the OpenGL library], [ARCHON_HAVE_OPENGL],  [ARCHON_DISPLAY_], [
    AX_CHECK_GL
    AS_IF([test "x$no_gl" = xyes], [archon_not_found=yes], [
      archon_cflags="$GL_CFLAGS"
      archon_libs="$GL_LIBS"
    ])
  ])


  # Check for GLU
  ARCHON_CHECK_LIB([glu], [support for OpenGL rendering], [the GLU library for OpenGL], [ARCHON_HAVE_GLU],  [ARCHON_DISPLAY_], [
    AX_CHECK_GLU
    AS_IF([test "x$no_glu" = xyes], [archon_not_found=yes], [
      archon_cflags="$GLU_CFLAGS"
      archon_libs="$GLU_LIBS"
    ])
  ], [test "x$archon_with_opengl" = xyes])


  # Check for the OpenGL Extension to X (GLX)
  ARCHON_CHECK_LIB([glx], [support for GLX (OpenGL Extension to X)], [the OpenGL Extension to X], [ARCHON_HAVE_GLX],  [ARCHON_DISPLAY_], [
    AC_CHECK_HEADER([GL/glx.h], [
      AC_CHECK_LIB([GL], [glXQueryExtension], [archon_libs="-lGL"], [archon_not_found=yes], [$X_LIBS])
    ], [archon_not_found=yes])
  ], [test "x$archon_with_xlib" = xyes -a "x$archon_with_opengl" = xyes])


  # Check for the Rendering Extension Xrender
  ARCHON_CHECK_LIB([xrender], [support XRender (X Rendering Extension)], [the X Rendering Extension], [ARCHON_HAVE_XRENDER],  [ARCHON_DISPLAY_], [
    AC_CHECK_HEADER([X11/extensions/Xrender.h], [
      AC_CHECK_LIB([Xrender], [XRenderQueryExtension], [archon_libs="-lXrender"], [archon_not_found=yes], [$X_LIBS])
    ], [archon_not_found=yes])
  ], [test "x$archon_with_xlib" = xyes])


  AC_SUBST([ARCHON_DISPLAY_CFLAGS])
  AC_SUBST([ARCHON_DISPLAY_LIBS])
])
