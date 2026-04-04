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
/// font::implementation::get_ident()). If the Archon Font Library was built with support
/// for FreeType, the implementation will be available (\ref
/// font::implementation::is_available()). Otherwise it will be unavailable.
///
/// In order for colored glyphs to be supported, the FreeType version must be at least
/// 2.13.1. If an older version of FreeType is used, \ref font::face::has_color() will
/// return false for all font faces.
///
/// \sa https://freetype.org/
/// \sa \ref font::new_freetype_loader_from_font_file()
///
auto get_freetype_implementation() noexcept -> const font::implementation&;


/// \brief FreeType-specific loader configuration.
///
/// Extra loader configuration that is specific to the FreeType-based implementation. The
/// application must construct an object of this type and register it with a main
/// configuration object. See \ref font::loader::config::sub.
///
struct freetype_subconfig : font::loader::config::subconfig {
    /// \brief Use auto-hinter on TrueType fonts.
    ///
    /// When grid-fitting is turned on during glyph loading (\ref
    /// font::face::try_load_glyph()), use FreeType's auto-hinter on TrueType fonts instead
    /// of the native TrueType hinting instructions.
    ///
    bool force_autohint = false;
};


/// \brief Construct FreeType font loader from single font file.
///
/// This function constructs a FreeType font loader that offers the font faces provided by
/// the specified font file (\p file), and only those font faces. This may be useful for
/// testing purposes where a test font can be passed.
///
/// The default font face in the returned loader is the first one offered by the specified
/// font file.
///
/// \sa \ref font::get_freetype_implementation(), \ref font::implementation::new_loader()
///
auto new_freetype_loader_from_font_file(core::FilesystemPathRef file, const std::locale& locale,
                                        const font::loader::config& config) -> std::unique_ptr<font::loader>;


} // namespace archon::font

#endif // ARCHON_X_FONT_X_FREETYPE_IMPLEMENTATION_HPP
