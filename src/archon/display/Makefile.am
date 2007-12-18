SUBDIRS =
if ARCHON_HAVE_XLIB
  SUBDIRS += x11
endif
SUBDIRS += . test

subincludedir = $(pkgincludedir)/display
subinclude_HEADERS = implementation.hpp geometry.hpp connection.hpp event.hpp drawable.hpp \
window.hpp pixel_buffer.hpp context.hpp keysyms.hpp

lib_LTLIBRARIES = libarchon_display.la

libarchon_display_la_SOURCES = implementation.cpp
libarchon_display_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/image/libarchon_image.la
if ARCHON_HAVE_XLIB
  libarchon_display_la_LIBADD += x11/libarchon_display_x11.la
endif

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
