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

#ifndef ARCHON_DISPLAY_PIXEL_BUFFER_HPP
#define ARCHON_DISPLAY_PIXEL_BUFFER_HPP

#include <string>

#include <archon/display/drawable.hpp>


namespace archon {
namespace Display {

/// An off-screen pixel buffer which can be used as a target for OpenGL
/// rendering.
///
/// On the X Window System this is a wrapper around a <tt>GLXPixmap</tt>.
///
/// New pixel buffers are created by calling Connection::new_pixel_buffer().
///
/// \sa Connection::new_pixmap
/// \sa http://www.opengl.org
/// \sa http://www.mesa3d.org
/// \sa http://www.x.org
class PixelBuffer: public virtual Drawable {
public:
    typedef core::SharedPtr<PixelBuffer> Ptr;
    typedef const Ptr& Arg;

    void save(std::string image_path);

    virtual Imaging::Image::Ref get_image() = 0;
};




// Implementation

inline void PixelBuffer::save(std::string image_path)
{
    get_image()->save(image_path);
}

} // namespace Display
} // namespace archon

#endif // ARCHON_DISPLAY_PIXEL_BUFFER_HPP
