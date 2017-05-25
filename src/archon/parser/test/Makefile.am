noinst_PROGRAMS = regex fsa cfg

regex_SOURCES = regex.cpp
fsa_SOURCES = fsa.cpp
cfg_SOURCES = cfg.cpp

LDADD = ../../core/libarchon_core.la ../libarchon_parser.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
