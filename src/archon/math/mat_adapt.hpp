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

#ifndef ARCHON_X_MATH_X_MAT_ADAPT_HPP
#define ARCHON_X_MATH_X_MAT_ADAPT_HPP

/// \file


#include <cstddef>

#include <archon/math/mat_val.hpp>
#include <archon/math/mat_adapt_fwd.hpp>
#include <archon/math/impl/mat_adapt_rep.hpp>


namespace archon::math {


template<std::size_t M, std::size_t N, std::size_t P = N, std::size_t Q = 1, class T>
constexpr auto mat_adapt(T* components) noexcept;




template<std::size_t M, std::size_t N, class T, std::size_t P, std::size_t Q> class MatAdapt
    : public math::MatVal<M, N, T, impl::MatAdaptRep<M, N, T, P, Q>, MatAdapt<M, N, T, P, Q>> {
public:
    using math::MatVal<M, N, T, impl::MatAdaptRep<M, N, T, P, Q>, MatAdapt<M, N, T, P, Q>>::operator=;

    constexpr MatAdapt(T* components) noexcept;

    constexpr auto operator=(const MatAdapt&) -> MatAdapt& = default;
};








// Implementation


template<std::size_t M, std::size_t N, std::size_t P, std::size_t Q, class T>
constexpr auto mat_adapt(T* components) noexcept
{
    return math::MatAdapt<M, N, T, P, Q>(components);
}


template<std::size_t M, std::size_t N, class T, std::size_t P, std::size_t Q>
constexpr MatAdapt<M, N, T, P, Q>::MatAdapt(T* components) noexcept
    : math::MatVal<M, N, T, impl::MatAdaptRep<M, N, T, P, Q>, MatAdapt<M, N, T, P, Q>>(components)
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MAT_ADAPT_HPP
