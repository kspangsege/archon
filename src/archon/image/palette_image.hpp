// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_IMAGE_X_PALETTE_IMAGE_HPP
#define ARCHON_X_IMAGE_X_PALETTE_IMAGE_HPP

/// \file


#include <algorithm>

#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Present array of colors as image useful as palette.
///
/// This class allows for an array of colors to be presented as an image in a way that makes
/// that image useful as a palatte (\ref image::Image::get_palette()).
///
/// Palettes (images functioning as palettes) can be used with images that use an indexed
/// pixel format. For example, a buffered image (\ref image::BufferedImage) using a pixel
/// format of type \ref image::IndexedPixelFormat.
///
/// \code{.cpp}
///
///   const archon::image::Pixel_RGBA_8 colors[] = {
///       archon::image::Pixel(archon::util::colors::black),
///       archon::image::Pixel(archon::util::colors::red),
///   };
///   archon::image::PaletteImage palette(colors);
///   archon::image::IndexedPixelFormat_8<> format(palette); // 8 bits per index
///   archon::image::BufferedImage image(image_size, std::move(format));
///
/// \endcode
///
/// An image of this type has a width equal to the number of colors in the palette and a
/// height of 1.
///
/// \sa \ref image::get_bw_palette(), \ref image::get_gray4_palette(), \ref
/// image::get_css16_palette()
///
template<class R> class PaletteImage
    : public image::Image {
public:
    using repr_type = R;

    using pixel_type = image::Pixel<repr_type>;
    using span_type  = core::Span<const pixel_type>;

    /// \brief Construct palette image from specified colors.
    ///
    /// This function constructs a palette image from the specified colors.
    ///
    /// The constructed palette image will contain a copy of the referece to the specified
    /// array of colors, and not a copy of the colors themselves, so the caller must ensure
    /// that the array stays alive for as long as the palette image remains in
    /// use. Destruction of the palette image is allowed to happen after the destruction of
    /// the array of colors.
    ///
    /// The number of colors must not exceed the largest value representable in `int`.
    ///
    PaletteImage(span_type colors);

    /// \brief Get colors of palette.
    ///
    /// This function returns a reference to the colors that were passed to the constructor.
    ///
    auto get_colors() const noexcept -> span_type;

    // Overriding virtual member functions of `image::Image`.
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

private:
    span_type m_colors;

    auto do_get_size() const noexcept -> image::Size;
};

template<class R, std::size_t N> PaletteImage(image::Pixel<R> (&)[N]) -> PaletteImage<R>;
template<class R, std::size_t N> PaletteImage(const image::Pixel<R> (&)[N]) -> PaletteImage<R>;
template<class R> PaletteImage(core::Span<image::Pixel<R>>) -> PaletteImage<R>;


using PaletteImage_Lum_8  = PaletteImage<image::Lum_8>;
using PaletteImage_LumA_8 = PaletteImage<image::LumA_8>;
using PaletteImage_RGB_8  = PaletteImage<image::RGB_8>;
using PaletteImage_RGBA_8 = PaletteImage<image::RGBA_8>;

using PaletteImage_Lum_16  = PaletteImage<image::Lum_16>;
using PaletteImage_LumA_16 = PaletteImage<image::LumA_16>;
using PaletteImage_RGB_16  = PaletteImage<image::RGB_16>;
using PaletteImage_RGBA_16 = PaletteImage<image::RGBA_16>;

using PaletteImage_Lum_F  = PaletteImage<image::Lum_F>;
using PaletteImage_LumA_F = PaletteImage<image::LumA_F>;
using PaletteImage_RGB_F  = PaletteImage<image::RGB_F>;
using PaletteImage_RGBA_F = PaletteImage<image::RGBA_F>;








// Implementation


template<class R>
inline PaletteImage<R>::PaletteImage(core::Span<const pixel_type> colors)
    : m_colors(colors)
{
    // Verify that the number of colors is no greater than the maximum width of an image.
    int size; // Dummy
    core::int_cast(colors.size(), size); // Throws
}


template<class R>
auto PaletteImage<R>::get_size() const noexcept -> image::Size
{
    return do_get_size();
}


template<class R>
bool PaletteImage<R>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false;
}


template<class R>
auto PaletteImage<R>::get_transfer_info() const -> TransferInfo
{
    return {
        repr_type::comp_repr,
        &repr_type::get_color_space(),
        repr_type::has_alpha,
        image::comp_repr_bit_width<repr_type::comp_repr>(),
    };
}


template<class R>
auto PaletteImage<R>::get_palette() const noexcept -> const Image*
{
    return nullptr;
}


template<class R>
void PaletteImage<R>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(do_get_size()));
    using comp_type = typename repr_type::comp_type;
    image::Tray tray_2 = tray.cast_to<comp_type>();
    if (tray_2.size.height > 0) {
        int n = tray_2.size.width;
        for (int i = 0; i < n; ++i) {
            const pixel_type& color = m_colors[pos.x + i];
            const comp_type* origin = color.data();
            comp_type* destin = tray_2(i, 0);
            int num_channels = repr_type::num_channels;
            std::copy(origin, origin + num_channels, destin);
        }
    }
}


template<class R>
inline auto PaletteImage<R>::do_get_size() const noexcept -> image::Size
{
    int width  = int(m_colors.size());
    int height = 1;
    return { width, height };
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PALETTE_IMAGE_HPP
