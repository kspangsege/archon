// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_MATH_X_QUATERNION_HPP
#define ARCHON_X_MATH_X_QUATERNION_HPP

/// \file


#include <cmath>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/math.hpp>
#include <archon/core/format.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>


namespace archon::math {


/// \brief Representation of mathematical concept of quaternion.
///
/// This class models the mathematical concept of a quaternion. As such, a quaternion is an
/// object containing 4 scalar components. In this formulation of a quaternion, the first
/// component is stored as scalar \p w and the three final components are stored as the
/// 3-vector \p v.
///
/// For two quaternions `q_1 = (w_1, v_1)` and `q_2 = (w_1, v_2)`, the product is defined
/// as:
///
///     (w_1 * w_2 - dot(v_1, v_2), cross(v_1, v_2) + w_2 * v_1 + w_1 * v_2)
///
/// The quaternion product may be used to combine rotations, that is, applying rotation
/// `q_1` and then `q_2` is the same as applying rotation `q_2 * q_1`.
///
/// Note that the quaternion product does not commute.
///
/// The product of two unit quaternions is again a unit quaternion.
///
/// A unit quaternion is also called a *versor*.
///
/// Quaternions are comparable. Comparison is lexicographical in terms of components.
///
/// Quaternions can be formatted (written to an output stream). A quaternion with components
/// 1.5, 2.5, 3.5, 4.5 will be formatted as `[1.5; 2.5, 3.4, 4.5]`. Note the semicolon. It
/// separates the scalar (or real) component from the 3 vector (or imaginary)
/// components. Quaternions will be formatted with numeric locale facets reverted to those
/// of the classic locale (as if by \ref core::with_reverted_numerics()). This is in order
/// to avoid ambiguity on the meaning of commas.
///
/// \sa \ref operator<<(std::basic_ostream<C, T>&, const math::Quaternion&)
/// \sa \ref operator+(const math::Quaternion&, const math::Quaternion&)
/// \sa \ref operator-(const math::Quaternion&, const math::Quaternion&)
/// \sa \ref operator-(const math::Quaternion&)
/// \sa \ref operator*(const math::Quaternion&, const math::Quaternion&)
/// \sa \ref operator/(const math::Quaternion&, const math::Quaternion&)
/// \sa \ref conj(const math::Quaternion&)
/// \sa \ref conj(const math::Quaternion&, const math::Quaternion&)
/// \sa \ref conj(const math::Quaternion::vector_type&, const math::Quaternion&)
/// \sa \ref len(const math::Quaternion&)
/// \sa \ref normalize(const math::Quaternion&)
/// \sa \ref sq_sum(const math::Quaternion&)
/// \sa \ref inv(const math::Quaternion&)
///
class Quaternion {
public:
    using comp_type = double;
    using vector_type = math::Vector<3, comp_type>;
    using matrix_type = math::Matrix<3, 3, comp_type>;

    /// \brief Scalar part of quaternion.
    ///
    /// This is the scalar, or "real" part of the quaternion.
    ///
    comp_type w = 0;

    /// \brief Vector part of quaternion.
    ///
    /// This is the vector, or "imaginary" part of the quaternion.
    ///
    vector_type v = {};

    /// \brief Construct quaternion with all components set to zero.
    ///
    /// This is the default constructor. A default constructed quaternion has all its components set to zero.
    ///
    constexpr Quaternion() noexcept = default;

    /// \brief Construct quaternion from scalar and vector parts.
    ///
    /// This constructor sets the scalar and vector parts as specified.
    ///
    constexpr Quaternion(comp_type w, const vector_type& v = {}) noexcept;

    /// \brief Construct quaternion from four separate components.
    ///
    /// This constructor sets the scalar part to \p w and the vector part to (\p v_1, \p
    /// v_2, \p v_3).
    ///
    constexpr Quaternion(comp_type w, comp_type v_1, comp_type v_2, comp_type v_3) noexcept;

    /// \{
    ///
    /// \brief Arithmetic compound assignment.
    ///
    /// If `quat` is a quaternion, then `quat += other` has the same effect as `quat = quat
    /// + other`. Likewise for operators `-=`, `*=`, and `/=`.
    ///
    constexpr auto operator+=(const Quaternion&) noexcept -> Quaternion&;
    constexpr auto operator-=(const Quaternion&) noexcept -> Quaternion&;
    constexpr auto operator*=(const Quaternion&) noexcept -> Quaternion&;
    constexpr auto operator/=(const Quaternion&) noexcept -> Quaternion&;
    /// \}

