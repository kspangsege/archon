noinst_PROGRAMS = cursor image
if ARCHON_HAVE_OPENGL
  noinst_PROGRAMS += shared_textures
endif

cursor_SOURCES = cursor.cpp
image_SOURCES = image.cpp
shared_textures_SOURCES = shared_textures.cpp

LDADD = ../../util/libarchon_util.la ../../image/libarchon_image.la ../libarchon_display.la ../x11/libarchon_display_x11.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
