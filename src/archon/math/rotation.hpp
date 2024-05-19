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

#ifndef ARCHON_X_MATH_X_ROTATION_HPP
#define ARCHON_X_MATH_X_ROTATION_HPP

/// \file


#include <ostream>

#include <archon/core/format.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/quaternion.hpp>


namespace archon::math {


/// \brief Axis-angle rotation.
///
/// A instance of this class represents a rotation around a specific axis (\ref axis) by a
/// specific angle (\ref angle). The axis should generally be a unit vector, or very close
/// to one.
///
/// Rotations are comparable. Comparison is lexicographical in terms of components.
///
/// Rotations can be formatted (written to an output stream). A rotation with axis (1, 0, 0)
/// and angle 1.5 will be formatted as `[1, 0, 0; 1.5]`. Note the semicolon. It separates
/// the components of the axis from the angle. Rotations will be formatted with numeric
/// locale facets reverted to those of the classic locale (as if by \ref
/// core::with_reverted_numerics()). This is in order to avoid ambiguity on the meaning of
/// commas.
///
/// \sa \ref operator<<(std::basic_ostream<C, T>&, const math::Rotation&)
/// \sa \ref operator+(const math::Rotation&, const math::Rotation&)
/// \sa \ref operator-(const math::Rotation&, const math::Rotation&)
/// \sa \ref operator-(const math::Rotation&)
/// \sa \ref operator*(math::Rotation::comp_type, const math::Rotation&)
/// \sa \ref operator*(const math::Rotation&, math::Rotation::comp_type)
/// \sa \ref operator/(const math::Rotation&, math::Rotation::comp_type)
///
class Rotation {
public:
    using comp_type   = double;
    using vector_type = math::Vector<3, comp_type>;

    /// \brief Rotation axis.
    ///
    /// This is the axis around which the rotation occurs.
    ///
    /// This rotation axis must generally be a unit vector. If it is not, some functions and
    /// operators will fail to work as intended (\ref to_versor(), for example).
    ///
    vector_type axis = { 1, 0, 0 };

    /// \brief Rotation angle.
    ///
    /// This is the angle of rotation in radians.
    ///
    comp_type angle = 0;

    /// \brief Default constructor.
    ///
    /// This is the default constructor. A default constructed rotation has an axis that
    /// points along the X-axis in the positive direction, and an angle of zero.
    ///
    constexpr Rotation() noexcept = default;

    /// \brief Construct rotation from axis and angle.
    ///
    /// This constructor sets the axis and angle as specified.
    ///
    constexpr Rotation(const vector_type& axis, comp_type angle) noexcept;

    /// \{
    ///
    /// \brief Arithmetic compound assignment.
    ///
    /// If `rot` is a rotation, then `rot += other` has the same effect as `rot = rot
    /// + other`. Likewise for operators `-=`, `*=`, and `/=`.
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    auto operator+=(const Rotation&) noexcept -> Rotation&;
    auto operator-=(const Rotation&) noexcept -> Rotation&;
    constexpr auto operator*=(comp_type) noexcept -> Rotation&;
    constexpr auto operator/=(comp_type) noexcept -> Rotation&;
    /// \}

    /// \{
    ///
    /// \brief Comparison of rotations
    ///
    /// These comparison operators compare this rotation with the specified
    /// rotation. Comparison is lexicographical with respect to the 4 components of the
    /// rotation (`axis[0]`, `axis[1]`, `axis[2]`, `angle`).
    ///
    constexpr bool operator==(const Rotation&) const noexcept;
    constexpr bool operator!=(const Rotation&) const noexcept;
    constexpr bool operator< (const Rotation&) const noexcept;
    constexpr bool operator<=(const Rotation&) const noexcept;
    constexpr bool operator> (const Rotation&) const noexcept;
    constexpr bool operator>=(const Rotation&) const noexcept;
    /// \}

    /// \brief Construct rotation from normalization of quaternion.
    ///
    /// This function constructs an axis-angle rotation from the normalization of the
    /// specified quaternion (a versor is a unit quaternion).
    ///
    /// This function guarantees that the axis of the resulting rotation is a unit vector,
    /// or very close to one.
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    static auto from_versor(const math::Quaternion&) noexcept -> Rotation;

