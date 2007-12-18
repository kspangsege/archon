noinst_PROGRAMS = socket_server server

socket_server_SOURCES = socket_server.cpp
server_SOURCES = server.cpp

LDADD = ../libarchon_web_server.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
