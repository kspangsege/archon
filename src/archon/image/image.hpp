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

#ifndef ARCHON_X_IMAGE_X_IMAGE_HPP
#define ARCHON_X_IMAGE_X_IMAGE_HPP

/// \file


#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/buffer_format.hpp>


namespace archon::image {


/// \brief Common base class for all image implementations.
///
/// This class serves as a common base class for all image implementations and offers
/// facilities for reading of pixels. Image implementations that offer facilities for
/// writing of pixels should inherit from \ref image::WritableImage, which, in turn,
/// inherits from this class. Examples of non-writable images are \ref image::ComputedImage
/// and \ref image::PaletteImage.
///
/// While the contents of an image can be read through direct use of the API offered by this
/// class, most applications will instead want to access the image via a reader (\ref
/// image::Reader) or a writer (\ref image::Writer).
///
///
/// #### Pixel transfer scheme
///
/// Every concrete image implementation effectively specifies a *pixel transfer
/// scheme*. This is the representation scheme that applies to pixels as they are passed out
/// of \ref read(), and in the case of a writable image, as they are passed into \ref
/// image::WritableImage::write() or \ref image::WritableImage::fill().
///
/// For an image that uses direct color (when \ref get_palette() returns null), the pixel
/// transfer scheme consists of three pieces of information, which are all returned by \ref
/// get_transfer_info(); a component representation scheme (see \ref image::CompRepr), a
/// color space (see \ref image::ColorSpace), and whether an alpha channel is present. In
/// this case, each passed pixel consists of a fixed number of channel components. The
/// meaning of these channel components is specified by the transfer color space, and the
/// representation of the component values is specified by the transfer component
/// representation scheme. The number of channel components per pixel is the number of
/// channels in the transfer color space plus one if an alpha channel is present in the
/// pixel transfer scheme. The order of channel components coincide with the canonical
/// channel order of the transfer color space, with the alpha component always coming last
/// when present.
///
/// For an image that uses indirect color (when \ref get_palette() does not return null),
/// the pixel transfer scheme is implied. Each passed pixel consists of a single channel
/// component, which is a color index (index into palette), and the component representation
/// scheme is `int8` (see \ref image::CompRepr).
///
/// The *resolved pixel transfer scheme* is the direct color pixel transfer scheme obtained
/// by looking at the palette image whenever the current image uses indirect color. More
/// formally, for an image that uses direct color, it is equal to the pixel transfer scheme
/// as defined above, and for an image that uses indirect color, it is the resolved pixel
/// transfer scheme of the palette image. See \ref get_transfer_info().
///
class Image {
public:
    /// \brief Get size of image.
    ///
    /// This function returns the size of the image as a number of pixels along the vertical
    /// and horizontal axes. Either or both of the components of the size (horizontal and
    /// vertical) can be zero. Neither can be negative.
    ///
    virtual auto get_size() const noexcept -> image::Size = 0;

    /// \brief Reveal pixel buffer and pixel storage format when possible.
    ///
    /// An application can call this function as a request to obtain direct access to the
    /// underlying pixel buffer. Such a request either succeeds or fails. When it succeeds,
    /// this function must return `true` after setting \p format and \p buffer
    /// appropriately. When it fails, this function must return `false` and leave \p format
    /// and \p buffer unchanged.
    ///
    /// On success, the returned buffer and buffer format shall remain valid until the image
    /// is destroyed.
    ///
    /// The general intention is that a request for direct access to the pixel buffer should
    /// succeed when, and only when pixels are stored in a memory buffer using a storage
    /// format that can be described using \ref image::BufferFormat. Each image
    /// implementation should document whether, or when a request for direct access to the
    /// pixel buffer will succeed.
    ///
    /// Note that \ref image::WritableImage adds a variant of this function that can grant
    /// write access to the pixel buffer (see \ref
    /// WritableImage::try_get_writable_buffer()).
    ///
    virtual bool try_get_buffer(image::BufferFormat& format, const void*& buffer) const = 0;

    struct TransferInfo;