    /// \brief Construct unit quaternion from axis-angle rotation.
    ///
    /// This function constructs a unit quaternion (versor) from this axis-angle rotation.
    ///
    /// The axis of this axis-angle rotation must be a unit vector, or be very close to
    /// one. If it is not, the rotation represented by the resulting quaternion will
    /// generally not be close to the rotation represented by this axis-angle rotation.
    ///
    /// FIXME: Make constexpr when switching to C++26
    ///
    auto to_versor() const noexcept -> math::Quaternion;
};


/// \brief Write textual representation of rotation to output stream.
///
/// This stream output operator writes a textual representation of the specified axis-angle
/// rotation (\p rot) to the specified output stream (\p out). See \ref math::Rotation for
/// information on the format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, const math::Rotation& rot) ->
    std::basic_ostream<C, T>&;


/// \brief Add rotations.
///
/// This operation combines the two specified rotations. The combined rotation is the result
/// of the first rotating (\p a) followed by the second rotation (\p b). The combined
/// rotation is as if computed by `math::Rotation::from_versor(b.to_versor() *
/// a.to_versor())` (note the inversion of order).
///
/// Note that addition of rotations does not commute, i.e, `a + b` is generally not equal to
/// `b + a`.
///
/// The axes of the two specified axis-angle rotations must be a unit vectors, or be very
/// close to unit vectors. If they are not, the resulting rotation will generally not be as
/// expected.
///
/// This function guarantees that the axis of the resulting rotation is a unit vector, or
/// very close to one.
///
/// FIXME: Make constexpr when switching to C++26
///
auto operator+(const math::Rotation& a, const math::Rotation& b) noexcept -> math::Rotation;


/// \brief Subtract rotations.
///
/// This operation combines the first specified rotation (\p a) with the inverse of second
/// specified rotation (\p b). The result is as if computed by `a + -b`.
///
/// FIXME: Make constexpr when switching to C++26
///
auto operator-(const math::Rotation& a, const math::Rotation& b) noexcept -> math::Rotation;


/// \brief Negate rotation.
///
/// This operation constructs the inverse of the specified rotation. The result is
/// `math::Rotation(a.axis, -a.angle)`.
///
constexpr auto operator-(const math::Rotation& a) noexcept -> math::Rotation;


/// \brief Scalar pre-multiplication for rotations.
///
/// This operation scales the specified rotation (\p b) by the specified value (\p a). The
/// result is `math::Rotation(b.axis, a * b.angle)`.
///
constexpr auto operator*(math::Rotation::comp_type a, const math::Rotation& b) noexcept -> math::Rotation;


/// \brief Scalar post-multiplication for rotations.
///
/// This operation scales the specified rotation (\p a) by the specified value (\p b). The
/// result is `math::Rotation(a.axis, a.angle * b)`.
///
constexpr auto operator*(const math::Rotation& a, math::Rotation::comp_type b) noexcept -> math::Rotation;


/// \brief Divide rotation by scalar.
///
/// This operation scales the specified rotation (\p a) by the reciprocal of the specified
/// value (\p b). The result is `math::Rotation(a.axis, a.angle / b)`.
///
constexpr auto operator/(const math::Rotation& a, math::Rotation::comp_type b) noexcept -> math::Rotation;








// Implementation


constexpr Rotation::Rotation(const vector_type& axis_2, comp_type angle_2) noexcept
    : axis(axis_2)
    , angle(angle_2)
{
}


inline auto Rotation::operator+=(const Rotation& other) noexcept -> Rotation&
{
    return (*this = *this + other);
}


inline auto Rotation::operator-=(const Rotation& other) noexcept -> Rotation&
{
    return (*this = *this - other);
}


constexpr auto Rotation::operator*=(comp_type other) noexcept -> Rotation&
{
    return (*this = *this * other);
}


constexpr auto Rotation::operator/=(comp_type other) noexcept -> Rotation&
{
    return (*this = *this / other);
}


constexpr bool Rotation::operator==(const Rotation& other) const noexcept
{
    return (axis == other.axis && angle == other.angle);
}


constexpr bool Rotation::operator!=(const Rotation& other) const noexcept
{
    return !(*this == other);
}


constexpr bool Rotation::operator<(const Rotation& other) const noexcept
{
    return (axis < other.axis || (axis == other.axis && angle < other.angle));
}


constexpr bool Rotation::operator<=(const Rotation& other) const noexcept
{
    return !(other < *this);
}


constexpr bool Rotation::operator>(const Rotation& other) const noexcept
{
    return (other < *this);
}


constexpr bool Rotation::operator>=(const Rotation& other) const noexcept
{
    return !(*this < other);
}


inline auto Rotation::from_versor(const math::Quaternion& quat) noexcept -> Rotation
{
    math::Rotation rot;
    quat.to_axis_angle(rot.axis, rot.angle);
    return rot;
}


inline auto Rotation::to_versor() const noexcept -> math::Quaternion
{
    return math::Quaternion::from_axis_angle(axis, angle);
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const math::Rotation& rot) -> std::basic_ostream<C, T>&
{
    return out << core::formatted_wrn("[%s, %s, %s; %s]", rot.axis[0], rot.axis[1], rot.axis[2], rot.angle); // Throws
}


inline auto operator+(const math::Rotation& a, const math::Rotation& b) noexcept -> math::Rotation
{
    return math::Rotation::from_versor(b.to_versor() * a.to_versor());
}


inline auto operator-(const math::Rotation& a, const math::Rotation& b) noexcept -> math::Rotation
{
    return a + -b;
}


constexpr auto operator-(const math::Rotation& a) noexcept -> math::Rotation
{
    return { a.axis, -a.angle };
}


constexpr auto operator*(math::Rotation::comp_type a, const math::Rotation& b) noexcept -> math::Rotation
{
    return { b.axis, a * b.angle };
}


constexpr auto operator*(const math::Rotation& a, math::Rotation::comp_type b) noexcept -> math::Rotation
{
    return { a.axis, a.angle * b };
}


constexpr auto operator/(const math::Rotation& a, math::Rotation::comp_type b) noexcept -> math::Rotation
{
    return { a.axis, a.angle / b };
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_ROTATION_HPP
