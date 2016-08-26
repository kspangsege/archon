/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_DISPLAY_DRAWABLE_HPP
#define ARCHON_DISPLAY_DRAWABLE_HPP

#include <archon/core/shared_ptr.hpp>
#include <archon/image/image.hpp>
#include <archon/display/geometry.hpp>


namespace archon {
namespace display {

/// Something on which you can draw.
///
/// \sa Window
/// \sa PixelBuffer
class Drawable {
public:
    typedef core::SharedPtr<Drawable> Ptr;
    typedef const Ptr& Arg;

    /// Write the pixels of the specified image to the specified position of
    /// this drawable.
    ///
    /// This is a shorthand for calling the 4-argument version of put_image()
    /// with the position of \a clip set to \a position and the size of \a area
    /// set to the size of \a image.
    void put_image(image::Image::ConstRefArg image, Point position = Point(),
                   util::PackedTRGB background = util::PackedTRGB());

    /// Fill the specified area of this drawable with pixels from the specified
    /// source image. If parts of the specified area fall outside the image
    /// boundary, those parts will be filled with the specified background
    /// color.
    ///
    /// \param image The source image.
    ///
    /// \param clip The area of this drawable to be filled. \a clip.x, \a clip.y
    /// specifies the rightwards, downwards displacement of the top-left corner
    /// of this area relative to the top-left corner of the drawable.
    ///
    /// \param position Specifies the rightwards (x-coordinate),
    /// downwards (y-coordinate) displacement of the top-left corner of the
    /// source image relative to the top-left corner of the drawable. Negative
    /// coordinates are allowed.
    ///
    /// \param background The background color. This color is used to fill areas
    /// that fall outsise the image boundary, and as background in transparent
    /// areas of the image.
    virtual void put_image(image::Image::ConstRefArg image, Box clip, Point position = Point(),
                           util::PackedTRGB background = util::PackedTRGB()) = 0;

    /// Get the index of the screen to which this drawable is tied.
    ///
    /// \note This method is thread-safe.
    virtual int get_screen() const = 0;

    /// Get the index of the visual that describes the buffers that are
    /// available to this window.
    ///
    /// \note This method is thread-safe.
    virtual int get_visual() const = 0;

    virtual ~Drawable() noexcept {}
};




// Implementation

inline void Drawable::put_image(image::Image::ConstRefArg image, Point position,
                                util::PackedTRGB background)
{
    Box clip;
    clip.x = position.x;
    clip.y = position.y;
    clip.width  = image->get_width();
    clip.height = image->get_height();
    put_image(image, clip, position, background); // Throws
}

} // namespace display
} // namespace archon

#endif // ARCHON_DISPLAY_DRAWABLE_HPP
