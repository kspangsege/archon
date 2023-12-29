// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_STRING_FORMATTER_HPP
#define ARCHON_X_CORE_X_STRING_FORMATTER_HPP

/// \file


#include <cstddef>
#include <array>
#include <string_view>
#include <locale>
#include <ostream>

#include <archon/core/char_mapper.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/format.hpp>


namespace archon::core {


/// \brief Efficient formatting of parameterized strings.
///
/// This class offers an easy way to efficiently format multiple parameterized strings in
/// the style of what is done by \ref core::format(std::basic_ostream<C, T>&,
/// std::basic_string_view<C, T>, const P&...).
///
/// Memory allocated during one formatting operation will be reused during the next, so the
/// amortized memory allocation cost for a single formatting operation is zero.
///
template<class C, class T = std::char_traits<C>> class BasicStringFormatter
    : private core::BasicStringWidener<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    /// \brief Construct formatter without seed memory.
    ///
    /// Construct a string formatter without seed memory. This means that it will have to
    /// dynamically allocate memory as soon as a nonempty string is formatted.
    ///
    explicit BasicStringFormatter(const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// This constructor is a shorthand for `BasicStringFormatter(core::Span(seed_memory),
    /// locale).`
    ///
    template<std::size_t N> explicit BasicStringFormatter(C(& seed_memory)[N], const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// This constructor is a shorthand for `BasicStringFormatter(core::Span(seed_memory),
    /// locale).`
    ///
    template<std::size_t N> explicit BasicStringFormatter(std::array<C, N>& seed_memory,
                                                          const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// Construct a string formatter with access to statically, or previously allocated
    /// memory in the form of the specified seed memory. This allows the formatter to delay
    /// dynamic memory allocation until the point where a formatting operation produces a
    /// string that does not fit in the specified buffer size, which may be never.
    ///
    explicit BasicStringFormatter(core::Span<C> seed_memory, const std::locale& locale = {});

    /// \brief Format a null-terminated parameterized string.
    ///
    /// This function has the same effect as \ref format(string_view_type, const P&...)
    /// except that the string (\p c_str) is specified as a null-terminated string (probably
    /// a string literal).
    ///
    /// The characters of the specified string will be widened as if by `widen()` of an
    /// output stream with the same character type as this formatter (`std::basic_ostream<C,
    /// T>`), and imbued with the locale that was passed to the constructor. It is therefore
    /// only safe to use characters from the basic source character set in \p c_str. The
    /// intention is that \p c_str is always a string literal. This mimics the intention of
    /// `widen()`.
    ///
    template<class... P> auto format(const char* c_str, const P&... params) -> string_view_type;

    /// \brief Format a parameterized string.
    ///
    /// The specified parameterized string (\p string) is formatted as if it was passed to
    /// \ref core::format(std::basic_ostream<C, T>&, std::basic_string_view<C, T>, const
    /// P&...) along with the specified parameter values (\p params) for an output stream
    /// imbued with the locale that was passed to the constructor.
    ///
    /// The returned string view refers to memory that is owned by the formatter
    /// object. Therefore, the string will be clobbered by a subsequent formatting
    /// operation.
    ///
    template<class... P> auto format(string_view_type string, const P&... params) -> string_view_type;

private:
    core::BasicSeedMemoryOutputStream<C, T> m_out;
};


using StringFormatter     = BasicStringFormatter<char>;
using WideStringFormatter = BasicStringFormatter<wchar_t>;








// Implementation


template<class C, class T>
inline BasicStringFormatter<C, T>::BasicStringFormatter(const std::locale& locale)
    : BasicStringFormatter(core::Span<C>(), locale) // Throws
{
}


template<class C, class T>
template<std::size_t N>
inline BasicStringFormatter<C, T>::BasicStringFormatter(C(& seed_memory)[N], const std::locale& locale)
    : BasicStringFormatter(core::Span(seed_memory), locale) // Throws
{
}


template<class C, class T>
template<std::size_t N>
inline BasicStringFormatter<C, T>::BasicStringFormatter(std::array<C, N>& seed_memory, const std::locale& locale)
    : BasicStringFormatter(core::Span(seed_memory), locale) // Throws
{
}


template<class C, class T>
inline BasicStringFormatter<C, T>::BasicStringFormatter(core::Span<C> seed_memory, const std::locale& locale)
    : core::BasicStringWidener<C, T>(locale) // Throws
    , m_out(seed_memory) // Throws
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(locale); // Throws
}


template<class C, class T>
template<class... P>
inline auto BasicStringFormatter<C, T>::format(const char* c_str, const P&... params) -> string_view_type
{
    std::array<C, 256> seed_memory;
    string_view_type string = this->widen(c_str, seed_memory); // Throws
    return format(string, params...); // Throws
}


template<class C, class T>
template<class... P>
inline auto BasicStringFormatter<C, T>::format(string_view_type string, const P&... params) -> string_view_type
{
    m_out.full_clear();
    core::format(m_out, string, params...); // Throws
    return m_out.view();
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_FORMATTER_HPP
