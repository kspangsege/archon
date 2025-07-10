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
#include <archon/image/image_fwd.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/buffer_format.hpp>


namespace archon::image {


/// \brief Common base class for all image types.
///
/// This class serves as a common base class for all image implementations and offers
/// facilities for reading of pixels. Image implementations that offer facilities for
/// writing of pixels should inherit from \ref image::WritableImage, which, in turn,
/// inherits from this class. Examples of non-writable images are \ref image::ComputedImage
/// and \ref image::PaletteImage.
///
/// While the contents of an image can be read through direct use of the API offered by this
/// class (\ref get_transfer_info() and \ref read()), most applications will instead want to
/// access the image data using a reader or a writer (\ref image::Reader, \ref
/// image::Writer).
///
/// \sa \ref image::Reader, \ref image::Writer
/// \sa \ref image::load(), \ref image::save()
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

    /// \brief Information on how pixels are transferred into and out of an image.
    ///
    /// This functions returns a description of how to transfer pixels into and out of an
    /// image using \ref read() and \ref image::WritableImage::write(). See \ref
    /// image::TransferInfo for details.
    ///
    /// The caller may assume that the exact description returned by one invocation will
    /// also be returned by all future invocations for the same image object.
    ///
    virtual auto get_transfer_info() const -> image::TransferInfo = 0;

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
    /// transfer scheme (see \ref image::TransferInfo). The specified tray must refer to
    /// components of type `image::comp_type<R>`, where `R` is
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


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_IMAGE_HPP
