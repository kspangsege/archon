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

#ifndef ARCHON_X_FONT_X_LOADER_FALLBACK_HPP
#define ARCHON_X_FONT_X_LOADER_FALLBACK_HPP

/// \file


#include <archon/core/span.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>


namespace archon::font {


auto loader_fallback_impl() noexcept -> const font::Loader::Implementation&;


/// \brief Create, or recreate the falback font.
///
/// If no code point ranges are specified, an attempt will be made to reuse the code point
/// ranges from the old fallback font. If this fails, the code point ranges will default to
/// 0 -> 127.
///
/// If a logger is specified through the configuration object, the locale associated with
/// that logger must be compatible with the locale that is passed directly to this
/// function. The important thing is that the character encodings agree (`std::codecvt`
/// facet).
///
void regen_fallback_font(font::Face&, bool try_keep_orig_font_size, core::Span<const font::CodePointRange>,
                         core::FilesystemPathRef resource_dir, const std::locale&, font::Loader::Config = {});


} // namespace archon::font

#endif // ARCHON_X_FONT_X_LOADER_FALLBACK_HPP
