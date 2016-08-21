SUBDIRS = . test server
#SUBDIRS = . test

subincludedir = $(pkgincludedir)/web
subinclude_HEADERS = yber_codec.hpp

lib_LTLIBRARIES = libarchon_web.la

libarchon_web_la_SOURCES = yber_codec.cpp
libarchon_web_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/util/libarchon_util.la

AM_CPPFLAGS = -I$(top_srcdir)/src
