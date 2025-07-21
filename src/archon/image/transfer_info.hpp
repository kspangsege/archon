// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_IMAGE_X_TRANSFER_INFO_HPP
#define ARCHON_X_IMAGE_X_TRANSFER_INFO_HPP

/// \file


#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/image_fwd.hpp>


namespace archon::image {


/// \brief Description of how pixels are transferred into and out of an image.
///
/// An object of this type describes how pixels are transferred into and out of a given
/// image (\ref image::Image). In particular, it specifies the pixel transfer scheme (see
/// below) which determines how pixels are received from \ref image::Image::read() and how
/// they are passed to \ref image::WritableImage::write().
///
/// To obtain a description for a particular image, call \ref
/// image::Image::get_transfer_info().
///
/// \sa \ref image::Image::get_transfer_info()
///
///
/// #### Pixel transfer scheme
///
/// Every concrete image implementation effectively specifies a *pixel transfer
/// scheme*. This is the representation scheme that applies to pixels as they are received
/// from \ref image::Image::read(), and in the case of a writable image, as they are passed
/// to \ref image::WritableImage::write() or \ref image::WritableImage::fill().
///
/// For an image that uses direct color (when \ref palette is null), the pixel transfer
/// scheme consists of three pieces of information; a color space, whether an alpha channel
/// is present, and a component representation scheme. These are specified by \ref
/// color_space, \ref has_alpha, and \ref comp_repr respectively. Under the direct color
/// scheme, each passed pixel consists of a fixed number of channel components. The meaning
/// of these channel components is specified by the color space, and the representation of
/// the component values is specified by the component representation scheme. The number of
/// channel components per pixel is the number of channels in the color space plus one if an
/// alpha channel is present. The order of channel components coincide with the canonical
/// channel order of the color space, with the alpha component always coming last when
/// present.
///
/// For an image that uses indirect color (when \ref palette is not null), the pixel
/// transfer scheme is implied. Each passed pixel consists of a single channel component,
/// which is a color index (index into palette), and the component representation scheme is
/// `int8` (see \ref image::CompRepr).
///
/// The *resolved pixel transfer scheme* is the direct color pixel transfer scheme obtained
/// by deferring the inquiry to the palette image whenever the inquired image uses indirect
/// color. More formally, for an image that uses direct color, the resolved pixel transfer
/// scheme is equal to the pixel transfer scheme as defined above, and for an image that
/// uses indirect color, it is the resolved pixel transfer scheme of the palette image.
///
struct TransferInfo {
    /// \brief Color space of resolved pixel transfer scheme.
    ///
    /// This is the color space of the resolved pixel transfer scheme of the described image
    /// (see class-level documentation).
    ///
    const image::ColorSpace* color_space;

    /// \brief Whether alpha channel is present in resolved pixel transfer scheme.
    ///
    /// This flag specifies whether an alpha channel is present in the resolved pixel
    /// transfer scheme of the described image (see class-level documentation).
    ///
    bool has_alpha;

    /// \brief Component representation scheme of resolved pixel transfer scheme.
    ///
    /// This is the component representation scheme of the resolved pixel transfer scheme of
    /// the described image (see class-level documentation).
    ///
    image::CompRepr comp_repr;

    /// \brief Number of bits used to store each channel component.
    ///
    /// This is the number of bits used by the described image to store each channel
    /// component (color or alpha channel). If the number differs among the channels, it is
    /// the greatest number of bits among them.
    ///
    /// For integer representations, this is the number of bits used per channel
    /// component. For floating point representations, it is the number of
    /// mantissa bits plus the number of exponent bits in the floating point type used to
    /// store or generate each channel component.
    ///
    /// If a channel uses a non-integer number of bits, such as a channel that uses N
    /// distinct values where N is not a power of two, the number of bits should be rounded
    /// up.
    ///
    /// When the described image uses indirect color, this information regards the storage
    /// of channel components (color and alpha channels) in the image acting as palette.
    ///
    int bit_depth;

    /// \brief Associated palette for indirect color images.
    ///
    /// If this is null, the described image specifies color directly and therefore has no
    /// palette. Otherwise, the described image specifies color indirectly, and this is the
    /// image that acts as palette.
    ///
    /// When an image acts as a palette, all, or some of its pixels become the color entries
    /// of the that palette (see \ref determine_palette_size()). The pixels that do become
    /// part of the palette will occur in the palette according to a top-to-bottom,
    /// left-to-right, row-major pixel traversal order.
    ///
    /// A palette can have zero colors. This is allowed because color indexes that refer
    /// outside the palette are allowed. The effect of such indexes will depend on
    /// context. In particular, the reader (\ref Reader) resolves them as the configured
    /// background color.
    ///
    /// Even though it would be silly, a palette image is itself allowed to specify colors
    /// indirectly.
    ///
    /// \sa \ref determine_palette_size()
    ///
    const Image* palette;

    // FIXME: Add `index_repr` here                   

    /// \brief Number of bits used to store each color index.
    ///
    /// If the described image uses indirect color, this is the number of bits used to store
    /// each color index. If the described image uses direct color, this is zero.
    ///
    /// If N is the value of `index_depth`, it follows that any retrieved color index will
    /// be less than two to the power of N, and, for a writable image, it follows that any
    /// index less than two to the power of N can be written to the image and then read back
    /// undamaged.
    ///
    /// `index_depth` is required to be less than, or equal to the number of bits in the
    /// component representation scheme used for color indexes, which is
    /// `image::comp_repr_int_bit_width(image::color_index_repr)`.
    ///
    /// FIXME: Adjust paragraph above when/if `index_repr` is added.        
    ///
    int index_depth;

    /// \brief Number of channels in resolved pixel transfer scheme.
    ///
    /// This function returns the number of channels in the resolved pixel transfer scheme
    /// of the described image. This is the number of color channels plus one if an alpha
    /// channel is present.
    ///
    int get_num_channels() const noexcept;

    /// \brief Determine number of colors in palette.
    ///
    /// If the described image uses indirect color (\ref palette is not null), this function
    /// determines the effective palette size (see below). If the attached image uses direct
    /// color (\ref palette is null), this function returns zero.
    ///
    /// The *effective palette size* is determined as the number of pixels in the image
    /// acting as palette clamped to the smaller of N and 2^M where N is the maximum
    /// representable value in `std::size_t` and M is the number of bits used to store each
    /// color index in the attached image (\ref index_depth).
    ///
    /// The effective palette size specifies how many pixels from the palette image that
    /// become part of the actual palette. When there are more pixels in the palette image
    /// than the effective palette size, the pixels that are used are those that occur first
    /// according to a top-to-bottom, left-to-right, row-major traversal.
    ///
    /// \sa \ref palette, \ref index_depth
    ///
    auto determine_palette_size() const noexcept -> std::size_t;
};








// Implementation


inline int TransferInfo::get_num_channels() const noexcept
{
    return color_space->get_num_channels() + int(has_alpha);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_TRANSFER_INFO_HPP
