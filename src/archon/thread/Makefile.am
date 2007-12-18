SUBDIRS = . test

subincludedir = $(pkgincludedir)/thread
subinclude_HEADERS = condition.hpp semaphore.hpp thread.hpp

lib_LTLIBRARIES = libarchon_thread.la

libarchon_thread_la_SOURCES = condition.cpp semaphore.cpp thread.cpp
libarchon_thread_la_LIBADD = $(top_builddir)/src/archon/core/libarchon_core.la

AM_CPPFLAGS = -I$(top_srcdir)/src
