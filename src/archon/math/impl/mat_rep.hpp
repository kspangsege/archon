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

#ifndef ARCHON_X_MATH_X_IMPL_X_MAT_REP_HPP
#define ARCHON_X_MATH_X_IMPL_X_MAT_REP_HPP

/// \file


#include <cstddef>
#include <array>

#include <archon/math/vec_adapt.hpp>
#include <archon/math/mat_adapt.hpp>


namespace archon::math::impl {


template<std::size_t M, std::size_t N, class T> class MatRep {
public:
    constexpr auto row(std::size_t i) noexcept
    {
        return math::vec_adapt<N>(m_components.data() + i * N);
    }

    constexpr auto row(std::size_t i) const noexcept
    {
        return math::vec_adapt<N>(m_components.data() + i * N);
    }

    constexpr auto col(std::size_t i) noexcept
    {
        return math::vec_adapt<M, N>(m_components.data() + i);
    }

    constexpr auto col(std::size_t i) const noexcept
    {
        return math::vec_adapt<M, N>(m_components.data() + i);
    }

    template<std::size_t P, std::size_t Q> constexpr auto sub() noexcept
    {
        return math::mat_adapt<P, Q, N, 1>(m_components.data());
    }

    template<std::size_t P, std::size_t Q> constexpr auto sub() const noexcept
    {
        return math::mat_adapt<P, Q, N, 1>(m_components.data());
    }

    constexpr auto transposed() noexcept
    {
        return math::mat_adapt<N, M, 1, N>(m_components.data());
    }

    constexpr auto transposed() const noexcept
    {
        return math::mat_adapt<N, M, 1, N>(m_components.data());
    }

    constexpr auto diag() noexcept
    {
        static_assert(M == N);
        return math::vec_adapt<M, N + 1>(m_components.data());
    }

    constexpr auto diag() const noexcept
    {
        static_assert(M == N);
        return math::vec_adapt<M, N + 1>(m_components.data());
    }

    constexpr auto components() noexcept -> core::Span<T>
    {
        return m_components;
    }

    constexpr auto components() const noexcept -> core::Span<const T>
    {
        return m_components;
    }

private:
    std::array<T, M * N> m_components = {};
};


} // namespace archon::math::impl

#endif // ARCHON_X_MATH_X_IMPL_X_MAT_REP_HPP
