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

#ifndef ARCHON_X_UTIL_X_AS_CSS_COLOR_HPP
#define ARCHON_X_UTIL_X_AS_CSS_COLOR_HPP

/// \file


#include <utility>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/util/color.hpp>
#include <archon/util/css_color.hpp>


namespace archon::util {


/// \brief Format and parse colors according to CSS syntax.
///
/// The specified color must be an object of type \ref util::Color. If a reference to a
/// non-const color object is specfied, the object returned by `as_css_color()` can be used
/// both for formatting (passed to stream output operator) and for parsing (passed to \ref
/// BasicValueParser::parse()).
///
/// Actual formatting and parsing is delegated to \ref util::CssColor. When formatting, if
/// the color is equal to a named color, the result is that name; otherwise, if the color is
/// fully opaque, the result is the 6 digit hex-form; otherwise the result is the 8 digit
/// hex-form. When parsing, all the forms allowed by CSS Level 3 are accepted.
///
template<class D> auto as_css_color(D&& color);








// Implementation


namespace impl {


template<class D> struct AsCssColor {
    D color; // May be a reference
};


template<class C, class T, class D>
inline auto operator<<(std::basic_ostream<C, T>& out, const impl::AsCssColor<D>& pod) -> std::basic_ostream<C, T>&
{
    util::CssColor::Hex hex {
        pod.color.red(),
        pod.color.green(),
        pod.color.blue(),
        pod.color.alpha(),
    };
    util::CssColor css_color;
    util::CssColor::Name name;
    if (util::CssColor::find_named_color_by_value(hex, name)) {
        css_color = name;
    }
    else {
        css_color = hex;
    }
    core::ArraySeededBuffer<char, 32> buffer;
    util::CssColor::FormatConfig config;
    config.disable_short_hex_form = true;
    std::string_view string = css_color.format(buffer, std::move(config)); // Throws
    using char_mapper_type = core::BasicCharMapper<C, T>;
    char_mapper_type char_mapper(out); // Throws
    typename char_mapper_type::template ArraySeededWidenBuffer<32> buffer_3;
    out << char_mapper.widen(string, buffer_3); // Throws
    return out;
}


template<class C, class T, class D>
inline bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsCssColor<D>& pod)
{
    using char_mapper_type = core::BasicCharMapper<C, T>;
    const char_mapper_type& char_mapper = src.get_char_mapper();
    typename char_mapper_type::template ArraySeededNarrowBuffer<32> buffer;
    char replacement = '\0'; // Invalid character from point of view of util::CssColor::parse()
    std::string_view string = char_mapper.narrow(src.string(), replacement, buffer); // Throws
    util::CssColor css_color;
    if (ARCHON_LIKELY(css_color.parse(string))) { // Throws
        CssColor::Hex hex = css_color.get_as_hex(); // Throws
        pod.color = util::Color(hex.r, hex.g, hex.b, hex.a);
        return true;
    }
    return false;
}


} // namespace impl


template<class D> inline auto as_css_color(D&& color)
{
    return impl::AsCssColor<D> { std::forward<D>(color) }; // Throws
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_AS_CSS_COLOR_HPP
