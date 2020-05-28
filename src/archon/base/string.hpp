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

#ifndef ARCHON__BASE__STRING_HPP
#define ARCHON__BASE__STRING_HPP

#include <cstddef>
#include <string_view>
#include <string>

#include <archon/base/features.h>
#include <archon/base/integer.hpp>


namespace archon::base {


/// \brief Concatenate strings using one allocation.
///
/// This function constructs the concatenation of the two specified strings
/// using at most one dynamic memory allocation.
///
template<class C, class T>
std::basic_string<C, T> concat(std::basic_string_view<C, T>, std::basic_string_view<C, T>);


template<class C, class T, class F>
void for_each_word(std::basic_string_view<C, T> string, F func)
    noexcept(noexcept(func(std::basic_string_view<C, T>())));








// Implementation


template<class C, class T>
std::basic_string<C, T> concat(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b)
{
    std::size_t size = a.size();
    if (ARCHON_LIKELY(base::try_int_add(size, b.size()))) {
        std::basic_string<C, T> string;
        string.reserve(size); // Throws
        string.append(a);
        string.append(b);
        return string;
    }
    throw std::length_error("String size");
}


template<class C, class T, class F> void for_each_word(std::basic_string_view<C, T> string, F func)
    noexcept(noexcept(func(std::basic_string_view<C, T>())))
{
    std::size_t i = 0, j, n = string.size();

  find_word_begin:
    if (ARCHON_LIKELY(i < n)) {
        if (ARCHON_LIKELY(string[i] == ' ')) {
            ++i;
            goto find_word_begin;
        }
    }
    else {
        return;
    }

    j = i;

  find_word_end:
    ++i;
    if (ARCHON_LIKELY(i != n && string[i] != ' '))
        goto find_word_end;

    func(string.substr(j, i - j)); // Throws

    if (ARCHON_LIKELY(i < n)) {
        ++i;
        goto find_word_begin;
    }
}


} // namespace archon::base

#endif // ARCHON__BASE__STRING_HPP
