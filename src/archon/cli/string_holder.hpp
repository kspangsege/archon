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

#ifndef ARCHON_X_CLI_X_STRING_HOLDER_HPP
#define ARCHON_X_CLI_X_STRING_HOLDER_HPP

/// \file


#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>


namespace archon::cli {


template<class C, class T = std::char_traits<C>> class BasicStringHolder {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    auto add(string_view_type) -> string_view_type;
    auto add_encoded(std::string_view) -> std::string_view;

private:
    std::vector<std::unique_ptr<char_type[]>> m_strings;
    std::vector<std::unique_ptr<char[]>> m_encoded_strings;
};


using StringHolder     = BasicStringHolder<char>;
using WideStringHolder = BasicStringHolder<wchar_t>;








// Implementation


template<class C, class T>
auto BasicStringHolder<C, T>::add(string_view_type string) -> string_view_type
{
    std::size_t size = string.size();
    std::unique_ptr<char_type[]> string_2 = std::make_unique<char_type[]>(size); // Throws
    std::copy_n(string.data(), string.size(), string_2.get());
    string_view_type string_3 { string_2.get(), size };
    m_strings.push_back(std::move(string_2));
    return string_3;
}


template<class C, class T>
auto BasicStringHolder<C, T>::add_encoded(std::string_view string) -> std::string_view
{
    std::size_t size = string.size();
    std::unique_ptr<char[]> string_2 = std::make_unique<char[]>(size); // Throws
    std::copy_n(string.data(), string.size(), string_2.get());
    std::string_view string_3 { string_2.get(), size };
    m_encoded_strings.push_back(std::move(string_2));
    return string_3;
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_STRING_HOLDER_HPP
