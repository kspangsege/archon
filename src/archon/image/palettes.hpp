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

#ifndef ARCHON_X_IMAGE_X_PALETTES_HPP
#define ARCHON_X_IMAGE_X_PALETTES_HPP

/// \file


#include <archon/image/palette_image.hpp>


namespace archon::image {


/// \brief Black and white palette.
///
/// This function returns a reference to a palette that contains two colors, black and
/// white.
///
/// Palettes can be used with images that use an indexed pixel format. For example, a
/// buffered image (\ref image::BufferedImage) using a pixel format of type \ref
/// image::IndexedPixelFormat.
///
auto get_bw_palette() -> const image::PaletteImage_RGBA_8&;


/// \brief Grayscale palette with 4 tones.
///
/// This function returns a reference to a grayscale palette with 4 tones.
///
/// Palettes can be used with images that use an indexed pixel format. For example, a
/// buffered image (\ref image::BufferedImage) using a pixel format of type \ref
/// image::IndexedPixelFormat.
///
auto get_gray4_palette() -> const image::PaletteImage_RGBA_8&;


/// \brief Palette with 16 basic colors of CSS.
///
/// This function returns a palette containing the 16 basic colors of CSS (Cascading Style
/// Sheets).
///
/// Palettes can be used with images that use an indexed pixel format. For example, a
/// buffered image (\ref image::BufferedImage) using a pixel format of type \ref
/// image::IndexedPixelFormat.
///
/// \sa https://www.w3.org/TR/css-color-3/
///
auto get_css16_palette() -> const image::PaletteImage_RGBA_8&;


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PALETTES_HPP