    /// \{
    ///
    /// \brief Quaternion comparison.
    ///
    /// These comparison operators compare this quaternion with the specified
    /// quaternion. Comparison is lexicographical with respect to the components of the
    /// quaternions (`w`, `v[0]`, `v[1]`, `v[2]`).
    ///
    constexpr bool operator==(const Quaternion&) const noexcept;
    constexpr bool operator!=(const Quaternion&) const noexcept;
    constexpr bool operator< (const Quaternion&) const noexcept;
    constexpr bool operator<=(const Quaternion&) const noexcept;
    constexpr bool operator> (const Quaternion&) const noexcept;
    constexpr bool operator>=(const Quaternion&) const noexcept;
    /// \}

    /// \brief Create versor from rotation axis and angle.
    ///
    /// This function creates a versor (unit quaternion) from the rotation corresponding to
    /// the specified axis and angle. The axis (\p axis) must be a unit vector (or be close
    /// to one). The angle (\p angle) will be clamped to the range [0;2*pi].
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    static auto from_axis_angle(const vector_type& axis, comp_type angle) noexcept -> Quaternion;

    /// \brief Create versor from rotation axis and cosine of angle.
    ///
    /// This function creates a versor (unit quaternion) from the rotation corresponding to
    /// the specified axis and cosine of angle. The axis (\p axis) must be a unit vector (or
    /// be close to one) and the cosine of the angle (\p cos_angle) must be in the range
    /// [-1;1].
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    static auto from_axis_cos_angle(const vector_type& axis, comp_type cos_angle) noexcept -> Quaternion;

    /// \brief Convert normalization of quaternion to rotation axis and angle.
    ///
    /// This function produces a rotation axis and angle from the normalization of the
    /// quaternion.
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    template<class T> void to_axis_angle(math::Vector<3, T>& axis, T& angle) const noexcept;

    /// \brief Construct a versor (unit quaternion) from proper Euler angles.
    ///
    /// The final rotation is obtained by first rotating by \p alpha around the Z-axis, then
    /// by \p beta around the new rotated X-axis, then finally by \p gamma around the new
    /// rotated Z-axis.
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    static auto from_proper_euler_angles(comp_type alpha, comp_type beta, comp_type gamma) noexcept -> Quaternion;

