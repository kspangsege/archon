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

#ifndef ARCHON_X_IMAGE_X_LIST_FILE_FORMATS_HPP
#define ARCHON_X_IMAGE_X_LIST_FILE_FORMATS_HPP

/// \file


#include <locale>

#include <archon/core/file.hpp>
#include <archon/image/file_format_registry.hpp>


namespace archon::image {


/// \brief Produce textual rendition of list of file formats.
///
/// This function writes a textual rendition of the list of file formats in the specified
/// registry (\p registry) to the specified file (\p file), which can be \ref
/// core::File::get_stdout(). The list is formatted with the assumption that it will be
/// displayed in a monospaced font, such as on a text terminal.
///
/// ANSI escape sequences will be emitted only when
/// `core::terminal::should_enable_escape_sequences(file.is_terminal(), locale)` returns
/// true.
///
/// \sa \ref image::FileFormatRegistry
/// \sa \ref core::terminal::should_enable_escape_sequences()
///
void list_file_formats(core::File& file, const std::locale& locale, const image::FileFormatRegistry& registry);


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_LIST_FILE_FORMATS_HPP
