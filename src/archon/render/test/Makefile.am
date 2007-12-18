noinst_PROGRAMS = billboard render scene_builder load_texture text_formatter multi_threaded \
phys hydrogen explorer conv_hull image_gamut bsp html_browser

billboard_SOURCES = billboard.cpp
render_SOURCES = render.cpp
scene_builder_SOURCES = scene_builder.cpp
load_texture_SOURCES = load_texture.cpp
text_formatter_SOURCES = text_formatter.cpp
multi_threaded_SOURCES = multi_threaded.cpp
phys_SOURCES = phys.cpp
hydrogen_SOURCES = hydrogen.cpp
explorer_SOURCES = explorer.cpp
conv_hull_SOURCES = conv_hull.cpp
image_gamut_SOURCES = image_gamut.cpp
bsp_SOURCES = bsp.cpp
html_browser_SOURCES = html_browser.cpp

LDADD = ../../core/libarchon_core.la ../../math/libarchon_math.la ../../thread/libarchon_thread.la ../../util/libarchon_util.la ../../image/libarchon_image.la ../../font/libarchon_font.la ../../dom/libarchon_dom.la ../../graphics/libarchon_graphics.la ../../display/libarchon_display.la ../libarchon_render.la ../../dom/impl/libarchon_dom_impl.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -Wold-style-cast
