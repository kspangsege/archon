// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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


                                            
#include <cerrno>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <utility>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/deque.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/string.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/endianness.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/math/vector.hpp>
#include <archon/image.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/mouse_button.hpp>
#include <archon/display/screen.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_x11.hpp>

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

using namespace archon;
namespace impl = display::impl;


namespace {


constexpr std::string_view g_implementation_ident = "x11";


#if HAVE_X11


// Compatible with XKeymapEvent::key_vector
class X11KeyCodeSet {
public:
    void assign(const char* bytes) noexcept
    {
        std::copy_n(bytes, 32, m_bytes);
    }

    bool contains(KeyCode keycode) const noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        return ((byte(i) & bit(i)) != 0);
    }

    void add(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) |= bit(i);
    }

    void remove(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) &= ~bit(i);
    }

private:
    char m_bytes[32] = {};

    auto byte(int i) const noexcept -> const unsigned char&
    {
        return reinterpret_cast<const unsigned char*>(m_bytes)[i / 8];
    }

    auto byte(int i) noexcept -> unsigned char&
    {
        return reinterpret_cast<unsigned char*>(m_bytes)[i / 8];
    }

    static int bit(int i) noexcept
    {
        return 1 << (i % 8);
    }
};


/*          
// Check whether the masks of the specified visual are all zero.
inline bool zero_mask_match(const XVisualInfo& info) noexcept
{
    return (info.red_mask == 0 && info.green_mask == 0 && info.blue_mask == 0);
}
*/


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is normal (normal channel order is RGB
// as opposed to BGR).
template<class P> inline bool norm_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2));
}


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is reversed (reversed channel order is
// BGR as opposed to RGB).
template<class P> inline bool rev_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0));
}


