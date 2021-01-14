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

#ifndef ARCHON_X_CLI_X_DETAIL_X_VALUE_PARSER_HPP
#define ARCHON_X_CLI_X_DETAIL_X_VALUE_PARSER_HPP

/// \file


#include <type_traits>
#include <string_view>
#include <locale>

#include <archon/base/value_parser.hpp>


namespace archon::cli::detail {


template<class C, class T> class ValueParser {
public:
    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;

    ValueParser(const std::locale&);

    template<class U> bool parse(string_view_type val, U& var);
    template<class U> bool parse(string_view_type val, std::optional<U>& var);

private:
    base::BasicValueParser<C, T> m_parser;
};








// Implementation


template<class C, class T> ValueParser<C, T>::ValueParser(const std::locale& locale) :
    m_parser(locale) // Throws
{
}


template<class C, class T>
template<class U> bool ValueParser<C, T>::parse(string_view_type val, U& var)
{
    static_assert(!std::is_pointer_v<U>);
    return m_parser.parse(val, var); // Throws
}


template<class C, class T>
template<class U> bool ValueParser<C, T>::parse(string_view_type val, std::optional<U>& var)
{
    var.emplace(); // Throws
    return m_parser.parse(val, *var); // Throws
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_VALUE_PARSER_HPP
