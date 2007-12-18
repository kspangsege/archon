SUBDIRS = impl . test

subincludedir = $(pkgincludedir)/dom
nobase_subinclude_HEADERS = util/types.hpp util/ref.hpp util/object.hpp util/string.hpp \
core.hpp bootstrap.hpp ls.hpp css.hpp html.hpp

lib_LTLIBRARIES = libarchon_dom.la

libarchon_dom_la_SOURCES = bootstrap.cpp
libarchon_dom_la_LIBADD = impl/libarchon_dom_impl.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
