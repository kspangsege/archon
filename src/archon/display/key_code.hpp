// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_KEY_CODE_HPP
#define ARCHON_X_DISPLAY_X_KEY_CODE_HPP

/// \file


#include <cstdint>


namespace archon::display {


/// \brief Implementation dependent identification of keys on keyboard.
///
/// A key code is an integer value (\ref code) that identifies a particular key on a
/// keyboard. Values may differ between display implementations (\ref
/// display::Implementation) and even between display connections (\ref
/// display::Connection). One can map a key code to a well known key (\ref display::Key)
/// using \ref display::Connection::try_map_key_code_to_key().
///
/// A key codes identifies the logical unmodified function of a key, rather than the
/// position of the key on the keyboard or the symbol displayed on the key. For example, on
/// most keyboards featuring the Latin letters, the symbols shown on the keys are the
/// capital letters, but the unmodified function of those keys is to generate the lower case
/// letters. In these cases, the key code for the "A"-key will map to \ref
/// display::Key::lower_case_a, and not to \ref display::Key::upper_case_a.
///
/// Key events carry key codes. See \ref display::KeyEvent.
///
/// \sa \ref display::Key
/// \sa \ref display::KeyEvent
///
struct KeyCode {
    /// \brief Key code value type.
    ///
    /// This is the integer type used to represent key code values (\ref code).
    ///
    using code_type = std::uint_fast32_t;

    /// \brief Key code value.
    ///
    /// This is the value of the key code.
    ///
    code_type code;

    /// \brief Comparison operator.
    ///
    /// This operator compares the two key codes by value (\ref code). It allows for key
    /// codes to be sorted, and to be used as keys in maps.
    ///
    constexpr bool operator<(const KeyCode& other) const noexcept;
};








// Implementation


constexpr bool KeyCode::operator<(const KeyCode& other) const noexcept
{
    return code < other.code;
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_KEY_CODE_HPP
