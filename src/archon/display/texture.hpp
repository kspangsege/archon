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

#ifndef ARCHON_X_DISPLAY_X_TEXTURE_HPP
#define ARCHON_X_DISPLAY_X_TEXTURE_HPP

/// \file


#include <archon/image/image.hpp>


namespace archon::display {


/// \brief Image stored for fast transfer of pixels to associated window.
///
/// A texture is an image that is stored in a way that makes it fast and efficient to copy
/// pixels between it and its associated window. A texture is created by \ref
/// display::Window::new_texture(), and the texture's associated window is the window from
/// which it was created. Textures of any size can be created.
///
/// Pixels can be efficiently copied from the texture to its associated window using \ref
/// display::Window::put_texture().
///
/// The contents of a texture can be set using \ref put_image(). The initial contents of a
/// texture is undefined.
///
/// Once a texture has been created, the application must ensure that the destruction of the
/// associated window does not happen until after the destruction of the texture. Behavior
/// is undefined if the window is destroyed before the texture.
///
class Texture {
public:
    /// \brief Reset texture contents using image.
    ///
    /// This function sets the contents of the texture using the specified image. Any part
    /// of the specified image that extends beyond the boundary of the texture is
    /// unused. Any part of the texture that is not convered by the specified image is made
    /// black, or fully transparent black if the texture has an alpha channel.
    ///
    /// FIXME: Clarify when and how transparency is supported                         
    ///
    virtual void put_image(const image::Image&) = 0;

    virtual ~Texture() noexcept = default;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_TEXTURE_HPP
