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

#ifndef ARCHON_X_MATH_X_MAT_VAL_HPP
#define ARCHON_X_MATH_X_MAT_VAL_HPP

/// \file


#include <cstddef>
#include <utility>
#include <array>

#include <archon/core/math.hpp>
#include <archon/math/type_traits.hpp>
#include <archon/math/vec_val.hpp>
#include <archon/math/vec_var.hpp>
#include <archon/math/mat_fwd.hpp>


namespace archon::math {


template<std::size_t M, std::size_t N, class T, class R, class A> class MatVal;




template<std::size_t M, std::size_t N, class T, class R, class A> class MatVal1 {
public:
    using comp_type = T;
    using rep_type  = R;

    static_assert(math::is_valid_scalar<comp_type>);

    static constexpr std::size_t tensor_order = 2;

    static constexpr std::size_t num_rows = M;
    static constexpr std::size_t num_cols = N;

    MatVal1(const MatVal1&) = delete;

    constexpr auto operator[](std::size_t) &;
    constexpr auto operator[](std::size_t) const &;
    constexpr auto operator[](std::size_t) const &&;

    constexpr auto row(std::size_t) &;
    constexpr auto row(std::size_t) const &;
    constexpr auto row(std::size_t) const &&;

    constexpr auto col(std::size_t) &;
    constexpr auto col(std::size_t) const &;
    constexpr auto col(std::size_t) const &&;

    template<std::size_t P, std::size_t Q> constexpr auto sub() &;
    template<std::size_t P, std::size_t Q> constexpr auto sub() const &;
    template<std::size_t P, std::size_t Q> constexpr auto sub() const &&;

    constexpr auto transposed() &;
    constexpr auto transposed() const &;
    constexpr auto transposed() const &&;

    template<class B> constexpr auto operator=(const B&) -> A&;
    constexpr auto operator=(const MatVal1&) -> MatVal1&;

    template<class B> constexpr auto operator+=(const B&) -> A&;
    template<class B> constexpr auto operator-=(const B&) -> A&;
    template<class B> constexpr auto operator*=(const B&) -> A&;
    template<class B> constexpr auto operator/=(const B&) -> A&;

    template<class U, class S, class B> constexpr bool operator==(const math::MatVal1<M, N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator!=(const math::MatVal1<M, N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator< (const math::MatVal1<M, N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator<=(const math::MatVal1<M, N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator> (const math::MatVal1<M, N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator>=(const math::MatVal1<M, N, U, S, B>&) const;

    template<class U, class S, class B> constexpr auto scale_cols(const math::VecVal<N, U, S, B>&) -> A&;

    /// \brief LU decomposition with partial pivoting.
    ///
    /// This function performs LU decomposition with partial pivoting (LUP decomposition).
    ///
    /// If A is a square matrix, LU decomposition is a factorization of A into a lower
    /// unitriangular matrix, L, an upper triangular matrix, U, and a permutation matrix P,
    /// such that A = P * L * U.
    ///
    /// The same is true if A is not a square matrix, except tyhat in this case, either L or
    /// U is a trapezoidal matrix rather than a triangular matrix.
    ///
    /// In general, if A is an M x N matrix, then L is an M x min(M,N) matrix, U is a
    /// min(M,N) x N matrix, and P is an M x M matrix. Thus, when A is 'high', L will be
    /// 'high' too and when A is 'wide', U will be 'wide'. Here is a 'high' example with M =
    /// 5 and N = 3:
    ///
    ///     [ A11 A12 A13 ]       [  1          ]
    ///     [ A21 A22 A23 ]       [ L21  1      ]   [ U11 U12 U13 ]
    ///     [ A31 A32 A33 ] = P * [ L31 L32  1  ] * [     U22 U23 ]
    ///     [ A41 A42 A43 ]       [ L41 L42 L43 ]   [         U33 ]
    ///     [ A51 A52 A53 ]       [ L51 L52 L53 ]
    ///
    /// Upon return, this matrix contains both L and U. For a 'high' matrix, this is done by
    /// removing the unit diagonal of L and then placing U in the vacated upper triange of
    /// L. Accoring to the example above, we get:
    ///
    ///         [ U11 U12 U13 ]
    ///         [ L21 U22 U23 ]
    ///     B = [ L31 L32 U33 ]
    ///         [ L41 L42 L43 ]
    ///         [ L51 L52 L53 ]
    ///
    /// Here, B is the value of the target matrix upon execution of `decompose()`. In
    /// general we have
    ///
    ///               {  U(i,j)   if i <= j
    ///     B(i,j) =  {                       for i < M and j < N
    ///               {  L(i,j)   otherwise
    ///
    /// The permutation matrix is given indirectly as a series of min(M,N) transpositions
    /// stored in the returned array. The returned array reflects the row transpositions
    /// that were caried out in the decomposition process. If `pivots` is the returned
    /// array, the permutation matrix, P, can be derived as follows:
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
    constexpr auto decompose() -> std::array<std::size_t, std::min(M, N)>;

protected:
    rep_type m_rep;

    template<class... P> constexpr MatVal1(P&&... params);

    constexpr auto set(const math::Mat<M, N, T>&) -> A&;

    constexpr auto self() noexcept -> A&;
};




template<std::size_t M, std::size_t N, class T, class R, class A> class MatVal2
    : public math::MatVal1<M, N, T, R, A> {
public:
    using math::MatVal1<M, N, T, R, A>::operator=;

    constexpr auto operator=(const MatVal2&) -> MatVal2& = default;

protected:
    using math::MatVal1<M, N, T, R, A>::MatVal1;
};


template<std::size_t N, class T, class R, class A> class MatVal2<N, N, T, R, A>
    : public math::MatVal1<N, N, T, R, A> {
public:
    using math::MatVal1<N, N, T, R, A>::operator=;

    constexpr auto operator=(const MatVal2&) -> MatVal2& = default;

    constexpr auto diag() &;
    constexpr auto diag() const &;
    constexpr auto diag() const &&;

    /// \brief Transpose square matrix.
    ///
    /// This function transposes the target matrix, which is a square matrix. Effectively,
    /// elements at (i, j) and (j, i) are swapped.
    ///
    /// \sa \ref math::transpose().
    ///
    constexpr void transpose();

    /// \brief Invert square matrix.
    ///
    /// This function attempts to invert the target matrix, which is a square matrix.
    ///
    /// If the target matrix is identified as singular, this function returns `false` and
    /// leaves the target matrix in an unspecified state. Otherwise this function returns
    /// `true`.
    ///
    constexpr bool try_inv();

    /// \brief Invert lower triangular matrix.
    ///
    /// This function attempts to invert the target matrix under the assumption that the
    /// target matrix is lower triangular.
    ///
    /// If the target matrix is identified as singular, this function returns `false` and
    /// leaves the target matrix in an unspecified state. Otherwise this function returns
    /// `true`.
    ///
    /// If \p assume_unitri is `true` (assume target matrix is unitriangular), the target
    /// matrix cannot be singular. This function will always return `true` in that case.
    ///
    /// NOTE: Elements above the diagonal will not be read, nor will they be modified, so
    /// they do not actually have to be zero.
    ///
    /// NOTE: If \p assume_unitri is `true`, all diagonal elements are assumed to be 1. In
    /// this case, the actual diagonal elements will not be read, nor will they be modified,
    /// so they do not actually have to be 1.
    ///
    /// An upper triangulat matrix, `u`, can be inverted using
    /// `u.transposed().try_lower_tri_inv()`.
    ///
    template<bool assume_unitri> constexpr bool try_lower_tri_inv();

protected:
    using math::MatVal1<N, N, T, R, A>::MatVal1;
};




template<std::size_t M, std::size_t N, class T, class R, class A> class MatVal
    : public math::MatVal2<M, N, T, R, A> {
public:
    using math::MatVal2<M, N, T, R, A>::operator=;

    constexpr auto operator=(const MatVal&) -> MatVal& = default;

protected:
    using math::MatVal2<M, N, T, R, A>::MatVal2;
};








// Implementation


// ============================ MatVal1 ============================


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::operator[](std::size_t i) &
{
    return row(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::operator[](std::size_t i) const &
{
    return row(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::operator[](std::size_t i) const &&
{
    return static_cast<const MatVal1&&>(*this).row(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::row(std::size_t i) &
{
    return m_rep.row(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::row(std::size_t i) const &
{
    return m_rep.row(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::row(std::size_t i) const &&
{
    // Copy to avoid dangling reference
    return math::Vec(m_rep.row(i)); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::col(std::size_t i) &
{
    return m_rep.col(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::col(std::size_t i) const &
{
    return m_rep.col(i); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::col(std::size_t i) const &&
{
    // Copy to avoid dangling reference
    return math::Vec(m_rep.col(i)); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<std::size_t P, std::size_t Q> constexpr auto MatVal1<M, N, T, R, A>::sub() &
{
    return m_rep.template sub<P, Q>(); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<std::size_t P, std::size_t Q> constexpr auto MatVal1<M, N, T, R, A>::sub() const &
{
    return m_rep.template sub<P, Q>(); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<std::size_t P, std::size_t Q> constexpr auto MatVal1<M, N, T, R, A>::sub() const &&
{
    // Copy to avoid dangling reference
    return math::Mat(m_rep.template sub<P, Q>()); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::transposed() &
{
    return m_rep.transposed(); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::transposed() const &
{
    return m_rep.transposed(); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::transposed() const &&
{
    // Copy to avoid dangling reference
    return math::Mat(m_rep.transposed()); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class B> constexpr auto MatVal1<M, N, T, R, A>::operator=(const B& x) -> A&
{
    return set(math::Mat<M, N, T>(x)); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::operator=(const MatVal1& x) -> MatVal1&
{
    return set(math::Mat<M, N, T>(x)); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class B> constexpr auto MatVal1<M, N, T, R, A>::operator+=(const B& x) -> A&
{
    return set(self() + x); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class B> constexpr auto MatVal1<M, N, T, R, A>::operator-=(const B& x) -> A&
{
    return set(self() - x); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class B> constexpr auto MatVal1<M, N, T, R, A>::operator*=(const B& x) -> A&
{
    return set(self() * x); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class B> constexpr auto MatVal1<M, N, T, R, A>::operator/=(const B& x) -> A&
{
    return set(self() / x); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator==(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    for (std::size_t i = 0; i < M; ++i) {
        if (!(a[i] == b[i])) // Throws
            return false;
    }
    return true;
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator!=(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(a == b); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator<(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    for (std::size_t i = 0; i < M; ++i) {
        if (a[i] < b[i]) // Throws
            return true;
        if (b[i] < a[i]) // Throws
            return false;
    }
    return false;
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator<=(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(b < a); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator>(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return b < a; // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool MatVal1<M, N, T, R, A>::operator>=(const math::MatVal1<M, N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(a < b); // Throws
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr auto MatVal1<M, N, T, R, A>::scale_cols(const math::VecVal<N, U, S, B>& x) -> A&
{
    // Guard against aliasing
    math::Vec<N, U> y(x); // Throws
    for (std::size_t i = 0; i < N; ++i)
        col(i) *= y[i]; // Throws
    return self();
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::decompose() -> std::array<std::size_t, std::min(M, N)>
{
    constexpr std::size_t n = std::min(M, N);
    std::array<std::size_t, n> pivots = {};

    for (std::size_t j = 0; j < n; ++j) {
        // Find pivot
        std::size_t p = j;
        T v = core::abs(row(p)[j]); // Throws
        for(std::size_t i = j + 1; i < M; ++i) {
            T w = core::abs(row(i)[j]); // Throws
            if (v < w) { // Throws
                p = i;
                v = w; // Throws
            }
        }
        pivots[j] = p;

        if (v != 0) { // Throws
            // Apply pivot
            math::Vec r = row(j); // Throws
            row(j) = row(p); // Throws
            row(p) = r; // Throws

            // Compute elements J+1:M of J-th column.
            if (j < M - 1) {
                v = T(1) / row(j)[j]; // Throws
                for (std::size_t i = j + 1; i < M; ++i)
                    row(i)[j] *= v; // Throws
            }
        }

        // Update trailing submatrix.
        if (j < N - 1) {
            for (std::size_t i = j + 1; i < M; ++i) {
                T a = row(i)[j]; // Throws
                for (std::size_t k = j + 1; k < N; ++k)
                    row(i)[k] -= a * row(j)[k]; // Throws
            }
        }
    }

    return pivots;
}


template<std::size_t M, std::size_t N, class T, class R, class A>
template<class... P> constexpr MatVal1<M, N, T, R, A>::MatVal1(P&&... params)
    : m_rep(std::forward<P>(params)...) // Throws
{
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::set(const math::Mat<M, N, T>& x) -> A&
{
    for (std::size_t i = 0; i < M; ++i)
        row(i) = x[i]; // Throws
    return self();
}


template<std::size_t M, std::size_t N, class T, class R, class A>
constexpr auto MatVal1<M, N, T, R, A>::self() noexcept -> A&
{
    return static_cast<A&>(*this);
}



// ============================ MatVal2 ============================


template<std::size_t N, class T, class R, class A>
constexpr auto MatVal2<N, N, T, R, A>::diag() &
{
    return this->m_rep.diag(); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto MatVal2<N, N, T, R, A>::diag() const &
{
    return this->m_rep.diag(); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto MatVal2<N, N, T, R, A>::diag() const &&
{
    // Copy to avoid dangling reference
    return math::Vec(this->m_rep.diag()); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr void MatVal2<N, N, T, R, A>::transpose()
{
    *this = this->transposed(); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr bool MatVal2<N, N, T, R, A>::try_inv()
{
    std::array<std::size_t, N> pivots = this->decompose(); // Throws

    constexpr bool assume_unitri = false;
    bool regular = this->transposed().template try_lower_tri_inv<assume_unitri>(); // Throws
    if (ARCHON_LIKELY(regular)) {
        // Solve the equation inv(x) * lower = inv(upper) for inv(x).
        for (std::size_t j = N - 1; j > 0; --j) {
            math::Vec<N, T> col = this->col(j - 1); // Throws
            for (std::size_t i = 0; i < N; ++i) {
                auto row = this->row(i); // Throws
                T x = 0;
                for (std::size_t k = j; k < N; ++k)
                    x += row[k] * col[k];
                if (ARCHON_LIKELY(i >= j)) {
                    row[j - 1] = -x;
                }
                else {
                    row[j - 1] -= x;
                }
            }
        }

        // Apply column transpositions to undo the effect of the pivoting done by the
        // LU-decomposition.
        for (std::size_t j = N - 1; j > 0; --j) {
            std::size_t p = pivots[j - 1];
            if (p != j - 1) {
                math::Vec x = this->col(j - 1); // Throws
                this->col(j - 1) = this->col(p); // Throws
                this->col(p) = x;
            }
        }

        return true;
    }

    return false;
}


template<std::size_t N, class T, class R, class A>
template<bool assume_unitri> constexpr bool MatVal2<N, N, T, R, A>::try_lower_tri_inv()
{
    for (std::size_t i = 0; i < N; ++i) {
        auto row = this->row(i); // Throws
        T x = -1; // Throws
        if (!assume_unitri) {
            x = row[i]; // Throws
            if (x == 0)
                return false;
            x = T(1) / x; // Throws
            row[i] = x; // Throws
            x = -x; // Throws
        }
        for (std::size_t j = 0; j < i; ++j) {
            auto col = this->col(j); // Throws
            T y = row[j]; // Throws
            if (!assume_unitri)
                y *= col[j]; // Throws
            for (std::size_t k = j + 1; k < i; ++k)
                y += col[k] * row[k]; // Throws
            row[j] = x * y; // Throws
        }
    }
    return true;
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MAT_VAL_HPP
