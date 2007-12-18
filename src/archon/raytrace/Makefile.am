SUBDIRS = . test

subincludedir = $(pkgincludedir)/raytracer
subinclude_HEADERS = light.hpp material.hpp texture.hpp surface.hpp object.hpp raytracer.hpp \
scene_build.hpp

lib_LTLIBRARIES = libarchon_raytrace.la

#libarchon_raytrace_la_SOURCES = light.cpp surface.cpp object.cpp scene.cpp raytracer.cpp
libarchon_raytrace_la_SOURCES = light.cpp material.cpp texture.cpp object.cpp raytracer.cpp \
scene_build.cpp
libarchon_raytrace_la_LIBADD = \
$(top_builddir)/src/archon/core/libarchon_core.la \
$(top_builddir)/src/archon/math/libarchon_math.la \
$(top_builddir)/src/archon/util/libarchon_util.la \
$(top_builddir)/src/archon/image/libarchon_image.la \
$(top_builddir)/src/archon/graphics/libarchon_graphics.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
