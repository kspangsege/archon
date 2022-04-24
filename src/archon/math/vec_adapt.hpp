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

#ifndef ARCHON_X_MATH_X_VEC_ADAPT_HPP
#define ARCHON_X_MATH_X_VEC_ADAPT_HPP

/// \file


#include <cstddef>

#include <archon/math/vec_val.hpp>
#include <archon/math/impl/vec_adapt_rep.hpp>


namespace archon::math {


template<std::size_t N, class T> constexpr auto vec_adapt(T (& components)[N]) noexcept;
template<std::size_t N, class T> constexpr auto vec_adapt(std::array<T, N>& components) noexcept;
template<std::size_t N, class T> constexpr auto vec_adapt(const std::array<T, N>& components) noexcept;

template<std::size_t N, std::size_t P = 1, class T> constexpr auto vec_adapt(T* components) noexcept;

template<class T> constexpr auto vec2_adapt(T* components) noexcept;
template<class T> constexpr auto vec3_adapt(T* components) noexcept;
template<class T> constexpr auto vec4_adapt(T* components) noexcept;




template<std::size_t N, class T, std::size_t P> class VecAdapt
    : public math::VecVal<N, T, impl::VecAdaptRep<N, T, P>, VecAdapt<N, T, P>> {
public:
    using math::VecVal<N, T, impl::VecAdaptRep<N, T, P>, VecAdapt<N, T, P>>::operator=;

    constexpr VecAdapt(T* components) noexcept;

    constexpr auto operator=(const VecAdapt&) -> VecAdapt& = default;
};








// Implementation


template<std::size_t N, class T> constexpr auto vec_adapt(T (& components)[N]) noexcept
{
    return math::vec_adapt<N>(static_cast<T*>(components));
}


template<std::size_t N, class T> constexpr auto vec_adapt(std::array<T, N>& components) noexcept
{
    return math::vec_adapt<N>(components.data());
}


template<std::size_t N, class T> constexpr auto vec_adapt(const std::array<T, N>& components) noexcept
{
    return math::vec_adapt<N>(components.data());
}


template<std::size_t N, std::size_t P, class T> constexpr auto vec_adapt(T* components) noexcept
{
    return math::VecAdapt<N, T, P>(components);
}


template<class T> constexpr auto vec2_adapt(T* components) noexcept
{
    return math::vec_adapt<2>(components);
}


template<class T> constexpr auto vec3_adapt(T* components) noexcept
{
    return math::vec_adapt<3>(components);
}


template<class T> constexpr auto vec4_adapt(T* components) noexcept
{
    return math::vec_adapt<4>(components);
}


template<std::size_t N, class T, std::size_t P>
constexpr VecAdapt<N, T, P>::VecAdapt(T* components) noexcept
    : math::VecVal<N, T, impl::VecAdaptRep<N, T, P>, VecAdapt<N, T, P>>(components)
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VEC_ADAPT_HPP
