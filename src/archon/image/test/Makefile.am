noinst_PROGRAMS = rect_packer color_space integer_buffer_format image_view_source \
image_view_target buffered_image imageio reader writer dither

rect_packer_SOURCES = rect_packer.cpp
color_space_SOURCES = color_space.cpp
integer_buffer_format_SOURCES = integer_buffer_format.cpp
image_view_source_SOURCES = image_view_source.cpp
image_view_target_SOURCES = image_view_target.cpp
buffered_image_SOURCES = buffered_image.cpp
imageio_SOURCES = imageio.cpp
reader_SOURCES = reader.cpp
writer_SOURCES = writer.cpp
#dither_SOURCES = dither.cpp cgal_convex_hull.cpp
dither_SOURCES = dither.cpp

#dither_LDADD = ../libarchon_image.la -lCGAL -lCGALcore++
#dither_LDADD = ../libarchon_image.la
LDADD = ../../core/libarchon_core.la ../../util/libarchon_util.la ../libarchon_image.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
