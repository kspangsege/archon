SUBDIRS = . test

subincludedir = $(pkgincludedir)/util
subinclude_HEADERS = prime.hpp circular_buffer.hpp map_cmp.hpp paged_map.hpp range_map.hpp \
remem_order_map.hpp rep_map_lookup_boost.hpp hashing.hpp hash_map.hpp statistics.hpp base64.hpp \
stream.hpp codec.hpp inc_conv.hpp transcode.hpp compress.hpp kd_tree.hpp progress.hpp ticker.hpp \
unit_frac.hpp tuple_grid.hpp filesys_listen.hpp mime_magic.hpp perspect_proj.hpp rect_packer.hpp \
permutation.hpp conv_hull.hpp readline.hpp color.hpp packed_trgb.hpp named_colors.hpp uri.hpp

lib_LTLIBRARIES = libarchon_util.la

libarchon_util_la_SOURCES = prime.cpp base64.cpp stream.cpp codec.cpp transcode.cpp compress.cpp \
progress.cpp ticker.cpp filesys_listen.cpp mime_magic.cpp rect_packer.cpp conv_hull.cpp \
readline.cpp color.cpp packed_trgb.cpp uri.cpp
libarchon_util_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/math/libarchon_math.la \
$(top_builddir)/src/archon/thread/libarchon_thread.la \
@ARCHON_LIBREADLINE_LIBS@ @ARCHON_LIBMAGIC_LIBS@

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_LIBREADLINE_CFLAGS@ @ARCHON_LIBMAGIC_CFLAGS@
