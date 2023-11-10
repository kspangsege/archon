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

#ifndef ARCHON_X_DISPLAY_X_AS_KEY_NAME_HPP
#define ARCHON_X_DISPLAY_X_AS_KEY_NAME_HPP

/// \file


#include <array>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/char_mapper.hpp>
#include <archon/core/format.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/implementation.hpp>


namespace archon::display {


/// \brief Format key code as key name when key name is available.
///
/// This function returns an object that, if written to an output stream, writes the name of
/// the specified key to that output stream. If the name is not available, a string on the
/// form `Key(<code>)` will be written in place of the actual key name.
///
/// The name of the key is determined by \ref display::Implementation::try_get_key_name().
///
auto as_key_name(display::KeyCode code, const display::Implementation& impl) noexcept;








// Implementation


namespace impl {


struct AsKeyName {
    display::KeyCode key_code;
    const display::Implementation& impl;
};


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsKeyName& pod) -> std::basic_ostream<C, T>&
{
    std::string_view name;
    if (ARCHON_LIKELY(pod.impl.try_get_key_name(pod.key_code, name))) { // Throws
        std::locale locale = out.getloc(); // Throws
        std::array<C, 64> seed_memory;
        core::BasicStringWidener widener(locale, seed_memory); // Throws
        return out << widener.widen(name); // Throws
    }
    return out << core::formatted("Key(%s)", core::as_int(pod.key_code.code)); // Throws
}


} // namespace impl


auto as_key_name(display::KeyCode code, const display::Implementation& impl) noexcept
{
    return impl::AsKeyName { code, impl };
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_AS_KEY_NAME_HPP
