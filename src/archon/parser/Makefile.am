SUBDIRS = . test

subincludedir = $(pkgincludedir)/parser
subinclude_HEADERS = regex.hpp regex_print.hpp fsa.hpp fsa_print.hpp fsa_regex.hpp cfg.hpp \
lexer.hpp

lib_LTLIBRARIES = libarchon_parser.la

libarchon_parser_la_SOURCES = cfg.cpp
libarchon_parser_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/util/libarchon_util.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
