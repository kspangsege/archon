// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_FONT_X_FREETYPE_IMPLEMENTATION_HPP
#define ARCHON_X_FONT_X_FREETYPE_IMPLEMENTATION_HPP

/// \file


#include <memory>
#include <locale>

#include <archon/core/filesystem.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/implementation.hpp>


namespace archon::font {


/// \brief Font implementation based on FreeType.
///
/// This function returns the font implementation that is based on the FreeType library
/// (https://freetype.org/). This implementation uses the identifier `freetype` (\ref
/// font::Implementation::get_ident()). If the Archon Font Library was built with support
/// for FreeType, the implementation will be available (\ref
/// font::Implementation::is_available()). Otherwise it will be unavailable.
///
/// \sa https://freetype.org/
/// \sa \ref font::new_freetype_loader_from_font_file()
///
auto get_freetype_implementation() noexcept -> const font::Implementation&;


/// \brief Construct FreeType font loader from single font file.
///
/// This function constructs a FreeType font loader that offers the font faces provided by
/// the specified font file (\p file), and only those font faces. This may be useful for
/// testing purposes where a test font can be passed.
///
/// FIXME: Clarify which face becomes the default font face for the loader
///
/// FIXME: Clarify whether this function is also needed when regenerating the fallback font
///
/// \sa \ref font::get_freetype_implementation(), \ref font::Implementation::new_loader()
///
auto new_freetype_loader_from_font_file(core::FilesystemPathRef file, const std::locale& locale,
                                        const font::Loader::Config& config) -> std::unique_ptr<font::Loader>;


} // namespace archon::font

#endif // ARCHON_X_FONT_X_FREETYPE_IMPLEMENTATION_HPP
