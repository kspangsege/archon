lib_LTLIBRARIES = libarchon_dom_impl.la

subincludedir = $(pkgincludedir)/dom/impl
subinclude_HEADERS = util.hpp core.hpp ls.hpp style.hpp render.hpp html_parser.hpp html.hpp \
impl.hpp

libarchon_dom_impl_la_SOURCES = core.cpp ls.cpp style.cpp html_parser.cpp render.cpp html.cpp \
impl.cpp
libarchon_dom_impl_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/util/libarchon_util.la \
@ARCHON_LIBEXPAT_LIBS@

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_LIBEXPAT_CFLAGS@
