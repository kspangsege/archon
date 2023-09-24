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


auto scalar_compare(long double eps) noexcept;

auto vector_compare(long double eps) noexcept;

auto matrix_compare(long double eps) noexcept;

auto quaternion_compare(long double eps) noexcept;

auto rotation_compare(long double eps) noexcept;








// Implementation


inline auto scalar_compare(long double eps) noexcept
{
    return [eps](auto x, auto y) noexcept {
        return std::abs(x - y) < eps;
    };
}


inline auto vector_compare(long double eps) noexcept
{
    return [eps](const auto& x, const auto& y) noexcept {
        using x_type = std::decay_t<decltype(x)>;
        using y_type = std::decay_t<decltype(y)>;
        static_assert(x_type::size == y_type::size);
        for (int i = 0; i < x.size; ++i) {
            if (ARCHON_LIKELY(std::abs(x[i] - y[i]) < eps))
                continue;
            return false;
        }
        return true;
    };
}


inline auto matrix_compare(long double eps) noexcept
{
    return [eps](const auto& x, const auto& y) noexcept {
        using x_type = std::decay_t<decltype(x)>;
        using y_type = std::decay_t<decltype(y)>;
        static_assert(x_type::num_rows == y_type::num_rows);
        static_assert(x_type::num_cols == y_type::num_cols);
        for (int i = 0; i < x.num_rows; ++i) {
            for (int j = 0; j < x.num_cols; ++j) {
                if (ARCHON_LIKELY(std::abs(x[i][j] - y[i][j]) < eps))
                    continue;
                return false;
            }
        }
        return true;
    };
}


inline auto quaternion_compare(long double eps) noexcept
{
    return [eps](const auto& x, const auto& y) noexcept {
        if (ARCHON_LIKELY(std::abs(x.w - y.w) < eps))
            return test::vector_compare(eps)(x.v, y.v);
        return false;
    };
}


inline auto rotation_compare(long double eps) noexcept
{
    return [eps](const auto& x, const auto& y) noexcept {
        if (ARCHON_LIKELY(test::vector_compare(eps)(x.axis, y.axis)))
            return std::abs(x.angle - y.angle) < eps;
        return false;
    };
}


} // namespace archon::math::test

#endif // ARCHON_X_MATH_X_TEST_X_UTIL_HPP
