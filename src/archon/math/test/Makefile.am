noinst_PROGRAMS = vector vector_adapt matrix matrix_adapt rotation intersect

vector_SOURCES = vector.cpp
vector_adapt_SOURCES = vector_adapt.cpp
matrix_SOURCES = matrix.cpp
matrix_adapt_SOURCES = matrix_adapt.cpp
rotation_SOURCES = rotation.cpp
intersect_SOURCES = intersect.cpp

LDADD = ../../core/libarchon_core.la ../libarchon_math.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
