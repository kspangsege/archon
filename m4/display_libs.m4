AC_DEFUN([ARCHON_CHECK_DISPLAY_LIBS], [

  ARCHON_DISPLAY_CFLAGS=""
  ARCHON_DISPLAY_LIBS=""


  # Check for Xlib
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
  ARCHON_CHECK_LIB([opengl], [support for OpenGL rendering], [the OpenGL library], [ARCHON_HAVE_OPENGL], [ARCHON_DISPLAY_], [
    AX_CHECK_GL
    AS_IF([test "x$no_gl" = xyes], [archon_not_found=yes], [
      archon_cflags="$GL_CFLAGS"
      archon_libs="$GL_LIBS"
    ])
  ])


  # Check for GLU
  ARCHON_CHECK_LIB([glu], [support for OpenGL rendering], [the GLU library for OpenGL], [ARCHON_HAVE_GLU], [ARCHON_DISPLAY_], [
    AX_CHECK_GLU
    AS_IF([test "x$no_glu" = xyes], [archon_not_found=yes], [
      archon_cflags="$GLU_CFLAGS"
      archon_libs="$GLU_LIBS"
    ])
  ], [test "x$archon_with_opengl" = xyes])


  # Check for the OpenGL Extension to X (GLX)
  ARCHON_CHECK_LIB([glx], [support for GLX (OpenGL Extension to X)], [the OpenGL Extension to X], [ARCHON_HAVE_GLX], [ARCHON_DISPLAY_], [
    AC_CHECK_HEADER([GL/glx.h], [
      AC_CHECK_LIB([GL], [glXQueryExtension], [archon_libs="-lGL"], [archon_not_found=yes], [$X_LIBS])
    ], [archon_not_found=yes])
  ], [test "x$archon_with_xlib" = xyes -a "x$archon_with_opengl" = xyes])


  # Check for the Rendering Extension Xrender
  ARCHON_CHECK_LIB([xrender], [support for XRender (X Rendering Extension)], [the X Rendering Extension], [ARCHON_HAVE_XRENDER], [ARCHON_DISPLAY_], [
    AC_CHECK_HEADER([X11/extensions/Xrender.h], [
      AC_CHECK_LIB([Xrender], [XRenderQueryExtension], [archon_libs="-lXrender"], [archon_not_found=yes], [$X_LIBS])
    ], [archon_not_found=yes])
  ], [test "x$archon_with_xlib" = xyes])


  # Check for the Generic Event Extension to X
  ARCHON_CHECK_LIB([xgevent], [support for Generic Event Extension to X], [the Generic Event Extension to X], [ARCHON_HAVE_X_GENERIC_EVENTS], [], [
    AC_MSG_CHECKING([X Generic Event Extension availability])
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[
        #include <X11/Xlib.h>
      ]], [[
        Display* display;
        XEvent event;
        XGenericEventCookie* cookie = &event.xcookie;
        XNextEvent(display, &event);
        XGetEventData(display, cookie);
        XFreeEventData(display, cookie);
      ]])
    ], [
      AC_MSG_RESULT([yes])
    ], [
      AC_MSG_RESULT([no])
      archon_not_found=yes
    ])
  ], [test "x$archon_with_xlib" = xyes])


  # Check for the advanced input extension to X (Xinput2)
  ARCHON_CHECK_LIB([xinput2], [support for XInput2], [the advanced input extension to X], [ARCHON_HAVE_XINPUT2], [ARCHON_DISPLAY_], [
    AC_CHECK_HEADER([X11/extensions/XInput2.h], [
      AC_CHECK_LIB([Xi], [XIQueryVersion], [archon_libs="-lXi"], [archon_not_found=yes], [$X_LIBS])
    ], [archon_not_found=yes])
  ], [test "x$archon_with_xlib" = xyes])


  AC_SUBST([ARCHON_DISPLAY_CFLAGS])
  AC_SUBST([ARCHON_DISPLAY_LIBS])
])
