// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_X11_SUPPORT_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_X11_SUPPORT_HPP


#include <cstddef>
#include <utility>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/log.hpp>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>
#include <archon/display/resolution.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>

#if !ARCHON_WINDOWS && ARCHON_DISPLAY_HAVE_X11 && ARCHON_DISPLAY_HAVE_X11_XKB
#  include <unistd.h>
#  if defined _POSIX_VERSION && _POSIX_VERSION >= 200112L // POSIX.1-2001
#    define HAVE_X11 1
#  else
#    define HAVE_X11 0
#  endif
#else
#  define HAVE_X11 0
#endif

#if HAVE_X11
#  if ARCHON_DISPLAY_HAVE_X11_XDBE
#    define HAVE_XDBE 1
#  else
#    define HAVE_XDBE 0
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRANDR
#    define HAVE_XRANDR 1
#  else
#    define HAVE_XRANDR 0
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRENDER
#    define HAVE_XRENDER 1
#  else
#    define HAVE_XRENDER 0
#  endif
#  if ARCHON_DISPLAY_HAVE_OPENGL_GLX
#    define HAVE_GLX 1
#  else
#    define HAVE_GLX 0
#  endif
#endif

#if HAVE_X11
#  include <poll.h>
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  include <X11/XKBlib.h>
#  if HAVE_XDBE
#    include <X11/extensions/Xdbe.h>
#  endif
#  if HAVE_XRANDR
#    include <X11/extensions/Xrandr.h>
#  endif
#  if HAVE_XRENDER
#    include <X11/extensions/Xrender.h>
#  endif
#  if HAVE_GLX
#    include <GL/glx.h>
#  endif
#endif

#if HAVE_X11
#  if defined X_HAVE_UTF8_STRING && X_HAVE_UTF8_STRING
#    define HAVE_X11_UTF8 1
#  else
#    define HAVE_X11_UTF8 0
#  endif
#endif


// According to Wikipedia, as of July 7, 2024, Release 7.7 is the latest release of X11. It
// was released on June 6, 2012.
//
//
// Relevant links
// --------------
//
// X11 documentation overview: https://www.x.org/releases/X11R7.7/doc/
//
// X11 API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
//
// Inter-Client Communication Conventions Manual: https://x.org/releases/X11R7.7/doc/xorg-docs/icccm/icccm.html
//
// Extended Window Manager Hints: https://specifications.freedesktop.org/wm-spec/latest/
//
// X11 protocol specification: https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html
//
// X.Org module-level source code releases: https://www.x.org/releases/individual/lib/
//
// X.Org Xlib source code repository: https://gitlab.freedesktop.org/xorg/lib/libx11
//
// X.Org Server source code repository: https://gitlab.freedesktop.org/xorg/xserver
//
//
// Xkb extension
// -------------
//
// API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/XKB/xkblib.html
//
// Protocol specification: https://www.x.org/releases/X11R7.7/doc/kbproto/xkbproto.html
//
//
// Xdbe extension
// --------------
//
// API documentation: https://www.x.org/releases/X11R7.7/doc/libXext/dbelib.html
//
// Protocol specification: https://www.x.org/releases/X11R7.7/doc/xextproto/dbe.html
//
//
// XRandR extension
// ----------------
//
// General documentation: https://www.x.org/wiki/libraries/libxrandr/
//
// Protocol specification: https://www.x.org/releases/X11R7.7/doc/randrproto/randrproto.txt
//
// NOTE: Version 1.6 of the protocol specification (from 2017-04-01) can be found as
// `/usr/share/doc/x11proto-dev/randrproto.txt.gz` in package `x11proto-dev` on Ubuntu
// 24.04.
//
//
// Xrender extension
// -----------------
//
// API documentation: https://www.x.org/releases/X11R7.7/doc/libXrender/libXrender.txt
//
// Protocol specification: https://www.x.org/releases/X11R7.7/doc/renderproto/renderproto.txt
//
//
// OpenGL GLX
// ----------
//
// Specification: https://registry.khronos.org/OpenGL/specs/gl/glx1.4.pdf
//
//
// Useful commands
// ---------------
//
// See contents of window with its colormap honored regardless of whether window manager has
// installed that colormap:
//
//     xwd | xwdtopnm | pnmtopng > /tmp/out.png
//
// Start "fake" X server with support for various uncommon visuals:
//
//     Xephyr :1 -screen 1024x1024x8
//     Xvfb :1 -screen 0 1024x1024x8 -fbdir /tmp
//
// Dump screen of "fake" X server when using `Xvfb`:
//
//     xwud -in /tmp/Xvfb_screen0 -vis default
//
// Start window manager for "fake" X server:
//
//     DISPLAY=:1 LANG=C twm
//
// Set up standard colormaps and corresponding attributes on root window:
//
//     xstdcmap -default
//
// Permit X11 connections from remote clients:
//
//     xhost +
//


