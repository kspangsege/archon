SUBDIRS = . test

subincludedir = $(pkgincludedir)/core
subinclude_HEADERS = assert.hpp terminate.hpp meta.hpp types.hpp type_list.hpp tuple.hpp bind.hpp \
functions.hpp numeric.hpp text_join.hpp series.hpp blocking.hpp generate.hpp mutex.hpp atomic.hpp \
codec.hpp utf16.hpp char_enc.hpp enum.hpp time.hpp sys.hpp term.hpp file.hpp dir_scan.hpp cxx.hpp \
random.hpp memory.hpp logger.hpp bind_ref.hpp refcnt.hpp unique_ptr.hpp shared_ptr.hpp \
weak_ptr.hpp iterator.hpp proxy_iter.hpp iseq.hpp stream_utils.hpp stream.hpp generator.hpp \
text.hpp text_table.hpp text_hist.hpp param_expand.hpp build_config.hpp config.hpp options.hpp

lib_LTLIBRARIES = libarchon_core.la

libarchon_core_la_SOURCES = terminate.cpp utf16.cpp enum.cpp time.cpp sys.cpp term.cpp file.cpp \
dir_scan.cpp cxx.cpp random.cpp memory.cpp logger.cpp stream.cpp text_hist.cpp build_config.cpp \
config.cpp options.cpp
libarchon_core_la_LIBADD = @ARCHON_LIBREALTIME_LIBS@

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = @ARCHON_LIBREALTIME_CFLAGS@

DISTCLEANFILES = build_config.dyn.h
build_config.dyn.h: Makefile
	echo '#define ARCHON_BUILD_CONFIG_DATA_DIR "$(pkgdatadir)/"' >$@

build_config.cpp: build_config.dyn.h
