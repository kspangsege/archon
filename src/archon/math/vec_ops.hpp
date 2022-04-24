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

#ifndef ARCHON_X_MATH_X_VEC_OPS_HPP
#define ARCHON_X_MATH_X_VEC_OPS_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <type_traits>

#include <archon/math/type_traits.hpp>
#include <archon/math/vec_val.hpp>
#include <archon/math/vec_var.hpp>


namespace archon::math {


/// \brief Add two vectors.
///
/// This operation constructs the sum of the two specified `N`-vectors. The sum is itself an
/// `N`-vector.
///
template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator+(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Subtract two vectors.
///
/// This operation constructs the difference between the two specified `N`-vectors. The
/// difference is itself an `N`-vector.
///
template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator-(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Negate vector.
///
/// This operation constructs the additive inverse of the specified `N`-vector. The result
/// is also an `N`-vector.
///
template<std::size_t N, class T, class R, class A>
constexpr auto operator-(const math::VecVal<N, T, R, A>& x) -> math::Vec<N, math::scalar_arith_type<T>>;


/// \brief Multiply vector and scalar.
///
/// This operation is the same as `math::outer(x, y)`.
///
template<std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator*(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Multiply scalar and vector.
///
/// This operation is the same as `math::outer(x, y)`.
///
template<class T, std::size_t N, class U, class R, class B,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator*(T x, const math::VecVal<N, U, R, B>& y) -> math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Cross product of two 3-vectors.
///
/// This operation is the same as `math::cross(x, y)`.
///
template<class T, class R, class A, class U, class S, class B>
constexpr auto operator*(const math::VecVal<3, T, R, A>& x, const math::VecVal<3, U, S, B>& y) ->
    math::Vec<3, math::scalar_arith_type<T, U>>;


/// \brief Divide vector by scalar.
///
/// This operation constructs a new `N`-vector from the specified `N`-vector (\p x). Each
/// component in the new vector is computed by taking the corresponding component from the
/// specified vector, and dividing it by the specified scalar (\p y).
///
template<std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator/(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Length of vector.
///
/// This operation computes the length of the specified `N`-vector. This is the same as
/// `std::sqrt(math::sq_sum(x))`.
///
/// \sa math::sq_sum().
///
template<std::size_t N, class T, class R, class A>
constexpr auto len(const math::VecVal<N, T, R, A>& x) -> decltype(std::sqrt(math::scalar_arith_type<T>()));


/// \brief Sum of vector components.
///
/// This operation computes the sum of the components of the specified `N`-vector.
///
template<std::size_t N, class T, class R, class A>
constexpr auto sum(const math::VecVal<N, T, R, A>& x) -> math::scalar_arith_type<T>;


/// \brief Square-sum of vector components.
///
/// This operation computes the square-sum of the components of the specified
/// `N`-vector. The square-sum of a sequence of numbers is the sum of the squares of those
/// numbers.
///
/// The square-sum of the vector components is also the square of the length of the vector
/// (Pythagoras).
///
/// \sa math::len().
///
template<std::size_t N, class T, class R, class A>
constexpr auto sq_sum(const math::VecVal<N, T, R, A>& x) -> math::scalar_arith_type<T>;


/// \brief Vector dot product.
///
/// This operation is the same as `math::inner(x, y)`.
///
template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto dot(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::scalar_arith_type<T, U>;


/// \brief Projection of vector on vector.
///
/// This operation constructs the projection of the first specified `N`-vector (\p x) on the
/// second specified `N`-vector (\p y).
///
/// This operation is the same as `(math::dot(x, y) / math::sq_sum(y)) * y`.
///
template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto proj(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Perpendicular 2-vector.
///
/// This operation constructs the perpendicular vector of the specified 2-vector. The result
/// is another 2-vector.
///
template<class T, class R, class A>
constexpr auto perp(const math::VecVal<2, T, R, A>& x) -> math::Vec<2, math::scalar_arith_type<T>>;


/// \brief Cross product of two 3-vectors.
///
/// This operation constructs the cross product of the two specified 3-vectors. The result
/// is another 3-vector.
///
template<class T, class R, class A, class U, class S, class B>
constexpr auto cross(const math::VecVal<3, T, R, A>& x, const math::VecVal<3, U, S, B>& y) ->
    math::Vec<3, math::scalar_arith_type<T, U>>;


/// \brief Outer product of vector and scalar.
///
/// This operation computes the outer product of the specified `N`-vector and the specified
/// scalar. The result is an `N`-vector. In tensor terminology, this is the outer product of
/// a tensor of order 1 (the vector) and a tensor of order 0 (the scalar).
///
template<std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto outer(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of scaler and vector.
///
/// This operation computes the outer product of the specified scalar and the specified
/// `N`-vector. The result is an `N`-vector. In tensor terminology, this is the outer
/// product of a tensor of order 0 (the scalar) and a tensor of order 1 (the vector).
///
template<class T, std::size_t N, class U, class R, class B,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto outer(T x, const math::VecVal<N, U, R, B>& y) -> math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of two vectors.
///
/// This function computes the inner propduct of the two specified `N`-vectors. The result
/// is a scalar. In tensor terminology, this is the inner product of two tensors of order 1
/// (the vectors).
///
template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto inner(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::scalar_arith_type<T, U>;








// Implementation


template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator+(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = x[i] + y[i]; // Throws
    return z;
}


template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator-(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = x[i] - y[i]; // Throws
    return z;
}


template<std::size_t N, class T, class R, class A>
constexpr auto operator-(const math::VecVal<N, T, R, A>& x) -> math::Vec<N, math::scalar_arith_type<T>>
{
    math::Vec<N, math::scalar_arith_type<T>> y; // Throws
    for (std::size_t i = 0; i < N; ++i)
        y[i] = -x[i]; // Throws
    return y;
}


template<std::size_t N, class T, class R, class A, class U, class>
constexpr auto operator*(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>
{
    return math::outer(x, y); // Throws
}


template<class T, std::size_t N, class U, class R, class B, class>
constexpr auto operator*(T x, const math::VecVal<N, U, R, B>& y) -> math::Vec<N, math::scalar_arith_type<T, U>>
{
    return math::outer(x, y); // Throws
}


template<class T, class R, class A, class U, class S, class B>
constexpr auto operator*(const math::VecVal<3, T, R, A>& x, const math::VecVal<3, U, S, B>& y) ->
    math::Vec<3, math::scalar_arith_type<T, U>>
{
    return math::cross(x, y); // Throws
}


template<std::size_t N, class T, class R, class A, class U, class>
constexpr auto operator/(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar<U>);
    static_assert(math::is_valid_scalar_pair<T, U>);
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = x[i] / y; // Throws
    return z;
}


template<std::size_t N, class T, class R, class A>
constexpr auto len(const math::VecVal<N, T, R, A>& x) -> decltype(std::sqrt(math::scalar_arith_type<T>()))
{
    return std::sqrt(math::sq_sum(x)); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto sum(const math::VecVal<N, T, R, A>& x) -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    type y = 0; // Throws
    for (std::size_t i = 0; i < N; ++i)
        y += x[i]; // Throws
    return y;
}


template<std::size_t N, class T, class R, class A>
constexpr auto sq_sum(const math::VecVal<N, T, R, A>& x) -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    type y = 0; // Throws
    for (std::size_t i = 0; i < N; ++i)
        y += x[i] * x[i]; // Throws
    return y;
}


template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto dot(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::scalar_arith_type<T, U>
{
    return math::inner(x, y); // Throws
}


template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto proj(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>
{
    return (math::dot(x, y) / math::sq_sum(y)) * y; // Throws
}


template<class T, class R, class A>
constexpr auto perp(const math::VecVal<2, T, R, A>& x) ->
    math::Vec<2, math::scalar_arith_type<T>>
{
    return {
        -x[1], // Throws
        +x[0], // Throws
    }; // Throws
}


template<class T, class R, class A, class U, class S, class B>
constexpr auto cross(const math::VecVal<3, T, R, A>& x, const math::VecVal<3, U, S, B>& y) ->
    math::Vec<3, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    return {
        x[1] * y[2] - x[2] * y[1], // Throws
        x[2] * y[0] - x[0] * y[2], // Throws
        x[0] * y[1] - x[1] * y[0], // Throws
    }; // Throws
}


template<std::size_t N, class T, class R, class A, class U, class>
constexpr auto outer(const math::VecVal<N, T, R, A>& x, U y) -> math::Vec<N, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar<U>);
    static_assert(math::is_valid_scalar_pair<T, U>);
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = x[i] * y; // Throws
    return z;
}


template<class T, std::size_t N, class U, class R, class B, class>
constexpr auto outer(T x, const math::VecVal<N, U, R, B>& y) -> math::Vec<N, math::scalar_arith_type<T, U>>
{
    static_assert(math::is_valid_scalar<T>);
    static_assert(math::is_valid_scalar_pair<T, U>);
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = x * y[i]; // Throws
    return z;
}


template<std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto inner(const math::VecVal<N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::scalar_arith_type<T, U>
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    using type = math::scalar_arith_type<T, U>;
    type z = 0; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z += x[i] * y[i]; // Throws
    return z;
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VEC_OPS_HPP
