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


#include <utility>
#include <memory>
#include <optional>
#include <locale>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/noinst/x11_support.hpp>


using namespace archon;
namespace impl = display::impl;


#if HAVE_X11


namespace {


// For TrueColor and DirectColor visuals
template<class T, class P, int N, bool R> class DirectColorPixelFormat final
    : public impl::PixelFormat {
public:
    using compound_type = T;
    using packing_type = P;
    static constexpr int bytes_per_pixel = N;
    static constexpr bool reverse_channel_order = R;

    DirectColorPixelFormat(Display* dpy, const XVisualInfo&, const XPixmapFormatValues&) noexcept;

    auto intern_color(util::Color) const -> unsigned long override;
    void setup_image_bridge(display::Size, std::unique_ptr<image::WritableImage>&, XImage&) const override;

private:
    Display* const m_dpy;
    const XVisualInfo& m_visual_info;
    const XPixmapFormatValues& m_pixmap_format;
};


template<class T, class P, int N, bool R>
inline DirectColorPixelFormat<T, P, N, R>::DirectColorPixelFormat(Display* dpy, const XVisualInfo& visual_info,
                                                                  const XPixmapFormatValues& pixmap_format) noexcept
    : m_dpy(dpy)
    , m_visual_info(visual_info)
    , m_pixmap_format(pixmap_format)
{
}


template<class T, class P, int N, bool R>
auto DirectColorPixelFormat<T, P, N, R>::intern_color(util::Color color) const -> unsigned long
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
void DirectColorPixelFormat<T, P, N, R>::setup_image_bridge(display::Size size,
                                                            std::unique_ptr<image::WritableImage>& img_1,
                                                            XImage& img_2) const
{
    using word_type = char;
    constexpr int bits_per_word = 8;
    constexpr core::Endianness word_order = core::Endianness::little;
    constexpr bool alpha_channel_first = false;

    using format_type = image::PackedPixelFormat<image::ChannelSpec_RGB, compound_type, packing_type, word_type,
                                                 bits_per_word, bytes_per_pixel, word_order, alpha_channel_first,
                                                 reverse_channel_order>;

    auto img_3 = std::make_unique<image::BufferedImage<format_type>>(size); // Throws

    // Xlib requires that the depth of the image (XImage) matches the depth of the window or
    // pixmap (target of XPutImage()). Only ZPixmap format is relevant. With ZPixmap, image
    // data is ordered by pixel rather than by bit-plane, and each scanline unit (word)
    // holds one or more pixels. The ZPixmap format supports the depths of any offered
    // visual. XPutImage() can handle byte swapping and changes in row alignment
    // (`scanline_pad` / `bitmap_pad`).
    int scanline_pad = m_pixmap_format.bits_per_pixel;
    img_2.width            = size.width;
    img_2.height           = size.height;
    img_2.xoffset          = 0;
    img_2.format           = ZPixmap;
    img_2.data             = img_3->get_buffer().data();
    img_2.byte_order       = LSBFirst;
    img_2.bitmap_unit      = BitmapUnit(m_dpy); // Immaterial
    img_2.bitmap_bit_order = BitmapBitOrder(m_dpy); // Immaterial
    img_2.bitmap_pad       = scanline_pad;
    img_2.depth            = m_visual_info.depth;
    img_2.bytes_per_line   = 0;
    img_2.bits_per_pixel   = m_pixmap_format.bits_per_pixel;
    img_2.red_mask         = m_visual_info.red_mask;
    img_2.green_mask       = m_visual_info.green_mask;
    img_2.blue_mask        = m_visual_info.blue_mask;
    Status status = XInitImage(&img_2);
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XInitImage() failed");

    img_1 = std::move(img_3);
}


} // unnamed namespace


auto impl::map_opt_visual_class(const std::optional<display::ConnectionConfigX11::VisualClass>& class_) noexcept ->
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


auto impl::get_visual_class_name(int class_) noexcept -> const char*
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


