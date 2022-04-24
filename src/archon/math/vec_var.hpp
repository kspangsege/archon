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

#ifndef ARCHON_X_MATH_X_VEC_VAR_HPP
#define ARCHON_X_MATH_X_VEC_VAR_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <array>

#include <archon/core/span.hpp>
#include <archon/math/vec_val.hpp>
#include <archon/math/impl/vec_rep.hpp>


namespace archon::math {


template<std::size_t N, class T, class A> class VecVar
    : public math::VecVal<N, T, impl::VecRep<N, T>, A> {
public:
    using math::VecVal<N, T, impl::VecRep<N, T>, A>::operator=;

    constexpr VecVar() = default;

    constexpr explicit VecVar(T);

    template<class U> constexpr VecVar(U (&)[N]);

    template<class U> constexpr VecVar(const std::array<U, N>&);

    template<class U, class R, class B> constexpr VecVar(const math::VecVal<N, U, R, B>&);

    constexpr VecVar(const VecVar&);

    constexpr auto operator=(const VecVar&) -> VecVar& = default;

    constexpr auto components()       noexcept -> core::Span<T>;
    constexpr auto components() const noexcept -> core::Span<const T>;
};




template<std::size_t N, class T, class A> class VecVar2
    : public math::VecVar<N, T, A> {
public:
    using math::VecVar<N, T, A>::VecVar;
    using math::VecVar<N, T, A>::operator=;

    constexpr VecVar2(const VecVar2&) = default;

    constexpr auto operator=(const VecVar2&) -> VecVar2& = default;
};


template<class T, class A> class VecVar2<2, T, A>
    : public math::VecVar<2, T, A> {
public:
    using math::VecVar<2, T, A>::VecVar;
    using math::VecVar<2, T, A>::operator=;

    constexpr VecVar2(T, T);
    constexpr VecVar2(const VecVar2&) = default;

    constexpr auto operator=(const VecVar2&) -> VecVar2& = default;
};


template<class T, class A> class VecVar2<3, T, A>
    : public math::VecVar<3, T, A> {
public:
    using math::VecVar<3, T, A>::VecVar;
    using math::VecVar<3, T, A>::operator=;

    constexpr VecVar2(T, T, T);
    constexpr VecVar2(const VecVar2&) = default;

    constexpr auto operator=(const VecVar2&) -> VecVar2& = default;
};


template<class T, class A> class VecVar2<4, T, A>
    : public math::VecVar<4, T, A> {
public:
    using math::VecVar<4, T, A>::VecVar;
    using math::VecVar<4, T, A>::operator=;

    constexpr VecVar2(T, T, T, T);
    constexpr VecVar2(const VecVar2&) = default;

    constexpr auto operator=(const VecVar2&) -> VecVar2& = default;
};




template<std::size_t N, class T = double> class Vec
    : public math::VecVar2<N, T, Vec<N, T>> {
public:
    using math::VecVar2<N, T, Vec<N, T>>::VecVar2;
    using math::VecVar2<N, T, Vec<N, T>>::operator=;

    constexpr Vec(const Vec&) = default;

    constexpr auto operator=(const Vec&) -> Vec& = default;
};


template<std::size_t N, class T> Vec(T (&)[N]) -> Vec<N, std::remove_const_t<T>>;

template<std::size_t N, class T> Vec(const std::array<T, N>&) -> Vec<N, std::remove_const_t<T>>;

template<std::size_t N, class T, class R, class A> Vec(const math::VecVal<N, T, R, A>&) ->
    Vec<N, std::remove_const_t<T>>;




template<std::size_t N> using VecF = math::Vec<N, float>;
template<std::size_t N> using VecL = math::Vec<N, long double>;


using Vec2 = math::Vec<2>;
using Vec3 = math::Vec<3>;
using Vec4 = math::Vec<4>;

using Vec2F = math::VecF<2>;
using Vec3F = math::VecF<3>;
using Vec4F = math::VecF<4>;

using Vec2L = math::VecL<2>;
using Vec3L = math::VecL<3>;
using Vec4L = math::VecL<4>;








// Implementation


// ============================ VecVar ============================


template<std::size_t N, class T, class A>
constexpr VecVar<N, T, A>::VecVar(T component)
    : math::VecVal<N, T, impl::VecRep<N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < N; ++i)
        this->comp(i) = component; // Throws
}


template<std::size_t N, class T, class A>
template<class U> constexpr VecVar<N, T, A>::VecVar(U (& components)[N])
    : math::VecVal<N, T, impl::VecRep<N, T>, A>() // Throws
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    for (std::size_t i = 0; i < N; ++i)
        this->comp(i) = components[i]; // Throws
}


template<std::size_t N, class T, class A>
template<class U> constexpr VecVar<N, T, A>::VecVar(const std::array<U, N>& components)
    : math::VecVal<N, T, impl::VecRep<N, T>, A>() // Throws
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    for (std::size_t i = 0; i < N; ++i)
        this->comp(i) = components[i]; // Throws
}


template<std::size_t N, class T, class A>
template<class U, class R, class B> constexpr VecVar<N, T, A>::VecVar(const math::VecVal<N, U, R, B>& x)
    : math::VecVal<N, T, impl::VecRep<N, T>, A>() // Throws
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    for (std::size_t i = 0; i < N; ++i)
        this->comp(i) = x[i]; // Throws
}


template<std::size_t N, class T, class A>
constexpr VecVar<N, T, A>::VecVar(const VecVar& x)
    : math::VecVal<N, T, impl::VecRep<N, T>, A>()
{
    for (std::size_t i = 0; i < N; ++i)
        this->comp(i) = x[i]; // Throws
}


template<std::size_t N, class T, class A>
constexpr auto VecVar<N, T, A>::components() noexcept -> core::Span<T>
{
    return this->m_rep.components();
}


template<std::size_t N, class T, class A>
constexpr auto VecVar<N, T, A>::components() const noexcept -> core::Span<const T>
{
    return this->m_rep.components();
}



// ============================ VecVar2 ============================


template<class T, class A>
constexpr VecVar2<2, T, A>::VecVar2(T a, T b)
    : math::VecVar<2, T, A>(std::array { a, b }) // Throws
{
}


template<class T, class A>
constexpr VecVar2<3, T, A>::VecVar2(T a, T b, T c)
    : math::VecVar<3, T, A>(std::array { a, b, c }) // Throws
{
}


template<class T, class A>
constexpr VecVar2<4, T, A>::VecVar2(T a, T b, T c, T d)
    : math::VecVar<4, T, A>(std::array { a, b, c, d }) // Throws
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VEC_VAR_HPP
