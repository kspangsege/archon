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

#ifndef ARCHON_X_MATH_X_MAT_VAR_HPP
#define ARCHON_X_MATH_X_MAT_VAR_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <array>

#include <archon/core/span.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat_val.hpp>
#include <archon/math/impl/mat_rep.hpp>


namespace archon::math {


template<std::size_t M, std::size_t N, class T, class A> class MatVar
    : public math::MatVal<M, N, T, impl::MatRep<M, N, T>, A> {
public:
    using math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>::operator=;

    constexpr MatVar() = default;

    template<class U, class R, class B> constexpr explicit MatVar(const math::VecVal<N, U, R, B>& row);

    template<class U> constexpr MatVar(math::Vec<N, U> (& rows)[M]);
    template<class U> constexpr MatVar(const math::Vec<N, U> (& rows)[M]);

    template<class U> constexpr MatVar(const std::array<math::Vec<N, U>, M>& rows);
    template<class U> constexpr MatVar(const std::array<const math::Vec<N, U>, M>& rows);

    template<class U, class R, class B> constexpr MatVar(const math::MatVal1<M, N, U, R, B>&);

    constexpr MatVar(const MatVar&);

    constexpr auto operator=(const MatVar&) -> MatVar& = default;

    constexpr auto components()       noexcept -> core::Span<T>;
    constexpr auto components() const noexcept -> core::Span<const T>;
};




template<std::size_t M, std::size_t N, class T, class A> class MatVar2
    : public math::MatVar<M, N, T, A> {
public:
    using math::MatVar<M, N, T, A>::MatVar;
    using math::MatVar<M, N, T, A>::operator=;

    constexpr MatVar2(const MatVar2&) = default;

    constexpr auto operator=(const MatVar2&) -> MatVar2& = default;
};


template<std::size_t N, class T, class A> class MatVar2<2, N, T, A>
    : public math::MatVar<2, N, T, A> {
public:
    using math::MatVar<2, N, T, A>::MatVar;
    using math::MatVar<2, N, T, A>::operator=;

    constexpr MatVar2(const math::Vec<N, T>&, const math::Vec<N, T>&);
    constexpr MatVar2(const MatVar2&) = default;

    constexpr auto operator=(const MatVar2&) -> MatVar2& = default;
};


template<std::size_t N, class T, class A> class MatVar2<3, N, T, A>
    : public math::MatVar<3, N, T, A> {
public:
    using math::MatVar<3, N, T, A>::MatVar;
    using math::MatVar<3, N, T, A>::operator=;

    constexpr MatVar2(const math::Vec<N, T>&, const math::Vec<N, T>&, const math::Vec<N, T>&);
    constexpr MatVar2(const MatVar2&) = default;

    constexpr auto operator=(const MatVar2&) -> MatVar2& = default;
};


template<std::size_t N, class T, class A> class MatVar2<4, N, T, A>
    : public math::MatVar<4, N, T, A> {
public:
    using math::MatVar<4, N, T, A>::MatVar;
    using math::MatVar<4, N, T, A>::operator=;

    constexpr MatVar2(const math::Vec<N, T>&, const math::Vec<N, T>&, const math::Vec<N, T>&, const math::Vec<N, T>&);
    constexpr MatVar2(const MatVar2&) = default;

    constexpr auto operator=(const MatVar2&) -> MatVar2& = default;
};




template<std::size_t M, std::size_t N = M, class T = double> class Mat
    : public math::MatVar2<M, N, T, Mat<M, N, T>> {
public:
    using math::MatVar2<M, N, T, Mat<M, N, T>>::MatVar2;
    using math::MatVar2<M, N, T, Mat<M, N, T>>::operator=;

    constexpr Mat(const Mat&) = default;

    constexpr auto operator=(const Mat&) -> Mat& = default;
};


template<std::size_t M, std::size_t N, class T> Mat(math::Vec<N, T> (&)[M]) -> Mat<M, N, std::remove_const_t<T>>;

template<std::size_t M, std::size_t N, class T> Mat(const math::Vec<N, T> (&)[M]) -> Mat<M, N, std::remove_const_t<T>>;

template<std::size_t M, std::size_t N, class T> Mat(const std::array<math::Vec<N, T>, M>&) ->
    Mat<M, N, std::remove_const_t<T>>;

template<std::size_t M, std::size_t N, class T> Mat(const std::array<const math::Vec<N, T>, M>&) ->
    Mat<M, N, std::remove_const_t<T>>;

template<std::size_t M, std::size_t N, class T, class R, class A> Mat(const math::MatVal1<M, N, T, R, A>&) ->
    Mat<M, N, std::remove_const_t<T>>;




template<std::size_t M, std::size_t N = M> using MatF = math::Mat<M, N, float>;
template<std::size_t M, std::size_t N = M> using MatL = math::Mat<M, N, long double>;


using Mat2 = math::Mat<2>;
using Mat3 = math::Mat<3>;
using Mat4 = math::Mat<4>;

using Mat2F = math::MatF<2>;
using Mat3F = math::MatF<3>;
using Mat4F = math::MatF<4>;

using Mat2L = math::MatL<2>;
using Mat3L = math::MatL<3>;
using Mat4L = math::MatL<4>;


using Mat2x2 = math::Mat<2, 2>;
using Mat2x3 = math::Mat<2, 3>;
using Mat2x4 = math::Mat<2, 4>;

using Mat3x2 = math::Mat<3, 2>;
using Mat3x3 = math::Mat<3, 3>;
using Mat3x4 = math::Mat<3, 4>;

using Mat4x2 = math::Mat<4, 2>;
using Mat4x3 = math::Mat<4, 3>;
using Mat4x4 = math::Mat<4, 4>;


using Mat2x2F = math::MatF<2, 2>;
using Mat2x3F = math::MatF<2, 3>;
using Mat2x4F = math::MatF<2, 4>;

using Mat3x2F = math::MatF<3, 2>;
using Mat3x3F = math::MatF<3, 3>;
using Mat3x4F = math::MatF<3, 4>;

using Mat4x2F = math::MatF<4, 2>;
using Mat4x3F = math::MatF<4, 3>;
using Mat4x4F = math::MatF<4, 4>;


using Mat2x2L = math::MatL<2, 2>;
using Mat2x3L = math::MatL<2, 3>;
using Mat2x4L = math::MatL<2, 4>;

using Mat3x2L = math::MatL<3, 2>;
using Mat3x3L = math::MatL<3, 3>;
using Mat3x4L = math::MatL<3, 4>;

using Mat4x2L = math::MatL<4, 2>;
using Mat4x3L = math::MatL<4, 3>;
using Mat4x4L = math::MatL<4, 4>;








// Implementation


// ============================ MatVar ============================


template<std::size_t M, std::size_t N, class T, class A>
template<class U, class R, class B> constexpr MatVar<M, N, T, A>::MatVar(const math::VecVal<N, U, R, B>& row)
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = row; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
template<class U> constexpr MatVar<M, N, T, A>::MatVar(math::Vec<N, U> (& rows)[M])
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = rows[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
template<class U> constexpr MatVar<M, N, T, A>::MatVar(const math::Vec<N, U> (& rows)[M])
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = rows[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
template<class U> constexpr MatVar<M, N, T, A>::MatVar(const std::array<math::Vec<N, U>, M>& rows)
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = rows[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
template<class U> constexpr MatVar<M, N, T, A>::MatVar(const std::array<const math::Vec<N, U>, M>& rows)
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = rows[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
template<class U, class R, class B> constexpr MatVar<M, N, T, A>::MatVar(const math::MatVal1<M, N, U, R, B>& x)
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = x[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
constexpr MatVar<M, N, T, A>::MatVar(const MatVar& x)
    : math::MatVal<M, N, T, impl::MatRep<M, N, T>, A>() // Throws
{
    for (std::size_t i = 0; i < M; ++i)
        this->row(i) = x[i]; // Throws
}


template<std::size_t M, std::size_t N, class T, class A>
constexpr auto MatVar<M, N, T, A>::components() noexcept -> core::Span<T>
{
    return this->m_rep.components();
}


template<std::size_t M, std::size_t N, class T, class A>
constexpr auto MatVar<M, N, T, A>::components() const noexcept -> core::Span<const T>
{
    return this->m_rep.components();
}



// ============================ MatVar2 ============================


template<std::size_t N, class T, class A>
constexpr MatVar2<2, N, T, A>::MatVar2(const math::Vec<N, T>& a, const math::Vec<N, T>& b)
    : math::MatVar<2, N, T, A>(std::array { a, b }) // Throws
{
}


template<std::size_t N, class T, class A>
constexpr MatVar2<3, N, T, A>::MatVar2(const math::Vec<N, T>& a, const math::Vec<N, T>& b, const math::Vec<N, T>& c)
    : math::MatVar<3, N, T, A>(std::array { a, b, c }) // Throws
{
}


template<std::size_t N, class T, class A>
constexpr MatVar2<4, N, T, A>::MatVar2(const math::Vec<N, T>& a, const math::Vec<N, T>& b, const math::Vec<N, T>& c,
                                       const math::Vec<N, T>& d)
    : math::MatVar<4, N, T, A>(std::array { a, b, c, d }) // Throws
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MAT_VAR_HPP
