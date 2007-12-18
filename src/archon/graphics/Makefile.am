subincludedir = $(pkgincludedir)/graphics
subinclude_HEADERS = scene_builder.hpp virt_trackball.hpp

lib_LTLIBRARIES = libarchon_graphics.la

libarchon_graphics_la_SOURCES = scene_builder.cpp virt_trackball.cpp
libarchon_graphics_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/math/libarchon_math.la \
$(top_builddir)/src/archon/image/libarchon_image.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
