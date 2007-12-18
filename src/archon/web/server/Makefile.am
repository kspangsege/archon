SUBDIRS = . test

subincludedir = $(pkgincludedir)/web/server
subinclude_HEADERS = stream.hpp socket_server.hpp header.hpp request.hpp response.hpp resource.hpp \
resolver.hpp server.hpp

lib_LTLIBRARIES = libarchon_web_server.la

libarchon_web_server_la_SOURCES = stream.cpp socket_server.cpp header.cpp request.cpp response.cpp \
server.cpp
libarchon_web_server_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/thread/libarchon_thread.la \
$(top_builddir)/src/archon/web/libarchon_web.la

AM_CPPFLAGS = -I$(top_srcdir)/src
