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

#ifndef ARCHON_X_CORE_X_VALUE_FORMATTER_HPP
#define ARCHON_X_CORE_X_VALUE_FORMATTER_HPP

/// \file


#include <cstddef>
#include <array>
#include <string_view>
#include <locale>

#include <archon/core/seed_memory_output_stream.hpp>


namespace archon::core {


/// \brief Efficient formatting of values.
///
/// This class offers an easy way to efficiently format multiple values of varying type.
///
/// Memory allocated during one formatting operation will be reused during the next, so the
/// amortized memory allocation cost for a single formatting operation is zero.
///
template<class C, class T = std::char_traits<C>> class BasicValueFormatter {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    /// \brief Construct formatter without seed memory.
    ///
    /// Construct a value formatter without seed memory. This means that it will have to
    /// dynamically allocate memory as soon as a value is formatted.
    ///
    explicit BasicValueFormatter(const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// This constructor is a shorthand for `BasicValueFormatter(Span(seed_memory),
    /// locale)`.
    ///
    template<std::size_t N> explicit BasicValueFormatter(C(& seed_memory)[N], const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// This constructor is a shorthand for `BasicValueFormatter(Span(seed_memory),
    /// locale)`.
    ///
    template<std::size_t N> explicit BasicValueFormatter(std::array<C, N>& seed_memory,
                                                         const std::locale& locale = {});

    /// \brief Construct formatter with seed memory.
    ///
    /// Construct a value formatter with access to statically, or previously allocated
    /// memory in the form of the specified seed memory. This allows the formatter to delay
    /// dynamic memory allocation until the point where a formatting operation produces a
    /// string that does not fit in the specified buffer size, which may be never.
    ///
    explicit BasicValueFormatter(Span<C> seed_memory, const std::locale& locale = {});

    /// \brief Format a value.
    ///
    /// The specified value (\p value) is formatted as if by `out << value` where `out` is
    /// an output stream imbued with the locale that was passed to the formatter
    /// constructor.
    ///
    /// The returned string view refers to memory that is owned by the formatter
    /// object. Therefore, the string will be clobbered by a subsequent formatting
    /// operation.
    ///
    template<class V> auto format(const V& value) -> string_view_type;

private:
    BasicSeedMemoryOutputStream<C, T> m_out;
};


using ValueFormatter     = BasicValueFormatter<char>;
using WideValueFormatter = BasicValueFormatter<wchar_t>;








// Implementation


template<class C, class T>
inline BasicValueFormatter<C, T>::BasicValueFormatter(const std::locale& locale)
    : BasicValueFormatter(Span<C>(), locale) // Throws
{
}


template<class C, class T>
template<std::size_t N>
inline BasicValueFormatter<C, T>::BasicValueFormatter(C(& seed_memory)[N], const std::locale& locale)
    : BasicValueFormatter(Span(seed_memory), locale) // Throws
{
}


template<class C, class T>
template<std::size_t N>
inline BasicValueFormatter<C, T>::BasicValueFormatter(std::array<C, N>& seed_memory, const std::locale& locale)
    : BasicValueFormatter(Span(seed_memory), locale) // Throws
{
}


template<class C, class T>
inline BasicValueFormatter<C, T>::BasicValueFormatter(Span<C> seed_memory, const std::locale& locale)
    : m_out(seed_memory) // Throws
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(locale); // Throws
}


template<class C, class T>
template<class V> inline auto BasicValueFormatter<C, T>::format(const V& value) -> string_view_type
{
    m_out.full_clear();
    m_out << value; // Throws
    return m_out.view();
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_VALUE_FORMATTER_HPP
