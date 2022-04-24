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

#ifndef ARCHON_X_MATH_X_TEST_X_UTIL_HPP
#define ARCHON_X_MATH_X_TEST_X_UTIL_HPP


#include <cstddef>
#include <cmath>
#include <type_traits>
#include <limits>

#include <archon/core/features.h>


namespace archon::math::test {


auto matrix_compare(long double eps);








// Implementation


inline auto matrix_compare(long double eps)
{
    return [eps](const auto& x, const auto& y)
    {
        using x_type = std::decay_t<decltype(x)>;
        using y_type = std::decay_t<decltype(y)>;
        static_assert(x_type::num_rows == y_type::num_rows);
        static_assert(x_type::num_cols == y_type::num_cols);
        for (std::size_t i = 0; i < x.num_rows; ++i) {
            for (std::size_t j = 0; j < x.num_cols; ++j) {
                if (ARCHON_LIKELY(std::abs(x[i][j] - y[i][j]) < eps))
                    continue;
                return false;
            }
        }
        return true;
    };
}


} // namespace archon::math::test

#endif // ARCHON_X_MATH_X_TEST_X_UTIL_HPP
