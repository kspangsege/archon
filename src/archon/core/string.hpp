/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_CORE_STRING_HPP
#define ARCHON_CORE_STRING_HPP

#include <cstdint>
#include <exception>
#include <memory>
#include <locale>
#include <utility>
#include <string>
#include <sstream>

#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>

namespace archon {
namespace core {

// @{
/// \brief Efficiently produce a string representation of the specified integer.
///
/// Efficent alternative to format_value() for values of integer type. This
/// function always formats integers according to the classic locale. The
/// specified locale is used only to control character encoding.
///
/// Note, the version that takes a buffer argument can be used to circumvent
/// dynamic allcoation for the string contents if the specified string has
/// enough reseved space.
///
/// These functions are thread-safe.
///
/// \sa format_value().

template<class C = char, class T = std::char_traits<C>, class A = std::allocator<C>, class V>
std::basic_string<C,T,A> format_int(V value, const std::locale& = std::locale::classic());

template<class C, class T, class A, class V>
void format_int(V value, std::basic_string<C,T,A>& buffer, const std::locale& = std::locale::classic());

// @}

template<class C, class T = std::char_traits<C>, class A = std::allocator<C>>
class BasicValueFormatter {
public:
    using string_type = std::basic_string<C,T,A>;

    BasicValueFormatter(const std::locale& = std::locale::classic());

    /// \brief Produce a string representation of the specified value.
    ///
    /// The returned string representation is the one that is produced when the
    /// value is writen to an instance of `std::basic_ostringstream<C,T,A>`, and
    /// that instance is imbued with the locale that was passed to the
    /// constructor of the formatter object.
    ///
    /// This function is thread-safe as long as each thread is using a different
    /// formatter object.
    template<class V> string_type format(const V& value);

private:
    std::basic_ostringstream<C,T,A> m_out;
};

using ValueFormatter = BasicValueFormatter<char>;


class ValueParseException;


template<class C, class T = std::char_traits<C>, class A = std::allocator<C>>
class BasicValueParser {
public:
    using string_type = std::basic_string<C,T,A>;

    BasicValueParser(const std::locale& = std::locale::classic());

    /// \brief Determine which value of type `V` is represented by the specified
    /// string.
    ///
    /// Same as parse(const string_type&, V&), but this one throws if parsing
    /// fails.
    ///
    /// \throw ValueParseException If the specified string does not represent a
    /// value of type `V`.
    template<class V> V parse(const string_type& string);

