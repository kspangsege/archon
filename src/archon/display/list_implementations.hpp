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

#ifndef ARCHON_X_DISPLAY_X_LIST_IMPLEMENTATIONS_HPP
#define ARCHON_X_DISPLAY_X_LIST_IMPLEMENTATIONS_HPP

/// \file


#include <locale>

#include <archon/core/file.hpp>
#include <archon/display/guarantees.hpp>


namespace archon::display {


/// \brief Produce textual rendition of list of display implementations.
///
/// This function writes a textual rendition of the list of display implementations to the
/// specified file (\p file), which can be \ref core::File::get_stdout(). The list is
/// formatted with the assumption that it will be displayed in a monospaced font, such as on
/// a text terminal. Both available and unavailable implementations will be
/// listed. Unavailable implementations will be marked as such.
///
/// ANSI escape sequences will be emitted only when
/// `core::terminal::should_enable_escape_sequences(file.is_terminal(), locale)` returns
/// true.
///
/// \sa \ref display::Implementation::Slot
/// \sa \ref display::Guarantees
/// \sa \ref core::terminal::should_enable_escape_sequences()
///
void list_implementations(core::File& file, const std::locale& locale, const display::Guarantees& guarantees);


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_LIST_IMPLEMENTATIONS_HPP
