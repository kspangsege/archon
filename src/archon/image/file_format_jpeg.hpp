// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_IMAGE_X_FILE_FORMAT_JPEG_HPP
#define ARCHON_X_IMAGE_X_FILE_FORMAT_JPEG_HPP

/// \file


#include <archon/image/file_format.hpp>


namespace archon::image {


/// \brief Interface to JPEG file format via libjpeg.
///
/// If the Archon Image Library was built with support for JPEG (Joint Photographic Experts
/// Group) via `libjpeg`, this function returns a pointer to the file format object
/// representing `libjpeg`. Otherwise, this function returns null.
///
/// \sa http://ijg.org/
/// \sa https://jpegclub.org/
///
auto get_file_format_jpeg() noexcept -> const image::FileFormat*;


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_FILE_FORMAT_JPEG_HPP