namespace archon::display::impl::x11 {


#if HAVE_X11


struct ExtensionInfo {
    bool have_xdbe;    // X Double Buffer Extension
    bool have_xkb;     // X Keyboard Extention
    bool have_xrandr;  // X Resize, Rotate and Reflect Extension
    bool have_xrender; // X Rendering Extension
    bool have_glx;     // X extension for rendering using OpenGL

    // Valid only when `have_xdbe` is `true`
    int xdbe_major, xdbe_minor;

    // Valid only when `have_xkb` is `true`
    int xkb_major, xkb_minor;

    // Valid only when `have_xrandr` is `true`
    int xrandr_event_base;
    int xrandr_major, xrandr_minor;

    // Valid only when `have_xrender` is `true`
    int xrender_major, xrender_minor;

    // Valid only when `have_glx` is `true`
    int glx_major, glx_minor;
};


struct TextPropertyWrapper {
    XTextProperty prop;

    TextPropertyWrapper(Display* dpy, std::string_view str, const std::locale& loc);
    ~TextPropertyWrapper() noexcept;
};


struct VisualSpec {
    XVisualInfo info;
    bool double_buffered;
    bool opengl_supported;
    bool opengl_double_buffered;
    bool opengl_stereo;
    int double_buffered_perflevel;
    int opengl_level;
    int opengl_num_aux_buffers;
    int opengl_depth_buffer_bits;
    int opengl_stencil_buffer_bits;
    int opengl_accum_buffer_bits;
};


struct FindVisualParams {
    std::optional<int> visual_depth;
    std::optional<int> visual_class;
    std::optional<VisualID> visual_type;

    bool prefer_default_visual_type = true;
    bool prefer_default_visual_depth = true;
    bool prefer_double_buffered = true;

    bool require_opengl = false;
    bool require_opengl_depth_buffer = false;
    bool require_opengl_stencil_buffer = false;
    bool require_opengl_accum_buffer = false;

    int min_opengl_depth_buffer_bits = 8;
    int min_opengl_stencil_buffer_bits = 1;
    int min_opengl_accum_buffer_bits = 32;
};


struct BitFields {
    int red_shift, red_width;
    int green_shift, green_width;
    int blue_shift, blue_width;
};


struct MultFields {
    unsigned long offset;
    unsigned long red_mult, red_max;
    unsigned long green_mult, green_max;
    unsigned long blue_mult, blue_max;

    constexpr MultFields() noexcept = default;
    constexpr MultFields(const x11::BitFields&, bool is_gray) noexcept;
    MultFields(const XStandardColormap&) noexcept;

    // Initialize for gray-scale visual. Number of levels must be strictly greater than zero
    // and strictly less than 2^32.
    constexpr MultFields(int num_levels) noexcept;

    // All three must be strictly greater than zero. Product of `num_red`, `num_green`, and
    // `num_blue` must be strictly less than 2^32.
    constexpr MultFields(int num_red, int num_green, int num_blue) noexcept;

    void assign_to(XStandardColormap&) const noexcept;

    constexpr auto operator<=>(const MultFields& other) const noexcept = default;

    auto pack(unsigned long red, unsigned long green, unsigned long blue) const noexcept -> unsigned long;
};



class DisplayWrapper {
public:
    DisplayWrapper() noexcept = default;
    DisplayWrapper(DisplayWrapper&&) noexcept;
    ~DisplayWrapper() noexcept;

    auto set(Display*) noexcept -> DisplayWrapper&;

    auto operator=(DisplayWrapper&&) noexcept -> DisplayWrapper&;

    operator Display*() const noexcept;

private:
    Display* m_dpy = nullptr;

    void destroy() noexcept;
    void steal(DisplayWrapper&) noexcept;
};



class ColormapWrapper {
public:
    ColormapWrapper() noexcept = default;
    ColormapWrapper(ColormapWrapper&&) noexcept;
    ~ColormapWrapper() noexcept;

    auto set_owned(Display*, Colormap) noexcept -> ColormapWrapper&;
    auto set_unowned(Colormap) noexcept -> ColormapWrapper&;

    void release_ownership() noexcept;

    auto operator=(ColormapWrapper&&) noexcept -> ColormapWrapper&;

    operator Colormap() const noexcept;

private:
    Display* m_dpy = nullptr;
    Colormap m_colormap = {};

