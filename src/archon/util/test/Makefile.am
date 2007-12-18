noinst_PROGRAMS = prime weighted_moving_average hash_map paged_map remem_order_map slow_stream \
codec transcode compress unit_frac tuple_grid kd_tree filesys_listener mime_magic \
rep_map_lookup_boost conv_hull readline packed_trgb nullable_string

prime_SOURCES = prime.cpp
weighted_moving_average_SOURCES = weighted_moving_average.cpp
hash_map_SOURCES = hash_map.cpp
paged_map_SOURCES = paged_map.cpp
remem_order_map_SOURCES = remem_order_map.cpp
rep_map_lookup_boost_SOURCES = rep_map_lookup_boost.cpp
slow_stream_SOURCES = slow_stream.cpp
codec_SOURCES = codec.cpp
transcode_SOURCES = transcode.cpp
compress_SOURCES = compress.cpp
unit_frac_SOURCES = unit_frac.cpp
tuple_grid_SOURCES = tuple_grid.cpp
kd_tree_SOURCES = kd_tree.cpp
filesys_listener_SOURCES = filesys_listener.cpp
mime_magic_SOURCES = mime_magic.cpp
conv_hull_SOURCES = conv_hull.cpp
readline_SOURCES = readline.cpp
packed_trgb_SOURCES = packed_trgb.cpp
nullable_string_SOURCES = nullable_string.cpp

LDADD = ../../core/libarchon_core.la ../libarchon_util.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
