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

#ifndef ARCHON_X_MATH_X_MATRIX_BASE_HPP
#define ARCHON_X_MATH_X_MATRIX_BASE_HPP

/// \file


#include <type_traits>
#include <array>

#include <archon/math/vector.hpp>


namespace archon::math {


/// \brief Indirect base of matrix class.
///
/// This is an indirect base class of \ref math::Matrix and provides the platform on top of
/// which the specializations of \ref math::MatrixBase2 are built.
///
template<int M, int N, class T> class MatrixBase1 {
public:
    using comp_type = T;

    static_assert(std::is_floating_point_v<T>);
    static_assert(M > 0);
    static_assert(N > 0);

    using row_type = math::Vector<N, T>;
    using array_type = std::array<row_type, M>;

    /// \brief Construct matrix with all components set to zero.
    ///
    /// This is the default constructor. A default constructed matrix has all its components
    /// set to zero.
    ///
    constexpr MatrixBase1() noexcept = default;

    /// \brief Construct matrix with all rows set to specific row vector.
    ///
    /// This constructor sets all the rows of the matrix equal to the specified row vector.
    ///
    constexpr MatrixBase1(const row_type&) noexcept;

    /// \{
    ///
    /// \brief Construct matrix from array of row vectors.
    ///
    /// These constructors set the rows of the matrix equal to the row vectors of the
    /// specified array.
    ///
    template<class R> constexpr MatrixBase1(R (& rows)[M]) noexcept;
    template<class R> constexpr MatrixBase1(const std::array<R, M>& rows) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Access rows of matrix as array of vectors.
    ///
    /// These functions expose the rows of the matrix as an array of row vectors.
    ///
    constexpr auto rows() noexcept       -> array_type&;
    constexpr auto rows() const noexcept -> const array_type&;
    /// \}

private:
    array_type m_rows = {};
};



/// \brief Direct base of matrix class.
///
/// This is a direct base class of \ref math::Matrix and exists to allow for specializations
/// to provide additional convenience constructors.
///
/// \sa \ref MatrixBase2<2, T>
/// \sa \ref MatrixBase2<3, T>
/// \sa \ref MatrixBase2<4, T>
///
template<int M, int N, class T> class MatrixBase2
    : public math::MatrixBase1<M, N, T> {
public:
    using math::MatrixBase1<M, N, T>::MatrixBase1;
};



/// \brief Direct base of matrix class for matrices with two rows.
///
/// This is a specialization of the direct base class of \ref math::Matrix for 2-by-N
/// matrices.
///
template<int N, class T> class MatrixBase2<2, N, T>
    : public math::MatrixBase1<2, N, T> {
public:
    using row_type = math::Vector<N, T>;

    using math::MatrixBase1<2, N, T>::MatrixBase1;

    /// \brief Construct 2-by-N matrix from two row vectors.
    ///
    /// This constructor sets the two rows of the matrix equal to the two specified row
    /// vectors.
    ///
    constexpr MatrixBase2(const row_type&, const row_type&) noexcept;
};



/// \brief Direct base of matrix class for matrices with three rows.
///
/// This is a specialization of the direct base class of \ref math::Matrix for 3-by-N
/// matrices.
///
template<int N, class T> class MatrixBase2<3, N, T>
    : public math::MatrixBase1<3, N, T> {
public:
    using row_type = math::Vector<N, T>;

    using math::MatrixBase1<3, N, T>::MatrixBase1;

    /// \brief Construct 3-by-N matrix from three row vectors.
    ///
    /// This constructor sets the three rows of the matrix equal to the three specified row
    /// vectors.
    ///
    constexpr MatrixBase2(const row_type&, const row_type&, const row_type&) noexcept;
};



/// \brief Direct base of matrix class for matrices with four rows.
///
/// This is a specialization of the direct base class of \ref math::Matrix for 4-by-N
/// matrices.
///
template<int N, class T> class MatrixBase2<4, N, T>
    : public math::MatrixBase1<4, N, T> {
public:
    using row_type = math::Vector<N, T>;

    using math::MatrixBase1<4, N, T>::MatrixBase1;

    /// \brief Construct 4-by-N matrix from four row vectors.
    ///
    /// This constructor sets the four rows of the matrix equal to the four specified row
    /// vectors.
    ///
    constexpr MatrixBase2(const row_type&, const row_type&, const row_type&, const row_type&) noexcept;
};








// Implementation


// ============================ MatrixBase1 ============================


template<int M, int N, class T>
constexpr MatrixBase1<M, N, T>::MatrixBase1(const row_type& row) noexcept
{
    for (int i = 0; i < M; ++i)
        m_rows[i] = row;
}


template<int M, int N, class T>
template<class R> constexpr MatrixBase1<M, N, T>::MatrixBase1(R (& rows)[M]) noexcept
{
    static_assert(noexcept(row_type(rows[int()])));
    for (int i = 0; i < M; ++i)
        m_rows[i] = row_type(rows[i]);
}


template<int M, int N, class T>
template<class R> constexpr MatrixBase1<M, N, T>::MatrixBase1(const std::array<R, M>& rows) noexcept
{
    static_assert(noexcept(row_type(rows[int()])));
    for (int i = 0; i < M; ++i)
        m_rows[i] = rows[i];
}


template<int M, int N, class T>
constexpr auto MatrixBase1<M, N, T>::rows() noexcept -> array_type&
{
    return m_rows;
}


template<int M, int N, class T>
constexpr auto MatrixBase1<M, N, T>::rows() const noexcept -> const array_type&
{
    return m_rows;
}



// ============================ MatrixBase2 ============================


template<int N, class T>
constexpr MatrixBase2<2, N, T>::MatrixBase2(const row_type& a, const row_type& b) noexcept
    : MatrixBase2(std::array { a, b })
{
}


template<int N, class T>
constexpr MatrixBase2<3, N, T>::MatrixBase2(const row_type& a, const row_type& b, const row_type& c) noexcept
    : MatrixBase2(std::array { a, b, c })
{
}


template<int N, class T>
constexpr MatrixBase2<4, N, T>::MatrixBase2(const row_type& a, const row_type& b, const row_type& c,
                                            const row_type& d) noexcept
    : MatrixBase2(std::array { a, b, c, d })
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_MATRIX_BASE_HPP