    /// \brief Determine which value of type `V` is represented by the specified
    /// string.
    ///
    /// This function parses the specified string under the assumption that it
    /// represents a value of type `V`. If parsing succeeds, the represented
    /// value is assigned to the specified value variable and true is returned,
    /// otherwise false is returned. Parsing happens as if by an instance of
    /// `std::basic_istringstream<C,T,A>` that is imbued with the locale that
    /// was passed to the constructor of the formatter object. All characters in
    /// the specified string (including white space) must participate in the
    /// string representation of the value, otherwise parsing fails.
    ///
    /// This function is thread-safe as long as each thread is using a different
    /// parser object.
    template<class V> bool parse(const string_type& string, V& value);

private:
    std::basic_istringstream<C,T,A> m_in;
};

using ValueParser = BasicValueParser<char>;


/// \brief Produce a string representation of the specified value.
///
/// Instantiates a formatter object of type `BasicValueFormatter<C,T,A>` for the
/// specified locale, and uses it to format the specified value.
///
/// Note: This function is relatively inefficient due to the need to construct a
/// formatter object (BasicValueFormatter). For maximum efficiency when
/// formatting multiple values, use a single formatter object to format them
/// all, or, if the values are integers, use format_int().
///
/// This function is thread-safe.
///
/// \sa format_int().
template<class C = char, class T = std::char_traits<C>, class A = std::allocator<C>, class V>
std::basic_string<C,T,A> format_value(const V& value, const std::locale& = std::locale::classic());

/// \brief Determine which value of type `V` is represented by the specified
/// string.
///
/// Instantiates a parser object of type `BasicValueFormatter<C,T,A>` for the
/// specified locale, and uses it to parse the specified string.
///
/// Note: This function is relatively inefficient due to the need to construct a
/// parser object (BasicValueParser). For maximum efficiency when parsing
/// multiple values, use a single parser object to parse them all.
///
/// This function is thread-safe.
///
/// \throw ValueParseException If the specified string does not represent a
/// value of type `V`.
template<class V, class C, class T, class A>
V parse_value(const std::basic_string<C,T,A>& string, const std::locale& = std::locale::classic());

template<class V, class C>
V parse_value(const C* c_str, const std::locale& = std::locale::classic());




// Implementation

namespace _impl {

template<class C> class FormatIntWiden {
public:
    FormatIntWiden(const std::locale& locale) noexcept:
        m_ctype{std::use_facet<std::ctype<C>>(locale)}
    {
    }
    C operator()(char c) const
    {
        return m_ctype.widen(c);
    }
private:
    const std::ctype<C>& m_ctype;
};

template<> class FormatIntWiden<char> {
public:
    FormatIntWiden(const std::locale&) noexcept
    {
    }
    char operator()(char c) const noexcept
    {
        // Assuming that the multi-byte encoding of characters in the
        // portable character set is invarinat across all locales.
        return c;
    }
};

template<class C, class T, class A> class StringSink {
public:
    StringSink(std::basic_string<C,T,A>& string):
        m_string{string}
    {
    }
    bool operator()(const C* begin, const C* end)
    {
        m_string.assign(begin, end); // Throws
        return true;
    }
private:
    std::basic_string<C,T,A>& m_string;
};

/// Returns null if the buffer has insufficient space, otherwise returns a
/// pointer that is one past the last produced character.
///
/// Maximum supported base/radix is 36
template<class C, int base, class V, class S>
bool format_int(V value, S sink, const std::locale& locale)
{
    static const char digits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z'
    };
    const int max_base = sizeof digits / sizeof digits[0];
    static_assert(base >= 2 && base <= max_base, "");
    const int max_digits = int_max_digits<V>(base);
    const int buffer_size = 1 + max_digits; // Make space for a minus sign
    C buffer[buffer_size];
    C* i = buffer + buffer_size;
    FormatIntWiden<C> widen{locale};
    if (value < 0) {
        // Negative numbers
        V val = value;
        do {
            V quot = V(val / base);
            int rem = int(quot * base - val);
            ARCHON_ASSERT(rem >= 0 && rem < max_base);
            *--i = widen(digits[rem]);
            val = quot;
        }
        while (val != 0);
        *--i = widen('-'); // Add the minus sign
    }
    else {
        // Positive numbers and zero
        V val = value;
        do {
            V quot = V(val / base);
            int rem = int(val - quot * base);
            ARCHON_ASSERT(rem >= 0 && rem < max_base);
            *--i = widen(digits[rem]);
            val = quot;
        }
        while (val != 0);
    }
    C* begin = i;
    C* end   = buffer + buffer_size;
    return sink(begin, end); // Throws
}

} // namespace _impl

template<class C, class T, class A, class V>
std::basic_string<C,T,A> format_int(V value, const std::locale& locale)
{
    std::basic_string<C,T,A> string;
    format_int(value, string, locale); // Throws
    return string;
}

template<class C, class T, class A, class V>
void format_int(V value, std::basic_string<C,T,A>& buffer, const std::locale& locale)
{
    const int base = 10;
    _impl::StringSink<C,T,A> sink{buffer};
    _impl::format_int<C, base>(value, sink, locale); // Throws
}

template<class C, class T, class A>
inline BasicValueFormatter<C,T,A>::BasicValueFormatter(const std::locale& locale)
{
    m_out.imbue(locale);
    m_out.exceptions(std::ios_base::failbit | std::ios_base::badbit);
}

template<class C, class T, class A>
template<class V> inline auto BasicValueFormatter<C,T,A>::format(const V& value) -> string_type
{
    m_out.str(string_type{});
    m_out.clear();
    m_out << value; // Throws
    return m_out.str(); // Throws
}

class ValueParseException: public std::exception {
public:
    const char* what() const noexcept override
    {
        return "Failed to parse value";
    }
};

template<class C, class T, class A>
inline BasicValueParser<C,T,A>::BasicValueParser(const std::locale& locale)
{
    m_in.imbue(locale);
    m_in.unsetf(std::ios_base::skipws);
}

template<class C, class T, class A>
template<class V> inline V BasicValueParser<C,T,A>::parse(const string_type& string)
{
    V value;
    if (!parse(string, value))
        throw ValueParseException{};
    return value;
}

template<class C, class T, class A>
template<class V> inline bool BasicValueParser<C,T,A>::parse(const string_type& string, V& value)
{
    m_in.str(string); // Throws
    m_in.clear();
    m_in >> value; // Throws
    return (m_in && m_in.get() == T::eof());
}

template<class C, class T, class A, class V>
inline std::basic_string<C,T,A> format_value(const V& value, const std::locale& locale)
{
    BasicValueFormatter<C,T,A> formatter{locale};
    return formatter.format(value); // Throws
}

template<class V, class C, class T, class A>
inline V parse_value(const std::basic_string<C,T,A>& string, const std::locale& locale)
{
    BasicValueParser<C,T,A> parser{locale};
    return parser.template parse<V>(string); // Throws
}

template<class V, class C> inline V parse_value(const C* c_str, const std::locale& locale)
{
    BasicValueParser<C> parser{locale};
    return parser.template parse<V>(c_str); // Throws
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_STRING_HPP
