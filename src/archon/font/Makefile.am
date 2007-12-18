SUBDIRS = . test

subincludedir = $(pkgincludedir)/font
subinclude_HEADERS = face.hpp loader.hpp list.hpp util.hpp cache.hpp text_format.hpp \
text_render.hpp layout_cfg.hpp

subdatadir = $(pkgdatadir)/font
subdata_DATA = LiberationSerif-Regular.ttf fallback-font.png fallback-font.conf

lib_LTLIBRARIES = libarchon_font.la

libarchon_font_la_SOURCES = list.cpp util.cpp cache.cpp text_format.cpp text_render.cpp \
layout_cfg.cpp
libarchon_font_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/image/libarchon_image.la \
@ARCHON_FONT_LIBS@

if ARCHON_HAVE_FREETYPE
libarchon_font_la_SOURCES += impl_freetype.cpp
else
libarchon_font_la_SOURCES += impl_fallback.cpp
endif

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_FONT_CFLAGS@
