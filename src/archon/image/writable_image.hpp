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

#ifndef ARCHON_X_IMAGE_X_WRITABLE_IMAGE_HPP
#define ARCHON_X_IMAGE_X_WRITABLE_IMAGE_HPP

/// \file


#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Common base class for writable images.
///
/// This class serves as a common base class for image implementations that offer
/// writability. One example of such an implementations is \ref image::BufferedImage.
///
/// While the contents of an image can be modified through direct use of the API offered by
/// this class, most applications will instead want to modify the image via a writer (\ref
/// image::Writer).
///
class WritableImage
    : public image::Image {
public:
    /// \brief Copy pixels from specified image into this image.
    ///
    /// This function is a shorthand for creating a writer as if by `image::Writer(img)`
    /// where `img` is this image (\ref image::Writer), enabling blending as specified (\ref
    /// Writer::set_blending_enabled()), and then calling `put_image(pos, image)` on the
    /// writer (\ref Writer::put_image()).
    ///
    void put_image(image::Pos pos, const Image& image, bool blend = false);

    using Image::try_get_buffer;

    /// \brief Reveal writable pixel buffer and pixel storage format when possible.
    ///
    /// This function is a variant of the \ref Image::try_get_buffer() that can grant write
    /// access to the pixel buffer given a non-const image object. See \ref
    /// Image::try_get_buffer() for general documentation.
    ///
    /// Ordinarily, this function should succeed for a particular image if, and only if \ref
    /// Image::try_get_buffer() would succeed for that image, however, such a guarantee is
    /// not required of implementations.
    ///
    virtual bool try_get_writable_buffer(image::BufferFormat&, void*& buffer) = 0;

    /// \brief Write specified pixels to image at specified position.
    ///
    /// This function writes the specified pixels (\p tray) to this image at the specified
    /// position (\p pos). It is a low-level function intended to primarily be invoked by a
    /// writer (see \ref image::Writer).
    ///
    /// Behavior is undefined if the target area extends beyond the boundaries of the
    /// image. The target area is `image::Box(pos, tray.size)`.
    ///
    /// The representation of pixels on the tray (\p tray) is as specified by the pixel
    /// transfer scheme (see the class-level documentation, \ref Image). The specified tray
    /// must refer to components of type `image::comp_type<R>`, where `R` is
    /// `get_transfer_info().comp_repr` for a direct color image, and `int8` (\ref
    /// image::CompRepr) for an indirect color image. This means that the implementation of
    /// `write()` can obtain a typed tray by invoking `tray.cast_to<T>()` where `T` is
    /// `image::comp_type<R>`.
    ///
    /// FIXME: Adjust paragraph above when support for varying index representation scheme is added                              
    ///
    /// For integer-based representation schemes, and when the image uses direct color, the
    /// caller must ensure that all component values are within range (see \ref
    /// image::CompRepr). Behavior is undefined if a channel component is out of range. For
    /// schemes using floating point values, the values are allowed to be out of range, but
    /// generally should not be.
    ///
    virtual void write(image::Pos pos, const image::Tray<const void>& tray) = 0;

    /// \brief Fill specified area with specified color.
    ///
    /// This function fills the specified area with the specified color. It is a low-level
    /// function intended to primarily be invoked by a writer (see \ref image::Writer).
    ///
    /// Behavior is undefined if the specified area extends beyond the boundaries of the
    /// image.
    ///
    /// The representation of the specified color (\p color) is as specified by the pixel
    /// transfer scheme (see the class-level documentation, \ref Image). The specified color
    /// must refer to components of type `image::comp_type<R>`, where `R` is
    /// `get_transfer_info().comp_repr` for a direct color image, and `int8` (\ref
    /// image::CompRepr) for an indirect color image. This means that the implementation of
    /// `fill()` can obtain a typed pointer through `static_cast<const T*>(color)` where `T`
    /// is `image::comp_type<R>`.
    ///
    /// FIXME: Adjust paragraph above when support for varying index representation scheme is added                              
    ///
    /// For integer-based representation schemes, and when the image uses direct color, the
    /// caller must ensure that component values of the specified color are within range
    /// (see \ref image::CompRepr). Behavior is undefined if a channel component is out of
    /// range. For schemes using floating point values, the values are allowed to be out of
    /// range, but generally should not be.
    ///
    virtual void fill(const image::Box& area, const void* color) = 0;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_WRITABLE_IMAGE_HPP
