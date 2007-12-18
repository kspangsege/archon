SUBDIRS = . test

subincludedir = $(pkgincludedir)/image
subinclude_HEADERS = color_space.hpp word_type.hpp buffer_format.hpp \
word_based_buffer_format.hpp integer_buffer_format.hpp buffer_codec.hpp image.hpp \
buffered_image.hpp file_format.hpp imageio.hpp pixel_converter.hpp misc.hpp reader_base.hpp \
reader.hpp writer_base.hpp writer.hpp oper.hpp

lib_LTLIBRARIES = libarchon_image.la

libarchon_image_la_SOURCES = color_space.cpp word_type.cpp buffer_format.cpp \
integer_buffer_format.cpp image.cpp buffered_image.cpp file_format.cpp format_gif.cpp \
imageio.cpp misc.cpp reader_base.cpp writer_base.cpp oper.cpp
libarchon_image_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/thread/libarchon_thread.la \
$(top_builddir)/src/archon/util/libarchon_util.la \
@ARCHON_IMAGE_LIBS@

if ARCHON_HAVE_LIBPNG
libarchon_image_la_SOURCES += format_png.cpp
endif

if ARCHON_HAVE_LIBJPEG
libarchon_image_la_SOURCES += format_jpeg.cpp
endif

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_IMAGE_CFLAGS@
