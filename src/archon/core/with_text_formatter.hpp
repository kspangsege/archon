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

#ifndef ARCHON_X_CORE_X_WITH_TEXT_FORMATTER_HPP
#define ARCHON_X_CORE_X_WITH_TEXT_FORMATTER_HPP

/// \file


#include <locale>
#include <ios>

#include <archon/core/file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/core/terminal.hpp>
#include <archon/core/text_formatter.hpp>


namespace archon::core {


/// \brief Format text using supplied text formatter.
///
/// This function constructs a text formatter and passes it to the the specified function
/// (\p func). The specified function must be such that `func(formatter)` is valid when
/// `formatter` is an object of type \ref core::TextFormatter.
///
/// The formatter must not be in compilation mode, and output must not be held back when \p
/// func returns.
///
/// ANSI escape sequences will be emitted only when
/// `core::terminal::should_enable_escape_sequences(file.is_terminal(), locale)` returns
/// true.
///
/// \sa \ref core::TextFormatter
/// \sa \ref core::terminal::should_enable_escape_sequences()
///
template<class F> void with_text_formatter(core::File& file, const std::locale& locale, F&& func);








// Implementation


template<class F> void with_text_formatter(core::File& file, const std::locale& locale, F&& func)
{
    core::TextFileStream out(&file); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    core::TextFormatter::Config config;
    config.enable_ansi_escape_sequences =
        core::terminal::should_enable_escape_sequences(file.is_terminal(), locale); // Throws
    core::TextFormatter formatter(out, config); // Throws
    func(formatter); // Throws
    formatter.finalize(); // Throws
    out.flush(); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_WITH_TEXT_FORMATTER_HPP
