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

#ifndef ARCHON_X_RENDER_X_LOAD_TEXTURE_HPP
#define ARCHON_X_RENDER_X_LOAD_TEXTURE_HPP

/// \file


#include <archon/image/image.hpp>
#include <archon/display/opengl.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

namespace archon::render {


/// \brief Load and configure OpenGL texture.
///
/// This function loads the specified image into the currently bound OpenGL texture object
/// and also sets the filtering parameters for the texture as specified. Additionally, if
/// mipmapping is not disabled, the mipmap levels will be generated.
///
/// \param no_interp Turn off linear interpolation between texture pixels during
/// magnification. In this case the nearest pixel is used.
///
/// \param no_interp Turn off mipmapping for this texture during minification. In this case
/// simple linear interpolation is used.
///
/// \sa \ref render::load_texture()
///
void load_and_configure_texture(const image::Image& image, bool no_interp = false, bool no_mipmap = false);


/// \brief Load OpenGL texture.
///
/// This function loads the specified image into the currently bound OpenGL texture object.
///
/// \sa \ref render::load_and_configure_texture()
/// \sa \ref render::load_texture_layer()
///
void load_texture(const image::Image& image);


/// \brief Load one layer of OpenGL texture array.
///
/// This function loads the specified image (\p image) into the specified layer (\p layer)
/// of the currently bound OpenGL texture array.
///
/// \param texture_has_alpha Must be true if, and only if the internal storage format of the
/// texture is one that includes an alpha channel.
///
/// \sa \ref render::load_texture()
///
void load_texture_layer(const image::Image& image, int layer, bool texture_has_alpha);


} // namespace archon::render

#endif // ARCHON_DISPLAY_HAVE_OPENGL

#endif // ARCHON_X_RENDER_X_LOAD_TEXTURE_HPP
