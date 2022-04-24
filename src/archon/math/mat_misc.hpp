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

#ifndef ARCHON_X_MATH_X_MAT_MISC_HPP
#define ARCHON_X_MATH_X_MAT_MISC_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <array>
#include <ostream>

#include <archon/core/as_list.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat_val.hpp>


namespace archon::math {


template<class C, class T, std::size_t M, std::size_t N, class U, class R, class A>
auto operator<<(std::basic_ostream<C, T>&, const math::MatVal<M, N, U, R, A>&) -> std::basic_ostream<C, T>&;








// Implementation


template<class C, class T, std::size_t M, std::size_t N, class U, class R, class A>
auto operator<<(std::basic_ostream<C, T>& out, const math::MatVal<M, N, U, R, A>& x) -> std::basic_ostream<C, T>&
{
    using type = std::remove_const_t<U>;
    std::array<math::Vec<N, type>, M> slices;
    for (std::size_t i = 0; i < M; ++i)
        slices[i] = x[i];
    return out << core::as_sbr_list(slices); // Throws
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MAT_MISC_HPP
