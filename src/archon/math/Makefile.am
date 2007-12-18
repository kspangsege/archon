SUBDIRS = . test

subincludedir = $(pkgincludedir)/math
subinclude_HEADERS = functions.hpp interval.hpp vec_ops.hpp vector.hpp vector_adapt.hpp \
matrix.hpp matrix_adapt.hpp coord_system.hpp rotation.hpp quaternion.hpp geometry.hpp \
quartic_solve.hpp intersect.hpp

lib_LTLIBRARIES = libarchon_math.la

libarchon_math_la_SOURCES = geometry.cpp rotation.cpp quaternion.cpp quartic_solve.cpp \
intersect.cpp
libarchon_math_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la

AM_CPPFLAGS = -I$(top_srcdir)/src
