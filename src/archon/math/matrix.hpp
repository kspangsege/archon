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

#ifndef ARCHON_X_MATH_X_MATRIX_HPP
#define ARCHON_X_MATH_X_MATRIX_HPP

/// \file


#include <algorithm>
#include <array>
#include <ostream>

#include <archon/core/as_list.hpp>
#include <archon/math/type_traits.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix_base.hpp>


namespace archon::math {


/// \brief Two-dimensional array of components.
///
/// This class models the mathematical concept of a matrix as an array of components with \p
/// M rows and \p N columns. A matrix is also a tensor of order 2.
///
/// Note that some constructors are inherited from \ref math::MatrixBase1 and others from
/// specializations of \ref math::MatrixBase2. A matrix constructed from one row vector gets
/// all its rows set equal to that vector. A matrix constructed from an array of row vectors
/// gets its rows set equal to those of the array. Due to the specializations of \ref
/// math::MatrixBase2 a 2-by-N matrix can be constructed from two row vectors specified as
/// two separate arguments. Likewise for 3-by-N and 3-by-N matrices.
///
/// The rows of a matrix can be accessed as an array of row vectors. See \ref
/// math::MatrixBase1::rows().
///
/// Matrices are comparable. Comparison is lexicographical in terms of the rows of the
/// matrices, when the rows are considered as vectors.
///
/// Matrices can be formatted (written to an output stream). A 2-by-2 matrix with rows [1.5,
/// 2.5] and [3.5, 4.5] is formatted as `[[1.5, 2.5], [3.5, 4.5]]`. Matrices will be
/// formatted with numeric locale facets reverted to those of the classic locale (as if by
/// \ref core::with_reverted_numerics()). This is in order to avoid ambiguity on the meaning
/// of commas.
///
/// \sa \ref operator<<(std::basic_ostream<C, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref operator+(const math::Matrix<M, N, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref operator-(const math::Matrix<M, N, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref operator-(const math::Matrix<M, N, T>&)
/// \sa \ref operator*(const math::Matrix<M, N, T>&, U)
/// \sa \ref operator*(T, const math::Matrix<M, N, U>&)
/// \sa \ref operator*(const math::Matrix<M, N, T>&, const math::Vector<N, U>&)
/// \sa \ref operator*(const math::Vector<M, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref operator*(const math::Matrix<M, N, T>&, const math::Matrix<N, P, U>&)
/// \sa \ref operator/(const math::Matrix<M, N, T>&, U)
/// \sa \ref operator/(T, const math::Matrix<N, N, U>&)
/// \sa \ref operator/(const math::Matrix<M, N, T>&, const math::Matrix<N, N, U>&)
/// \sa \ref transpose(const math::Matrix<M, N, T>&)
/// \sa \ref tr(const math::Matrix<N, N, T>&)
/// \sa \ref det(const math::Matrix<N, N, T>&)
/// \sa \ref inv(const math::Matrix<N, N, T>&)
/// \sa \ref outer(const math::Vector<M, T>&, const math::Vector<N, U>&)
/// \sa \ref outer(const math::Matrix<M, N, T>&, U)
/// \sa \ref outer(T, const math::Matrix<M, N, U>&)
/// \sa \ref inner(const math::Matrix<M, N, T>&, const math::Vector<N, U>&)
/// \sa \ref inner(const math::Vector<M, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref inner(const math::Matrix<M, N, T>&, const math::Matrix<N, P, U>&)
/// \sa \ref extend(const math::Matrix<P, Q, U>&, int, int)
/// \sa \ref math::try_inv()
/// \sa \ref math::try_lower_tri_inv()
/// \sa \ref math::try_upper_tri_inv()
/// \sa \ref math::decompose()
///
template<int M, int N = M, class T = double> class Matrix
    : public math::MatrixBase2<M, N, T> {
public:
    using comp_type = T;

    static constexpr int num_rows = M;
    static constexpr int num_cols = N;

    using row_type = math::Vector<N, T>;
    using col_type = math::Vector<M, T>;
    using diag_type = math::Vector<std::min(M, N), T>;

    using math::MatrixBase2<M, N, T>::MatrixBase2;

    /// \brief Identity matrix.
    ///
    /// This function returns a matrix with where all components in the main diagonal are 1,
    /// and all other components are zero. When the matrix is square, this is the identity
    /// matrix.
    ///
    static constexpr auto identity() noexcept -> Matrix;

    /// \{
    ///
    /// \brief Access specific row of matrix.
    ///
    /// These subscription operators provide access to the row at the specified index.
    ///
    constexpr auto operator[](int i) noexcept       -> row_type&;
    constexpr auto operator[](int i) const noexcept -> const row_type&;
    /// \}

    /// \{
    ///
    /// \brief Arithmetic compound assignment.
    ///
    /// If `mat` is an M-by-N matrix and `mat = mat + other` is a valid expression, then
    /// `mat += other` has the same effect as `mat = mat + other`. Likewise for operators
    /// `-=`, `*=`, and `/=`.
    ///
    /// For operators `+=` and `-=`, `other` must be an M-by-N matrix.
    ///
    /// For operator `*=`, `other` can be a scalar (outer product), a vector with \p N
    /// components (inner product), or a matrix with \p N rows (inner product).
    ///
    /// For operator `/=`, `other` can be a scalar (outer product with reciprocal of scalar)
    /// or a square matrix with N columns (and rows) (inner product with inverse of matrix).
    ///
    template<class O> constexpr auto operator+=(const O& other) noexcept -> Matrix&;
    template<class O> constexpr auto operator-=(const O& other) noexcept -> Matrix&;
    template<class O> constexpr auto operator*=(const O& other) noexcept -> Matrix&;
    template<class O> constexpr auto operator/=(const O& other) noexcept -> Matrix&;
    /// \}

    /// \{
    ///
    /// \brief Matrix comparison.
    ///
    /// These comparison operators compare this matrix with the specified matrix. Comparison
    /// is lexicographical in terms of the rows of the two matrices, when the rows are
    /// considered as vectors.
    ///
    template<class U> constexpr bool operator==(const Matrix<M, N, U>&) const;
    template<class U> constexpr bool operator!=(const Matrix<M, N, U>&) const;
    template<class U> constexpr bool operator< (const Matrix<M, N, U>&) const;
    template<class U> constexpr bool operator<=(const Matrix<M, N, U>&) const;
    template<class U> constexpr bool operator> (const Matrix<M, N, U>&) const;
    template<class U> constexpr bool operator>=(const Matrix<M, N, U>&) const;
    /// \}

    /// \brief Convert between component types.
    ///
    /// This conversion operator allows for conversion between different numeric component
    /// types. Note that the conversion operation must be explicit unless the component
    /// conversion is lossless (see \ref math::is_lossless_conv).
    ///
    template<class U> explicit(!math::is_lossless_conv<T, U>) constexpr operator Matrix<M, N, U>() const noexcept;

    /// \brief Get column as vector.
    ///
    /// This function returns the column at the specified index (\p i) as an M-vector.
    ///
    constexpr auto get_col(int i) const noexcept -> col_type;

    /// \brief Set column from vector.
    ///
    /// This function sets the column at the specified index (\p i) equal to the specified
    /// M-vector (\p col).
    ///
    constexpr auto set_col(int i, col_type col) noexcept -> Matrix&;

    /// \brief Get main diagonal as vector.
    ///
    /// This function returns the main diagonal as an P-vector, where P is the minimum of \p
    /// M and \p N.
    ///
    constexpr auto get_diag() const noexcept -> diag_type;

    /// \brief Set main diagonal from vector.
    ///
    /// This function sets the main diagonal equal to the specified P-vector (\p diag),
    /// where P is the minimum of \p M and \p N.
    ///
    constexpr auto set_diag(diag_type diag) noexcept -> Matrix&;

    /// \brief Get sub-matrix.
    ///
    /// This function returns a sub-matrix of this matrix. The sub-matrix spans \p P rows
    /// starting with the row at index \p i, and pans \p Q columns starting with the column
    /// at index \p j.
    ///
    template<int P, int Q> constexpr auto get_submatrix(int i, int j) const noexcept -> Matrix<P, Q, T>;

    /// \brief Set sub-matrix.
    ///
    /// This function sets a sub-matrix of this matrix equal to the specified matrix. The
    /// sub-matrix spans \p P rows starting with the row at index \p i, and spans \p Q
    /// columns starting with the column at index \p j.
    ///
    template<int P, int Q> constexpr auto set_submatrix(int i, int j, const Matrix<P, Q, T>& mat) noexcept -> Matrix&;

    /// \brief Generate matrix using callers function to determine components.
    ///
    /// This function generates a matrix by calling the specified function for each position
    /// in the matrix setting the component to the returned value. The specified function
    /// (\p func) will be called with two parameters of type `int`. The first parameter is
    /// the row index and the second parameter is the column index.
    ///
    template<class F> static constexpr auto generate(F func) noexcept(noexcept(T(func(int(), int())))) -> Matrix;
};


template<int M, int N = M> using MatrixF = math::Matrix<M, N, float>;
template<int M, int N = M> using MatrixL = math::Matrix<M, N, long double>;


using Matrix2 = math::Matrix<2>;
using Matrix3 = math::Matrix<3>;
using Matrix4 = math::Matrix<4>;

using Matrix2F = math::MatrixF<2>;
using Matrix3F = math::MatrixF<3>;
using Matrix4F = math::MatrixF<4>;

using Matrix2L = math::MatrixL<2>;
using Matrix3L = math::MatrixL<3>;
using Matrix4L = math::MatrixL<4>;


using Matrix2x2 = math::Matrix<2, 2>;
using Matrix2x3 = math::Matrix<2, 3>;
using Matrix2x4 = math::Matrix<2, 4>;

using Matrix3x2 = math::Matrix<3, 2>;
using Matrix3x3 = math::Matrix<3, 3>;
using Matrix3x4 = math::Matrix<3, 4>;

using Matrix4x2 = math::Matrix<4, 2>;
using Matrix4x3 = math::Matrix<4, 3>;
using Matrix4x4 = math::Matrix<4, 4>;


using Matrix2x2F = math::MatrixF<2, 2>;
using Matrix2x3F = math::MatrixF<2, 3>;
using Matrix2x4F = math::MatrixF<2, 4>;

using Matrix3x2F = math::MatrixF<3, 2>;
using Matrix3x3F = math::MatrixF<3, 3>;
using Matrix3x4F = math::MatrixF<3, 4>;

using Matrix4x2F = math::MatrixF<4, 2>;
using Matrix4x3F = math::MatrixF<4, 3>;
using Matrix4x4F = math::MatrixF<4, 4>;


using Matrix2x2L = math::MatrixL<2, 2>;
using Matrix2x3L = math::MatrixL<2, 3>;
using Matrix2x4L = math::MatrixL<2, 4>;

using Matrix3x2L = math::MatrixL<3, 2>;
using Matrix3x3L = math::MatrixL<3, 3>;
using Matrix3x4L = math::MatrixL<3, 4>;

using Matrix4x2L = math::MatrixL<4, 2>;
using Matrix4x3L = math::MatrixL<4, 3>;
using Matrix4x4L = math::MatrixL<4, 4>;




/// \brief Write textual representation of matrix to output stream.
///
/// This stream output operator writes a textual representation of the specified matrix (\p
/// mat) to the specified output stream (\p out). See \ref math::Matrix for information on
/// the format of the textual representation.
///
template<class C, class T, int M, int N, class U>
auto operator<<(std::basic_ostream<C, T>& out, const math::Matrix<M, N, U>& mat) -> std::basic_ostream<C, T>&;


/// \brief Add two matrices.
///
/// This operation constructs the sum of the two specified `M`-by-`N` matrices. The sum is
/// itself an `M`-by-`N` matrix.
///
template<int M, int N, class T, class U>
constexpr auto operator+(const math::Matrix<M, N, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Subtract two matrices.
///
/// This operation constructs the difference between the two specified `M`-by-`N`
/// matrices. The difference is itself an `M`-by-`N` matrix.
///
template<int M, int N, class T, class U>
constexpr auto operator-(const math::Matrix<M, N, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Negate matrix.
///
/// This operation constructs the additive inverse of the specified `M`-by-`N` matrix. The
/// result is also an `M`-by-`N` matrix.
///
template<int M, int N, class T> constexpr auto operator-(const math::Matrix<M, N, T>& a) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T>>;


/// \brief Multiply matrix and scalar.
///
/// This operation is the same as `math::outer(a, b)`.
///
template<int M, int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto operator*(const math::Matrix<M, N, T>& a, U b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Multiply scalar and matrix.
///
/// This operation is the same as `math::outer(a, b)`.
///
template<class T, int M, int N, class U, class = std::enable_if_t<math::is_scalar<T>>>
constexpr auto operator*(T a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Multiply matrix and vector.
///
/// This operation is the same as `math::inner(a, b)`.
///
template<int M, int N, class T, class U>
constexpr auto operator*(const math::Matrix<M, N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<M, math::scalar_arith_type<T, U>>;


/// \brief Multiply vector and matrix.
///
/// This operation is the same as `math::inner(a, b)`.
///
template<int M, class T, int N, class U>
constexpr auto operator*(const math::Vector<M, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Multiply matrices.
///
/// This operation is the same as `math::inner(a, b)`.
///
template<int M, int N, class T, int P, class U>
constexpr auto operator*(const math::Matrix<M, N, T>& a, const math::Matrix<N, P, U>& b) noexcept ->
    math::Matrix<M, P, math::scalar_arith_type<T, U>>;


/// \brief Divide matrix by scalar.
///
/// This operation constructs a new `M`-by-`N` matrix from the specified `M`-by-`N` matrix
/// (\p a). Each component in the new matrix is computed by taking the corresponding
/// component from the specified matrix, and dividing it by the specified scalar (\p b).
///
template<int M, int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto operator/(const math::Matrix<M, N, T>& a, U b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Divide scalar by square matrix.
///
/// This operation is a shorthand for `a * archon::math::inv(b)`.
///
template<class T, int N, class U, class = std::enable_if_t<math::is_scalar<T>>>
constexpr auto operator/(T a, const math::Matrix<N, N, U>& b) noexcept ->
    math::Matrix<N, N, math::scalar_arith_type<T, U>>;


/// \brief Divide matrix by square matrix.
///
/// This operation is a shorthand for `a * archon::math::inv(b)`.
///
template<int M, int N, class T, class U>
constexpr auto operator/(const math::Matrix<M, N, T>& a, const math::Matrix<N, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Transpose matrix.
///
/// This function constructs the transpose of the specified matrix.
///
template<int M, int N, class T> constexpr auto transpose(const math::Matrix<M, N, T>& a) noexcept ->
    math::Matrix<N, M, T>;


/// \brief Trace of square matrix.
///
/// This function computes the trace of the specified square matrix. The trace of a square
/// matrix is the sum of the elements on its main diagonal.
///
template<int N, class T> constexpr auto tr(const math::Matrix<N, N, T>& a) noexcept -> math::scalar_arith_type<T>;


/// \brief Determinant of square matrix.
///
/// This function computes the determinant of the specified square matrix.
///
template<int N, class T> constexpr auto det(const math::Matrix<N, N, T>& a) noexcept -> math::scalar_arith_type<T>;


/// \brief Invert square matrix.
///
/// This function computes the multiplicative inverse of the specified square matrix.
///
/// If the specified matrix is identified as singular, the result is a zero-matrix.
///
template<int N, class T> constexpr auto inv(const math::Matrix<N, N, T>& a) ->
    math::Matrix<N, N, math::scalar_arith_type<T>>;


/// \brief Outer product of vectors.
///
/// This operation computes the outer product of the specified `M`-vector (\p x) and the
/// specified `N`-vector (\p y). The result is an `M`-by-`N` matrix. In tensor terminology,
/// this is the outer product of two tensors of order 1 (the vectors).
///
template<int M, class T, int N, class U>
constexpr auto outer(const math::Vector<M, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of matrix and scalar.
///
/// This operation computes the outer product of the specified `M`-by-`N` matrix and the
/// specified scalar. The result is an `M`-by-`N` matrix. In tensor terminology, this is the
/// outer product of a tensor of order 2 (the matrix) and a tensor of order 0 (the scalar).
///
template<int M, int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto outer(const math::Matrix<M, N, T>& a, U b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of scalar and matrix.
///
/// This operation computes the outer product of the specified scalar and the specified
/// `M`-by-`N` matrix. The result is an `M`-by-`N` matrix. In tensor terminology, this is
/// the outer product of a tensor of order 0 (the scalar) and a tensor of order 2 (the
/// matrix).
///
template<class T, int M, int N, class U, class = std::enable_if_t<math::is_scalar<T>>>
constexpr auto outer(T a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of matrix and vector.
///
/// This operation constructs the inner product of the specified `M`-by-`N` matrix (\p a)
/// and the specified `N`-vector (\p b). The result is an `M`-vector. In tensor terminology,
/// this is the inner product of a tensor of order 2 (the matrix) and a tensor of order 1
/// (the vector).
///
template<int M, int N, class T, class U>
constexpr auto inner(const math::Matrix<M, N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<M, math::scalar_arith_type<T, U>>;


/// \brief Inner product of vector and matrix.
///
/// This operation constructs the inner product of the specified `M`-vector (\p a) and the
/// specified `M`-by-`N` matrix (\p b). The result is an `N`-vector. In tensor terminology,
/// this is the inner product of a tensor of order 1 (the vector) and a tensor of order 2
/// (the matrix).
///
template<int M, class T, int N, class U>
constexpr auto inner(const math::Vector<M, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of two matrices.
///
/// This operation constructs the inner product of the specified `M`-by-`N` matrix (\p a)
/// and the specified `N`-by-`P` matrix (\p b). The result is an `M`-by-`P` matrix. In
/// tensor terminology, this is the inner product of two tensors of order 2 (the matrices).
///
template<int M, int N, class T, int P, class U>
constexpr auto inner(const math::Matrix<M, N, T>& a, const math::Matrix<N, P, U>& b) noexcept ->
    math::Matrix<M, P, math::scalar_arith_type<T, U>>;


/// \brief Extend matrix.
///
/// This function constructs an extension of the specified matrix.
///
/// `math::extend<M, N, T>(mat, i, j)` is shorthand for constructing a default-initialized
/// matrix of type `math::Matrix<M, N, T>`, and then calling \ref
/// math::Matrix::set_submatrix() on it, passing `i`, `j`, and `mat` as arguments in that
/// order.
///
template<int M, int N, class T = double, int P, int Q, class U>
constexpr auto extend(const math::Matrix<P, Q, U>& mat, int i, int j) -> math::Matrix<M, N, T>;


/// \brief Invert square matrix.
///
/// This function attempts to invert the specified square matrix, updating it in place.
///
/// If the matrix is identified as singular, this function returns `false` and leaves the
/// matrix in an unspecified state. Otherwise this function returns `true`.
///
template<int N, class T> constexpr bool try_inv(math::Matrix<N, N, T>&) noexcept;


/// \brief Invert lower triangular matrix.
///
/// This function attempts to invert the specified matrix under the assumption that it is a
/// lower triangular matrix. The matrix is updated in place.
///
/// If the matrix is identified as singular, this function returns `false` and leaves the
/// matrix in an unspecified state. Otherwise this function returns `true`.
///
/// If \p assume_unitri is `true` (assume matrix is unitriangular), the matrix cannot be
/// singular. This function will always return `true` in that case.
///
/// \note Elements above the diagonal will not be read, nor will they be modified, so they
/// do not actually have to be zero.
///
/// \note If \p assume_unitri is `true`, all diagonal elements are assumed to be 1. In this
/// case, the actual diagonal elements will not be read, nor will they be modified, so they
/// do not actually have to be 1.
///
template<bool assume_unitri, int N, class T> constexpr bool try_lower_tri_inv(math::Matrix<N, N, T>&) noexcept;


/// \brief Invert upper triangular matrix.
///
/// This function attempts to invert the specified matrix under the assumption that it is an
/// upper triangular matrix. The matrix is updated in place.
///
/// This function is the diagonally mirrored version of \ref math::try_lower_tri_inv(). It
/// could be computed by first transposing the matrix, then calling
/// `math::try_lower_tri_inv()`, and then transposing again.
///
template<bool assume_unitri, int N, class T> constexpr bool try_upper_tri_inv(math::Matrix<N, N, T>&) noexcept;


/// \brief LU decomposition with partial pivoting.
///
/// This function performs LU decomposition with partial pivoting (LUP decomposition).
///
/// If A is a square matrix, LU decomposition is a factorization of A into a lower
/// unitriangular matrix, L, an upper triangular matrix, U, and a permutation matrix P, such
/// that A = P * L * U.
///
/// The same is true if A is not a square matrix, except that in this case, either L or U is
/// a trapezoidal matrix rather than a triangular matrix.
///
/// In general, if A is an M x N matrix, then L is an M x min(M,N) matrix, U is a min(M,N) x
/// N matrix, and P is an M x M matrix. Thus, when A is 'high', L will be 'high' too and
/// when A is 'wide', U will be 'wide'. Here is a 'high' example with M = 5 and N = 3:
///
///     [ A11 A12 A13 ]       [  1          ]
///     [ A21 A22 A23 ]       [ L21  1      ]   [ U11 U12 U13 ]
///     [ A31 A32 A33 ] = P * [ L31 L32  1  ] * [     U22 U23 ]
///     [ A41 A42 A43 ]       [ L41 L42 L43 ]   [         U33 ]
///     [ A51 A52 A53 ]       [ L51 L52 L53 ]
///
/// Upon return, this matrix contains both L and U. For a 'high' matrix, this is done by
/// removing the unit diagonal of L and then placing U in the vacated upper triangle of
/// L. According to the example above, we get:
///
///         [ U11 U12 U13 ]
///         [ L21 U22 U23 ]
///     B = [ L31 L32 U33 ]
///         [ L41 L42 L43 ]
///         [ L51 L52 L53 ]
///
/// Here, B is the value of the target matrix upon execution of `decompose()`. In general we
/// have:
///
///               {  U(i,j)   if i <= j
///     B(i,j) =  {                       for i < M and j < N
///               {  L(i,j)   otherwise
///
/// The permutation matrix is given indirectly as a series of min(M,N) transpositions stored
/// in the returned array. The returned array reflects the row transpositions that were
/// carried out in the decomposition process. If `pivots` is the returned array, the
/// permutation matrix, P, can be derived as follows:
///
///                  {  k           if i = pivots[k]
///     perm_k(i) =  {  pivots[k]   if i = k           for i < M
///                  {  i           otherwise
///
///                 {  1   if perm_k(j) = i
///     P_k(i,j) =  {                         for i < M and j < M
///                 {  0   otherwise
///
///     P = P_0 * P_2 * P_3 * .... * P_(min(M,N)-1)
///
/// Where `perm_k` and `P_k` are the permutation function and permutation matrix,
/// respectively, that correspond to the transposition at `pivots[k]`.
///
/// \note This method works for both regular and singular, and for both square and
/// non-square matrices.
///
template<int M, int N, class T> constexpr auto decompose(math::Matrix<M, N, T>&) noexcept ->
    std::array<int, std::min(M, N)>;








// Implementation


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::identity() noexcept -> Matrix
{
    return generate([](int i, int j) noexcept { return (i == j ? 1 : 0); });
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::operator[](int i) noexcept -> row_type&
{
    return this->rows()[i];
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::operator[](int i) const noexcept -> const row_type&
{
    return this->rows()[i];
}


template<int M, int N, class T>
template<class O> constexpr auto Matrix<M, N, T>::operator+=(const O& other) noexcept -> Matrix&
{
    return (*this = *this + other);
}


template<int M, int N, class T>
template<class O> constexpr auto Matrix<M, N, T>::operator-=(const O& other) noexcept -> Matrix&
{
    return (*this = *this - other);
}


template<int M, int N, class T>
template<class O> constexpr auto Matrix<M, N, T>::operator*=(const O& other) noexcept -> Matrix&
{
    return (*this = *this * other);
}


template<int M, int N, class T>
template<class O> constexpr auto Matrix<M, N, T>::operator/=(const O& other) noexcept -> Matrix&
{
    return (*this = *this / other);
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator==(const Matrix<M, N, U>& other) const
{
    auto& a = this->rows();
    auto& b = other.rows();
    return std::equal(a.begin(), a.end(), b.begin());
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator!=(const Matrix<M, N, U>& other) const
{
    return !(*this == other);
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator<(const Matrix<M, N, U>& other) const
{
    auto& a = this->rows();
    auto& b = other.rows();
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator<=(const Matrix<M, N, U>& other) const
{
    return !(other < *this);
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator>(const Matrix<M, N, U>& other) const
{
    return (other < *this);
}


template<int M, int N, class T>
template<class U> constexpr bool Matrix<M, N, T>::operator>=(const Matrix<M, N, U>& other) const
{
    return !(*this < other);
}


template<int M, int N, class T>
template<class U> constexpr Matrix<M, N, T>::operator Matrix<M, N, U>() const noexcept
{
    return Matrix<M, N, U>(this->rows());
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::get_col(int i) const noexcept -> col_type
{
    col_type col;
    for (int j = 0; j < M; ++j)
        col[j] = (*this)[j][i];
    return col;
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::set_col(int i, col_type col) noexcept -> Matrix&
{
    for (int j = 0; j < M; ++j)
        (*this)[j][i] = col[j];
    return *this;
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::get_diag() const noexcept -> diag_type
{
    diag_type diag;
    for (int i = 0; i < std::min(M, N); ++i)
        diag[i] = (*this)[i][i];
    return diag;
}


template<int M, int N, class T>
constexpr auto Matrix<M, N, T>::set_diag(diag_type diag) noexcept -> Matrix&
{
    for (int i = 0; i < std::min(M, N); ++i)
        (*this)[i][i] = diag[i];
    return *this;
}


template<int M, int N, class T>
template<int P, int Q> constexpr auto Matrix<M, N, T>::get_submatrix(int i, int j) const noexcept -> Matrix<P, Q, T>
{
    math::Matrix<P, Q, T> mat;
    for (int k = 0; k < P; ++k) {
        for (int l = 0; l < Q; ++l)
            mat[k][l] = (*this)[i + k][j + l];
    }
    return mat;
}


template<int M, int N, class T>
template<int P, int Q>
constexpr auto Matrix<M, N, T>::set_submatrix(int i, int j, const Matrix<P, Q, T>& mat) noexcept -> Matrix&
{
    for (int k = 0; k < P; ++k) {
        for (int l = 0; l < Q; ++l)
            (*this)[i + k][j + l] = mat[k][l];
    }
    return *this;
}


template<int M, int N, class T>
template<class F> constexpr auto Matrix<M, N, T>::generate(F func) noexcept(noexcept(T(func(int(), int())))) -> Matrix
{
    Matrix mat;
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j)
            mat[i][j] = T(func(i, j)); // Throws
    }
    return mat;
}


template<class C, class T, int M, int N, class U>
auto operator<<(std::basic_ostream<C, T>& out, const math::Matrix<M, N, U>& mat) -> std::basic_ostream<C, T>&
{
    return out << core::as_sbr_list(mat.rows()); // Throws
}


template<int M, int N, class T, class U>
constexpr auto operator+(const math::Matrix<M, N, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = a[i] + b[i];
    return c;
}


template<int M, int N, class T, class U>
constexpr auto operator-(const math::Matrix<M, N, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = a[i] - b[i];
    return c;
}


template<int M, int N, class T> constexpr auto operator-(const math::Matrix<M, N, T>& a) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T>>
{
    math::Matrix<M, N, math::scalar_arith_type<T>> b;
    for (int i = 0; i < M; ++i)
        b[i] = -a[i];
    return b;
}


template<int M, int N, class T, class U, class>
constexpr auto operator*(const math::Matrix<M, N, T>& a, U b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    return math::outer(a, b);
}


template<class T, int M, int N, class U, class>
constexpr auto operator*(T a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    return math::outer(a, b);
}


template<int M, int N, class T, class U>
constexpr auto operator*(const math::Matrix<M, N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<M, math::scalar_arith_type<T, U>>
{
    return math::inner(a, b);
}


template<int M, class T, int N, class U>
constexpr auto operator*(const math::Vector<M, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>
{
    return math::inner(a, b);
}


template<int M, int N, class T, int P, class U>
constexpr auto operator*(const math::Matrix<M, N, T>& a, const math::Matrix<N, P, U>& b) noexcept ->
    math::Matrix<M, P, math::scalar_arith_type<T, U>>
{
    return math::inner(a, b);
}


template<int M, int N, class T, class U, class>
constexpr auto operator/(const math::Matrix<M, N, T>& a, U b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = a[i] / b;
    return c;
}


template<class T, int N, class U, class> constexpr auto operator/(T a, const math::Matrix<N, N, U>& b) noexcept ->
    math::Matrix<N, N, math::scalar_arith_type<T, U>>
{
    return a * math::inv(b);
}


template<int M, int N, class T, class U>
constexpr auto operator/(const math::Matrix<M, N, T>& a, const math::Matrix<N, N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    return a * math::inv(b);
}


template<int M, int N, class T>
constexpr auto transpose(const math::Matrix<M, N, T>& a) noexcept -> math::Matrix<N, M, T>
{
    math::Matrix<N, M, T> b;
    for (int i = 0; i < M; ++i)
        b.set_col(i, a[i]);
    return b;
}


template<int N, class T> constexpr auto tr(const math::Matrix<N, N, T>& a) noexcept -> math::scalar_arith_type<T>
{
    return math::sum(a.get_diag());
}


template<int N, class T> constexpr auto det(const math::Matrix<N, N, T>& a) noexcept -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    math::Matrix<N, N, type> b = a;
    std::array<int, N> pivots = math::decompose(b);
    int n = 0;
    for (int i = 0; i < N; ++i) {
        if (pivots[i] != i)
            ++n;
    }
    type c = 1;
    for (int i = 0; i < N; ++i)
        c *= b[i][i];
    return (n % 2 == 0 ? c : -c);
}


template<int N, class T>
constexpr auto inv(const math::Matrix<N, N, T>& a) -> math::Matrix<N, N, math::scalar_arith_type<T>>
{
    using type = math::scalar_arith_type<T>;
    math::Matrix<N, N, type> b = a;
    if (ARCHON_LIKELY(math::try_inv(b)))
        return b;
    return {};
}


template<int M, class T, int N, class U>
constexpr auto outer(const math::Vector<M, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = math::outer(a[i], b);
    return c;
}


template<int M, int N, class T, class U, class>
constexpr auto outer(const math::Matrix<M, N, T>& a, U b) noexcept -> math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = math::outer(a[i], b);
    return c;
}


template<class T, int M, int N, class U, class>
constexpr auto outer(T a, const math::Matrix<M, N, U>& b) noexcept -> math::Matrix<M, N, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = math::outer(a, b[i]);
    return c;
}


template<int M, int N, class T, class U>
constexpr auto inner(const math::Matrix<M, N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<M, math::scalar_arith_type<T, U>>
{
    math::Vector<M, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = math::inner(a[i], b);
    return c;
}


template<int M, class T, int N, class U>
constexpr auto inner(const math::Vector<M, T>& a, const math::Matrix<M, N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = math::inner(a, b.get_col(i));
    return c;
}


template<int M, int N, class T, int P, class U>
constexpr auto inner(const math::Matrix<M, N, T>& a, const math::Matrix<N, P, U>& b) noexcept ->
    math::Matrix<M, P, math::scalar_arith_type<T, U>>
{
    math::Matrix<M, P, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < M; ++i)
        c[i] = math::inner(a[i], b);
    return c;
}


template<int M, int N, class T, int P, int Q, class U>
constexpr auto extend(const math::Matrix<P, Q, U>& mat, int i, int j) -> math::Matrix<M, N, T>
{
    math::Matrix<M, N, T> mat_2;
    mat_2.set_submatrix(i, j, mat);
    return mat_2;
}


template<int N, class T> constexpr bool try_inv(math::Matrix<N, N, T>& mat) noexcept
{
    std::array<int, N> pivots = math::decompose(mat);

    constexpr bool assume_unitri = false;
    bool regular = math::try_upper_tri_inv<assume_unitri>(mat);
    if (ARCHON_LIKELY(regular)) {
        // Solve the equation inv(mat) * lower = inv(upper) for inv(mat).
        for (int j = N - 1; j > 0; --j) {
            math::Vector col = mat.get_col(j - 1);
            for (int i = 0; i < N; ++i) {
                T sum = 0;
                for (int k = j; k < N; ++k)
                    sum += mat[i][k] * col[k];
                if (ARCHON_LIKELY(i >= j)) {
                    mat[i][j - 1] = -sum;
                }
                else {
                    mat[i][j - 1] -= sum;
                }
            }
        }

        // Apply column transpositions to undo the effect of the pivoting done by the
        // LU-decomposition.
        for (int j = N - 1; j > 0; --j) {
            int p = pivots[j - 1];
            if (p != j - 1) {
                math::Vector col = mat.get_col(j - 1);
                mat.set_col(j - 1, mat.get_col(p));
                mat.set_col(p, col);
            }
        }

        return true;
    }

    return false;
}


template<bool assume_unitri, int N, class T> constexpr bool try_lower_tri_inv(math::Matrix<N, N, T>& mat) noexcept
{
    for (int i = 0; i < N; ++i) {
        T x = -1;
        if constexpr (!assume_unitri) {
            x = mat[i][i];
            if (x == 0)
                return false;
            x = T(1) / x;
            mat[i][i] = x;
            x = -x;
        }
        for (int j = 0; j < i; ++j) {
            T y = mat[i][j];
            if constexpr (!assume_unitri)
                y *= mat[j][j];
            for (int k = j + 1; k < i; ++k)
                y += mat[k][j] * mat[i][k];
            mat[i][j] = x * y;
        }
    }
    return true;
}


template<bool assume_unitri, int N, class T> constexpr bool try_upper_tri_inv(math::Matrix<N, N, T>& mat) noexcept
{
    for (int j = 0; j < N; ++j) {
        T x = -1;
        if constexpr (!assume_unitri) {
            x = mat[j][j];
            if (x == 0)
                return false;
            x = T(1) / x;
            mat[j][j] = x;
            x = -x;
        }
        for (int i = 0; i < j; ++i) {
            T y = mat[i][j];
            if constexpr (!assume_unitri)
                y *= mat[i][i];
            for (int k = i + 1; k < j; ++k)
                y += mat[i][k] * mat[k][j];
            mat[i][j] = x * y;
        }
    }
    return true;
}


template<int M, int N, class T>
constexpr auto decompose(math::Matrix<M, N, T>& mat) noexcept -> std::array<int, std::min(M, N)>
{
    constexpr int n = std::min(M, N);
    std::array<int, n> pivots = {};

    for (int j = 0; j < n; ++j) {
        // Find pivot
        int p = j;
        T v = core::abs(mat[p][j]);
        for (int i = j + 1; i < M; ++i) {
            T w = core::abs(mat[i][j]);
            if (v < w) {
                p = i;
                v = w;
            }
        }
        pivots[j] = p;

        if (v != 0) {
            // Apply pivot
            math::Vector row = mat[j];
            mat[j] = mat[p];
            mat[p] = row;

            // Compute elements J+1:M of J-th column.
            if (j < M - 1) {
                v = T(1) / mat[j][j];
                for (int i = j + 1; i < M; ++i)
                    mat[i][j] *= v;
            }
        }

        // Update trailing sub-matrix.
        if (j < N - 1) {
            for (int i = j + 1; i < M; ++i) {
                T a = mat[i][j];
                for (int k = j + 1; k < N; ++k)
                    mat[i][k] -= a * mat[j][k];
            }
        }
    }

    return pivots;
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MATRIX_HPP
