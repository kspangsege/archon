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

#ifndef ARCHON_X_IMAGE_X_FILE_FORMAT_PNG_HPP
#define ARCHON_X_IMAGE_X_FILE_FORMAT_PNG_HPP

/// \file


#include <archon/image/file_format.hpp>


namespace archon::image {


/// \brief PNG-specific load parameters.
///
/// These are the parameters that are specific to the PNG file format, and can be used to
/// control the loading of PNG images.
///
struct PNGLoadConfig : image::FileFormat::SpecialLoadConfig {
};


/// \brief PNG-spec save parameters.
///
/// These are the parameters that are specific to the PNG file format, and can be used to
/// control the saving of PNG images.
///
struct PNGSaveConfig : image::FileFormat::SpecialSaveConfig {
    /// \brief Turn on Adam7 interlacing.
    ///
    /// If set to `true`, images will be saved in interlaced form using the Adam7
    /// interlacing scheme.
    ///
    bool use_adam7_interlacing = false;
};


/// \brief Interface to PNG file format via libpng.
///
/// If the Archon Image Library was built with support for PNG (Portable Network Graphics)
/// via `libpng`, this function returns a pointer to the file format object representing
/// `libpng`. Otherwise, this function returns null.
///
/// \sa http://www.libpng.org/
///
/// See \ref image::PNGLoadConfig and \ref image::PNGSaveConfig for parameters that can be
/// used to control the loading and saving of PNG images, and that are specific to the PNG
/// file format.
///
auto get_file_format_png() noexcept -> const image::FileFormat*;


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_FILE_FORMAT_PNG_HPP
