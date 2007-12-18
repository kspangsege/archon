noinst_PROGRAMS = show_fonts regen_fallback text_render

show_fonts_SOURCES = show_fonts.cpp
regen_fallback_SOURCES = regen_fallback.cpp
text_render_SOURCES = text_render.cpp

LDADD = ../../core/libarchon_core.la ../../util/libarchon_util.la ../../image/libarchon_image.la ../libarchon_font.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
