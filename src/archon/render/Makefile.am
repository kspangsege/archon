lib_LTLIBRARIES = libarchon_render.la

if ARCHON_HAVE_OPENGL

SUBDIRS = . test

subincludedir = $(pkgincludedir)/render
subinclude_HEADERS = scene_builder.hpp billboard.hpp object.hpp load_texture.hpp \
texture_cache.hpp font_provider.hpp text_formatter.hpp dom_renderer.hpp app.hpp

subdatadir = $(pkgdatadir)/render
subdata_DATA = viewer_interact.png viewer_trackball.png

libarchon_render_la_SOURCES = scene_builder.cpp billboard.cpp object.cpp load_texture.cpp \
texture_cache.cpp font_provider.cpp text_formatter.cpp dom_renderer.cpp app.cpp

libarchon_render_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/util/libarchon_util.la \
$(top_builddir)/src/archon/image/libarchon_image.la \
$(top_builddir)/src/archon/font/libarchon_font.la \
$(top_builddir)/src/archon/dom/libarchon_dom.la \
$(top_builddir)/src/archon/dom/impl/libarchon_dom_impl.la \
$(top_builddir)/src/archon/graphics/libarchon_graphics.la \
$(top_builddir)/src/archon/display/libarchon_display.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast

else

libarchon_render_la_SOURCES = dummy.cpp

endif