auto map_opt_visual_class(const std::optional<display::ConnectionConfigX11::VisualClass>& class_) noexcept ->
    std::optional<int>
{
    if (ARCHON_LIKELY(!class_.has_value()))
        return {};
    switch (class_.value()) {
        case display::ConnectionConfigX11::VisualClass::static_gray:
            return StaticGray;
        case display::ConnectionConfigX11::VisualClass::gray_scale:
            return GrayScale;
        case display::ConnectionConfigX11::VisualClass::static_color:
            return StaticColor;
        case display::ConnectionConfigX11::VisualClass::pseudo_color:
            return PseudoColor;
        case display::ConnectionConfigX11::VisualClass::true_color:
            return TrueColor;
        case display::ConnectionConfigX11::VisualClass::direct_color:
            return DirectColor;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


auto get_visual_class_name(int class_) noexcept -> const char*
{
    switch (class_) {
        case StaticGray:
            return "StaticGray";
        case GrayScale:
            return "GrayScale";
        case StaticColor:
            return "StaticColor";
        case PseudoColor:
            return "PseudoColor";
        case TrueColor:
            return "TrueColor";
        case DirectColor:
            return "DirectColor";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


auto map_opt_visual_type(const std::optional<std::uint_fast32_t>& type) -> std::optional<VisualID>
{
    if (ARCHON_LIKELY(!type.has_value()))
        return {};
    std::uint_fast32_t type_2 = type.value();
    if (ARCHON_LIKELY(type_2 <= core::int_mask<std::uint_fast32_t>(32)))
        return VisualID(type_2);
    throw std::invalid_argument("Visual type out of range");
}


class PixelCodec {
public:
    virtual auto intern_color(util::Color color) const -> unsigned long = 0;
    virtual void create_image(display::Size size, std::unique_ptr<image::WritableImage>& img, char*& data) const = 0;
    virtual ~PixelCodec() = default;
};


/*                 
template<class T, class P, int N, bool R> class DirectGrayPixelCodec final
    : public PixelCodec {
public:
    using compound_type = T;
    using packing_type = P;
    static constexpr int bytes_per_pixel = N;
    static constexpr bool reverse_channel_order = R;

    auto intern_color(util::Color) const -> unsigned long override;
    void create_image(display::Size, std::unique_ptr<image::WritableImage>&, char*&) const override;
};
*/


template<class T, class P, int N, bool R> class DirectColorPixelCodec final
    : public PixelCodec {
public:
    using compound_type = T;
    using packing_type = P;
    static constexpr int bytes_per_pixel = N;
    static constexpr bool reverse_channel_order = R;

    auto intern_color(util::Color) const -> unsigned long override;
    void create_image(display::Size, std::unique_ptr<image::WritableImage>&, char*&) const override;
};


template<class T, class P, int N, bool R>
auto DirectColorPixelCodec<T, P, N, R>::intern_color(util::Color color) const -> unsigned long
{
    using channel_packing_type = P;
    static_assert(channel_packing_type::num_fields == 3);

    constexpr int red_width   = image::get_bit_field_width(channel_packing_type::fields, 3, 0);
    constexpr int green_width = image::get_bit_field_width(channel_packing_type::fields, 3, 1);
    constexpr int blue_width  = image::get_bit_field_width(channel_packing_type::fields, 3, 2);

    constexpr int red_shift   = image::get_bit_field_shift(channel_packing_type::fields, 3, 0);
    constexpr int green_shift = image::get_bit_field_shift(channel_packing_type::fields, 3, 1);
    constexpr int blue_shift  = image::get_bit_field_shift(channel_packing_type::fields, 3, 2);

    using ulong = unsigned long;
    namespace uf = util::unit_frac;
    ulong r, g, b;
    if (ARCHON_LIKELY(color.is_opaque())) {
        r = image::int_to_int<8, ulong, red_width>(color.red());
        g = image::int_to_int<8, ulong, green_width>(color.green());
        b = image::int_to_int<8, ulong, blue_width>(color.blue());
    }
    else {
        image::float_type a = image::int_to_float<8, image::float_type>(color.alpha());
        r = image::float_to_compressed_int<ulong, red_width>(a * image::compressed_int_to_float<8>(color.red()));
        g = image::float_to_compressed_int<ulong, green_width>(a * image::compressed_int_to_float<8>(color.green()));
        b = image::float_to_compressed_int<ulong, blue_width>(a * image::compressed_int_to_float<8>(color.blue()));
    }
    return (r << red_shift) | (g << green_shift) | (b << blue_shift);
}


template<class T, class P, int N, bool R>
void DirectColorPixelCodec<T, P, N, R>::create_image(display::Size size, std::unique_ptr<image::WritableImage>& img,
                                                     char*& data) const
{
    using word_type = char;
    constexpr int bits_per_word = 8;
    constexpr core::Endianness word_order = core::Endianness::little;
    constexpr bool alpha_channel_first = false;

    using format_type = image::PackedPixelFormat<image::ChannelSpec_RGB, compound_type, packing_type, word_type,
                                                 bits_per_word, bytes_per_pixel, word_order, alpha_channel_first,
                                                 reverse_channel_order>;

    auto img_2 = std::make_unique<image::BufferedImage<format_type>>(size); // Throws
    data = img_2->get_buffer().data();
    img = std::move(img_2);
}


auto make_pixel_codec(const XVisualInfo& info, int bits_per_pixel,
                      const std::locale& locale) -> std::unique_ptr<PixelCodec>
{
    std::string msg;

    if (info.depth == 8) {
        if (bits_per_pixel != 8)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 1;
/*            
        if (info.c_class == StaticGray || info.c_class == GrayScale) {
            if (ARCHON_UNLIKELY(info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (ARCHON_LIKELY(zero_mask_match(info)))
                return std::make_unique<DirectGrayPixelCodec<image::int8_type, 8, bytes_per_pixel>>(); // Throws
            goto unsupported_channel_masks;
        }
*/
/*
        if (info.c_class == StaticColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 256))
                goto unexpected_colormap_size;
            // According to the X specification, masks only make sense for TrueColor and
            // DirectColor visuals. Despite that, it appears that some X servers choose
            // to expose the color structure of StaticColor visuals using masks, most
            // notably Xvfb + X.Org (e.g., using `Xvfb :1 -screen 0 1600x1200x8).
            if (norm_mask_match<image::ChannelPacking_332>(info)) {
                constexpr bool reverse_channel_order = false;
                auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                             reverse_channel_order>(img_size); // Throws
                data = img->get_buffer().data();
                img_2 = std::move(img);
                goto matched;
            }
            if (rev_mask_match<image::ChannelPacking_233>(info)) {
                constexpr bool reverse_channel_order = true;
                auto img = make_packed_image<image::int8_type, image::ChannelPacking_233, bytes_per_pixel,
                                             reverse_channel_order>(img_size); // Throws
                data = img->get_buffer().data();
                img_2 = std::move(img);
                goto matched;
            }
            if (zero_mask_match(info)) {
                int n = 1 << 8;
                auto colors = std::make_unique<XColor[]>(n); // Throws
                for (int i = 0; i < n; ++i) {
                    XColor& color = colors[i];
                    color.pixel = unsigned(i);
                }
                XQueryColors(dpy, colormap, colors.get(), n);
/
                for (int i = 0; i < n; ++i) {
                    const XColor& color = colors[i];
                    using comp_type = util::Color::comp_type;
                    util::Color color_2(comp_type(color.red   >> 8),
                                        comp_type(color.green >> 8),
                                        comp_type(color.blue  >> 8));
                    logger.info("Color %s: %s", core::as_int(i + 1), util::as_css_color(color_2));
                }
/
                // Create image with indirect color     
                // FIXME: Read out palette and produce indirect color version of image with respect to that palette                                        
                bool implemented = false;
                ARCHON_STEADY_ASSERT(implemented);                    
                goto matched;
            }
            goto unsupported_channel_masks;
        }
        if (info.c_class == PseudoColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (zero_mask_match(info)) {
                // FIXME: Consider XGetRGBColormaps() --> https://tronche.com/gui/x/xlib/ICC/standard-colormaps/XGetRGBColormaps.html --> Fetch all available colormaps, look for one with matching visual ID. If one is found, use that colormap. This requires that the image is converted to indirect color pixel format.                
                // FIXME: Consider alternative: Generate optimal palette for image of, say 248, entries, then request that many color slots, then initialize those slots with the colors of the palette, then convert image to indirect color using that palette.      
                constexpr bool reverse_channel_order = false;
                auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                             reverse_channel_order>(img_size); // Throws
                data = img->get_buffer().data();
                img_2 = std::move(img);
                int red_width   = 3;
                int green_width = 3;
                int blue_width  = 2;
                setup_pseudo_color_colormap(dpy, colormap, red_width, green_width, blue_width, !preallocate_colors,
                                            use_weird_palette); // Throws
                goto matched;
            }
            goto unsupported_channel_masks;
        }
*/
        if (info.c_class == TrueColor || info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 8))
                goto unexpected_colormap_size;
            if (norm_mask_match<image::ChannelPacking_332>(info)) {
                constexpr bool reverse_channel_order = false;
                return std::make_unique<DirectColorPixelCodec<image::int8_type, image::ChannelPacking_332,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_233>(info)) {
                constexpr bool reverse_channel_order = true;
                return std::make_unique<DirectColorPixelCodec<image::int8_type, image::ChannelPacking_233,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (info.depth == 15) {
        if (bits_per_pixel != 16)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 2;
        if (info.c_class == TrueColor || info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 32))
                goto unexpected_colormap_size;
            if (norm_mask_match<image::ChannelPacking_555>(info)) {
                constexpr bool reverse_channel_order = false;
                return std::make_unique<DirectColorPixelCodec<image::int16_type, image::ChannelPacking_555,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_555>(info)) {
                constexpr bool reverse_channel_order = true;
                return std::make_unique<DirectColorPixelCodec<image::int16_type, image::ChannelPacking_555,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (info.depth == 16) {
        if (bits_per_pixel != 16)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 2;
        if (info.c_class == TrueColor || info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 64))
                goto unexpected_colormap_size;
            if (norm_mask_match<image::ChannelPacking_565>(info)) {
                constexpr bool reverse_channel_order = false;
                return std::make_unique<DirectColorPixelCodec<image::int16_type, image::ChannelPacking_565,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            else if (rev_mask_match<image::ChannelPacking_565>(info)) {
                constexpr bool reverse_channel_order = true;
                return std::make_unique<DirectColorPixelCodec<image::int16_type, image::ChannelPacking_565,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (info.depth == 24) {
        if (bits_per_pixel != 32)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 4;
        if (info.c_class == TrueColor || info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (norm_mask_match<image::ChannelPacking_888>(info)) {
                constexpr bool reverse_channel_order = false;
                return std::make_unique<DirectColorPixelCodec<image::int32_type, image::ChannelPacking_888,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_888>(info)) {
                constexpr bool reverse_channel_order = true;
                return std::make_unique<DirectColorPixelCodec<image::int32_type, image::ChannelPacking_888,
                                                              bytes_per_pixel, reverse_channel_order>>(); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    goto unsupported_depth;

  unsupported_depth:
    msg = core::format(locale, "Unsupported depth: %s", info.depth); // Throws
    throw std::runtime_error(msg);

  unsupported_bits_per_pixel:
    msg = core::format(locale, "Unsupported number bits per pixel for depth %s: %s", info.depth,
                       bits_per_pixel); // Throws
    throw std::runtime_error(msg);

  unexpected_visual_class:
    msg = core::format(locale, "Unexpected class for visual 0x%s: %s", core::as_hex_int(info.visualid),
                 get_visual_class_name(info.c_class)); // Throws
    throw std::runtime_error(msg);

  unsupported_channel_masks:
    msg = core::format(locale, "Unsupported channel masks in visual 0x%s: red = %s, green = %s, blue = %s",
                 core::as_hex_int(info.visualid), core::as_hex_int(info.red_mask),
                 core::as_hex_int(info.green_mask), core::as_hex_int(info.blue_mask)); // Throws
    throw std::runtime_error(msg);

  unexpected_colormap_size:
    msg = core::format(locale, "Unexpected colormap size for visual 0x%s: %s", core::as_hex_int(info.visualid),
                 info.colormap_size); // Throws
    throw std::runtime_error(msg);
}


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


#if HAVE_XRANDR

struct ProtoScreen {
    core::IndexRange output_name;
    display::Box bounds;
    std::optional<core::IndexRange> monitor_name;
    std::optional<display::Resolution> resolution;
    std::optional<double> refresh_rate;
};

#endif // HAVE_XRANDR


// One slot for each X11 screen
struct ScreenSlot {
    bool is_initialized = false;
    int screen = {};
    Window root = {};
    VisualID default_visual = {};
    core::Slab<VisualSpec> visual_specs;
    core::FlatMap<VisualID, Colormap> colormaps;

#if HAVE_XRANDR
    std::vector<ProtoScreen> screens;
    core::Buffer<char> screens_string_buffer;
    std::size_t screens_string_buffer_used_size = 0;
#endif // HAVE_XRANDR
};


bool map_key(display::KeyCode, display::Key&) noexcept;
bool rev_map_key(display::Key, display::KeyCode&) noexcept;

bool try_map_mouse_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                          math::Vector2F& amount) noexcept;


class WindowImpl;
class TextureImpl;


class ImplementationImpl final
    : public display::Implementation {
public:
    ImplementationImpl(Slot&) noexcept;

    auto new_connection(const std::locale&, const display::Connection::Config&) const ->
        std::unique_ptr<display::Connection> override;
    auto get_slot() const noexcept -> const Slot& override;

private:
    const Slot& m_slot;
};


class SlotImpl final
    : public display::Implementation::Slot {
public:
    SlotImpl() noexcept;

    auto ident() const noexcept -> std::string_view override;
    auto get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation* override;

private:
    ImplementationImpl m_impl;
};


class ConnectionImpl final
    : private display::ConnectionEventHandler
    , public display::Connection {
public:
    const ImplementationImpl& impl;
    const std::locale locale;
    log::Logger& logger;
    Display* dpy = nullptr;

    Atom atom_wm_protocols;
    Atom atom_wm_delete_window;
    Atom atom_net_wm_state;
    Atom atom_net_wm_state_fullscreen;

#if HAVE_XRANDR
    Atom atom_edid;
#endif

    ConnectionImpl(const ImplementationImpl&, const std::locale&, log::Logger*, const display::ConnectionConfigX11&);
    ~ConnectionImpl() noexcept override;

    void open(const display::ConnectionConfigX11&);
    void register_window(::Window id, WindowImpl&);
    void unregister_window(::Window id) noexcept;

    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override;
    auto new_window(std::string_view, display::Size, display::WindowEventHandler&, const display::Window::Config&) ->
        std::unique_ptr<display::Window> override;
    void process_events(display::ConnectionEventHandler*) override;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override;
    int get_num_displays() const override;
    int get_default_display() const override;
    bool try_get_display_conf(int, core::Buffer<display::Screen>&, core::Buffer<char>&, std::size_t&,
                              bool&) const override;
    auto get_implementation() const noexcept -> const display::Implementation& override;

private:
    const std::optional<int> m_depth_override;
    const std::optional<int> m_class_override;
    const std::optional<VisualID> m_visual_override;
    const bool m_disable_double_buffering;
    const bool m_disable_glx_direct_rendering;

    // X protocol extension availability
    bool m_have_xdbe    = false; // X Double Buffer Extension
    bool m_have_xrandr  = false; // X Resize, Rotate and Reflect Extension
    bool m_have_xrender = false; // X Rendering Extension
    bool m_have_glx     = false; // X extension for rendering using OpenGL

    bool m_detectable_autorepeat_enabled = false;
    bool m_expect_keymap_notify = false;
    bool m_have_curr_window = false;

    int m_xrandr_event_base = 0;

    // Key is visual depth
    core::FlatMap<int, XPixmapFormatValues> m_pixmap_formats;

    mutable std::unique_ptr<ScreenSlot[]> m_screen_slots;
    mutable core::FlatMap<::Window, int> m_x11_screens_by_root;

#if HAVE_XRANDR
    mutable std::optional<impl::EdidParser> m_edid_parser;
#endif

    X11KeyCodeSet m_pressed_keys;

    core::FlatMap<::Window, WindowImpl&> m_windows;

    // Track pointer grabs so that "mouse over" and "mouse out" events can be ignored when
    // they occur during a grab.
    //
    // If the pointer leaves the window during a pointer grab and the grab ends outside the
    // window, there is a question of whether the "mouse out" event should occur when the
    // pointer leaves the window or when the grab ends. SDL (Simple DirectMedia Layer) opts
    // to let the "mouse out" event occur when the grab ends, and, unfortunately, there is
    // no way to emulate the other behavior when using SDL.
    //
    // X11, on the other hand, generates a "mouse out" event in both cases, that is when the
    // pointer leaves the window and when the grab ends. With this, we can emulate the SDL
    // behavior using X11 by ignoring all "mouse over" and "mouse out" event while a grab is
    // in progress.
    //
    // In the interest of alignment across display implementations and with the SDL-based
    // implementation in particular (`implementation_sdl.cpp`), the required behavior of
    // display implementations is to generate the "mouse out" event when the grab ends. See
    // also display::EventHandler::on_mouseover().
    //
    core::FlatSet<::Window> m_pointer_grab_buttons;
    ::Window m_pointer_grab_window_id = {};

    // A queue of windows with pending expose events (push to back and pop from
    // front). Windows occur at most once in this queue.
    //
    // INVARIANT: A window is in `m_exposed_windows` if and only if it is in `m_windows` and
    // has `has_pending_expose_event` set to `true`.
    core::Deque<::Window> m_exposed_windows;

    // X11 timestamps are 32-bit unsigned integers and `Time` refers to the unsigned integer
    // type that X11 uses to store these timestamps.
    using timestamp_unwrapper_type = impl::TimestampUnwrapper<Time, 32>;
    timestamp_unwrapper_type m_timestamp_unwrapper;

    // If `m_have_curr_window` is true, then `m_curr_window` specifies the window identified
    // by `m_curr_window_id`. If `m_have_curr_window` is false, `m_curr_window_id` and
    // `m_curr_window` have no meaning.
    //
    // If `m_have_curr_window` is true, but `m_curr_window` is null, it means that the X
    // client has no knowledge of a window with the ID specified by `m_curr_window_id`. This
    // state is entered if the window specified by `m_curr_window_id` is unregistered
    // (unregister_window()). The state is updated whenever a new window is registered
    // (register_window()). This takes care of the case where a new window reuses the ID
    // specified by `m_curr_window_id`.
    //
    ::Window m_curr_window_id = {};
    WindowImpl* m_curr_window = nullptr;

    int m_num_events = 0;

    auto intern_string(const char*) noexcept -> Atom;
    auto ensure_screen_slot(int screen) const -> ScreenSlot&;
    auto determine_visual_spec(const ScreenSlot&, bool prefer_double_buffered, bool require_opengl) const ->
        const VisualSpec&;
    auto ensure_colormap(ScreenSlot&, VisualID) const -> Colormap;
    bool do_process_events(const time_point_type* deadline, display::ConnectionEventHandler*);
    bool lookup_window(::Window window_id, WindowImpl*& window) noexcept;
    void track_pointer_grabs(::Window window_id, unsigned button, bool is_press);
    bool is_pointer_grabbed() const noexcept;

#if HAVE_XRANDR
    bool update_display_info(ScreenSlot& slot) const;
    bool try_update_display_info(ScreenSlot& slot, bool& changed) const;
    auto ensure_edid_parser() const -> const impl::EdidParser&;
#endif
};


class WindowImpl final
    : public display::Window {
public:
    ConnectionImpl& conn;
    const ScreenSlot& screen_slot;
    const VisualSpec& visual_spec;
    const XPixmapFormatValues& pixmap_format;
    display::WindowEventHandler& event_handler;
    const int cookie;

    ::Window win = None;

    bool has_pending_expose_event = false;

    WindowImpl(ConnectionImpl&, const ScreenSlot&, const VisualSpec&, const XPixmapFormatValues&,
               display::WindowEventHandler&, int cookie, std::unique_ptr<const PixelCodec>) noexcept;
    ~WindowImpl() noexcept override;

    void create(display::Size size, Colormap colormap, const Config& config, bool enable_double_buffering,
                bool enable_opengl, bool enable_glx_direct_rendering);
    auto ensure_graphics_context() noexcept -> GC;

    void show() override;
    void hide() override;
    void set_title(std::string_view) override;
    void set_size(display::Size) override;
    void set_fullscreen_mode(bool) override;
    void fill(util::Color) override;
    void fill(util::Color, const display::Box&) override;
    auto new_texture(display::Size) -> std::unique_ptr<display::Texture> override;
    void put_texture(const display::Texture&, const display::Pos&) override;
    void put_texture(const display::Texture&, const display::Box&, const display::Pos&) override;
    void present() override;
    void opengl_make_current() override;
    void opengl_swap_buffers() override;

private:
    bool m_is_registered = false;
    bool m_is_double_buffered = false;

    const std::unique_ptr<const PixelCodec> m_pixel_codec;
    GC m_gc = None;

    Drawable m_drawable;
#if HAVE_XDBE
    XdbeSwapAction m_swap_action;
#endif

#if HAVE_GLX
    GLXContext m_ctx = nullptr;
#endif

    void set_property(Atom name, Atom value) noexcept;
    void do_fill(util::Color color, int x, int y, unsigned w, unsigned h);
    void do_put_texture(const TextureImpl&, const display::Box& source_area, const display::Pos& pos);
    auto create_graphics_context() noexcept -> GC;
    auto intern_color(util::Color) -> unsigned long;
    auto get_pixel_codec() -> const PixelCodec&;
};


class TextureImpl final
    : public display::Texture {
public:
    WindowImpl& win;
    const display::Size size;
    Pixmap pixmap = None;

    TextureImpl(WindowImpl&, const display::Size& size);
    ~TextureImpl() noexcept override;

    void create(const PixelCodec&);

    void put_image(const image::Image&) override;

private:
    std::unique_ptr<image::WritableImage> m_img;
    char* m_img_data = nullptr;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Connection::Config& config) const ->
    std::unique_ptr<display::Connection>
{
    auto conn = std::make_unique<ConnectionImpl>(*this, locale, config.logger, config.x11); // Throws
    conn->open(config.x11); // Throws
    return conn;
}


auto ImplementationImpl::get_slot() const noexcept -> const Slot&
{
    return m_slot;
}



inline SlotImpl::SlotImpl() noexcept
    : m_impl(*this)
{
}


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees& guarantees) const noexcept ->
    const display::Implementation*
{
    bool is_available = (guarantees.no_other_use_of_x11 &&
                         guarantees.main_thread_exclusive);
    if (ARCHON_LIKELY(is_available))
        return &m_impl;
    return nullptr;
}



inline ConnectionImpl::ConnectionImpl(const ImplementationImpl& impl_2, const std::locale& locale_2,
                                      log::Logger* logger, const display::ConnectionConfigX11& config)
    : impl(impl_2)
    , locale(locale_2)
    , logger(log::Logger::or_null(logger))
    , m_depth_override(config.visual_depth)
    , m_class_override(map_opt_visual_class(config.visual_class))
    , m_visual_override(map_opt_visual_type(config.visual_type)) // Throws
    , m_disable_double_buffering(config.disable_double_buffering)
    , m_disable_glx_direct_rendering(config.disable_glx_direct_rendering)
{
}


ConnectionImpl::~ConnectionImpl() noexcept
{
    if (dpy)
        XCloseDisplay(dpy);
}


void ConnectionImpl::open(const display::ConnectionConfigX11& config)
{
    std::string display_name;
    if (!config.display.empty()) {
        display_name = std::string(config.display); // Throws
    }
    else if (char* val = std::getenv("DISPLAY")) {
        display_name = std::string(val); // Throws
    }
    Display* dpy_2 = XOpenDisplay(display_name.data());
    if (ARCHON_UNLIKELY(!dpy_2)) {
        std::string message = core::format(locale, "Failed to open X11 display connection (%s)",
                                           core::quoted(std::string_view(display_name))); // Throws
        throw std::runtime_error(message);
    }

    dpy = dpy_2;

    if (ARCHON_UNLIKELY(config.synchronous_mode))
        XSynchronize(dpy, True);

#if HAVE_XDBE
    {
        int major = 0;
        int minor = 0;
        if (ARCHON_LIKELY(XdbeQueryExtension(dpy, &major, &minor))) {
            if (ARCHON_LIKELY(major >= 1))
                m_have_xdbe = true;
        }
    }
#endif // HAVE_XDBE

    {
        int have_xkb = false;
        int opcode = 0;     // Unused
        int event_base = 0; // Unused
        int error_base = 0; // Unused
        int major = 0;
        int minor = 0;
        if (ARCHON_LIKELY(XkbQueryExtension(dpy, &opcode, &event_base, &error_base, &major, &minor))) {
            if (ARCHON_LIKELY(major >= 1))
                have_xkb = true;
        }
        if (ARCHON_UNLIKELY(!have_xkb))
            throw std::runtime_error("X Keyboard Extension is required but not available");
    }

    if (!config.disable_detectable_autorepeat) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            m_detectable_autorepeat_enabled = true;
    }

#if HAVE_XRANDR
    {
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRRQueryExtension(dpy, &m_xrandr_event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRRQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRRQueryVersion() failed");
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 5)))
                m_have_xrandr = true;
        }
    }
#endif // HAVE_XRANDR

#if HAVE_XRENDER
    {
        int event_base = 0; // Unused
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRenderQueryExtension(dpy, &event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRenderQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRenderQueryVersion() failed");
            if (ARCHON_LIKELY(major > 0 || (major == 0 && minor >= 7)))
                m_have_xrender = true;
        }
    }
#endif // HAVE_XRENDER

#if HAVE_GLX
    {
        int error_base = 0; // Unused
        int event_base = 0; // Unused
        if (ARCHON_LIKELY(glXQueryExtension(dpy, &error_base, &event_base))) {
            int major = 0;
            int minor = 0;
            Bool success = glXQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(!success))
                throw std::runtime_error("glXQueryVersion() failed");
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 4)))
                m_have_glx = true;
        }
    }
#endif // HAVE_GLX

    // Fetch ZPixmap formats
    {
        int n = 0;
        XPixmapFormatValues* entries = XListPixmapFormats(dpy, &n);
        if (!entries)
            throw std::runtime_error("XListPixmapFormats() failed");
        ARCHON_SCOPE_EXIT {
            XFree(entries);
        };
        std::size_t n_2 = {};
        core::int_cast(n, n_2); // Throws
        m_pixmap_formats.reserve(n_2); // Throws
        m_pixmap_formats.clear();
        for (std::size_t i = 0; i < n_2; ++i) {
            XPixmapFormatValues& format = entries[i];
            m_pixmap_formats[format.depth] = format;
        }
    }

    atom_wm_protocols            = intern_string("WM_PROTOCOLS");
    atom_wm_delete_window        = intern_string("WM_DELETE_WINDOW");
    atom_net_wm_state            = intern_string("_NET_WM_STATE");
    atom_net_wm_state_fullscreen = intern_string("_NET_WM_STATE_FULLSCREEN");
#if HAVE_XRANDR
    atom_edid = intern_string(RR_PROPERTY_RANDR_EDID);
#endif

    m_screen_slots = std::make_unique<ScreenSlot[]>(std::size_t(ScreenCount(dpy))); // Throws
}


inline void ConnectionImpl::register_window(::Window id, WindowImpl& window)
{
    auto i = m_windows.emplace(id, window); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Because a new window might reuse the ID currently specified by `m_curr_window_id`, it
    // is necessary, and not just desirable to reset the "current window state" here.
    m_curr_window_id = id;
    m_curr_window = &window;
    m_have_curr_window = true;
}


inline void ConnectionImpl::unregister_window(::Window id) noexcept
{
    std::size_t n = m_windows.erase(id);
    ARCHON_ASSERT(n == 1);

    if (ARCHON_UNLIKELY(m_pointer_grab_window_id == id))
        m_pointer_grab_buttons.clear();

    auto i = std::find(m_exposed_windows.begin(), m_exposed_windows.end(), id);
    if (i != m_exposed_windows.end())
        m_exposed_windows.erase(i);

    if (ARCHON_LIKELY(m_have_curr_window && id == m_curr_window_id))
        m_curr_window = nullptr;
}


bool ConnectionImpl::try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const
{
    return ::rev_map_key(key, key_code);
}


bool ConnectionImpl::try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const
{
    return ::map_key(key_code, key);
}


bool ConnectionImpl::try_get_key_name(display::KeyCode key_code, std::string_view& name) const
{
    // XKeysymToString() returns a string consisting entirely of characters from the X
    // Portable Character Set. Since all locales, that are compatible with Xlib, agree on
    // the encoding of characters in this character set, and since we assume that the
    // selected locale is compatible with Xlib, we can assume that the returned string is
    // valid in the selected locale.

    auto keysym = KeySym(key_code.code);
    const char* c_str = XKeysymToString(keysym);
    if (ARCHON_LIKELY(c_str)) {
        name = std::string_view(c_str);
        return true;
    }
    return false;
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size, display::WindowEventHandler& event_handler,
                                const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    int screen = config.display;
    if (ARCHON_LIKELY(screen < 0)) {
        screen = DefaultScreen(dpy);
    }
    else if (ARCHON_UNLIKELY(screen < 0 || screen >= int(ScreenCount(dpy)))) {
        throw std::invalid_argument("Bad display index");
    }
    bool prefer_double_buffered = false;
    if (config.enable_basic_rendering) {
        if (ARCHON_LIKELY(!m_disable_double_buffering))
            prefer_double_buffered = true;
    }
    bool enable_opengl = false;
    if (config.enable_opengl_rendering) {
        if (ARCHON_UNLIKELY(!m_have_glx))
            throw std::runtime_error("OpenGL rendering not available");
        enable_opengl = true;
    }
    ScreenSlot& screen_slot = ensure_screen_slot(screen); // Throws
    const VisualSpec& visual_spec = determine_visual_spec(screen_slot, prefer_double_buffered,
                                                          enable_opengl); // Throws
    logger.detail("Using visual depth %s and visual type %s for new X11 window", visual_spec.info.depth,
                  core::as_flex_int_h(visual_spec.info.visualid)); // Throws
    Colormap colormap = ensure_colormap(screen_slot, visual_spec.info.visualid); // Throws
    const XPixmapFormatValues* pixmap_format;
    {
        auto i = m_pixmap_formats.find(visual_spec.info.depth);
        ARCHON_STEADY_ASSERT(i != m_pixmap_formats.end());
        pixmap_format = &i->second;
    }
    std::unique_ptr<const PixelCodec> pixel_codec;
    if (config.enable_basic_rendering)
        pixel_codec = make_pixel_codec(visual_spec.info, pixmap_format->bits_per_pixel, locale); // Throws
    auto win = std::make_unique<WindowImpl>(*this, screen_slot, visual_spec, *pixmap_format, event_handler,
                                            config.cookie, std::move(pixel_codec)); // Throws
    bool enable_double_buffering = visual_spec.double_buffered && !m_disable_double_buffering;
    bool enable_glx_direct_rendering = !m_disable_glx_direct_rendering;
    win->create(size, colormap, config, enable_double_buffering, enable_opengl, enable_glx_direct_rendering); // Throws
    win->set_title(title); // Throws
    if (ARCHON_UNLIKELY(config.fullscreen))
        win->set_fullscreen_mode(true); // Throws
    return win;
}


void ConnectionImpl::process_events(display::ConnectionEventHandler* connection_event_handler)
{
    const time_point_type* deadline = nullptr;
    do_process_events(deadline, connection_event_handler); // Throws
}


bool ConnectionImpl::process_events(time_point_type deadline,
                                    display::ConnectionEventHandler* connection_event_handler)
{
    return do_process_events(&deadline, connection_event_handler); // Throws
}


int ConnectionImpl::get_num_displays() const
{
    return int(ScreenCount(dpy));
}


int ConnectionImpl::get_default_display() const
{
    return int(DefaultScreen(dpy));
}


bool ConnectionImpl::try_get_display_conf(int display, core::Buffer<display::Screen>& screens,
                                          core::Buffer<char>& strings, std::size_t& num_screens, bool& reliable) const
{
    if (ARCHON_UNLIKELY(display < 0 || display >= int(ScreenCount(dpy))))
        throw std::invalid_argument("Bad display index");

#if HAVE_XRANDR
    const ScreenSlot& slot = ensure_screen_slot(display); // Throws
    std::size_t n = slot.screens.size();
    screens.reserve(n); // Throws
    const char* strings_base = slot.screens_string_buffer.data();
    strings.assign({ strings_base, slot.screens_string_buffer_used_size }); // Throws
    const char* strings_base_2 = strings.data();
    for (std::size_t i = 0; i < n; ++i) {
        const ProtoScreen& proto = slot.screens[i];
        std::optional<std::string_view> monitor_name;
        if (proto.monitor_name.has_value())
            monitor_name = proto.monitor_name.value().resolve_string(strings_base_2); // Throws
        screens[i] = {
            proto.output_name.resolve_string(strings_base_2), // Throws
            proto.bounds,
            monitor_name,
            proto.resolution,
            proto.refresh_rate,
        };
    }
    num_screens = n;
    reliable = true;
    return true;
#else // !HAVE_XRANDR
    static_cast<void>(screens);
    static_cast<void>(strings);
    static_cast<void>(num_screens);
    static_cast<void>(reliable);
    return false;
#endif // !HAVE_XRANDR
}


auto ConnectionImpl::get_implementation() const noexcept -> const display::Implementation&
{
    return impl;
}


inline auto ConnectionImpl::intern_string(const char* string) noexcept -> Atom
{
    Atom atom = XInternAtom(dpy, string, False);
    ARCHON_STEADY_ASSERT(atom != None);
    return atom;
}


auto ConnectionImpl::ensure_screen_slot(int screen) const -> ScreenSlot&
{
    ARCHON_ASSERT(m_screen_slots);
    ARCHON_ASSERT(screen >= 0 && screen <= ScreenCount(dpy));
    ScreenSlot& slot = m_screen_slots[screen];
    if (ARCHON_UNLIKELY(!slot.is_initialized)) {
        ::Window root = RootWindow(dpy, screen);
        slot.screen = screen;
        slot.root = root;
        slot.default_visual = XVisualIDFromVisual(DefaultVisual(dpy, screen));
        m_x11_screens_by_root[root] = screen; // Throws

        // Fetch information about supported visuals
        core::FlatMap<core::Pair<int, VisualID>, int> double_buffered_visuals;
#if HAVE_XDBE
        if (ARCHON_LIKELY(m_have_xdbe)) {
            Drawable screen_specifiers[] = {
                root,
            };
            int n = 1;
            XdbeScreenVisualInfo* entries = XdbeGetVisualInfo(dpy, screen_specifiers, &n);
            ARCHON_STEADY_ASSERT(entries);
            ARCHON_STEADY_ASSERT(n == 1);
            ARCHON_SCOPE_EXIT {
                XdbeFreeVisualInfo(entries);
            };
            const XdbeScreenVisualInfo& entry = entries[0];
            std::size_t n_2 = {};
            core::int_cast(entry.count, n_2); // Throws
            double_buffered_visuals.reserve(n_2); // Throws
            for (std::size_t i = 0; i < n_2; ++i) {
                const XdbeVisualInfo& subentry = entry.visinfo[i];
                auto p = double_buffered_visuals.emplace(core::Pair(subentry.depth, subentry.visual),
                                                         subentry.perflevel); // Throws
                bool was_inserted = p.second;
                ARCHON_ASSERT(was_inserted);
            }
        }
#endif // HAVE_XDBE
        int n = {};
        long vinfo_mask = VisualScreenMask;
        XVisualInfo vinfo_template = {};
        vinfo_template.screen = screen;
        XVisualInfo* entries = XGetVisualInfo(dpy, vinfo_mask, &vinfo_template, &n);
        if (ARCHON_LIKELY(entries)) {
            ARCHON_SCOPE_EXIT {
                XFree(entries);
            };
            std::size_t n_2 = {};
            core::int_cast(n , n_2); // Throws
            slot.visual_specs.recreate(n_2); // Throws
            for (std::size_t i = 0; i < n_2; ++i) {
                XVisualInfo& info = entries[i];
                bool double_buffered = false;
                int double_buffered_perflevel = 0;
                {
                    auto i = double_buffered_visuals.find(core::Pair(info.depth, info.visualid));
                    if (i != double_buffered_visuals.end()) {
                        double_buffered = true;
                        double_buffered_perflevel = i->second;
                    }
                }
                bool opengl_supported = false;
                int opengl_level = 0;
                bool opengl_double_buffered = false;
                bool opengl_stereo = false;
                int opengl_num_aux_buffers = 0;
                int opengl_depth_buffer_bits = 0;
                int opengl_stencil_buffer_bits = 0;
                int opengl_accum_buffer_bits = 0;
#if HAVE_GLX
                if (ARCHON_LIKELY(m_have_glx)) {
                    auto get = [&](int attrib) {
                        int value = {};
                        int ret = glXGetConfig(dpy, &info, attrib, &value);
                        if (ARCHON_UNLIKELY(ret != 0))
                            throw std::runtime_error("glXGetConfig() failed");
                        return value;
                    };
                    if (get(GLX_USE_GL) != 0) { // Throws
                        opengl_supported           = true;
                        opengl_level               = get(GLX_LEVEL); // Throws
                        opengl_double_buffered     = (get(GLX_DOUBLEBUFFER) != 0); // Throws
                        opengl_stereo              = (get(GLX_STEREO) != 0); // Throws
                        opengl_num_aux_buffers     = get(GLX_AUX_BUFFERS); // Throws
                        opengl_depth_buffer_bits   = get(GLX_DEPTH_SIZE); // Throws
                        opengl_stencil_buffer_bits = get(GLX_STENCIL_SIZE); // Throws
                        opengl_accum_buffer_bits   = (get(GLX_ACCUM_RED_SIZE) + get(GLX_ACCUM_GREEN_SIZE) +
                                                      get(GLX_ACCUM_BLUE_SIZE) + get(GLX_ACCUM_ALPHA_SIZE)); // Throws
                    }
                }
#endif // HAVE_GLX
                VisualSpec spec = {
                    info,
                    double_buffered,
                    opengl_supported,
                    opengl_double_buffered,
                    opengl_stereo,
                    double_buffered_perflevel,
                    opengl_level,
                    opengl_num_aux_buffers,
                    opengl_depth_buffer_bits,
                    opengl_stencil_buffer_bits,
                    opengl_accum_buffer_bits,
                };
                slot.visual_specs.add(spec);
            }
        }

        // Fetch initial screen configuration
#if HAVE_XRANDR
        if (ARCHON_LIKELY(m_have_xrandr)) {
            int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
            XRRSelectInput(dpy, root, mask);
            update_display_info(slot); // Throws
        }
#endif // HAVE_XRANDR

        slot.is_initialized = true;
    }
    return slot;
}


auto ConnectionImpl::determine_visual_spec(const ScreenSlot& screen_slot, bool prefer_double_buffered,
                                           bool require_opengl) const -> const VisualSpec&
{
    bool prefer_default_visual_type = true;             
    bool prefer_default_visual_depth = true;             
    bool require_opengl_depth_buffer = require_opengl;             
    bool require_opengl_stencil_buffer = false;             
    bool require_opengl_accum_buffer = false;             
    int min_opengl_depth_buffer_bits = 8;             
    int min_opengl_stencil_buffer_bits = 1;             
    int min_opengl_accum_buffer_bits = 32;             
    auto filter = [&](const VisualSpec& spec) {
        if (m_depth_override.has_value() && spec.info.depth != m_depth_override.value())
            return false;
        if (m_class_override.has_value() && spec.info.c_class != m_class_override.value())
            return false;
        if (m_visual_override.has_value() && spec.info.visualid != m_visual_override.value())
            return false;
        if (require_opengl && !spec.opengl_supported)
            return false;
        if (spec.opengl_level != 0)
            return false;
        if (spec.opengl_stereo)
            return false;
        if (require_opengl_depth_buffer && spec.opengl_depth_buffer_bits < min_opengl_depth_buffer_bits)
            return false;
        if (require_opengl_stencil_buffer && spec.opengl_stencil_buffer_bits < min_opengl_stencil_buffer_bits)
            return false;
        if (require_opengl_accum_buffer && spec.opengl_accum_buffer_bits < min_opengl_accum_buffer_bits)
            return false;
        return true;
    };
    auto get_class_value = [](int class_) {
        switch (class_) {
            case StaticGray:
                return 1;
            case GrayScale:
                return 0;
            case StaticColor:
                return 3;
            case PseudoColor:
                return 2;
            case TrueColor:
                return 5;
            case DirectColor:
                return 4;
        }
        ARCHON_ASSERT_UNREACHABLE();
        return 0;
    };
    int screen = screen_slot.screen;
    int default_depth = DefaultDepth(dpy, screen);
    auto less = [&](const VisualSpec& a, const VisualSpec& b) {
        constexpr int max_criteria = 18;
        int values_1[max_criteria];
        int values_2[max_criteria];
        int num_criteria = 0;

        // Criterion 1: Prefer default visual
        if (prefer_default_visual_type) {
            int i = num_criteria++;
            values_1[i] = int(a.info.visualid == screen_slot.default_visual);
            values_2[i] = int(b.info.visualid == screen_slot.default_visual);
        }

        // Critera 2 and 3: Prefer default depth
        if (prefer_default_visual_depth) {
            int i_1 = num_criteria++;
            int i_2 = num_criteria++;
            values_1[i_1] = 0;
            values_1[i_2] = 0;
            if (a.info.depth >= default_depth) {
                values_1[i_1] = 1;
                values_1[i_2] = -a.info.depth; // Non-positive
            }
            values_2[i_1] = 0;
            values_2[i_2] = 0;
            if (b.info.depth >= default_depth) {
                values_2[i_1] = 1;
                values_2[i_2] = -b.info.depth; // Non-positive
            }
        }

        // Criterion 4: Best class
        {
            int i = num_criteria++;
            values_1[i] = get_class_value(a.info.c_class);
            values_2[i] = get_class_value(b.info.c_class);
        }

        // Criterion 5: Prefer double buffered
        if (prefer_double_buffered) {
            int i = num_criteria++;
            values_1[i] = int(a.double_buffered);
            values_2[i] = int(b.double_buffered);
        }

        // Criterion 6: Prefer OpenGL double buffered
        if (require_opengl) {
            int i = num_criteria++;
            values_1[i] = int(a.opengl_double_buffered);
            values_2[i] = int(b.opengl_double_buffered);
        }

        // Criterion 7: Greatest depth
        {
            int i = num_criteria++;
            values_1[i] = a.info.depth;
            values_2[i] = b.info.depth;
        }

        // Criterion 8: Highest depth buffer bit width
        if (require_opengl_depth_buffer) {
            int i = num_criteria++;
            values_1[i] = a.opengl_depth_buffer_bits;
            values_2[i] = b.opengl_depth_buffer_bits;
        }

        // Criterion 9: Highest stencil buffer bit width
        if (require_opengl_stencil_buffer) {
            int i = num_criteria++;
            values_1[i] = a.opengl_stencil_buffer_bits;
            values_2[i] = b.opengl_stencil_buffer_bits;
        }

        // Criterion 10: Highest accumulation buffer bit width
        if (require_opengl_accum_buffer) {
            int i = num_criteria++;
            values_1[i] = a.opengl_accum_buffer_bits;
            values_2[i] = b.opengl_accum_buffer_bits;
        }

        // Criterion 11: Highest double buffer performance
        if (prefer_double_buffered) {
            int i = num_criteria++;
            values_1[i] = a.double_buffered_perflevel;
            values_2[i] = b.double_buffered_perflevel;
        }

        // Criterion 12: Prefer not double buffered
        if (!prefer_double_buffered) {
            int i = num_criteria++;
            values_1[i] = -int(a.double_buffered);
            values_2[i] = -int(b.double_buffered);
        }

        // Criterion 13: Prefer not OpenGL double buffered
        if (!require_opengl) {
            int i = num_criteria++;
            values_1[i] = -int(a.opengl_double_buffered);
            values_2[i] = -int(b.opengl_double_buffered);
        }

        // Criterion 14: Lowest depth buffer bit width
        if (!require_opengl_depth_buffer) {
            int i = num_criteria++;
            values_1[i] = -a.opengl_depth_buffer_bits;
            values_2[i] = -b.opengl_depth_buffer_bits;
        }

        // Criterion 15: Lowest stencil buffer bit width
        if (!require_opengl_stencil_buffer) {
            int i = num_criteria++;
            values_1[i] = -a.opengl_stencil_buffer_bits;
            values_2[i] = -b.opengl_stencil_buffer_bits;
        }

        // Criterion 16: Lowest accumulation buffer bit width
        if (!require_opengl_accum_buffer) {
            int i = num_criteria++;
            values_1[i] = -a.opengl_accum_buffer_bits;
            values_2[i] = -b.opengl_accum_buffer_bits;
        }

        // Criterion 17: Lowest number of OpenGL auxiliary buffers
        {
            int i = num_criteria++;
            values_1[i] = -a.opengl_num_aux_buffers;
            values_2[i] = -b.opengl_num_aux_buffers;
        }

        // Criterion 18: Prefer no OpenGL support
        if (!require_opengl) {
            int i = num_criteria++;
            values_1[i] = (a.opengl_supported ? 0 : 1);
            values_2[i] = (b.opengl_supported ? 0 : 1);
        }

        ARCHON_ASSERT(num_criteria <= max_criteria);
        return std::lexicographical_compare(values_1, values_1 + num_criteria, values_2, values_2 + num_criteria);
    };
    const VisualSpec* best = nullptr;
    for (const VisualSpec& spec : screen_slot.visual_specs) {
        bool have_new_best = (filter(spec) && (!best || less(*best, spec)));
        if (ARCHON_LIKELY(!have_new_best))
            continue;
        best = &spec;
    }
    if (best)
        return *best;
    throw std::runtime_error("No suitable X11 visual found");
}


auto ConnectionImpl::ensure_colormap(ScreenSlot& screen_slot, VisualID visual) const -> Colormap
{
    if (ARCHON_LIKELY(visual == screen_slot.default_visual))
        return DefaultColormap(dpy, screen_slot.screen);
    auto i = screen_slot.colormaps.find(visual);
    if (ARCHON_LIKELY(i != screen_slot.colormaps.end()))
        return i->second;

    throw std::runtime_error("Need non-default colormap");                                         
    
    // If can find standard colormap for visual:
    //   Use standard colormap
    // Create and initialize new colormap

/*    
        Colormap colormap = DefaultColormap(dpy, screen);
        bool colormap_owned = false;
        ARCHON_SCOPE_EXIT {
            if (colormap_owned)
                XFreeColormap(dpy, colormap);
        };
        if (ARCHON_UNLIKELY(visual_info.visualid != default_visualid)) {
            // By creating a new colormap, rather than reusing the one used by the root
            // window, it becomes possible to use a visual for the new window that differs
            // from the one used by the root window. The colormap and the new window must
            // agree on visual.
            colormap = XCreateColormap(dpy, root, visual_info.visual, AllocNone);
            colormap_owned = true;
        }
*/
}


bool ConnectionImpl::do_process_events(const time_point_type* deadline,
                                       display::ConnectionEventHandler* connection_event_handler)
{
    // This function takes care to meet the following requirements:
    //
    // - XFlush() must be called before waiting (poll()) whenever there is a chance that
    //   there are unflushed commands.
    //
    // - XEventsQueued() must be called immediately before waiting (poll()) to ensure that
    //   there are no events that are already queued (must be called after XFlush()).
    //
    // - There must be no way for the execution of WindowEventHandler::on_expose() and
    //   ConnectionEventHandler::before_sleep() to be starved indefinitely by event
    //   saturation. This is ensured by fully exhausting one batch of events at a time
    //   (m_remaining_events_in_batch).
    //
    // - There must be no way for the return from do_process_events() due to expiration of
    //   the deadline to be starved indefinitely by event saturation. This is ensured by
    //   fully exhausting one batch of events at a time (m_remaining_events_in_batch).
    //

    display::ConnectionEventHandler& connection_event_handler_2 =
        (connection_event_handler ? *connection_event_handler : *this);

    XEvent ev = {};
    WindowImpl* window = {};
    timestamp_unwrapper_type::Session unwrap_session(m_timestamp_unwrapper);
    bool expect_keymap_notify;

  process_1:
    if (ARCHON_LIKELY(m_num_events > 0))
        goto process_2;
    goto post;

  process_2:
    ARCHON_ASSERT(m_num_events > 0);
    XNextEvent(dpy, &ev);
    --m_num_events;
    expect_keymap_notify = m_expect_keymap_notify;
    m_expect_keymap_notify = false;
    ARCHON_ASSERT(!expect_keymap_notify || ev.type == KeymapNotify);
    switch (ev.type) {
        case MotionNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xmotion.window, window))) {
                display::MouseEvent event;
                event.cookie = window->cookie;
                event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xmotion.time); // Throws
                event.pos = { ev.xmotion.x, ev.xmotion.y };
                bool proceed = window->event_handler.on_mousemove(event); // Throws
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case ConfigureNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xconfigure.window, window))) {
                // When there is a window manager, the window manager will generally
                // re-parent the client's window. This generally means that the client's
                // window will remain at a fixed position relative to it's parent, so there
                // will be no configure notifications when the window is moved through user
                // interaction. Also, if the user's window is moved relative to its parent,
                // the reported position will be unreliable, as it will be relative to its
                // parent, which is not the root window of the screen. Fortunately, in all
                // those cases, the window manager is obligated to generate synthetic
                // configure notifications in which the positions are absolute (relative to
                // the root window of the screen).
                bool proceed;
                if (ev.xconfigure.send_event) {
                    display::WindowPosEvent event;
                    event.cookie = window->cookie;
                    event.pos = { ev.xconfigure.x, ev.xconfigure.y };
                    proceed = window->event_handler.on_reposition(event); // Throws
                }
                else {
                    display::WindowSizeEvent event;
                    event.cookie = window->cookie;
                    event.size = { ev.xconfigure.width, ev.xconfigure.height };
                    proceed = window->event_handler.on_resize(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case Expose:
            if (ARCHON_LIKELY(!lookup_window(ev.xexpose.window, window) || window->has_pending_expose_event))
                break;
            m_exposed_windows.push_back(ev.xexpose.window); // Throws
            window->has_pending_expose_event = true;
            break;

        case ButtonPress:
        case ButtonRelease:
            if (ARCHON_LIKELY(lookup_window(ev.xbutton.window, window))) {
                track_pointer_grabs(ev.xbutton.window, ev.xbutton.button, ev.type == ButtonPress); // Throws
                bool is_scroll = {};
                display::MouseButton button = {};
                math::Vector2F amount;
                if (ARCHON_LIKELY(try_map_mouse_button(ev.xbutton.button, is_scroll, button, amount))) {
                    if (ARCHON_LIKELY(is_scroll)) {
                        display::ScrollEvent event;
                        event.cookie = window->cookie;
                        event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xbutton.time); // Throws
                        event.amount = amount;
                        bool proceed = window->event_handler.on_scroll(event); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    else {
                        display::MouseButtonEvent event;
                        event.cookie = window->cookie;
                        event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xbutton.time); // Throws
                        event.pos = { ev.xbutton.x, ev.xbutton.y };
                        event.button = button;
                        bool proceed;
                        if (ev.type == ButtonPress) {
                            proceed = window->event_handler.on_mousedown(event); // Throws
                        }
                        else {
                            proceed = window->event_handler.on_mouseup(event); // Throws
                        }
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                }
            }
            break;

        case KeyPress:
        case KeyRelease:
            if (ARCHON_LIKELY(lookup_window(ev.xkey.window, window))) {
                using timestamp_type = display::TimedWindowEvent::Timestamp;
                timestamp_type timestamp = unwrap_session.unwrap_next_timestamp(ev.xkey.time); // Throws
                bool is_repetition = false;
                if (ARCHON_LIKELY(m_detectable_autorepeat_enabled)) {
                    if (ev.type == KeyPress) {
                        if (!m_pressed_keys.contains(ev.xkey.keycode)) {
                            m_pressed_keys.add(ev.xkey.keycode);
                        }
                        else {
                            is_repetition = true;
                        }
                    }
                    else {
                        ARCHON_ASSERT(m_pressed_keys.contains(ev.xkey.keycode));
                        m_pressed_keys.remove(ev.xkey.keycode);
                    }
                }
                else {
                    // When "detectable auto-repeat" mode was not enabled, we need to use a
                    // fall-back detection mechanism, which works as follows: On "key up",
                    // if the next event is "key down" for the same key and at almost the
                    // same time, consider the pair to be caused by key repetition. This
                    // scheme assumes that the second "key down" event is immediately
                    // available, i.e., without having to block. This assumption appears to
                    // hold in practice, but it could conceivably fail, in which case the
                    // pair will be treated as genuine "key up" and "key down" events.
                    if (ev.type == KeyPress) {
                        ARCHON_ASSERT(!m_pressed_keys.contains(ev.xkey.keycode));
                        m_pressed_keys.add(ev.xkey.keycode);
                    }
                    else {
                        ARCHON_ASSERT(m_pressed_keys.contains(ev.xkey.keycode));
                        if (m_num_events == 0) {
                            int n = XEventsQueued(dpy, QueuedAfterReading);  // Non-blocking
                            if (n > 0)
                                m_num_events = 1;
                        }
                        if (m_num_events > 0) {
                            XEvent ev_2 = {};
                            XPeekEvent(dpy, &ev_2);
                            if (ev_2.type == KeyPress && ev_2.xkey.keycode == ev.xkey.keycode) {
                                ARCHON_ASSERT(ev_2.xkey.window == ev.xkey.window);
                                timestamp_type timestamp_2 =
                                    unwrap_session.unwrap_next_timestamp(ev_2.xkey.time); // Throws
                                ARCHON_ASSERT(timestamp_2 >= timestamp);
                                if ((timestamp_2 - timestamp).count() <= 1) {
                                    XNextEvent(dpy, &ev);
                                    timestamp = timestamp_2;
                                    --m_num_events;
                                    is_repetition = true;
                                }
                            }
                        }
                        if (!is_repetition)
                            m_pressed_keys.remove(ev.xkey.keycode);
                    }
                }
                // Map key code to a keyboard independent symbol identifier (in general the
                // symbol in the upper left corner on the corresponding key). See also
                // https://tronche.com/gui/x/xlib/input/keyboard-encoding.html.
                unsigned group = XkbGroup1Index;
                unsigned level = 0;
                KeySym keysym = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, group, level);
                ARCHON_ASSERT(keysym != NoSymbol);
                display::KeyEvent event;
                event.cookie = window->cookie;
                event.timestamp = timestamp;
                event.key_code = { display::KeyCode::code_type(keysym) };
                bool proceed;
                if (ev.type == KeyPress) {
                    if (ARCHON_LIKELY(!is_repetition)) {
                        proceed = window->event_handler.on_keydown(event); // Throws
                    }
                    else {
                        proceed = window->event_handler.on_keyrepeat(event); // Throws
                    }
                }
                else {
                    proceed = window->event_handler.on_keyup(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case KeymapNotify:
            // Note: For some unclear reason, `ev.xkeymap.window` does not specify the
            // target window like it does for other types of events. Instead, one can rely
            // on `KeymapNotify` to be generated immediately after every `FocusIn` event, so
            // this provides an implicit target window.
            if (expect_keymap_notify)
                m_pressed_keys.assign(ev.xkeymap.key_vector);
            break;

        case EnterNotify:
        case LeaveNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xcrossing.window, window) && !is_pointer_grabbed())) {
                display::TimedWindowEvent event;
                event.cookie = window->cookie;
                event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xcrossing.time); // Throws
                bool proceed;
                if (ev.type == EnterNotify) {
                    proceed = window->event_handler.on_mouseover(event); // Throws
                }
                else {
                    proceed = window->event_handler.on_mouseout(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case FocusIn:
        case FocusOut:
            if (ev.type == FocusIn)
                m_expect_keymap_notify = true;
            if (ARCHON_LIKELY(lookup_window(ev.xfocus.window, window))) {
                display::WindowEvent event;
                event.cookie = window->cookie;
                bool proceed;
                if (ev.type == FocusIn) {
                    proceed = window->event_handler.on_focus(event); // Throws
                }
                else {
                    proceed = window->event_handler.on_blur(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case ClientMessage:
            bool is_close = (ev.xclient.format == 32 && Atom(ev.xclient.data.l[0]) == atom_wm_delete_window);
            if (ARCHON_LIKELY(is_close && lookup_window(ev.xclient.window, window))) {
                display::WindowEvent event;
                event.cookie = window->cookie;
                bool proceed = window->event_handler.on_close(event); // Throws
                if (ARCHON_LIKELY(!proceed))
                    return false; // Interrupt
            }
            break;
    }

#if HAVE_XRANDR
    if (m_have_xrandr && ev.type == m_xrandr_event_base + RRNotify) {
        const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
        switch (ev_2.subtype) {
            case RRNotify_CrtcChange:
            case RRNotify_OutputChange:
                ::Window root = ev_2.window;
                auto i = m_x11_screens_by_root.find(root);
                ARCHON_ASSERT(i != m_x11_screens_by_root.end());
                int screen = i->second;
                ARCHON_ASSERT(screen >= 0 && screen < ScreenCount(dpy));
                ScreenSlot& slot = m_screen_slots[screen];
                if (update_display_info(slot)) // Throws
                    connection_event_handler_2.on_display_change(screen); // Throws
        }
    }
#endif // HAVE_XRANDR
    goto process_1;

  post:
    for (;;) {
        if (ARCHON_LIKELY(m_exposed_windows.empty()))
            break;
        ::Window window_id = m_exposed_windows.front();
        m_exposed_windows.pop_front();
        WindowImpl* window = {};
        if (ARCHON_LIKELY(lookup_window(window_id, window))) {
            window->has_pending_expose_event = false;
            display::WindowEvent event;
            event.cookie = window->cookie;
            bool proceed = window->event_handler.on_expose(event); // Throws
            if (ARCHON_LIKELY(!proceed))
                return false; // Interrupt
        }
    }
    {
        bool proceed = connection_event_handler_2.before_sleep(); // Throws
        if (ARCHON_UNLIKELY(!proceed))
            return false; // Interrupt
    }
    XFlush(dpy);

  read:
    m_num_events = XEventsQueued(dpy, QueuedAfterReading); // Non-blocking

  wait:
    {
        int timeout = -1;
        bool complete = false;
        if (ARCHON_LIKELY(deadline)) {
            time_point_type now = clock_type::now();
            if (ARCHON_LIKELY(*deadline > now))
                goto not_expired;
            return true; // Deadline expired
          not_expired:
            auto duration = std::chrono::ceil<std::chrono::milliseconds>(*deadline - now).count();
            timeout = core::int_max<int>();
            if (ARCHON_LIKELY(core::int_less_equal(duration, timeout))) {
                timeout = int(duration);
                complete = true;
            }
        }

        if (ARCHON_LIKELY(m_num_events > 0)) {
            unwrap_session.reset_now();
            goto process_2;
        }

        pollfd fds[1] {};
        int nfds = 1;
        fds[0].fd = ConnectionNumber(dpy);
        fds[0].events = POLLIN;
        int ret, err;
      poll:
        ret = ::poll(fds, nfds, timeout);
        if (ARCHON_LIKELY(ret > 0)) {
            ARCHON_ASSERT(ret == 1);
            goto read;
        }
        if (ARCHON_LIKELY(ret == 0)) {
            ARCHON_ASSERT(timeout >= 0);
            if (ARCHON_LIKELY(complete))
                return true; // Deadline expired
            goto wait;
        }
        err = errno; // Eliminate any risk of clobbering
        if (ARCHON_LIKELY(err == EINTR))
            goto poll;
        core::throw_system_error(err, "Failed to poll file descriptor of X11 connection"); // Throws
    }
}


bool ConnectionImpl::lookup_window(::Window window_id, WindowImpl*& window) noexcept
{
    WindowImpl* window_2 = nullptr;
    if (ARCHON_LIKELY(m_have_curr_window && window_id == m_curr_window_id)) {
        window_2 = m_curr_window;
    }
    else {
        auto i = m_windows.find(window_id);
        if (ARCHON_LIKELY(i != m_windows.end()))
            window_2 = &i->second;
        m_curr_window_id = window_id;
        m_curr_window = window_2;
        m_have_curr_window = true;
    }
    if (ARCHON_LIKELY(window_2)) {
        window = window_2;
        return true;
    }
    return false;
}


void ConnectionImpl::track_pointer_grabs(::Window window_id, unsigned button, bool is_press)
{
    ARCHON_ASSERT(!is_pointer_grabbed() || window_id == m_pointer_grab_window_id);
    if (is_press) {
        bool grab_in_progress = is_pointer_grabbed();
        auto p = m_pointer_grab_buttons.insert(button); // Throws
        bool was_inserted = p.second;
        ARCHON_ASSERT(was_inserted);
        if (ARCHON_LIKELY(!grab_in_progress))
            m_pointer_grab_window_id = window_id;
    }
    else {
        auto n = m_pointer_grab_buttons.erase(button);
        ARCHON_ASSERT(n == 1);
    }
}


inline bool ConnectionImpl::is_pointer_grabbed() const noexcept
{
    return !m_pointer_grab_buttons.empty();
}


#if HAVE_XRANDR


bool ConnectionImpl::update_display_info(ScreenSlot& slot) const
{
    int max_attempts = 16;
    for (int i = 0; i < max_attempts; ++i) {
        bool changed = false;
        if (ARCHON_LIKELY(try_update_display_info(slot, changed)))
            return changed;
    }
    throw std::runtime_error("Failed to fetch screen configuration using XRandR within the allotted number of "
                             "attempts");
}


bool ConnectionImpl::try_update_display_info(ScreenSlot& slot, bool& changed) const
{
    XRRScreenResources* resources = XRRGetScreenResourcesCurrent(dpy, slot.root);
    if (ARCHON_UNLIKELY(!resources))
        throw std::runtime_error("XRRGetScreenResourcesCurrent() failed");
    ARCHON_SCOPE_EXIT {
        XRRFreeScreenResources(resources);
    };
    struct Crtc {
        bool enabled;
        display::Box bounds;
        std::optional<double> refresh_rate;
    };
    core::FlatMap<RRCrtc, Crtc, 16> crtcs;
    crtcs.reserve(std::size_t(resources->ncrtc)); // Throws
    auto ensure_crtc = [&](RRCrtc id) -> const Crtc* {
        auto i = crtcs.find(id);
        if (i != crtcs.end())
            return &i->second;
        XRRCrtcInfo* info = XRRGetCrtcInfo(dpy, resources, id);
        if (ARCHON_UNLIKELY(!info))
            return nullptr;
        ARCHON_SCOPE_EXIT {
            XRRFreeCrtcInfo(info);
        };
        bool enabled = (info->mode != None);
        display::Size size = {};
        core::int_cast(info->width, size.width); // Throws
        core::int_cast(info->height, size.height); // Throws
        display::Box bounds = { display::Pos(info->x, info->y), size };
        std::optional<double> refresh_rate;
        if (enabled) {
            bool found = false;
            for (int j = 0; j < resources->nmode; ++j) {
                const XRRModeInfo& mode = resources->modes[j];
                if (ARCHON_LIKELY(mode.id != info->mode))
                    continue;
                found = true;
                if (mode.dotClock != 0)
                    refresh_rate = mode.dotClock / (mode.hTotal * double(mode.vTotal));
                break;
            }
            ARCHON_ASSERT(found);
        }
        ARCHON_STEADY_ASSERT(crtcs.size() < crtcs.capacity());
        Crtc& crtc =  crtcs[id];
        crtc = { enabled, bounds, refresh_rate };
        return &crtc;
    };
    core::Vector<ProtoScreen, 16> new_screens;
    std::array<char, 16 * 24> strings_seed_memory = {};
    core::Buffer strings_buffer(strings_seed_memory);
    core::StringBufferContents strings(strings_buffer);
    const impl::EdidParser& edid_parser = ensure_edid_parser(); // Throws
    for (int i = 0; i < resources->noutput; ++i) {
        RROutput id = resources->outputs[i];
        XRROutputInfo* info = XRRGetOutputInfo(dpy, resources, id);
        if (ARCHON_UNLIKELY(!info))
            return false;
        ARCHON_SCOPE_EXIT {
            XRRFreeOutputInfo(info);
        };
        // Note: Treating RR_UnknownConnection same as RR_Connected
        bool connected = (info->connection != RR_Disconnected);
        if (!connected || info->crtc == None)
            continue;
        const Crtc* crtc = ensure_crtc(info->crtc); // Throws
        if (ARCHON_UNLIKELY(!crtc))
            return false;
        if (!crtc->enabled)
            continue;
        // FIXME: Consider character encoding in output name                
        std::size_t offset = strings.size();
        std::size_t size = std::size_t(info->nameLen);
        strings.append({ info->name, size }); // Throws
        // The base address is not necessarily correct anymore, but this Will be fixed up later
        core::IndexRange output_name = { offset, size }; // Throws
        std::optional<display::Resolution> resolution;
        if (info->mm_width != 0 && info->mm_height != 0) {
            double horz_ppcm = crtc->bounds.size.width  / double(info->mm_width)  * 10;
            double vert_ppcm = crtc->bounds.size.height / double(info->mm_height) * 10;
            resolution = display::Resolution { horz_ppcm, vert_ppcm };
        }
        // Extract monitor name from EDID data when available
        std::optional<core::IndexRange> monitor_name;
        int nprop = {};
        Atom* props = XRRListOutputProperties(dpy, id, &nprop);
        if (ARCHON_LIKELY(props))  {
            ARCHON_SCOPE_EXIT {
                XFree(props);
            };
            for (int j = 0; j < nprop; ++j) {
                if (props[j] == atom_edid) {
                    long offset = 0;
                    long length = 128 / 4; // 128 bytes (32 longs) in basic EDID block
                    Bool _delete = False;
                    Bool pending = False;
                    Atom req_type = AnyPropertyType;
                    Atom actual_type = {};
                    int actual_format = {};
                    unsigned long nitems = {};
                    unsigned long bytes_after = {};
                    unsigned char* prop = {};
                    int ret = XRRGetOutputProperty(dpy, id, props[j], offset, length, _delete, pending, req_type,
                                                   &actual_type, &actual_format, &nitems, &bytes_after, &prop);
                    if (ARCHON_LIKELY(ret == Success)) {
                        ARCHON_SCOPE_EXIT {
                            XFree(prop);
                        };
                        if (ARCHON_LIKELY(actual_type == XA_INTEGER && actual_format == 8)) {
                            std::size_t size = {};
                            if (ARCHON_LIKELY(core::try_int_cast(nitems, size))) {
                                std::string_view str = { reinterpret_cast<char*>(prop), size };
                                impl::EdidInfo info = {};
                                if (ARCHON_LIKELY(edid_parser.parse(str, info, strings))) // Throws
                                    monitor_name = info.monitor_name;
                            }
                        }
                    }
                }
            }
        }
        ProtoScreen screen = { output_name, crtc->bounds, monitor_name, resolution, crtc->refresh_rate };
        new_screens.push_back(screen); // Throws
    }
    {
        const char* base_1 = strings.data();
        const char* base_2 = slot.screens_string_buffer.data();
        auto cmp_opt_str = [&](const std::optional<core::IndexRange>& a, const std::optional<core::IndexRange>& b) {
            return (a.has_value() ?
                    b.has_value() && a.value().resolve_string(base_1) == b.value().resolve_string(base_2) :
                    !b.has_value());
        };
        auto cmp = [&](const ProtoScreen& a, const ProtoScreen& b) {
            return (a.bounds == b.bounds &&
                    a.resolution == b.resolution &&
                    a.refresh_rate == b.refresh_rate &&
                    a.output_name.resolve_string(base_1) == b.output_name.resolve_string(base_2) &&
                    cmp_opt_str(a.monitor_name, b.monitor_name));
        };
        bool equal = std::equal(new_screens.begin(), new_screens.end(), slot.screens.begin(), slot.screens.end(),
                                std::move(cmp));
        if (equal) {
            changed = false;
            return true;
        }
    }
    slot.screens.reserve(new_screens.size()); // Throws
    slot.screens_string_buffer.reserve(strings.size(), slot.screens_string_buffer_used_size); // Throws
    // Non-throwing from here
    slot.screens.clear();
    slot.screens.insert(slot.screens.begin(), new_screens.begin(), new_screens.end());
    slot.screens_string_buffer.assign(strings);
    slot.screens_string_buffer_used_size = strings.size();
    changed = true;
    return true;
}


auto ConnectionImpl::ensure_edid_parser() const -> const impl::EdidParser&
{
    if (ARCHON_LIKELY(m_edid_parser.has_value()))
        return m_edid_parser.value();
    m_edid_parser.emplace(locale); // Throws
    return m_edid_parser.value();
}


#endif // HAVE_XRANDR



inline WindowImpl::WindowImpl(ConnectionImpl& conn_2, const ScreenSlot& screen_slot_2, const VisualSpec& visual_spec_2,
                              const XPixmapFormatValues& pixmap_format_2, display::WindowEventHandler& event_handler_2,
                              int cookie_2, std::unique_ptr<const PixelCodec> pixel_codec) noexcept
    : conn(conn_2)
    , screen_slot(screen_slot_2)
    , visual_spec(visual_spec_2)
    , pixmap_format(pixmap_format_2)
    , event_handler(event_handler_2)
    , cookie(cookie_2)
    , m_pixel_codec(std::move(pixel_codec))
{
}


WindowImpl::~WindowImpl() noexcept
{
#if HAVE_GLX
    if (m_ctx)
        glXDestroyContext(conn.dpy, m_ctx);
#endif // HAVE_GLX

    if (ARCHON_LIKELY(win != None)) {
        if (ARCHON_LIKELY(m_is_registered)) {
            if (m_gc != None)
                XFreeGC(conn.dpy, m_gc);
            conn.unregister_window(win);
        }
        XDestroyWindow(conn.dpy, win);
    }
}


void WindowImpl::create(display::Size size, Colormap colormap, const Config& config, bool enable_double_buffering,
                        bool enable_opengl, bool enable_glx_direct_rendering)
{
    display::Size adjusted_size = size;
    bool has_minimum_size = (config.resizable && config.minimum_size.has_value());
    if (has_minimum_size)
        adjusted_size = max(adjusted_size, config.minimum_size.value());

    ::Window parent = screen_slot.root;
    int x = 0, y = 0;
    unsigned width  = unsigned(adjusted_size.width);
    unsigned height = unsigned(adjusted_size.height);
    unsigned border_width = 0;
    int depth = visual_spec.info.depth;
    unsigned class_ = InputOutput;
    Visual* visual = visual_spec.info.visual;
    unsigned long valuemask = CWEventMask | CWColormap;
    XSetWindowAttributes attributes = {};
    attributes.event_mask = (KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | ButtonReleaseMask |
                             ButtonMotionMask |
                             EnterWindowMask | LeaveWindowMask |
                             FocusChangeMask |
                             ExposureMask |
                             StructureNotifyMask |
                             KeymapStateMask);
    attributes.colormap = colormap;
    win = XCreateWindow(conn.dpy, parent, x, y, width, height, border_width, depth, class_, visual,
                        valuemask, &attributes);

    conn.register_window(win, *this); // Throws
    m_is_registered = true;

    // Ask X server to notify rather than close connection when window is closed
    set_property(conn.atom_wm_protocols, conn.atom_wm_delete_window);

    // Disable resizability if requested
    if (!config.resizable) {
        XSizeHints size_hints;
        size_hints.flags = PMinSize | PMaxSize;
        size_hints.min_width  = adjusted_size.width;
        size_hints.min_height = adjusted_size.height;
        size_hints.max_width  = adjusted_size.width;
        size_hints.max_height = adjusted_size.height;
        XSetWMSizeHints(conn.dpy, win, &size_hints, XA_WM_NORMAL_HINTS);
    }

    // Set minimum window size if requested
    if (has_minimum_size) {
        display::Size min_size = config.minimum_size.value();
        XSizeHints size_hints;
        size_hints.flags = PMinSize;
        size_hints.min_width  = min_size.width;
        size_hints.min_height = min_size.height;
        XSetWMSizeHints(conn.dpy, win, &size_hints, XA_WM_NORMAL_HINTS);
    }

    // Enable double buffering
    m_drawable = win;
#if HAVE_XDBE
    if (ARCHON_LIKELY(enable_double_buffering)) {
        m_swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
        XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(conn.dpy, win, m_swap_action);
        m_drawable = back_buffer;
        m_is_double_buffered = true;
    }
#endif // HAVE_XDBE

    // Create OpenGL rendering context
#if HAVE_GLX
    if (enable_opengl) {
        XVisualInfo vis = visual_spec.info;
        GLXContext share_list = nullptr; // No sharing
        Bool direct = Bool(enable_glx_direct_rendering);
        GLXContext ctx = glXCreateContext(conn.dpy, &vis, share_list, direct);
        if (ARCHON_UNLIKELY(!ctx))
            throw std::runtime_error("glXCreateContext() failed");
        m_ctx = ctx;
    }
#else // !HAVE_GLX
    static_cast<void>(enable_opengl);
#endif // !HAVE_GLX
}


inline auto WindowImpl::ensure_graphics_context() noexcept -> GC
{
    if (ARCHON_LIKELY(m_gc != None))
        return m_gc;
    return create_graphics_context();
}


void WindowImpl::show()
{
    XMapWindow(conn.dpy, win);
}


void WindowImpl::hide()
{
    XUnmapWindow(conn.dpy, win);
}


void WindowImpl::set_title(std::string_view title)
{
    // FIXME: Tend to character encoding. How does SDL do it? Doe it support UTF-8? See also https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XmbTextListToTextProperty.html              
    // FIXME: Consider placing a reusable string buffer in ConnectionImpl for use cases like the one below      
    std::string name_1 = std::string(title); // Throws
    char* name_2 = name_1.data();
    XTextProperty name_3;
    Status status = XStringListToTextProperty(&name_2, 1, &name_3);
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XStringListToTextProperty() failed");
    XSetWMName(conn.dpy, win, &name_3);                             
    XFree(name_3.value);
}


void WindowImpl::set_size(display::Size size)
{
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    unsigned w = unsigned(size.width);
    unsigned h = unsigned(size.height);
    XResizeWindow(conn.dpy, win, w, h);
}


void WindowImpl::set_fullscreen_mode(bool on)
{
    XClientMessageEvent event = {};
    event.type = ClientMessage;
    event.window = win;
    event.message_type = conn.atom_net_wm_state;
    event.format = 32;
    event.data.l[0] = (on ? 1 : 0); // Add / remove property
    event.data.l[1] = conn.atom_net_wm_state_fullscreen;
    event.data.l[2] = 0; // No second property to alter
    event.data.l[3] = 1; // Request is from normal application
    Bool propagate = False;
    long event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    Status status = XSendEvent(conn.dpy, screen_slot.root, propagate, event_mask, reinterpret_cast<XEvent*>(&event));
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XSendEvent() failed");
}


void WindowImpl::fill(util::Color color)
{
    int x = 0;
    int y = 0;
    unsigned w = core::int_max<unsigned>();
    unsigned h = core::int_max<unsigned>();
    do_fill(color, x, y, w, h); // Throws
}


void WindowImpl::fill(util::Color color, const display::Box& area)
{
    if (ARCHON_LIKELY(area.is_valid())) {
        int x = area.pos.x;
        int y = area.pos.y;
        unsigned w = unsigned(area.size.width);
        unsigned h = unsigned(area.size.height);
        do_fill(color, x, y, w, h); // Throws
        return;
    }
    throw std::invalid_argument("Fill area");
}


auto WindowImpl::new_texture(display::Size size) -> std::unique_ptr<display::Texture>
{
    auto tex = std::make_unique<TextureImpl>(*this, size); // Throws
    tex->create(*m_pixel_codec); // Throws
    return tex;
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Pos& pos)
{
    const TextureImpl& tex_2 = dynamic_cast<const TextureImpl&>(tex); // Throws
    do_put_texture(tex_2, tex_2.size, pos);
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Box& source_area, const display::Pos& pos)
{
    const TextureImpl& tex_2 = dynamic_cast<const TextureImpl&>(tex); // Throws
    if (ARCHON_UNLIKELY(!source_area.contained_in(tex_2.size)))
        throw std::invalid_argument("Source area escapes texture boundary");
    do_put_texture(tex_2, source_area, pos);
}


void WindowImpl::present()
{
#if HAVE_XDBE
    if (m_is_double_buffered) {
        XdbeSwapInfo info;
        info.swap_window = win;
        info.swap_action = m_swap_action;
        Status status = XdbeSwapBuffers(conn.dpy, &info, 1);
        if (ARCHON_UNLIKELY(status == 0))
            throw std::runtime_error("XdbeSwapBuffers() failed");
    }
#endif // HAVE_XDBE
}


void WindowImpl::opengl_make_current()
{
#if HAVE_GLX
    glXMakeCurrent(conn.dpy, win, m_ctx);
#endif
}


void WindowImpl::opengl_swap_buffers()
{
#if HAVE_GLX
    glXSwapBuffers(conn.dpy, win);
#endif
}


void WindowImpl::set_property(Atom name, Atom value) noexcept
{
    XChangeProperty(conn.dpy, win, name, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&value), 1);
}


void WindowImpl::do_fill(util::Color color, int x, int y, unsigned w, unsigned h)
{
    GC gc = ensure_graphics_context();
    unsigned long color_2 = intern_color(color); // Throws
    XSetForeground(conn.dpy, gc, color_2);
    XFillRectangle(conn.dpy, m_drawable, gc, x, y, w, h);
}


void WindowImpl::do_put_texture(const TextureImpl& tex, const display::Box& source_area, const display::Pos& pos)
{
    GC gc = ensure_graphics_context();
    int src_x = source_area.pos.x, src_y = source_area.pos.y;
    unsigned width = unsigned(source_area.size.width), height = unsigned(source_area.size.height);
    int dest_x = pos.x, dest_y = pos.y;
    XCopyArea(conn.dpy, tex.pixmap, m_drawable, gc, src_x, src_y, width, height, dest_x, dest_y);
}


auto WindowImpl::create_graphics_context() noexcept -> GC
{
    ARCHON_ASSERT(m_gc == None);
    unsigned long valuemask = GCGraphicsExposures;
    XGCValues values = {};
    values.graphics_exposures = False;
    m_gc = XCreateGC(conn.dpy, m_drawable, valuemask, &values);
    return m_gc;
}


auto WindowImpl::intern_color(util::Color color) -> unsigned long
{
    const PixelCodec& pixel_codec = get_pixel_codec(); // Throws
    return pixel_codec.intern_color(color);
}


auto WindowImpl::get_pixel_codec() -> const PixelCodec&
{
    if (ARCHON_LIKELY(m_pixel_codec))
        return *m_pixel_codec;
    throw std::runtime_error("Basic rendering was not enabled");
}


inline TextureImpl::TextureImpl(WindowImpl& win_2, const display::Size& size_2)
    : win(win_2)
    , size(size_2)
{
    if (ARCHON_LIKELY(size.width >= 0 && size.height >= 0))
        return;
    throw std::invalid_argument("Invalid texture size");
}


TextureImpl::~TextureImpl() noexcept
{
    if (ARCHON_LIKELY(pixmap != None))
        XFreePixmap(win.conn.dpy, pixmap);
}


void TextureImpl::create(const PixelCodec& pixel_codec)
{
    if (ARCHON_LIKELY(!size.is_empty())) {
        pixmap = XCreatePixmap(win.conn.dpy, win.screen_slot.root, unsigned(size.width), unsigned(size.height),
                               win.visual_spec.info.depth);
        pixel_codec.create_image(size, m_img, m_img_data); // Throws
    }
}


void TextureImpl::put_image(const image::Image& img)
{
    if (ARCHON_LIKELY(!size.is_empty())) {
        GC gc = win.ensure_graphics_context();

        m_img->put_image({ 0, 0 }, img); // Throws

        // Xlib requires that the depth of the image matches the depth of the pixmap. X11
        // requires that the depth of pixmap matches the depth of the target window (during
        // XCopyArea()). Only ZPixmap format is relevant. With ZPixmap, image data is
        // ordered by pixel rather than by bit-plane, and each scanline unit (word) holds
        // one or more pixels. The ZPixmap format supports the depths of any offered
        // visual. XPutImage() can handle byte swapping and changes in row alignment
        // (`scanline_pad` / `bitmap_pad`).
        int scanline_pad = win.pixmap_format.bits_per_pixel;
        XImage img_2;
        img_2.width            = size.width;
        img_2.height           = size.height;
        img_2.xoffset          = 0;
        img_2.format           = ZPixmap;
        img_2.data             = m_img_data;
        img_2.byte_order       = LSBFirst;
        img_2.bitmap_unit      = BitmapUnit(win.conn.dpy); // Immaterial
        img_2.bitmap_bit_order = BitmapBitOrder(win.conn.dpy); // Immaterial
        img_2.bitmap_pad       = scanline_pad;
        img_2.depth            = win.visual_spec.info.depth;
        img_2.bytes_per_line   = 0;
        img_2.bits_per_pixel   = win.pixmap_format.bits_per_pixel;
        img_2.red_mask         = win.visual_spec.info.red_mask;
        img_2.green_mask       = win.visual_spec.info.green_mask;
        img_2.blue_mask        = win.visual_spec.info.blue_mask;
        Status status = XInitImage(&img_2);
        if (ARCHON_UNLIKELY(status == 0))
            throw std::runtime_error("XInitImage() failed");
        int src_x = 0, src_y = 0;
        int dest_x = 0, dest_y = 0;
        XPutImage(win.conn.dpy, pixmap, gc, &img_2, src_x, src_y, dest_x, dest_y,
                  unsigned(size.width), unsigned(size.height));
    }
}



constexpr std::pair<KeySym, display::Key> key_assocs[] {
    // TTY functions
    { XK_BackSpace,             display::Key::backspace                   },
    { XK_Tab,                   display::Key::tab                         },
    { XK_Linefeed,              display::Key::line_feed                   },
    { XK_Clear,                 display::Key::clear                       },
    { XK_Return,                display::Key::return_                     },
    { XK_Pause,                 display::Key::pause                       },
    { XK_Scroll_Lock,           display::Key::scroll_lock                 },
    { XK_Sys_Req,               display::Key::sys_req                     },
    { XK_Escape,                display::Key::escape                      },
    { XK_Delete,                display::Key::delete_                     },

    // Cursor control
    { XK_Left,                  display::Key::left                        },
    { XK_Right,                 display::Key::right                       },
    { XK_Up,                    display::Key::up                          },
    { XK_Down,                  display::Key::down                        },
    { XK_Prior,                 display::Key::prior                       },
    { XK_Next,                  display::Key::next                        },
    { XK_Home,                  display::Key::home                        },
    { XK_Begin,                 display::Key::begin                       },
    { XK_End,                   display::Key::end                         },

    // Misc functions
    { XK_Select,                display::Key::select                      },
    { XK_Print,                 display::Key::print_screen                },
    { XK_Execute,               display::Key::execute                     },
    { XK_Insert,                display::Key::insert                      },
    { XK_Undo,                  display::Key::undo                        },
    { XK_Redo,                  display::Key::redo                        },
    { XK_Menu,                  display::Key::menu                        },
    { XK_Find,                  display::Key::find                        },
    { XK_Cancel,                display::Key::cancel                      },
    { XK_Help,                  display::Key::help                        },
    { XK_Break,                 display::Key::break_                      },
    { XK_Mode_switch,           display::Key::mode_switch                 },
    { XK_Num_Lock,              display::Key::num_lock                    },

    // Keypad
    { XK_KP_Add,                display::Key::keypad_add                  },
    { XK_KP_Subtract,           display::Key::keypad_subtract             },
    { XK_KP_Multiply,           display::Key::keypad_multiply             },
    { XK_KP_Divide,             display::Key::keypad_divide               },
    { XK_KP_Left,               display::Key::keypad_left                 },
    { XK_KP_Right,              display::Key::keypad_right                },
    { XK_KP_Up,                 display::Key::keypad_up                   },
    { XK_KP_Down,               display::Key::keypad_down                 },
    { XK_KP_Prior,              display::Key::keypad_prior                },
    { XK_KP_Next,               display::Key::keypad_next                 },
    { XK_KP_Home,               display::Key::keypad_home                 },
    { XK_KP_Begin,              display::Key::keypad_begin                },
    { XK_KP_End,                display::Key::keypad_end                  },
    { XK_KP_Insert,             display::Key::keypad_insert               },
    { XK_KP_Delete,             display::Key::keypad_delete               },
    { XK_KP_Enter,              display::Key::keypad_enter                },
    { XK_KP_0,                  display::Key::keypad_digit_0              },
    { XK_KP_1,                  display::Key::keypad_digit_1              },
    { XK_KP_2,                  display::Key::keypad_digit_2              },
    { XK_KP_3,                  display::Key::keypad_digit_3              },
    { XK_KP_4,                  display::Key::keypad_digit_4              },
    { XK_KP_5,                  display::Key::keypad_digit_5              },
    { XK_KP_6,                  display::Key::keypad_digit_6              },
    { XK_KP_7,                  display::Key::keypad_digit_7              },
    { XK_KP_8,                  display::Key::keypad_digit_8              },
    { XK_KP_9,                  display::Key::keypad_digit_9              },
    { XK_KP_Decimal,            display::Key::keypad_decimal_separator    },
    { XK_KP_Separator,          display::Key::keypad_thousands_separator  },
    { XK_KP_Equal,              display::Key::keypad_equal_sign           },
    { XK_KP_Space,              display::Key::keypad_space                },
    { XK_KP_Tab,                display::Key::keypad_tab                  },
    { XK_KP_F1,                 display::Key::keypad_f1                   },
    { XK_KP_F2,                 display::Key::keypad_f2                   },
    { XK_KP_F3,                 display::Key::keypad_f3                   },
    { XK_KP_F4,                 display::Key::keypad_f4                   },

    // Function keys
    { XK_F1,                    display::Key::f1                          },
    { XK_F2,                    display::Key::f2                          },
    { XK_F3,                    display::Key::f3                          },
    { XK_F4,                    display::Key::f4                          },
    { XK_F5,                    display::Key::f5                          },
    { XK_F6,                    display::Key::f6                          },
    { XK_F7,                    display::Key::f7                          },
    { XK_F8,                    display::Key::f8                          },
    { XK_F9,                    display::Key::f9                          },
    { XK_F10,                   display::Key::f10                         },
    { XK_F11,                   display::Key::f11                         },
    { XK_F12,                   display::Key::f12                         },
    { XK_F13,                   display::Key::f13                         },
    { XK_F14,                   display::Key::f14                         },
    { XK_F15,                   display::Key::f15                         },
    { XK_F16,                   display::Key::f16                         },
    { XK_F17,                   display::Key::f17                         },
    { XK_F18,                   display::Key::f18                         },
    { XK_F19,                   display::Key::f19                         },
    { XK_F20,                   display::Key::f20                         },
    { XK_F21,                   display::Key::f21                         },
    { XK_F22,                   display::Key::f22                         },
    { XK_F23,                   display::Key::f23                         },
    { XK_F24,                   display::Key::f24                         },
    { XK_F25,                   display::Key::f25                         },
    { XK_F26,                   display::Key::f26                         },
    { XK_F27,                   display::Key::f27                         },
    { XK_F28,                   display::Key::f28                         },
    { XK_F29,                   display::Key::f29                         },
    { XK_F30,                   display::Key::f30                         },
    { XK_F31,                   display::Key::f31                         },
    { XK_F32,                   display::Key::f32                         },
    { XK_F33,                   display::Key::f33                         },
    { XK_F34,                   display::Key::f34                         },
    { XK_F35,                   display::Key::f35                         },

    // Modifier keys
    { XK_Shift_L,               display::Key::shift_left                  },
    { XK_Shift_R,               display::Key::shift_right                 },
    { XK_Control_L,             display::Key::ctrl_left                   },
    { XK_Control_R,             display::Key::ctrl_right                  },
    { XK_Alt_L,                 display::Key::alt_left                    },
    { XK_Alt_R,                 display::Key::alt_right                   },
    { XK_Meta_L,                display::Key::meta_left                   },
    { XK_Meta_R,                display::Key::meta_right                  },
    { XK_Caps_Lock,             display::Key::caps_lock                   },
    { XK_Shift_Lock,            display::Key::shift_lock                  },
    { XK_dead_grave,            display::Key::dead_grave                  },
    { XK_dead_acute,            display::Key::dead_acute                  },
    { XK_dead_circumflex,       display::Key::dead_circumflex             },
    { XK_dead_tilde,            display::Key::dead_tilde                  },
    { XK_dead_macron,           display::Key::dead_macron                 },
    { XK_dead_breve,            display::Key::dead_breve                  },
    { XK_dead_abovedot,         display::Key::dead_abovedot               },
    { XK_dead_diaeresis,        display::Key::dead_diaeresis              },
    { XK_dead_abovering,        display::Key::dead_abovering              },
    { XK_dead_doubleacute,      display::Key::dead_doubleacute            },
    { XK_dead_caron,            display::Key::dead_caron                  },
    { XK_dead_cedilla,          display::Key::dead_cedilla                },
    { XK_dead_ogonek,           display::Key::dead_ogonek                 },
    { XK_dead_iota,             display::Key::dead_iota                   },
    { XK_dead_voiced_sound,     display::Key::dead_voiced_sound           },
    { XK_dead_semivoiced_sound, display::Key::dead_semivoiced_sound       },
    { XK_dead_belowdot,         display::Key::dead_belowdot               },
    { XK_dead_hook,             display::Key::dead_hook                   },
    { XK_dead_horn,             display::Key::dead_horn                   },
    { XK_dead_stroke,           display::Key::dead_stroke                 },
    { XK_dead_psili,            display::Key::dead_psili                  },
    { XK_dead_dasia,            display::Key::dead_dasia                  },
    { XK_dead_doublegrave,      display::Key::dead_doublegrave            },
    { XK_dead_belowring,        display::Key::dead_belowring              },
    { XK_dead_belowmacron,      display::Key::dead_belowmacron            },
    { XK_dead_belowcircumflex,  display::Key::dead_belowcircumflex        },
    { XK_dead_belowtilde,       display::Key::dead_belowtilde             },
    { XK_dead_belowbreve,       display::Key::dead_belowbreve             },
    { XK_dead_belowdiaeresis,   display::Key::dead_belowdiaeresis         },
    { XK_dead_invertedbreve,    display::Key::dead_invertedbreve          },
    { XK_dead_belowcomma,       display::Key::dead_belowcomma             },
    { XK_dead_currency,         display::Key::dead_currency               },

    // Basic Latin
    { XK_space,                 display::Key::space                       },
    { XK_exclam,                display::Key::exclamation_mark            },
    { XK_quotedbl,              display::Key::quotation_mark              },
    { XK_numbersign,            display::Key::number_sign                 },
    { XK_dollar,                display::Key::dollar_sign                 },
    { XK_percent,               display::Key::percent_sign                },
    { XK_ampersand,             display::Key::ampersand                   },
    { XK_apostrophe,            display::Key::apostrophe                  },
    { XK_parenleft,             display::Key::left_parenthesis            },
    { XK_parenright,            display::Key::right_parenthesis           },
    { XK_asterisk,              display::Key::asterisk                    },
    { XK_plus,                  display::Key::plus_sign                   },
    { XK_comma,                 display::Key::comma                       },
    { XK_minus,                 display::Key::hyphen_minus                },
    { XK_period,                display::Key::full_stop                   },
    { XK_slash,                 display::Key::solidus                     },
    { XK_0,                     display::Key::digit_0                     },
    { XK_1,                     display::Key::digit_1                     },
    { XK_2,                     display::Key::digit_2                     },
    { XK_3,                     display::Key::digit_3                     },
    { XK_4,                     display::Key::digit_4                     },
    { XK_5,                     display::Key::digit_5                     },
    { XK_6,                     display::Key::digit_6                     },
    { XK_7,                     display::Key::digit_7                     },
    { XK_8,                     display::Key::digit_8                     },
    { XK_9,                     display::Key::digit_9                     },
    { XK_colon,                 display::Key::colon                       },
    { XK_semicolon,             display::Key::semicolon                   },
    { XK_less,                  display::Key::less_than_sign              },
    { XK_equal,                 display::Key::equals_sign                 },
    { XK_greater,               display::Key::greater_than_sign           },
    { XK_question,              display::Key::question_mark               },
    { XK_at,                    display::Key::commercial_at               },
    { XK_A,                     display::Key::capital_a                   },
    { XK_B,                     display::Key::capital_b                   },
    { XK_C,                     display::Key::capital_c                   },
    { XK_D,                     display::Key::capital_d                   },
    { XK_E,                     display::Key::capital_e                   },
    { XK_F,                     display::Key::capital_f                   },
    { XK_G,                     display::Key::capital_g                   },
    { XK_H,                     display::Key::capital_h                   },
    { XK_I,                     display::Key::capital_i                   },
    { XK_J,                     display::Key::capital_j                   },
    { XK_K,                     display::Key::capital_k                   },
    { XK_L,                     display::Key::capital_l                   },
    { XK_M,                     display::Key::capital_m                   },
    { XK_N,                     display::Key::capital_n                   },
    { XK_O,                     display::Key::capital_o                   },
    { XK_P,                     display::Key::capital_p                   },
    { XK_Q,                     display::Key::capital_q                   },
    { XK_R,                     display::Key::capital_r                   },
    { XK_S,                     display::Key::capital_s                   },
    { XK_T,                     display::Key::capital_t                   },
    { XK_U,                     display::Key::capital_u                   },
    { XK_V,                     display::Key::capital_v                   },
    { XK_W,                     display::Key::capital_w                   },
    { XK_X,                     display::Key::capital_x                   },
    { XK_Y,                     display::Key::capital_y                   },
    { XK_Z,                     display::Key::capital_z                   },
    { XK_bracketleft,           display::Key::left_square_bracket         },
    { XK_backslash,             display::Key::reverse_solidus             },
    { XK_bracketright,          display::Key::right_square_bracket        },
    { XK_asciicircum,           display::Key::circumflex_accent           },
    { XK_underscore,            display::Key::low_line                    },
    { XK_grave,                 display::Key::grave_accent                },
    { XK_a,                     display::Key::small_a                     },
    { XK_b,                     display::Key::small_b                     },
    { XK_c,                     display::Key::small_c                     },
    { XK_d,                     display::Key::small_d                     },
    { XK_e,                     display::Key::small_e                     },
    { XK_f,                     display::Key::small_f                     },
    { XK_g,                     display::Key::small_g                     },
    { XK_h,                     display::Key::small_h                     },
    { XK_i,                     display::Key::small_i                     },
    { XK_j,                     display::Key::small_j                     },
    { XK_k,                     display::Key::small_k                     },
    { XK_l,                     display::Key::small_l                     },
    { XK_m,                     display::Key::small_m                     },
    { XK_n,                     display::Key::small_n                     },
    { XK_o,                     display::Key::small_o                     },
    { XK_p,                     display::Key::small_p                     },
    { XK_q,                     display::Key::small_q                     },
    { XK_r,                     display::Key::small_r                     },
    { XK_s,                     display::Key::small_s                     },
    { XK_t,                     display::Key::small_t                     },
    { XK_u,                     display::Key::small_u                     },
    { XK_v,                     display::Key::small_v                     },
    { XK_w,                     display::Key::small_w                     },
    { XK_x,                     display::Key::small_x                     },
    { XK_y,                     display::Key::small_y                     },
    { XK_z,                     display::Key::small_z                     },
    { XK_braceleft,             display::Key::left_curly_bracket          },
    { XK_bar,                   display::Key::vertical_line               },
    { XK_braceright,            display::Key::right_curly_bracket         },
    { XK_asciitilde,            display::Key::tilde                       },

    // Latin-1 Supplement
    { XK_nobreakspace,          display::Key::nobreak_space               },
    { XK_exclamdown,            display::Key::inverted_exclamation_mark   },
    { XK_cent,                  display::Key::cent_sign                   },
    { XK_sterling,              display::Key::pound_sign                  },
    { XK_currency,              display::Key::currency_sign               },
    { XK_yen,                   display::Key::yen_sign                    },
    { XK_brokenbar,             display::Key::broken_bar                  },
    { XK_section,               display::Key::section_sign                },
    { XK_diaeresis,             display::Key::diaeresis                   },
    { XK_copyright,             display::Key::copyright_sign              },
    { XK_ordfeminine,           display::Key::feminine_ordinal_indicator  },
    { XK_guillemotleft,         display::Key::left_guillemet              },
    { XK_notsign,               display::Key::not_sign                    },
    { XK_hyphen,                display::Key::soft_hyphen                 },
    { XK_registered,            display::Key::registered_sign             },
    { XK_macron,                display::Key::macron                      },
    { XK_degree,                display::Key::degree_sign                 },
    { XK_plusminus,             display::Key::plus_minus_sign             },
    { XK_twosuperior,           display::Key::superscript_two             },
    { XK_threesuperior,         display::Key::superscript_three           },
    { XK_acute,                 display::Key::acute_accent                },
    { XK_mu,                    display::Key::micro_sign                  },
    { XK_paragraph,             display::Key::pilcrow_sign                },
    { XK_periodcentered,        display::Key::middle_dot                  },
    { XK_cedilla,               display::Key::cedilla                     },
    { XK_onesuperior,           display::Key::superscript_one             },
    { XK_masculine,             display::Key::masculine_ordinal_indicator },
    { XK_guillemotright,        display::Key::right_guillemet             },
    { XK_onequarter,            display::Key::one_quarter                 },
    { XK_onehalf,               display::Key::one_half                    },
    { XK_threequarters,         display::Key::three_quarters              },
    { XK_questiondown,          display::Key::inverted_question_mark      },
    { XK_Agrave,                display::Key::capital_a_grave             },
    { XK_Aacute,                display::Key::capital_a_acute             },
    { XK_Acircumflex,           display::Key::capital_a_circumflex        },
    { XK_Atilde,                display::Key::capital_a_tilde             },
    { XK_Adiaeresis,            display::Key::capital_a_diaeresis         },
    { XK_Aring,                 display::Key::capital_a_ring              },
    { XK_AE,                    display::Key::capital_ae_ligature         },
    { XK_Ccedilla,              display::Key::capital_c_cedilla           },
    { XK_Egrave,                display::Key::capital_e_grave             },
    { XK_Eacute,                display::Key::capital_e_acute             },
    { XK_Ecircumflex,           display::Key::capital_e_circumflex        },
    { XK_Ediaeresis,            display::Key::capital_e_diaeresis         },
    { XK_Igrave,                display::Key::capital_i_grave             },
    { XK_Iacute,                display::Key::capital_i_acute             },
    { XK_Icircumflex,           display::Key::capital_i_circumflex        },
    { XK_Idiaeresis,            display::Key::capital_i_diaeresis         },
    { XK_ETH,                   display::Key::capital_eth                 },
    { XK_Ntilde,                display::Key::capital_n_tilde             },
    { XK_Ograve,                display::Key::capital_o_grave             },
    { XK_Oacute,                display::Key::capital_o_acute             },
    { XK_Ocircumflex,           display::Key::capital_o_circumflex        },
    { XK_Otilde,                display::Key::capital_o_tilde             },
    { XK_Odiaeresis,            display::Key::capital_o_diaeresis         },
    { XK_multiply,              display::Key::multiplication_sign         },
    { XK_Oslash,                display::Key::capital_o_stroke            },
    { XK_Ugrave,                display::Key::capital_u_grave             },
    { XK_Uacute,                display::Key::capital_u_acute             },
    { XK_Ucircumflex,           display::Key::capital_u_circumflex        },
    { XK_Udiaeresis,            display::Key::capital_u_diaeresis         },
    { XK_Yacute,                display::Key::capital_y_acute             },
    { XK_THORN,                 display::Key::capital_thorn               },
    { XK_ssharp,                display::Key::sharp_s                     },
    { XK_agrave,                display::Key::small_a_grave               },
    { XK_aacute,                display::Key::small_a_acute               },
    { XK_acircumflex,           display::Key::small_a_circumflex          },
    { XK_atilde,                display::Key::small_a_tilde               },
    { XK_adiaeresis,            display::Key::small_a_diaeresis           },
    { XK_aring,                 display::Key::small_a_ring                },
    { XK_ae,                    display::Key::small_ae_ligature           },
    { XK_ccedilla,              display::Key::small_c_cedilla             },
    { XK_egrave,                display::Key::small_e_grave               },
    { XK_eacute,                display::Key::small_e_acute               },
    { XK_ecircumflex,           display::Key::small_e_circumflex          },
    { XK_ediaeresis,            display::Key::small_e_diaeresis           },
    { XK_igrave,                display::Key::small_i_grave               },
    { XK_iacute,                display::Key::small_i_acute               },
    { XK_icircumflex,           display::Key::small_i_circumflex          },
    { XK_idiaeresis,            display::Key::small_i_diaeresis           },
    { XK_eth,                   display::Key::small_eth                   },
    { XK_ntilde,                display::Key::small_n_tilde               },
    { XK_ograve,                display::Key::small_o_grave               },
    { XK_oacute,                display::Key::small_o_acute               },
    { XK_ocircumflex,           display::Key::small_o_circumflex          },
    { XK_otilde,                display::Key::small_o_tilde               },
    { XK_odiaeresis,            display::Key::small_o_diaeresis           },
    { XK_division,              display::Key::division_sign               },
    { XK_oslash,                display::Key::small_o_stroke              },
    { XK_ugrave,                display::Key::small_u_grave               },
    { XK_uacute,                display::Key::small_u_acute               },
    { XK_ucircumflex,           display::Key::small_u_circumflex          },
    { XK_udiaeresis,            display::Key::small_u_diaeresis           },
    { XK_yacute,                display::Key::small_y_acute               },
    { XK_thorn,                 display::Key::small_thorn                 },
    { XK_ydiaeresis,            display::Key::small_y_diaeresis           },
};


constexpr core::LiteralHashMap g_key_map     = core::make_literal_hash_map(key_assocs);
constexpr core::LiteralHashMap g_rev_key_map = core::make_rev_literal_hash_map(key_assocs);


inline bool map_key(display::KeyCode key_code, display::Key& key) noexcept
{
    auto keysym = KeySym(key_code.code);
    return g_key_map.find(keysym, key);
}


inline bool rev_map_key(display::Key key, display::KeyCode& key_code) noexcept
{
    KeySym keysym = {};
    if (ARCHON_LIKELY(g_rev_key_map.find(key, keysym))) {
        key_code = { display::KeyCode::code_type(keysym) };
        return true;
    }
    return false;
}


bool try_map_mouse_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                          math::Vector2F& amount) noexcept
{
    switch (x11_button) {
        case 1:
            is_scroll = false;
            button = display::MouseButton::left;
            return true;
        case 2:
            is_scroll = false;
            button = display::MouseButton::middle;
            return true;
        case 3:
            is_scroll = false;
            button = display::MouseButton::right;
            return true;
        case 4:
            is_scroll = true;
            amount = { 0, +1 }; // Scroll up
            return true;
        case 5:
            is_scroll = true;
            amount = { 0, -1 }; // Scroll down
            return true;
        case 6:
            is_scroll = true;
            amount = { -1, 0 }; // Scroll left
            return true;
        case 7:
            is_scroll = true;
            amount = { +1, 0 }; // Scroll right
            return true;
        case 8:
            is_scroll = false;
            button = display::MouseButton::x1;
            return true;
        case 9:
            is_scroll = false;
            button = display::MouseButton::x2;
            return true;
    }
    return false;
}


#else // !HAVE_X11


class SlotImpl final
    : public display::Implementation::Slot {
public:
    auto ident() const noexcept -> std::string_view override;
    auto get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation* override;
};


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation*
{
    return nullptr;
}


#endif // !HAVE_X11


} // unnamed namespace


auto display::get_x11_implementation_slot() noexcept -> const display::Implementation::Slot&
{
    static SlotImpl slot;
    return slot;
}
