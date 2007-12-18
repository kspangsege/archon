SUBDIRS = . test

subincludedir = $(pkgincludedir)/render
subinclude_HEADERS = context.hpp conductor.hpp view.hpp

lib_LTLIBRARIES = libarchon_render.la

libarchon_render_la_SOURCES = implementation.cpp
libarchon_render_la_LIBADD = \
$(top_builddir)/src/archon/math/libarchon_math.la \
$(top_builddir)/src/archon/thread/libarchon_thread.la \
$(top_builddir)/src/archon/display/libarchon_display.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