auto impl::create_pixel_format(Display* dpy, const XVisualInfo& visual_info, const XPixmapFormatValues& pixmap_format,
                               Colormap colormap, const std::locale& locale) -> std::unique_ptr<impl::PixelFormat>
{
    static_cast<void>(colormap);                         

    std::string msg;

    if (visual_info.depth == 8) {
        if (pixmap_format.bits_per_pixel != 8)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 1;
/*            
        if (visual_info.c_class == StaticGray || visual_info.c_class == GrayScale) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (ARCHON_LIKELY(impl::zero_mask_match(visual_info)))
                return std::make_unique<DirectGrayPixelFormat<image::int8_type, 8, bytes_per_pixel>>(); // Throws              
            goto unsupported_channel_masks;
        }
*/
/*
        if (visual_info.c_class == StaticColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                goto unexpected_colormap_size;
            // According to the X specification, masks only make sense for TrueColor and
            // DirectColor visuals. Despite that, it appears that some X servers choose
            // to expose the color structure of StaticColor visuals using masks, most
            // notably Xephyr + X.Org (e.g., using `Xephyr :1 -screen 1024x1024x8`).
            if (impl::norm_mask_match<image::ChannelPacking_332>(visual_info)) {
                constexpr bool reverse_channel_order = false;
                auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                             reverse_channel_order>(img_size); // Throws
                data = img->get_buffer().data();
                img_2 = std::move(img);
                goto matched;
            }
            if (impl::rev_mask_match<image::ChannelPacking_233>(visual_info)) {
                constexpr bool reverse_channel_order = true;
                auto img = make_packed_image<image::int8_type, image::ChannelPacking_233, bytes_per_pixel,
                                             reverse_channel_order>(img_size); // Throws
                data = img->get_buffer().data();
                img_2 = std::move(img);
                goto matched;
            }
            if (impl::zero_mask_match(visual_info)) {
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
        if (visual_info.c_class == PseudoColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (impl::zero_mask_match(visual_info)) {
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
        if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 8))
                goto unexpected_colormap_size;
            if (impl::norm_mask_match<image::ChannelPacking_332>(visual_info)) {
                constexpr bool reverse_channel_order = false;
                using pixel_format_type = DirectColorPixelFormat<image::int8_type, image::ChannelPacking_332,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            if (impl::rev_mask_match<image::ChannelPacking_233>(visual_info)) {
                constexpr bool reverse_channel_order = true;
                using pixel_format_type = DirectColorPixelFormat<image::int8_type, image::ChannelPacking_233,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (visual_info.depth == 15) {
        if (pixmap_format.bits_per_pixel != 16)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 2;
        if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 32))
                goto unexpected_colormap_size;
            if (impl::norm_mask_match<image::ChannelPacking_555>(visual_info)) {
                constexpr bool reverse_channel_order = false;
                using pixel_format_type = DirectColorPixelFormat<image::int16_type, image::ChannelPacking_555,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            if (impl::rev_mask_match<image::ChannelPacking_555>(visual_info)) {
                constexpr bool reverse_channel_order = true;
                using pixel_format_type = DirectColorPixelFormat<image::int16_type, image::ChannelPacking_555,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (visual_info.depth == 16) {
        if (pixmap_format.bits_per_pixel != 16)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 2;
        if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 64))
                goto unexpected_colormap_size;
            if (impl::norm_mask_match<image::ChannelPacking_565>(visual_info)) {
                constexpr bool reverse_channel_order = false;
                using pixel_format_type = DirectColorPixelFormat<image::int16_type, image::ChannelPacking_565,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            else if (impl::rev_mask_match<image::ChannelPacking_565>(visual_info)) {
                constexpr bool reverse_channel_order = true;
                using pixel_format_type = DirectColorPixelFormat<image::int16_type, image::ChannelPacking_565,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    if (visual_info.depth == 24) {
        if (pixmap_format.bits_per_pixel != 32)
            goto unsupported_bits_per_pixel;
        constexpr int bytes_per_pixel = 4;
        if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
            if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                goto unexpected_colormap_size;
            if (impl::norm_mask_match<image::ChannelPacking_888>(visual_info)) {
                constexpr bool reverse_channel_order = false;
                using pixel_format_type = DirectColorPixelFormat<image::int32_type, image::ChannelPacking_888,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            if (impl::rev_mask_match<image::ChannelPacking_888>(visual_info)) {
                constexpr bool reverse_channel_order = true;
                using pixel_format_type = DirectColorPixelFormat<image::int32_type, image::ChannelPacking_888,
                                                                 bytes_per_pixel, reverse_channel_order>;
                return std::make_unique<pixel_format_type>(dpy, visual_info, pixmap_format); // Throws
            }
            goto unsupported_channel_masks;
        }
        goto unexpected_visual_class;
    }
    goto unsupported_depth;

  unsupported_depth:
    msg = core::format(locale, "Unsupported depth: %s", core::as_int(visual_info.depth)); // Throws
    throw std::runtime_error(msg);

  unsupported_bits_per_pixel:
    msg = core::format(locale, "Unsupported number bits per pixel for depth %s: %s", core::as_int(visual_info.depth),
                       core::as_int(pixmap_format.bits_per_pixel)); // Throws
    throw std::runtime_error(msg);

  unexpected_visual_class:
    msg = core::format(locale, "Unexpected class for visual 0x%s: %s", core::as_hex_int(visual_info.visualid),
                       impl::get_visual_class_name(visual_info.c_class)); // Throws
    throw std::runtime_error(msg);

  unsupported_channel_masks:
    msg = core::format(locale, "Unsupported channel masks in visual 0x%s: red = %s, green = %s, blue = %s",
                       core::as_hex_int(visual_info.visualid), core::as_hex_int(visual_info.red_mask),
                       core::as_hex_int(visual_info.green_mask), core::as_hex_int(visual_info.blue_mask)); // Throws
    throw std::runtime_error(msg);

  unexpected_colormap_size:
    msg = core::format(locale, "Unexpected colormap size for visual 0x%s: %s", core::as_hex_int(visual_info.visualid),
                       core::as_int(visual_info.colormap_size)); // Throws
    throw std::runtime_error(msg);
}


#endif // HAVE_X11
