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


#include <memory>
#include <optional>
#include <locale>

#include <archon/core/features.h>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/connection_config_x11.hpp>

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


// As of Jan 7, 2024, Release 7.7 is the latest release of X11. It was released on June 6, 2012.
//
//
// Relevant links:
//
// * X11 documentation overview: https://www.x.org/releases/X11R7.7/doc/
//
// * X11 protocol specification: https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html
//
// * X11 API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
//
// * Inter-Client Communication Conventions Manual: https://x.org/releases/X11R7.7/doc/xorg-docs/icccm/icccm.html
//
// * Extended Window Manager Hints: https://specifications.freedesktop.org/wm-spec/latest/
//
// * Xkb protocol specification: https://www.x.org/releases/X11R7.7/doc/kbproto/xkbproto.html
//
// * Xkb API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/XKB/xkblib.html
//
// * Xdbe protocol specification: https://www.x.org/releases/X11R7.7/doc/xextproto/dbe.html
//
// * Xdbe API documentation: https://www.x.org/releases/X11R7.7/doc/libXext/dbelib.html
//
// * XRandR protocol specification: https://www.x.org/releases/X11R7.7/doc/randrproto/randrproto.txt
//
// * XRandR general documentation: https://www.x.org/wiki/libraries/libxrandr/
//
// * XRandR library source code: https://gitlab.freedesktop.org/xorg/lib/libxrandr
//
// * Xrender protocol specification: https://www.x.org/releases/X11R7.7/doc/renderproto/renderproto.txt
//
// * Xrender API documentation: https://www.x.org/releases/X11R7.7/doc/libXrender/libXrender.txt
//
// * OpenGL GLX specification: https://registry.khronos.org/OpenGL/specs/gl/glx1.4.pdf
//
//
// Useful commands:
//
// * Start "fake" X server with support for various uncommon visuals: `Xephyr :1 -screen
//   1024x1024x8` or `Xvfb :1 -screen 0 1024x1024x8 -fbdir /tmp`
//
// * Dump screen of "fake" X server when using `Xvfb`: `xwud -in /tmp/Xvfb_screen0 -vis default`
//
// * Set standard colormaps and corresponding attributes on root window: `xstdcmap -default`
//


namespace archon::display::impl {


#if HAVE_X11


auto map_opt_visual_class(const std::optional<display::ConnectionConfigX11::VisualClass>& class_) noexcept ->
    std::optional<int>;

auto get_visual_class_name(int class_) noexcept -> const char*;


// Check whether the masks of the specified visual are all zero.
inline bool zero_mask_match(const XVisualInfo&) noexcept;


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is normal (normal channel order is RGB
// as opposed to BGR).
template<class P> inline bool norm_mask_match(const XVisualInfo&) noexcept;


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is reversed (reversed channel order is
// BGR as opposed to RGB).
template<class P> inline bool rev_mask_match(const XVisualInfo&) noexcept;


class PixelFormat {
public:
    virtual auto intern_color(util::Color color) const -> unsigned long = 0;
    virtual void setup_image_bridge(display::Size size, std::unique_ptr<image::WritableImage>& img_1,
                                    XImage& img_2) const = 0;
    virtual ~PixelFormat() = default;
};


// Caller must keep display connection, visual info object, and pixmap format object alive
// for as long as the created pixel format remains in use.
auto create_pixel_format(Display* dpy, const XVisualInfo&, const XPixmapFormatValues&, Colormap, const std::locale&) ->
    std::unique_ptr<impl::PixelFormat>;


#endif // HAVE_X11








// Implementation


#if HAVE_X11


inline bool zero_mask_match(const XVisualInfo& info) noexcept
{
    return (info.red_mask == 0 && info.green_mask == 0 && info.blue_mask == 0);
}


template<class P> inline bool norm_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2));
}


template<class P> inline bool rev_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0));
}


#endif // HAVE_X11


} // namespace archon::display::impl


#endif // ARCHON_X_DISPLAY_X_NOINST_X_X11_SUPPORT_HPP
