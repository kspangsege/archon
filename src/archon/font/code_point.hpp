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

#ifndef ARCHON_X_FONT_X_CODE_POINT_HPP
#define ARCHON_X_FONT_X_CODE_POINT_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>


namespace archon::font {


class CodePoint {
public:
    using char_type   = wchar_t;
    using traits_type = std::char_traits<char_type>;
    using int_type    = traits_type::int_type;

    constexpr auto to_int() const noexcept -> int_type;
    constexpr auto to_char() const noexcept -> char_type;
    constexpr bool try_from_int(int_type) noexcept;
    constexpr bool try_from_char(char_type) noexcept;

private:
    int_type m_val = 0;
};


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, font::CodePoint) -> std::basic_ostream<C, T>&;

template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>&, font::CodePoint&);



class CodePointRange {
public:
    constexpr CodePointRange(font::CodePoint = {}) noexcept;
    constexpr CodePointRange(font::CodePoint first, font::CodePoint last) noexcept;

    constexpr auto first() const noexcept -> font::CodePoint;
    constexpr auto last() const noexcept  -> font::CodePoint;

private:
    font::CodePoint m_first, m_last;
};


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, const font::CodePointRange&) ->
    std::basic_ostream<C, T>&;

template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>&, font::CodePointRange&);








// Implementation


constexpr auto CodePoint::to_int() const noexcept -> int_type
{
    return m_val;
}


constexpr auto CodePoint::to_char() const noexcept -> char_type
{
    return traits_type::to_char_type(m_val);
}


constexpr bool CodePoint::try_from_int(int_type val) noexcept
{
    char_type ch = traits_type::to_char_type(val);
    bool valid = (!core::is_negative(val) && traits_type::to_int_type(ch) == val && val != traits_type::eof());
    if (ARCHON_LIKELY(valid)) {
        m_val = val;
        return true;
    }
    return false;
}


constexpr bool CodePoint::try_from_char(char_type ch) noexcept
{
    int_type val = traits_type::to_int_type(ch);
    bool valid = (val != traits_type::eof());
    if (ARCHON_LIKELY(valid)) {
        m_val = val;
        return true;
    }
    return false;
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, font::CodePoint cp) -> std::basic_ostream<C, T>&
{
    return out << core::as_int(cp.to_int()); // Throws
}


template<class C, class T> inline bool parse_value(core::BasicValueParserSource<C, T>& src, font::CodePoint& cp)
{
    using int_type = font::CodePoint::int_type;
    int_type val = {};
    if (ARCHON_LIKELY(src.delegate(core::as_int(val))))
        return cp.try_from_int(val);
    return false;
}


constexpr CodePointRange::CodePointRange(font::CodePoint cp) noexcept
    : m_first(cp)
    , m_last(cp)
{
}


constexpr CodePointRange::CodePointRange(font::CodePoint first, font::CodePoint last) noexcept
    : m_first(first)
    , m_last(last)
{
}


constexpr auto CodePointRange::first() const noexcept -> font::CodePoint
{
    return m_first;
}


constexpr auto CodePointRange::last() const noexcept -> font::CodePoint
{
    return m_last;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const font::CodePointRange& range) -> std::basic_ostream<C, T>&
{
    if (range.last().to_int() > range.first().to_int())
        return out << core::formatted("%s-%s", range.first(), range.last()); // Throws
    return out << range.first(); // Throws
}


template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>& src, font::CodePointRange& range)
{
    using string_view_type = std::basic_string_view<C, T>;
    string_view_type str = src.string();
    C dash = src.widen('-'); // Throws
    std::size_t i = str.find(dash);
    if (i != std::size_t(-1)) {
        font::CodePoint first, last;
        if (ARCHON_LIKELY(src.delegate(str.substr(0, i), first) &&
                          src.delegate(str.substr(i + 1), last))) { // Throws
            if (ARCHON_LIKELY(first.to_int() <= last.to_int())) {
                range = { first, last };
                return true;
            }
        }
        return false;
    }
    font::CodePoint cp;
    if (ARCHON_LIKELY(src.delegate(str, cp))) { // Throws
        range = { cp };
        return true;
    }
    return false;
}


} // namespace archon::font

#endif // ARCHON_X_FONT_X_CODE_POINT_HPP
