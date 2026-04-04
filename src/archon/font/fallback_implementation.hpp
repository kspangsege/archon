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

#ifndef ARCHON_X_FONT_X_FALLBACK_IMPLEMENTATION_HPP
#define ARCHON_X_FONT_X_FALLBACK_IMPLEMENTATION_HPP

/// \file


#include <memory>
#include <string_view>
#include <vector>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/implementation.hpp>


namespace archon::font {


/// \brief Fallback font implementation.
///
/// This function returns the fallback font implementation using the identifier `fallback`
/// (\ref font::implementation::get_ident()). The fallback font implementation is always
/// available (\ref font::implementation::is_available()).
///
/// \sa \ref font::new_fallback_loader()
///
auto get_fallback_implementation() noexcept -> const font::implementation&;


/// \brief Construct fallback font loader.
///
/// This function constructs a fallback font loader. It has the same effect as calling \ref
/// font::implementation::new_loader() on the implementation object returned by \ref
/// font::get_fallback_implementation().
///
/// This function expects to find the two fallback font files, `fallback-font.txt` and
/// `fallback-font.png`, in the specified resource directory (\p resource_dir). The
/// specified resource directory is not used for anything else by this function.
///
/// \sa \ref font::get_fallback_implementation(), \ref font::implementation::new_loader()
/// \sa \ref font::regen_fallback_font()
///
auto new_fallback_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                         const font::loader::config& config) -> std::unique_ptr<font::loader>;


/// \brief Recreate the fallback font.
///
/// This function recreates the fallback font. The two generated files, `fallback-font.txt`
/// and `fallback-font.png`, are placed in the specified resource directory. The specified
/// resource directory is not used in any other way by this function. The file names can be
/// modified by a qualifier (\p file_name_qual). It will be injected into the names
/// immediately before the dot (`.`). If the specified qualifier is the empty string, the
/// names are generated exactly as they are expected by the fallback implementation.
///
/// If a logger is specified through the configuration object, the locale associated with
/// that logger must be compatible with the locale that is passed directly to this
/// function. The important thing is that the character encodings agree (`std::codecvt`
/// facet).
///
/// \note The caller must assume that rendering parameters have been modified in the font
/// face object upon return.
///
/// \sa \ref font::try_get_fallback_font_params()
/// \sa \ref font::new_fallback_loader()
///
void regen_fallback_font(font::face& face, core::Span<const font::code_point_range> ranges,
                         core::FilesystemPathRef resource_dir, std::string_view file_name_qual,
                         const std::locale& locale, const font::loader::config& config);


/// \brief Fallback font face selection parameters.
///
/// An object of this type specifies the parameters needed to select the font face that was
/// last used as a basis for creating the fallback font.
///
/// \sa \ref font::try_get_fallback_font_params()
///
struct fallback_font_params {
    std::string_view family_name;
    std::string_view style_name;
    bool is_bold;
    bool is_italic;
    bool is_monospace;
};


/// \brief Extract basic fallback font parameters.
///
/// This function attempts to extract the basic font parameters used when the fallback font
/// was last recreated. This succeeds precisely when the `fallback-font.txt` file exists and
/// can be successfully loaded and parsed.
///
/// \sa \ref font::regen_fallback_font()
///
bool try_get_fallback_font_params(core::FilesystemPathRef resource_dir, const std::locale& locale,
                                  std::vector<font::code_point_range>& ranges, font::fallback_font_params& params,
                                  std::unique_ptr<char[]>& string_owner, font::size& size);


} // namespace archon::font

#endif // ARCHON_X_FONT_X_FALLBACK_IMPLEMENTATION_HPP
