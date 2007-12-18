noinst_PROGRAMS = raytracer

raytracer_SOURCES = raytracer.cpp

LDADD = ../libarchon_raytrace.la

raytracer_LDADD = ../../core/libarchon_core.la ../../math/libarchon_math.la ../../util/libarchon_util.la ../../image/libarchon_image.la ../../display/libarchon_display.la ../../render/libarchon_render.la ../libarchon_raytrace.la 

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