    /// \brief Convert normalization of quaternion to corresponding rotation matrix.
    ///
    /// This function constructs the 3-by-3 rotation matrix that corresponds to the rotation
    /// represented by the normalization of this quaternion.
    ///
    constexpr auto to_rotation_matrix() const noexcept -> matrix_type;
};


/// \brief Write textual representation of quaternion to output stream.
///
/// This stream output operator writes a textual representation of the specified quaternion
/// (\p quat) to the specified output stream (\p out). See \ref math::Quaternion for
/// information on the format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, const math::Quaternion& quat) ->
    std::basic_ostream<C, T>&;


/// \brief Add quaternions.
///
/// This operation constructs the sum of the two specified quaternions.
///
constexpr auto operator+(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion;


/// \brief Subtract quaternions.
///
/// This operation constructs the difference between the two specified quaternions.
///
constexpr auto operator-(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion;


/// \brief Negate quaternion.
///
/// This operation constructs the additive inverse of the specified quaternion.
///
constexpr auto operator-(const math::Quaternion& a) noexcept -> math::Quaternion;


/// \brief Multiply quaternions.
///
/// This operation constructs the product of the two specified quaternions.
///
/// Note that the quaternion product does not commute, i.e, `a * b` is generally not equal
/// to `b * a`.
///
constexpr auto operator*(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion;


/// \brief Divide quaternions.
///
/// This operation constructs the quotient of the two specified quaternions.
///
constexpr auto operator/(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion;


/// \brief Quaternion conjugate.
///
/// This operation constructs the conjugate of the specified quaternion.
///
constexpr auto conj(const math::Quaternion& a) noexcept -> math::Quaternion;


/// \brief Conjugate of one quaternion by other quaternion.
///
/// This operation constructs the conjugate of the first specified quaternion (\p a) by the
/// second specified quaternion (\p b). `conj(a, b)` is the same as `b * a * math::conj(b)`,
/// but computed more efficiently.
///
constexpr auto conj(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion;


/// \brief Conjugate of vector by quaternion.
///
/// This operation constructs the conjugate of the specified vector (\p a) by the specified
/// quaternion (\p b). `conj(a, b)` is the same as `math::conj(math::Quaternion(0, a),
/// b).v`, but computed more efficiently.
///
/// \sa \ref conj(const math::Quaternion&, const math::Quaternion&)
///
constexpr auto conj(const math::Quaternion::vector_type& a, const math::Quaternion& b) noexcept ->
    math::Quaternion::vector_type;


/// \brief Magnitude of quaternion.
///
/// This operation computes the magnitude (or length) of the specified quaternion.
///
/// FIXME: Make constexpr when switching to C++26
///
auto len(const math::Quaternion& a) noexcept -> math::Quaternion::comp_type;


/// \brief Normalize quaternion.
///
/// This operation constructs a normalized version of the specified quaternion.
///
/// FIXME: Make constexpr when switching to C++26
///
auto normalize(const math::Quaternion& a) noexcept -> math::Quaternion;


/// \brief Squared magnitude of quaternion.
///
/// This operation computes the square of the magnitude of the specified quaternion.
///
constexpr auto sq_sum(const math::Quaternion& a) noexcept -> math::Quaternion::comp_type;


/// \brief Multiplicative inverse of quaternion.
///
/// This operation constructs the multiplicative inverse of the specified quaternion.
///
constexpr auto inv(const math::Quaternion& a) noexcept -> math::Quaternion;








// Implementation


constexpr Quaternion::Quaternion(comp_type w_2, const vector_type& v_2) noexcept
    : w(w_2)
    , v(v_2)
{
}


constexpr Quaternion::Quaternion(comp_type w, comp_type v_1, comp_type v_2, comp_type v_3) noexcept
    : Quaternion(w, { v_1, v_2, v_3 })
{
}


constexpr auto Quaternion::operator+=(const Quaternion& other) noexcept -> Quaternion&
{
    return (*this = *this + other);
}


constexpr auto Quaternion::operator-=(const Quaternion& other) noexcept -> Quaternion&
{
    return (*this = *this - other);
}


constexpr auto Quaternion::operator*=(const Quaternion& other) noexcept -> Quaternion&
{
    return (*this = *this * other);
}


constexpr auto Quaternion::operator/=(const Quaternion& other) noexcept -> Quaternion&
{
    return (*this = *this / other);
}


constexpr bool Quaternion::operator==(const Quaternion& other) const noexcept
{
    return (w == other.w && v == other.v);
}


constexpr bool Quaternion::operator!=(const Quaternion& other) const noexcept
{
    return !(*this == other);
}


constexpr bool Quaternion::operator<(const Quaternion& other) const noexcept
{
    return (w < other.w || (w == other.w && v < other.v));
}


constexpr bool Quaternion::operator<=(const Quaternion& other) const noexcept
{
    return !(other < *this);
}


constexpr bool Quaternion::operator>(const Quaternion& other) const noexcept
{
    return (other < *this);
}


constexpr bool Quaternion::operator>=(const Quaternion& other) const noexcept
{
    return !(*this < other);
}


inline auto Quaternion::from_axis_angle(const vector_type& axis, comp_type angle) noexcept -> Quaternion
{
    return { std::cos(angle / 2), axis * std::sin(angle / 2) };
}


inline auto Quaternion::from_axis_cos_angle(const vector_type& axis, comp_type cos_angle) noexcept -> Quaternion
{
    comp_type k = (cos_angle + 1) / 2;
    return { std::sqrt(k),  std::sqrt(k - cos_angle) * axis };
}


template<class T> void Quaternion::to_axis_angle(math::Vector<3, T>& axis, T& angle) const noexcept
{
    comp_type s = math::sq_sum(v);
    if (ARCHON_LIKELY(s != 0)) {
        comp_type l = std::sqrt(s);
        axis = v / l;
        angle = 2 * std::atan2(l, w);
        return;
    }
    axis = { 1, 0, 0 };
    angle = 0;
}


constexpr auto Quaternion::to_rotation_matrix() const noexcept -> matrix_type
{
    comp_type s = 2 / math::sq_sum(*this);

    comp_type x = v[0];
    comp_type y = v[1];
    comp_type z = v[2];

    comp_type sx = s * x;
    comp_type sy = s * y;
    comp_type sz = s * z;

    comp_type wsx = w * sx;
    comp_type xsx = x * sx;

    comp_type wsy = w * sy;
    comp_type xsy = x * sy;
    comp_type ysy = y * sy;

    comp_type wsz = w * sz;
    comp_type xsz = x * sz;
    comp_type ysz = y * sz;
    comp_type zsz = z * sz;

    return {{ 1 - ysy - zsz, xsy - wsz, xsz + wsy },
            { xsy + wsz, 1 - xsx - zsz, ysz - wsx },
            { xsz - wsy, ysz + wsx, 1 - xsx - ysy }};
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const math::Quaternion& quat) -> std::basic_ostream<C, T>&
{
    return out << core::with_reverted_numerics(core::formatted("[%s; %s, %s, %s]", quat.w, quat.v[0], quat.v[1],
                                                               quat.v[2])); // Throws
}


constexpr auto operator+(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion
{
    return { a.w + b.w, a.v + b.v };
}


constexpr auto operator-(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion
{
    return { a.w - b.w, a.v - b.v };
}


constexpr auto operator-(const math::Quaternion& a) noexcept -> math::Quaternion
{
    return { -a.w, -a.v };
}


constexpr auto operator*(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion
{
    // Note: This scalar/vector formulation uses precisely as many multiplications and
    // additions as the canonical formulation (16 multiplications and 12 additions).
    return { a.w * b.w - math::dot(a.v, b.v), a.v * b.w + a.w * b.v + math::cross(a.v, b.v) };
}


constexpr auto operator/(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion
{
    return a * math::inv(b);
}


constexpr auto conj(const math::Quaternion& a) noexcept -> math::Quaternion
{
    return { a.w, -a.v };
}


constexpr auto conj(const math::Quaternion& a, const math::Quaternion& b) noexcept -> math::Quaternion
{
    auto sbw = core::square(b.w);
    auto sbv = math::sq_sum(b.v);
    return {
        (sbw + sbv) * a.w,
        (sbw - sbv) * a.v + 2 * math::dot(a.v, b.v) * b.v - 2 * b.w * math::cross(a.v, b.v),
    };

    // c = { b.w * a.w - math::dot(b.v, a.v), b.v * a.w + b.w * a.v + math::cross(b.v, a.v) }
    // d = { b.w, -b.v }

    // e = { c.w * d.w - math::dot(c.v, d.v), c.v * d.w + c.w * d.v + math::cross(c.v, d.v) }

    // e.w = c.w * d.w - math::dot(c.v, d.v)
    // e.w = (b.w * a.w - math::dot(b.v, a.v)) * b.w - math::dot(b.v * a.w + b.w * a.v + math::cross(b.v, a.v), -b.v)
    // e.w = a.w * sq(b.w) - b.w * math::dot(a.v, b.v) + math::dot(b.v * a.w + b.w * a.v + math::cross(b.v, a.v), b.v)
    // e.w = a.w * sq(b.w) - b.w * math::dot(a.v, b.v) + math::dot(b.v * a.w, b.v) + math::dot(b.w * a.v, b.v) + math::dot(math::cross(b.v, a.v), b.v)
    // e.w = a.w * sq(b.w) + a.w * math::sq_sum(b.v) + math::dot(math::cross(b.v, a.v), b.v)
    // e.w = a.w * sq(b.w) + a.w * math::sq_sum(b.v) + math::dot(a.v, math::cross(b.v, b.v))
    // e.w = a.w * (sq(b.w) + math::sq_sum(b.v))
}


constexpr auto conj(const math::Quaternion::vector_type& a, const math::Quaternion& b) noexcept ->
    math::Quaternion::vector_type
{
    return (core::square(b.w) - math::sq_sum(b.v)) * a + 2 * math::dot(a, b.v) * b.v - 2 * b.w * math::cross(a, b.v);
}


inline auto len(const math::Quaternion& a) noexcept -> math::Quaternion::comp_type
{
    return std::sqrt(math::sq_sum(a));
}


inline auto normalize(const math::Quaternion& a) noexcept -> math::Quaternion
{
    return a / math::len(a);
}


constexpr auto sq_sum(const math::Quaternion& a) noexcept -> math::Quaternion::comp_type
{
    return core::square(a.w) + math::sq_sum(a.v);
}


constexpr auto inv(const math::Quaternion& a) noexcept -> math::Quaternion
{
    math::Quaternion::comp_type f = 1 / math::sq_sum(a);
    return { f * a.w, -f * a.v };
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_QUATERNION_HPP
