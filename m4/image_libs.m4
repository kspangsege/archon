AC_DEFUN([ARCHON_CHECK_IMAGE_LIBS], [

  ARCHON_IMAGE_CFLAGS=
  ARCHON_IMAGE_LIBS=

  ARCHON_CHECK_LIB([png], [support for PNG images], [the PNG library], [ARCHON_HAVE_LIBPNG],  [ARCHON_IMAGE_], [
    PKG_CHECK_EXISTS([libpng], [
      archon_cflags="`$PKG_CONFIG --cflags libpng`"
      archon_libs="`$PKG_CONFIG --libs libpng`"
    ], [
      AC_CHECK_HEADER([png.h], [
        AC_CHECK_LIB([png], [png_create_read_struct], [archon_libs="-lpng -lm -lz"], [archon_not_found=yes], [-lm -lz])
      ], [archon_not_found=yes])
    ])
  ])

  ARCHON_CHECK_LIB([tiff], [support for TIFF images], [the TIFF library], [ARCHON_HAVE_LIBTIFF], [ARCHON_IMAGE_], [
    AC_CHECK_HEADER([tiff.h], [
      AC_CHECK_LIB([tiff], [_TIFFmalloc], [archon_libs="-ltiff -lm"], [archon_not_found=yes], [-lm])
    ], [archon_not_found=yes])
  ])

  ARCHON_CHECK_LIB([pnm], [support for PNM images], [the PNM library], [ARCHON_HAVE_LIBPNM],  [ARCHON_IMAGE_], [
    AC_CHECK_HEADER([pam.h], [
      AC_CHECK_LIB([netpbm], [pm_init], [archon_libs="-lnetpbm"], [archon_not_found=yes])
    ],[archon_not_found=yes])
  ])

  ARCHON_CHECK_LIB([jpeg], [support for JPEG images], [the JPEG library], [ARCHON_HAVE_LIBJPEG], [ARCHON_IMAGE_], [
    AC_CHECK_HEADER([jpeglib.h], [
      AC_CHECK_LIB([jpeg], [jpeg_read_header], [archon_libs="-ljpeg"], [archon_not_found=yes])
    ], [archon_not_found=yes])
  ])


#  ARCHON_CHECK_LIB([gif], [support for GIF images], [the GIF library], [ARCHON_HAVE_LIBGIF],  [ARCHON_IMAGE_], [
#    AC_CHECK_HEADER([gif_lib.h], [
#      AC_CHECK_LIB([gif], [DGifOpenFileName], [archon_libs="-lgif"], [
#        AC_CHECK_LIB([ungif], [DGifOpenFileName], [archon_libs="-lungif"], [archon_not_found=yes])
#      ])
#    ], [archon_not_found=yes])
#  ])

  AC_SUBST([ARCHON_IMAGE_CFLAGS])
  AC_SUBST([ARCHON_IMAGE_LIBS])

])
