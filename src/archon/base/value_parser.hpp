// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__VALUE_PARSER_HPP
#define ARCHON__BASE__VALUE_PARSER_HPP

#include <cstddef>
#include <array>
#include <string_view>
#include <locale>

#include <archon/base/memory_input_stream.hpp>


namespace archon::base {


/// \brief Robust stream-based value parsing.
///
/// This class offers an easy and robust way to obtain values from their
/// respective string representations. Parsing is performed by way of stream
/// input.
///
template<class C, class T = std::char_traits<C>> class BasicValueParser {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    /// \brief Construct parser.
    ///
    /// Construct a parser with the specified locale. The input stream, that is
    /// used internally for parsing, will be imbued with this locale.
    ///
    explicit BasicValueParser(const std::locale& locale = {});

    /// \{
    ///
    /// \brief Parse a value.
    ///
    /// The first function overload (second argument is not a string and not a
    /// string view) attempts to parse the specified string as a value of the
    /// type of the specified \p value argument. Parsing happens by way of
    /// stream input (`in >> value` where `in` is of type `std::basic_istream<C,
    /// T>`). When the type of the parsed value is some unsigned integer type,
    /// however, negative values will be rejected (strings with a leading dash
    /// (`-`)). The plain stream input operator for unsigned integer types is
    /// required by the C++ standard to accept negative values.
    ///
    /// The second and third function overloads (second argument is a string
    /// view and a string respectively) simply assign \p string to \p value. In
    /// the case of a string view, there referenced memory in \p value is the
    /// same as is referenced by \p string.
    ///
    /// If parsing succeeds, this function assigns the parsed value to the \p
    /// value argument, and returns `true`. If parsing fails, this function
    /// returns `false` and leaves \p value "untouched". When the second
    /// argument is a string or a string view, parsing always succeeds.
    ///
    template<class V> bool parse(string_view_type string, V& value);
    bool parse(string_view_type string, string_view_type& value);
    template<class A> bool parse(string_view_type string, std::basic_string<C, T, A>& value);
    /// \}

private:
    BasicMemoryInputStream<C, T> m_in;
    C m_dash;
};


using ValueParser     = BasicValueParser<char>;
using WideValueParser = BasicValueParser<wchar_t>;








// Implementation


template<class C, class T>
inline BasicValueParser<C, T>::BasicValueParser(const std::locale& locale)
{
    m_in.imbue(locale); // Throws
    m_in.unsetf(std::ios_base::skipws);
    m_dash = m_in.widen('-'); // Throws
}


template<class C, class T>
template<class V> inline bool BasicValueParser<C, T>::parse(string_view_type string, V& value)
{
    bool is_unsigned_int = std::is_unsigned_v<std::remove_cv_t<V>>;
    if (ARCHON_LIKELY(!is_unsigned_int || string.empty() || string.front() != m_dash)) {
        m_in.reset(string); // Throws
        V value_2 {}; // Throws
        m_in >> value_2; // Throws
        if (ARCHON_LIKELY(m_in && m_in.peek() == T::eof())) { // Throws
            value = std::move(value_2); // Throws
            return true;
        }
    }
    return false;
}


template<class C, class T>
inline bool BasicValueParser<C, T>::parse(string_view_type string, string_view_type& value)
{
    value = string; // Throws
    return true;
}


template<class C, class T> template<class A>
inline bool BasicValueParser<C, T>::parse(string_view_type string,
                                          std::basic_string<C, T, A>& value)
{
    value = string; // Throws
    return true;
}


} // namespace archon::base

#endif // ARCHON__BASE__VALUE_PARSER_HPP