    void destroy() noexcept;
    void steal(ColormapWrapper&) noexcept;
};



class ImageBridge {
public:
    image::WritableImage& img_1;
    XImage img_2;

    virtual ~ImageBridge() = default;

protected:
    ImageBridge(image::WritableImage&) noexcept;
};



class PixelFormat {
public:
    auto get_colormap() const noexcept -> Colormap;
    virtual auto intern_color(util::Color color) const -> unsigned long = 0;
    virtual auto create_image_bridge(display::Size size) const -> std::unique_ptr<ImageBridge> = 0;
    virtual ~PixelFormat() = default;

protected:
    PixelFormat(x11::ColormapWrapper) noexcept;

private:
    x11::ColormapWrapper m_colormap;
};



class ColormapFinder {
public:
    virtual bool find_default_colormap(VisualID, Colormap&) const noexcept = 0;
    virtual bool find_standard_colormap(VisualID, XStandardColormap&) const = 0;

protected:
    ~ColormapFinder() = default;
};



class ServerGrab {
public:
    ServerGrab(Display*) noexcept;
    ~ServerGrab() noexcept;

    // Disable copy and move
    ServerGrab(ServerGrab&&) noexcept = delete;
    auto operator=(ServerGrab&&) noexcept -> ServerGrab& = delete;

private:
    Display* m_dpy;
};



#if HAVE_XRANDR


struct ProtoViewport {
    core::IndexRange output_name;
    display::Box bounds;
    std::optional<core::IndexRange> monitor_name;
    std::optional<display::Resolution> resolution;
    std::optional<double> refresh_rate;
};


struct ScreenConf {
    std::vector<x11::ProtoViewport> viewports;
    core::Buffer<char> string_buffer;
    std::size_t string_buffer_used_size = 0;
};


#endif // HAVE_XRANDR



auto map_opt_visual_class(const std::optional<display::x11_connection_config::VisualClass>& class_) noexcept ->
    std::optional<int>;


auto get_visual_class_name(int class_) noexcept -> const char*;


// If no display string is specified, the value of the `DISPLAY` environment variable will
// be used.
//
auto connect(std::optional<std::string_view> display, const std::locale& locale) -> x11::DisplayWrapper;


// If no display string is specified, the value of the `DISPLAY` environment variable will
// be returned.
//
auto get_display_string(const std::optional<std::string_view>& display) -> std::string_view;


bool try_connect(std::string_view display, x11::DisplayWrapper& dpy_owner);


auto init_extensions(Display* dpy) -> x11::ExtensionInfo;


// If no screen is specified, the default screen for the display will be returned.
//
int get_screen_index(Display* dpy, std::optional<int> screen) noexcept;


bool valid_screen_index(Display* dpy, int screen) noexcept;


bool has_property(Display* dpy, ::Window win, Atom name);


// Key in returned map is visual depth.
//
auto fetch_pixmap_formats(Display* dpy) -> core::FlatMap<int, XPixmapFormatValues>;


auto fetch_standard_colormaps(Display* dpy, ::Window root) -> core::FlatMap<VisualID, XStandardColormap>;


auto load_visuals(Display* dpy, int screen, const x11::ExtensionInfo& extension_info) -> core::Slab<x11::VisualSpec>;


bool find_visual(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs,
                 const x11::FindVisualParams& params, std::size_t& index);

auto find_visuals(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs,
                  const x11::FindVisualParams& params, core::Buffer<std::size_t>& index) -> std::size_t;


auto record_bit_fields(const XVisualInfo& visual_info) -> x11::BitFields;


void init_ximage(Display* dpy, XImage& img, const XVisualInfo& visual_info, const XPixmapFormatValues& pixmap_format,
                 int byte_order, const display::Size& size, char* buffer);


// This function uses only the red field of `fields`. The red field must cover a compact
// section of the colormap. This function also requires that `fields.red_max` is strictly
// less than 2^16.
//
// This function assumes that all colormap entries have been allocated writable.
//
// If `fill` is `true`, the entire colormap is initialized. Otherwise, only the compact
// section covered by the specified red field is initialized. When the entire colormap is
// initialized, entries not covered by the specified fields will be set to "black".
//
void init_grayscale_colormap(Display* dpy, Colormap colormap, const x11::MultFields& fields, int colormap_size,
                             bool fill, bool weird);


// This function requires that the specified fields cover a compact section of the
// colormap. This function also requires that `fields.red_max`, `fields.green_max`, and
// `fields.blue_max` are all strictly less than 2^16.
//
// This function assumes that all colormap entries have been allocated writable.
//
// If `fill` is `true`, the entire colormap is initialized. Otherwise, only the compact
// section covered by the specified fields is initialized. When the entire colormap is
// initialized, entries not covered by the specified fields will be set to "black".
//
void init_pseudocolor_colormap(Display* dpy, Colormap colormap, const x11::MultFields& fields, int colormap_size,
                               bool fill, bool weird);


// This function requires that all three channel widths are less than, or equal to 16.
//
// This function assumes that the specified fields reflect the visual that is associated
// with the specified colormap. This function also assumes that all colormap entries have
// been allocated writable.
//
void init_directcolor_colormap(Display* dpy, Colormap colormap, const x11::BitFields& fields, int colormap_size,
                               bool weird);


void setup_standard_grayscale_colormap(Display* dpy, Colormap colormap, int depth, int colormap_size, bool weird);
void setup_standard_pseudocolor_colormap(Display* dpy, Colormap colormap, int depth, int colormap_size,
                                         x11::BitFields& fields, bool weird);


// Caller must keep display connection, visual info object, and pixmap format object alive
// for as long as the created pixel format remains in use.
//
auto create_pixel_format(Display* dpy, ::Window root, const XVisualInfo&, const XPixmapFormatValues&,
                         const x11::ColormapFinder&, const std::locale&, log::Logger&,
                         bool prefer_default_nondecomposed_colormap, bool weird) -> std::unique_ptr<x11::PixelFormat>;


#if HAVE_XRANDR

bool update_screen_conf(Display* dpy, ::Window root, Atom atom_edid, const impl::EdidParser& edid_parser,
                        const std::locale& locale, x11::ScreenConf& conf);

#endif // HAVE_XRANDR


// These need to be called while window is mapped.
//
// FIXME: Why is it not possible to set fullscreen mode or "fullscreen monitors"
// speciffication before window is mapped?                  
//
void set_fullscreen_monitors(Display* dpy, ::Window win, const display::x11_fullscreen_monitors& spec,
                             ::Window root, Atom atom_net_wm_fullscreen_monitors);
void set_fullscreen_mode(Display* dpy, ::Window win, bool on, ::Window root, Atom atom_net_wm_state,
                         Atom atom_net_wm_state_fullscreen);


#endif // HAVE_X11








// Implementation


#if HAVE_X11


inline TextPropertyWrapper::~TextPropertyWrapper() noexcept
{
    XFree(prop.value);
}


constexpr MultFields::MultFields(const x11::BitFields& fields, bool is_gray) noexcept
{
    ARCHON_ASSERT(fields.red_shift < 32);
    ARCHON_ASSERT(fields.green_shift < 32);
    ARCHON_ASSERT(fields.blue_shift < 32);
    ARCHON_ASSERT(fields.red_width <= 16);
    ARCHON_ASSERT(fields.green_width <= 16);
    ARCHON_ASSERT(fields.blue_width <= 16);

    using ulong = unsigned long;
    offset = 0;

    red_max   = core::int_mask<ulong>(fields.red_width);
    green_max = core::int_mask<ulong>(fields.green_width);
    blue_max  = core::int_mask<ulong>(fields.blue_width);

    red_mult   = ulong(1) << fields.red_shift;
    green_mult = ulong(1) << fields.green_shift;
    blue_mult  = ulong(1) << fields.blue_shift;

    if (is_gray) {
        green_mult = 0;
        blue_mult  = 0;
    }
}


inline MultFields::MultFields(const XStandardColormap& params) noexcept
    : offset(params.base_pixel)
    , red_mult(params.red_mult)
    , red_max(params.red_max)
    , green_mult(params.green_mult)
    , green_max(params.green_max)
    , blue_mult(params.blue_mult)
    , blue_max(params.blue_max)
{
}


constexpr MultFields::MultFields(int num_levels) noexcept
{
    ARCHON_ASSERT(num_levels > 0);

    *this = {};

    using ulong = unsigned long;
    ulong num_levels_2 = ulong(num_levels);
    ulong max = core::int_mask<ulong>(32);
    ARCHON_ASSERT(num_levels_2 <= max);

    red_mult = 1;
    red_max = num_levels_2 - 1;
}


constexpr MultFields::MultFields(int num_red, int num_green, int num_blue) noexcept
{
    ARCHON_ASSERT(num_red > 0);
    ARCHON_ASSERT(num_green > 0);
    ARCHON_ASSERT(num_blue > 0);

    using ulong = unsigned long;
    ulong num_red_2   = ulong(num_red);
    ulong num_green_2 = ulong(num_green);
    ulong num_blue_2  = ulong(num_blue);

    ulong max = core::int_mask<ulong>(32);
    ARCHON_ASSERT(num_red_2 <= max / num_green_2);
    ARCHON_ASSERT(num_red_2 * num_green_2 <= max / num_blue_2);

    offset = 0;

    ulong mult = 1;
    blue_mult = mult;
    mult *= num_blue_2;
    green_mult = mult;
    mult *= num_green_2;
    red_mult = mult;

    red_max   = num_red_2   - 1;
    green_max = num_green_2 - 1;
    blue_max  = num_blue_2  - 1;
}


inline void MultFields::assign_to(XStandardColormap& params) const noexcept
{
    params.red_max    = red_max;
    params.red_mult   = red_mult;
    params.green_max  = green_max;
    params.green_mult = green_mult;
    params.blue_max   = blue_max;
    params.blue_mult  = blue_mult;
    params.base_pixel = offset;
}


inline auto MultFields::pack(unsigned long red, unsigned long green,
                             unsigned long blue) const noexcept -> unsigned long
{
    return red * red_mult + green * green_mult + blue * blue_mult;
}


inline DisplayWrapper::DisplayWrapper(DisplayWrapper&& other) noexcept
{
    steal(other);
}


inline DisplayWrapper::~DisplayWrapper() noexcept
{
    destroy();
}


inline auto DisplayWrapper::set(Display* dpy) noexcept -> DisplayWrapper&
{
    destroy();
    m_dpy = dpy;
    return *this;
}


inline auto DisplayWrapper::operator=(DisplayWrapper&& other) noexcept -> DisplayWrapper&
{
    destroy();
    steal(other);
    return *this;
}


inline DisplayWrapper::operator Display*() const noexcept
{
    return m_dpy;
}


inline void DisplayWrapper::destroy() noexcept
{
    if (m_dpy)
        XCloseDisplay(m_dpy);
}


inline void DisplayWrapper::steal(DisplayWrapper& other) noexcept
{
    m_dpy = other.m_dpy;
    other.m_dpy = nullptr;
}


inline ColormapWrapper::ColormapWrapper(ColormapWrapper&& other) noexcept
{
    steal(other);
}


inline ColormapWrapper::~ColormapWrapper() noexcept
{
    destroy();
}


inline auto ColormapWrapper::set_owned(Display* dpy, Colormap colormap) noexcept -> ColormapWrapper&
{
    destroy();
    m_dpy = dpy;
    m_colormap = colormap;
    return *this;
}


inline auto ColormapWrapper::set_unowned(Colormap colormap) noexcept -> ColormapWrapper&
{
    destroy();
    m_dpy = nullptr;
    m_colormap = colormap;
    return *this;
}


inline void ColormapWrapper::release_ownership() noexcept
{
    m_dpy = nullptr;
}


inline auto ColormapWrapper::operator=(ColormapWrapper&& other) noexcept -> ColormapWrapper&
{
    destroy();
    steal(other);
    return *this;
}


inline ColormapWrapper::operator Colormap() const noexcept
{
    return m_colormap;
}


inline void ColormapWrapper::destroy() noexcept
{
    if (m_dpy)
        XFreeColormap(m_dpy, m_colormap);
}


inline void ColormapWrapper::steal(ColormapWrapper& other) noexcept
{
    m_dpy = other.m_dpy;
    m_colormap = other.m_colormap;
    other.m_dpy = nullptr;
    other.m_colormap = {};
}


inline ImageBridge::ImageBridge(image::WritableImage& img) noexcept
    : img_1(img)
{
}


inline auto PixelFormat::get_colormap() const noexcept -> Colormap
{
    return m_colormap;
}


inline PixelFormat::PixelFormat(x11::ColormapWrapper colormap) noexcept
    : m_colormap(std::move(colormap))
{
}


inline ServerGrab::ServerGrab(Display* dpy) noexcept
    : m_dpy(dpy)
{
    XGrabServer(m_dpy);
}


inline ServerGrab::~ServerGrab() noexcept
{
    XUngrabServer(m_dpy);
}


inline int get_screen_index(Display* dpy, std::optional<int> screen) noexcept
{
    if (ARCHON_LIKELY(!screen.has_value()))
        return DefaultScreen(dpy);
    return screen.value();
}


inline bool valid_screen_index(Display* dpy, int screen) noexcept
{
    return (screen >= 0 && screen < ScreenCount(dpy));
}


#endif // HAVE_X11


} // namespace archon::display::impl::x11


#endif // ARCHON_X_DISPLAY_X_NOINST_X_X11_SUPPORT_HPP
