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

#ifndef ARCHON_X_MATH_X_MAT_OPS_HPP
#define ARCHON_X_MATH_X_MAT_OPS_HPP

/// \file


#include <archon/math/type_traits.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat_val.hpp>
#include <archon/math/mat_var.hpp>


namespace archon::math {


/// \brief Add two matrices.
///
/// This operation constructs the sum of the two specified `M`-by-`N` matrices. The sum is
/// itself an `M`-by-`N` matrix.
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator+(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Subtract two matrices.
///
/// This operation constructs the difference between the two specified `M`-by-`N`
/// matrices. The difference is itself an `M`-by-`N` matrix.
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator-(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Negate matrix.
///
/// This operation constructs the additive inverse of the specified `M`-by-`N` matrix. The
/// result is also an `M`-by-`N` matrix.
///
template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto operator-(const math::MatVal<M, N, T, R, A>& x) -> math::Mat<M, N, math::scalar_arith_type<T>>;


/// \brief Multiply matrix and scalar.
///
/// This operation is the same as `math::outer(x, y)`.
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Multiply scalar and matrix.
///
/// This operation is the same as `math::outer(x, y)`.
///
template<class T, std::size_t M, std::size_t N, class U, class R, class B,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator*(T x, const math::MatVal<M, N, U, R, B>& y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Multiply matrix and vector.
///
/// This operation is the same as `math::inner(x, y)`.
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<M, math::scalar_arith_type<T, U>>;


/// \brief Multiply vetor and matrix.
///
/// This operation is the same as `math::inner(x, y)`.
///
template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto operator*(const math::VecVal<M, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Multiply matrices.
///
/// This operation is the same as `math::inner(x, y)`.
///
template<std::size_t M, std::size_t N, class T, class R, class A, std::size_t O, class U, class S, class B>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, O, U, S, B>& y) ->
    math::Mat<M, O, math::scalar_arith_type<T, U>>;


/// \brief Divide matrix by scalar.
///
/// This operation constructs a new `M`-by-`N` matrix from the specified `M`-by-`N` matrix
/// (\p x). Each component in the new matrix is computed by taking the corresponding
/// component from the specified matrix, and dividing it by the specified scalar (\p y).
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator/(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Divide scalar by square matrix.
///
/// This operation is a shorthand for `x * archon::math::inv(y)`.
///
template<class T, std::size_t N, class U, class R, class A,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto operator/(T x, const math::MatVal<N, N, U, R, A>& y) -> math::Mat<N, N, math::scalar_arith_type<T, U>>;


/// \brief Divide matrix by square matrix.
///
/// This operation is a shorthand for `x * archon::math::inv(y)`.
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator/(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Transpose matrix.
///
/// This function constructs the transpose of the specified matrix.
///
template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto transpose(const math::MatVal<M, N, T, R, A>& x) -> math::Mat<N, M, T>;


/// \brief Trace of square matrix.
///
/// This function computes the trace of the specified square matrix. The trace of a square
/// matrix is the sum of the elements on its main diagonal.
///
template<std::size_t N, class T, class R, class A>
constexpr auto tr(const math::MatVal<N, N, T, R, A>& x) -> math::scalar_arith_type<T>;


/// \brief Determinant of square matrix.
///
/// This function computes the determinant of the specified square matrix.
///
template<std::size_t N, class T, class R, class A>
constexpr auto det(const math::MatVal<N, N, T, R, A>& x) -> math::scalar_arith_type<T>;


/// \brief Invert square matrix.
///
/// This function computes the multiplicative inverse of the specified square matrix.
///
/// If the specified matrix is identified as singular, the result is a zero-matrix.
///
template<std::size_t N, class T, class R, class A>
constexpr auto inv(const math::MatVal<N, N, T, R, A>& x) -> math::Mat<N, N, math::scalar_arith_type<T>>;


/// \brief Outer product of vectors.
///
/// This operation computes the outer product of the specified `M`-vector (\p x) and the
/// specified `N`-vector (\p y). The result is an `M`-by-`N` matrix. In tensor terminology,
/// this is the outer product of two tensors of order 1 (the vectors).
///
template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto outer(const math::VecVal<M, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of matrix and scalar.
///
/// This operation computes the outer product of the specified `M`-by-`N` matrix and the
/// specified scalar. The result is an `M`-by-`N` matrix. In tensor terminology, this is the
/// outer product of a tensor of order 2 (the matrix) and a tensor of order 0 (the scalar).
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto outer(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of scaler and matrix.
///
/// This operation computes the outer product of the specified scalar and the specified
/// `M`-by-`N` matrix. The result is an `M`-by-`N` matrix. In tensor terminology, this is
/// the outer product of a tensor of order 0 (the scalar) and a tensor of order 2 (the
/// matrix).
///
template<class T, std::size_t M, std::size_t N, class U, class R, class B,
         class = std::enable_if_t<math::is_compat_scalar_pair<T, U>>>
constexpr auto outer(T x, const math::MatVal<M, N, U, R, B>& y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of matrix and vector.
///
/// This operation constructs the inner product of the specified `M`-by-`N` matrix (\p x)
/// and the specified `N`-vector (\p y). The result is an `M`-vector. In tensor terminology,
/// this is the inner product of a tensor of order 2 (the matrix) and a tensor of order 1
/// (the vector).
///
template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto inner(const math::MatVal<M, N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<M, math::scalar_arith_type<T, U>>;


/// \brief Inner product of vector and matrix.
///
/// This operation constructs the inner product of the specified `M`-vector (\p x) and the
/// specified `M`-by-`N` matrix (\p y). The result is an `N`-vector. In tensor terminology,
/// this is the inner product of a tensor of order 1 (the vector) and a tensor of order 2
/// (the matrix).
///
template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto inner(const math::VecVal<M, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of two matrices.
///
/// This operation constructs the inner product of the specified `M`-by-`N` matrix (\p x)
/// and the specified `N`-by-`O` matrix (\p y). The result is an `M`-by-`O` matrix. In
/// tensor terminology, this is the inner product of two tensors of order 2 (the matrices).
///
template<std::size_t M, std::size_t N, class T, class R, class A, std::size_t O, class U, class S, class B>
constexpr auto inner(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, O, U, S, B>& y) ->
    math::Mat<M, O, math::scalar_arith_type<T, U>>;


/// \brief Identity matrix.
///
/// This function constructs an identity matrix of the specified size and with the specified
/// component type.
///
template<std::size_t N, class T = double> constexpr auto ident() -> math::Mat<N, N, T>;


/// \brief Extend matrix.
///
/// This function constructs an extension of the specified matrix.
///
template<std::size_t M, std::size_t N, class T = double, std::size_t P, std::size_t Q, class U, class R, class A>
constexpr auto extend(const math::MatVal<P, Q, U, R, A>&) -> math::Mat<M, N, T>;


/// \brief Generate matrix.
///
/// This function constructs a matrix from components produced by the specified
/// function. The specified function is called with two arguments; a row index, and a
/// colument index, both of type `std::size_t`. The order in which the components are
/// requested is unspecified.
///
template<std::size_t M, std::size_t N = M, class T = double, class F>
constexpr auto gen_mat(F&& func) -> math::Mat<M, N, T>;








// Implementation


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator+(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = x[i] + y[i]; // Throws
    return z;
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator-(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = x[i] - y[i]; // Throws
    return z;
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto operator-(const math::MatVal<M, N, T, R, A>& x) -> math::Mat<M, N, math::scalar_arith_type<T>>
{
    math::Mat<M, N, math::scalar_arith_type<T>> y; // Throws
    for (std::size_t i = 0; i < M; ++i)
        y[i] = -x[i]; // Throws
    return y;
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    return math::outer(x, y); // Throws
}


template<class T, std::size_t M, std::size_t N, class U, class R, class B, class>
constexpr auto operator*(T x, const math::MatVal<M, N, U, R, B>& y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    return math::outer(x, y); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<M, math::scalar_arith_type<T, U>>
{
    return math::inner(x, y); // Throws
}


template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto operator*(const math::VecVal<M, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>
{
    return math::inner(x, y); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A, std::size_t O, class U, class S, class B>
constexpr auto operator*(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, O, U, S, B>& y) ->
    math::Mat<M, O, math::scalar_arith_type<T, U>>
{
    return math::inner(x, y); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class>
constexpr auto operator/(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = x[i] / y; // Throws
    return z;
}


template<class T, std::size_t N, class U, class R, class A, class>
constexpr auto operator/(T x, const math::MatVal<N, N, U, R, A>& y) -> math::Mat<N, N, math::scalar_arith_type<T, U>>
{
    return x * math::inv(y); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto operator/(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    return x * math::inv(y); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto transpose(const math::MatVal<M, N, T, R, A>& x) -> math::Mat<N, M, T>
{
    return x.transposed(); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto tr(const math::MatVal<N, N, T, R, A>& x) -> math::scalar_arith_type<T>
{
    return math::sum(x.diag()); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto det(const math::MatVal<N, N, T, R, A>& x) -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    math::Mat<N, N, type> y = x;
    std::array<std::size_t, N> pivots = y.decompose();
    int n = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (pivots[i] != i)
            ++n;
    }
    type z = 1;
    for (std::size_t i = 0; i < N; ++i)
        z *= y[i][i];
    return (n % 2 == 0 ? z : -z);
}


template<std::size_t N, class T, class R, class A>
constexpr auto inv(const math::MatVal<N, N, T, R, A>& x) -> math::Mat<N, N, math::scalar_arith_type<T>>
{
    using type = math::scalar_arith_type<T>;
    math::Mat<N, N, type> y = x; // Throws
    if (ARCHON_LIKELY(y.try_inv())) // Throws
        return y; // Throws
    return {}; // Throws
}


template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto outer(const math::VecVal<M, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = x[i] * y; // Throws
    return z;
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class>
constexpr auto outer(const math::MatVal<M, N, T, R, A>& x, U y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = math::outer(x[i], y); // Throws
    return z;
}


template<class T, std::size_t M, std::size_t N, class U, class R, class B, class>
constexpr auto outer(T x, const math::MatVal<M, N, U, R, B>& y) -> math::Mat<M, N, math::scalar_arith_type<T, U>>
{
    math::Mat<M, N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = math::outer(x, y[i]); // Throws
    return z;
}


template<std::size_t M, std::size_t N, class T, class R, class A, class U, class S, class B>
constexpr auto inner(const math::MatVal<M, N, T, R, A>& x, const math::VecVal<N, U, S, B>& y) ->
    math::Vec<M, math::scalar_arith_type<T, U>>
{
    math::Vec<M, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = math::inner(x.row(i), y); // Throws
    return z;
}


template<std::size_t M, class T, class R, class A, std::size_t N, class U, class S, class B>
constexpr auto inner(const math::VecVal<M, T, R, A>& x, const math::MatVal<M, N, U, S, B>& y) ->
    math::Vec<N, math::scalar_arith_type<T, U>>
{
    math::Vec<N, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < N; ++i)
        z[i] = math::inner(x, y.col(i)); // Throws
    return z;
}


template<std::size_t M, std::size_t N, class T, class R, class A, std::size_t O, class U, class S, class B>
constexpr auto inner(const math::MatVal<M, N, T, R, A>& x, const math::MatVal<N, O, U, S, B>& y) ->
    math::Mat<M, O, math::scalar_arith_type<T, U>>
{
    math::Mat<M, O, math::scalar_arith_type<T, U>> z; // Throws
    for (std::size_t i = 0; i < M; ++i)
        z[i] = math::inner(x.row(i), y); // Throws
    return z;
}


template<std::size_t N, class T> constexpr auto ident() -> math::Mat<N, N, T>
{
    return math::gen_mat<N, N, T>([](std::size_t i, std::size_t j) {
        return (i == j ? 1 : 0);
    }); // Throws
}


template<std::size_t M, std::size_t N, class T, std::size_t P, std::size_t Q, class U, class R, class A>
constexpr auto extend(const math::MatVal<P, Q, U, R, A>& x) -> math::Mat<M, N, T>
{
    math::Mat<M, N, T> y; // Throws
    y.template sub<P, Q>() = x; // Throws
    return y;
}


template<std::size_t M, std::size_t N, class T, class F>
constexpr auto gen_mat(F&& func) -> math::Mat<M, N, T>
{
    math::Mat<M, N, T> x; // Throws
    for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t j = 0; j < N; ++j)
            x[i][j] = func(i, j); // Throws
    }
    return x;
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MAT_OPS_HPP
