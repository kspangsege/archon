noinst_PROGRAMS = dom parse async_parse parse_dtd parse_html html_dom

dom_SOURCES = dom.cpp
parse_SOURCES = parse.cpp
async_parse_SOURCES = async_parse.cpp
parse_dtd_SOURCES = parse_dtd.cpp
parse_html_SOURCES = parse_html.cpp
html_dom_SOURCES = html.cpp

LDADD = ../../core/libarchon_core.la ../libarchon_dom.la ../impl/libarchon_dom_impl.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
