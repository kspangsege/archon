lib_LTLIBRARIES = libarchon_display_x11.la

libarchon_display_x11_la_SOURCES = implementation.hpp implementation.cpp
libarchon_display_x11_la_LIBADD = \
$(top_builddir)/src/archon/util/libarchon_util.la \
$(top_builddir)/src/archon/thread/libarchon_thread.la \
$(top_builddir)/src/archon/image/libarchon_image.la \
@ARCHON_DISPLAY_LIBS@

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_DISPLAY_CFLAGS@