    /// \brief Information on how pixels are transferred into and out of an image.
    ///
    /// See \ref TransferInfo.
    ///
    virtual auto get_transfer_info() const -> TransferInfo = 0;

    /// \brief Associated palette for indirect color images.
    ///
    /// If this function returns null, this image specifies color directly and therefore has
    /// no palette. Otherwise, this image specifies color indirectly, and the returned image
    /// is the associated palette.
    ///
    /// When an image acts as a palette, each pixel in the image becomes a color in the
    /// palette, and the order of colors in the palette is the row-major order of pixels in
    /// the image.
    ///
    /// A palette can have zero colors. This is allowed because pixels that refer outside
    /// the palette are allowed, and the color of such pixels resolve to the background
    /// color as specified for the reader / writer (see \ref
    /// image::Reader::set_background_color()).
    ///
    /// Even though it would be silly, a palette image is itself allowed to specify colors
    /// indirectly.
    ///
    virtual auto get_palette() const noexcept -> const Image* = 0;

    /// \brief Read pixels from specified area.
    ///
    /// This function reads pixels from the specified area and places them on the specified
    /// tray (\p tray). The specified area is `image::Box(pos, tray.size)`. This is a
    /// low-level function intended to primarily be invoked by a reader (see \ref
    /// image::Reader).
    ///
    /// Behavior is undefined if the specified area (`image::Box(pos, tray.size)`) extends
    /// beyond the boundaries of the image.
    ///
    /// The representation of pixels on the tray (\p tray) is as specified by the pixel
    /// transfer scheme (see the class-level documentation, \ref Image). The specified tray
    /// must refer to components of type `image::comp_type<R>`, where `R` is
    /// `get_transfer_info().comp_repr` for a direct color image, and `int8` (\ref
    /// image::CompRepr) for an indirect color image. This means that the implementation of
    /// `read()` can obtain a typed tray by invoking `tray.cast_to<T>()` where `T` is
    /// `image::comp_type<R>`.
    ///
    /// FIXME: Adjust paragraph above when support for varying index representation scheme is added                              
    ///
    /// For integer-based component representation schemes, and when the image uses direct
    /// color, the callee must ensure that all component values are within range (see \ref
    /// image::CompRepr). For schemes using floating point component values, values may be
    /// out of range, but generally should not be.
    ///
    virtual void read(image::Pos pos, const image::Tray<void>& tray) const = 0;

    virtual ~Image() noexcept = default;
};



/// \brief Description of how pixels are transferred into and out of an image.
///
/// An object of this type describes how pixels are transferred into and out of the
/// image. To this end, it specifies the resolved pixel transfer scheme (see the class-level
/// documentation for \ref Image) as well as the possibly reduced bit depth imposed by the
/// pixel storage scheme (\ref bit_depth).
///
/// \sa \ref get_transfer_info()
///
struct Image::TransferInfo {
    /// \brief Component representation scheme of resolved pixel transfer scheme.
    ///
    /// This is the component representation scheme of the resolved pixel transfer scheme of
    /// the described image (see the class-level documentation for \ref Image).
    ///
    image::CompRepr comp_repr;

    /// \brief Color space of resolved pixel transfer scheme.
    ///
    /// This is the color space of the resolved pixel transfer scheme of the described image
    /// (see the class-level documentation for \ref Image).
    ///
    const image::ColorSpace* color_space;

    /// \brief Whether alpha channel is present in resolved pixel transfer scheme.
    ///
    /// This flag specifies whether an alpha channel is present in the resolved pixel
    /// transfer scheme of the described image (see the class-level documentation for \ref
    /// Image).
    ///
    bool has_alpha;

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

    /// \brief Number of channels in resolved pixel transfer scheme.
    ///
    /// This function returns the number of channels in the resolved pixel transfer scheme
    /// of the described image. This is the number of color channels plus one if an alpha
    /// channel is present.
    ///
    int get_num_channels() const noexcept;
};








// Implementation


inline int Image::TransferInfo::get_num_channels() const noexcept
{
    return color_space->get_num_channels() + int(has_alpha);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_IMAGE_HPP
