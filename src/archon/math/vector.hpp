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

#ifndef ARCHON_X_MATH_X_VECTOR_HPP
#define ARCHON_X_MATH_X_VECTOR_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <type_traits>
#include <algorithm>
#include <array>
#include <ostream>

#include <archon/core/math.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/math/type_traits.hpp>
#include <archon/math/vector_base.hpp>


namespace archon::math {


/// \brief Element of N-dimensional vector space.
///
/// This class models the mathematical concept of a vector as an element of an N-dimensional
/// vector space. A vector is also a tensor of order 1.
///
/// Note that some constructors are inherited from \ref math::VectorBase1 and others from
/// specializations of \ref math::VectorBase2. A vector constructed from one component value
/// gets all its components set to that value. A vector constructed from an array of
/// component values gets its components set equal to those of the array. Due to the
/// specializations of \ref math::VectorBase2 a 2-vector can be constructed from two
/// component values specified as two separate arguments. Likewise for 3-vectors and
/// 4-vectors.
///
/// The components of a vector can be accessed as an array of components. See \ref
/// math::VectorBase1::components().
///
/// Vectors are comparable. Comparison is lexicographical in terms of the components of the
/// vectors.
///
/// Vectors can be formatted (written to an output stream). A 2-vector with components 1.5
/// and 2.5 will be formatted as `[1.5, 2.5]`. Vectors will be formatted with numeric locale
/// facets reverted to those of the classic locale (as if by \ref
/// core::with_reverted_numerics()). This is in order to avoid ambiguity on the meaning of
/// commas.
///
/// \sa \ref operator<<(std::basic_ostream<C, T>&, const math::Vector<N, U>&)
/// \sa \ref operator+(const math::Vector<N, T>&, const math::Vector<N, U>&)
/// \sa \ref operator-(const math::Vector<N, T>&, const math::Vector<N, U>&)
/// \sa \ref operator-(const math::Vector<N, T>&)
/// \sa \ref operator*(const math::Vector<N, T>&, U)
/// \sa \ref operator*(T, const math::Vector<N, U>&)
/// \sa \ref operator*(const math::Vector<3, T>&, const math::Vector<3, U>&)
/// \sa \ref operator*(const math::Matrix<M, N, T>&, const math::Vector<N, U>&)
/// \sa \ref operator*(const math::Vector<M, T>&, const math::Matrix<M, N, U>&)
/// \sa \ref operator/(const math::Vector<N, T>&, U)
/// \sa \ref len(const math::Vector<N, T>&)
/// \sa \ref sum(const math::Vector<N, T>&)
/// \sa \ref sq_sum(const math::Vector<N, T>&)
/// \sa \ref dot(const math::Vector<N, T>&, const math::Vector<N, U>&)
/// \sa \ref proj(const math::Vector<N, T>&, const math::Vector<N, U>&)
/// \sa \ref perp(const math::Vector<2, T>&)
/// \sa \ref cross(const math::Vector<3, T>&, const math::Vector<3, U>&)
/// \sa \ref outer(const math::Vector<N, T>&, U)
/// \sa \ref outer(T, const math::Vector<N, U>&)
/// \sa \ref outer(const math::Vector<M, T>&, const math::Vector<N, U>&)
/// \sa \ref inner(const math::Vector<N, T>&, const math::Vector<N, U>&)
/// \sa \ref inner(const math::Matrix<M, N, T>&, const math::Vector<N, U>&)
/// \sa \ref inner(const math::Vector<M, T>&, const math::Matrix<M, N, U>&)
///
template<int N, class T = double> class Vector
    : public math::VectorBase2<N, T> {
public:
    using comp_type = T;

    static constexpr int size = N;

    using math::VectorBase2<N, T>::VectorBase2;

    /// \{
    ///
    /// \brief Access specific component of vector.
    ///
    /// These subscription operators provide access to the component at the specified index.
    ///
    constexpr auto operator[](int i) noexcept       -> comp_type&;
    constexpr auto operator[](int i) const noexcept -> const comp_type&;
    /// \}

    /// \{
    ///
    /// \brief Arithmetic compound assignment.
    ///
    /// If `vec` is an N-vector and `vec = vec + other` is a valid expression, then `vec +=
    /// other` has the same effect as `vec = vec + other`. Likewise for operators `-=`,
    /// `*=`, and `/=`.
    ///
    /// For operators `+=` and `-=`, `other` must be an N-vector.
    ///
    /// For operator `*=`, `other` can be a scalar (outer product), an N-vector (cross
    /// product), or a matrix with N rows (inner product).
    ///
    /// For operator `/=`, `other` can be a scalar (outer product with reciprocal of scalar)
    /// or a square matrix with N columns (and rows) (inner product with inverse of matrix).
    ///
    template<class O> constexpr auto operator+=(const O& other) noexcept -> Vector&;
    template<class O> constexpr auto operator-=(const O& other) noexcept -> Vector&;
    template<class O> constexpr auto operator*=(const O& other) noexcept -> Vector&;
    template<class O> constexpr auto operator/=(const O& other) noexcept -> Vector&;
    /// \}

    /// \{
    ///
    /// \brief Vector comparison.
    ///
    /// These comparison operators compare this vector with the specified vector. Comparison
    /// is lexicographical in terms of the components of the two vectors.
    ///
    template<class U> constexpr bool operator==(const Vector<N, U>&) const noexcept;
    template<class U> constexpr bool operator!=(const Vector<N, U>&) const noexcept;
    template<class U> constexpr bool operator< (const Vector<N, U>&) const noexcept;
    template<class U> constexpr bool operator<=(const Vector<N, U>&) const noexcept;
    template<class U> constexpr bool operator> (const Vector<N, U>&) const noexcept;
    template<class U> constexpr bool operator>=(const Vector<N, U>&) const noexcept;
    /// \}

    /// \brief Convert between component types.
    ///
    /// This conversion operator allows for conversion between different numeric component
    /// types. Note that the conversion operation must be explicit unless the component
    /// conversion is lossless (see \ref math::is_lossless_conv).
    ///
    template<class U> explicit(!math::is_lossless_conv<T, U>) constexpr operator Vector<N, U>() const noexcept;
};

template<std::size_t N, class T> Vector(T (&)[N]) -> Vector<N, std::remove_const_t<T>>;
template<std::size_t N, class T> Vector(const std::array<T, N>&) -> Vector<N, std::remove_const_t<T>>;


template<int N> using VectorF = math::Vector<N, float>;
template<int N> using VectorL = math::Vector<N, long double>;

using Vector2 = math::Vector<2>;
using Vector3 = math::Vector<3>;
using Vector4 = math::Vector<4>;

using Vector2F = math::VectorF<2>;
using Vector3F = math::VectorF<3>;
using Vector4F = math::VectorF<4>;

using Vector2L = math::VectorL<2>;
using Vector3L = math::VectorL<3>;
using Vector4L = math::VectorL<4>;




/// \brief Write textual representation of vector to output stream.
///
/// This stream output operator writes a textual representation of the specified vector (\p
/// vec) to the specified output stream (\p out). See \ref math::Vector for information on
/// the format of the textual representation.
///
template<class C, class T, int N, class U>
auto operator<<(std::basic_ostream<C, T>& out, const math::Vector<N, U>& vec) -> std::basic_ostream<C, T>&;


/// \brief Add two vectors.
///
/// This operation constructs the sum of the two specified `N`-vectors. The sum is itself an
/// `N`-vector.
///
template<int N, class T, class U>
constexpr auto operator+(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Subtract two vectors.
///
/// This operation constructs the difference between the two specified `N`-vectors. The
/// difference is itself an `N`-vector.
///
template<int N, class T, class U>
constexpr auto operator-(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Negate vector.
///
/// This operation constructs the additive inverse of the specified `N`-vector. The result
/// is itself an `N`-vector.
///
template<int N, class T> constexpr auto operator-(const math::Vector<N, T>& a) noexcept ->
    math::Vector<N, math::scalar_arith_type<T>>;


/// \brief Multiply vector and scalar.
///
/// This operation is the same as `math::outer(a, b)`.
///
template<int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto operator*(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Multiply scalar and vector.
///
/// This operation is the same as `math::outer(a, b)`.
///
template<class T, int N, class U, class = std::enable_if_t<math::is_scalar<T>>>
constexpr auto operator*(T a, const math::Vector<N, U>& b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Cross product of two 3-vectors.
///
/// This operation is the same as `math::cross(a, b)`.
///
template<class T, class U>
constexpr auto operator*(const math::Vector<3, T>& a, const math::Vector<3, U>& b) noexcept ->
    math::Vector<3, math::scalar_arith_type<T, U>>;


/// \brief Divide vector by scalar.
///
/// This operation constructs a new `N`-vector from the specified `N`-vector (\p a). Each
/// component in the new vector is computed by taking the corresponding component from the
/// specified vector, and dividing it by the specified scalar (\p b).
///
template<int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto operator/(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Length of vector.
///
/// This operation computes the length of the specified `N`-vector. This is the same as
/// `std::sqrt(math::sq_sum(a))`.
///
/// \sa \ref math::sq_sum().
///
/// FIXME: Make constexpr when switching to C++26
///
template<int N, class T> auto len(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>;


/// \brief Sum of vector components.
///
/// This operation computes the sum of the components of the specified `N`-vector.
///
template<int N, class T> constexpr auto sum(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>;


/// \brief Square-sum of vector components.
///
/// This operation computes the square-sum of the components of the specified
/// `N`-vector. The square-sum of a sequence of numbers is the sum of the squares of those
/// numbers.
///
/// The square-sum of the vector components is also the square of the length of the vector
/// (Pythagoras).
///
/// \sa \ref math::len().
///
template<int N, class T> constexpr auto sq_sum(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>;


/// \brief Vector dot product.
///
/// This operation is the same as `math::inner(a, b)`.
///
template<int N, class T, class U>
constexpr auto dot(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept -> math::scalar_arith_type<T, U>;


/// \brief Projection of vector on vector.
///
/// This operation constructs the projection of the first specified `N`-vector (\p a) on the
/// second specified `N`-vector (\p b).
///
/// This operation is the same as `(math::dot(a, b) / math::sq_sum(b)) * b`.
///
template<int N, class T, class U>
constexpr auto proj(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Perpendicular 2-vector.
///
/// This operation constructs the perpendicular vector of the specified 2-vector. The result
/// is another 2-vector.
///
template<class T> constexpr auto perp(const math::Vector<2, T>& a) noexcept ->
    math::Vector<2, math::scalar_arith_type<T>>;


/// \brief Cross product of two 3-vectors.
///
/// This operation constructs the cross product of the two specified 3-vectors. The result
/// is another 3-vector.
///
template<class T, class U> constexpr auto cross(const math::Vector<3, T>& a, const math::Vector<3, U>& b) noexcept ->
    math::Vector<3, math::scalar_arith_type<T, U>>;


/// \brief Outer product of vector and scalar.
///
/// This operation computes the outer product of the specified `N`-vector and the specified
/// scalar. The result is an `N`-vector. In tensor terminology, this is the outer product of
/// a tensor of order 1 (the vector) and a tensor of order 0 (the scalar).
///
template<int N, class T, class U, class = std::enable_if_t<math::is_scalar<U>>>
constexpr auto outer(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Outer product of scalar and vector.
///
/// This operation computes the outer product of the specified scalar and the specified
/// `N`-vector. The result is an `N`-vector. In tensor terminology, this is the outer
/// product of a tensor of order 0 (the scalar) and a tensor of order 1 (the vector).
///
template<class T, int N, class U, class = std::enable_if_t<math::is_scalar<T>>>
constexpr auto outer(T a, const math::Vector<N, U>& b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>;


/// \brief Inner product of two vectors.
///
/// This function computes the inner product of the two specified `N`-vectors. The result
/// is a scalar. In tensor terminology, this is the inner product of two tensors of order 1
/// (the vectors).
///
template<int N, class T, class U>
constexpr auto inner(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::scalar_arith_type<T, U>;








// Implementation


template<int N, class T>
constexpr auto Vector<N, T>::operator[](int i) noexcept -> comp_type&
{
    return this->components()[i];
}


template<int N, class T>
constexpr auto Vector<N, T>::operator[](int i) const noexcept -> const comp_type&
{
    return this->components()[i];
}


template<int N, class T>
template<class O> constexpr auto Vector<N, T>::operator+=(const O& other) noexcept -> Vector&
{
    return (*this = *this + other);
}


template<int N, class T>
template<class O> constexpr auto Vector<N, T>::operator-=(const O& other) noexcept -> Vector&
{
    return (*this = *this - other);
}


template<int N, class T>
template<class O> constexpr auto Vector<N, T>::operator*=(const O& other) noexcept -> Vector&
{
    return (*this = *this * other);
}


template<int N, class T>
template<class O> constexpr auto Vector<N, T>::operator/=(const O& other) noexcept -> Vector&
{
    return (*this = *this / other);
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator==(const Vector<N, U>& other) const noexcept
{
    auto& a = this->components();
    auto& b = other.components();
    return std::equal(a.begin(), a.end(), b.begin());
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator!=(const Vector<N, U>& other) const noexcept
{
    return !(*this == other);
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator<(const Vector<N, U>& other) const noexcept
{
    auto& a = this->components();
    auto& b = other.components();
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator<=(const Vector<N, U>& other) const noexcept
{
    return !(other < *this);
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator>(const Vector<N, U>& other) const noexcept
{
    return (other < *this);
}


template<int N, class T>
template<class U> constexpr bool Vector<N, T>::operator>=(const Vector<N, U>& other) const noexcept
{
    return !(*this < other);
}


template<int N, class T>
template<class U> constexpr Vector<N, T>::operator Vector<N, U>() const noexcept
{
    return Vector<N, U>(this->components());
}


template<class C, class T, int N, class U>
auto operator<<(std::basic_ostream<C, T>& out, const math::Vector<N, U>& vec) -> std::basic_ostream<C, T>&
{
    return out << core::with_reverted_numerics(core::as_sbr_list(vec.components())); // Throws
}


template<int N, class T, class U>
constexpr auto operator+(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = a[i] + b[i];
    return c;
}


template<int N, class T, class U>
constexpr auto operator-(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = a[i] - b[i];
    return c;
}


template<int N, class T> constexpr auto operator-(const math::Vector<N, T>& a) noexcept ->
    math::Vector<N, math::scalar_arith_type<T>>
{
    math::Vector<N, math::scalar_arith_type<T>> b;
    for (int i = 0; i < N; ++i)
        b[i] = -a[i];
    return b;
}


template<int N, class T, class U, class>
constexpr auto operator*(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>
{
    return math::outer(a, b);
}


template<class T, int N, class U, class>
constexpr auto operator*(T a, const math::Vector<N, U>& b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>
{
    return math::outer(a, b);
}


template<class T, class U>
constexpr auto operator*(const math::Vector<3, T>& a, const math::Vector<3, U>& b) noexcept ->
    math::Vector<3, math::scalar_arith_type<T, U>>
{
    return math::cross(a, b);
}


template<int N, class T, class U, class>
constexpr auto operator/(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = a[i] / b;
    return c;
}


template<int N, class T> inline auto len(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>
{
    return std::sqrt(math::sq_sum(a));
}


template<int N, class T> constexpr auto sum(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    type b = 0;
    for (int i = 0; i < N; ++i)
        b += a[i];
    return b;
}


template<int N, class T> constexpr auto sq_sum(const math::Vector<N, T>& a) noexcept -> math::scalar_arith_type<T>
{
    using type = math::scalar_arith_type<T>;
    type b = 0;
    for (int i = 0; i < N; ++i)
        b += core::square(a[i]);
    return b;
}


template<int N, class T, class U>
constexpr auto dot(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept -> math::scalar_arith_type<T, U>
{
    return math::inner(a, b);
}


template<int N, class T, class U>
constexpr auto proj(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::Vector<N, math::scalar_arith_type<T, U>>
{
    return (math::dot(a, b) / math::sq_sum(b)) * b;
}


template<class T>
constexpr auto perp(const math::Vector<2, T>& a) noexcept -> math::Vector<2, math::scalar_arith_type<T>>
{
    return {
        -a[1],
        +a[0],
    };
}


template<class T, class U> constexpr auto cross(const math::Vector<3, T>& a, const math::Vector<3, U>& b) noexcept ->
    math::Vector<3, math::scalar_arith_type<T, U>>
{
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}


template<int N, class T, class U, class>
constexpr auto outer(const math::Vector<N, T>& a, U b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = a[i] * b;
    return c;
}


template<class T, int N, class U, class>
constexpr auto outer(T a, const math::Vector<N, U>& b) noexcept -> math::Vector<N, math::scalar_arith_type<T, U>>
{
    math::Vector<N, math::scalar_arith_type<T, U>> c;
    for (int i = 0; i < N; ++i)
        c[i] = a * b[i];
    return c;
}


template<int N, class T, class U>
constexpr auto inner(const math::Vector<N, T>& a, const math::Vector<N, U>& b) noexcept ->
    math::scalar_arith_type<T, U>
{
    using type = math::scalar_arith_type<T, U>;
    type c = 0;
    for (int i = 0; i < N; ++i)
        c += a[i] * b[i];
    return c;
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VECTOR_HPP
