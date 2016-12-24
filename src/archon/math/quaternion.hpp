/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_MATH_QUATERNION_HPP
#define ARCHON_MATH_QUATERNION_HPP

#include <cmath>
#include <algorithm>
#include <iostream>

#include <archon/math/rotation.hpp>


namespace archon {
namespace math {

class Quaternion {
public:
    double w;
    Vec3 v;

    explicit constexpr Quaternion(double w = 0, const Vec3& v = Vec3::zero());

    static constexpr Quaternion zero();
    static constexpr Quaternion one();

    double sq_sum() const;
    double length() const;

    Quaternion operator+(const Quaternion&) const;
    Quaternion operator-(const Quaternion&) const;
    Quaternion operator*(const Quaternion&) const;
    Quaternion operator/(const Quaternion&) const;

    Quaternion operator+(double) const;
    Quaternion operator-(double) const;
    Quaternion operator*(double) const;
    Quaternion operator/(double) const;

    Quaternion operator-() const;

    Quaternion& operator+=(const Quaternion&);
    Quaternion& operator-=(const Quaternion&);

    /// Multiply this quaternion with the one specified (Hamilton product.)
    ///
    /// For two quaternions <tt>q1 = (w1, V1)</tt> and <tt>q2 = (w1, V2)</tt>
    /// the product is defined as:
    ///
    /// <pre>
    ///
    ///   (w1 * w2 - dot(V1,V2), V1 * V2 + w2 * V1 + w1 * V2)
    ///
    /// </pre>
    ///
    /// The quaternion product may be used to combine rotations, that is,
    /// applying rotation \c q1 and then \c q2 is the same as applying the
    /// rotation <tt>q2 * q1</tt>.
    ///
    /// \note The quaternion product does not commute.
    ///
    /// \note The product of two unit quaternions is again a unit quaternion.
    Quaternion& operator*=(const Quaternion&);

    Quaternion& operator/=(const Quaternion&);

    Quaternion& operator+=(double f);
    Quaternion& operator-=(double f);
    Quaternion& operator*=(double f);
    Quaternion& operator/=(double f);

    bool operator==(const Quaternion&) const;
    bool operator!=(const Quaternion&) const;

    Quaternion& neg();
    Quaternion& normalize();
    Quaternion& conj();
    Quaternion& inv();

    /// Create a versor (unit quaternion) from the specified axis-angle
    /// rotation.
    ///
    /// \note `r.axis` must be a unit vector.
    explicit Quaternion(const Rotation3&);

    /// Convert this quaternion to an axis-angle rotation. This quaternion does
    /// not need to be a unit quaternion.
    explicit operator Rotation3() const;

    /// Creates a versor (unit quaternion) from the specified axis-cosine of
    /// angle rotation.
    ///
    /// \note \a axis must be a unit vector and that \a cosine_of_angle must be
    /// strictly in the range [-1;1].
    Quaternion& set_rotation(const Vec3& axis, double cosine_of_angle);

    void get_rot_matrix(Mat3&) const;
};


Quaternion operator+(double, const Quaternion&);
Quaternion operator-(double, const Quaternion&);
Quaternion operator*(double, const Quaternion&);
Quaternion operator/(double, const Quaternion&);
Quaternion normalize(const Quaternion&);
Quaternion conj(const Quaternion&);
Quaternion inv(const Quaternion&);


/// \brief Construct a versor (unit quaternion) from proper Euler angles.
///
/// The final rotation is obtained by first rotating by \a alpha around the
/// Z-axis, then by \a beta around the new rotated X-axis, then finally by \a
/// gamma around the new rotated Z-axis.
Quaternion versor_from_proper_euler_angles(double alpha, double beta, double gamma);


/// @{ \brief Convert between horizontal and vertical coordinate bases.
///
/// See horiz_to_vert(Vec3) and vert_to_horiz(Vec3) for details.
constexpr Quaternion horiz_to_vert(Quaternion);
constexpr Quaternion vert_to_horiz(Quaternion);
/// @}

std::ostream& operator<<(std::ostream&, const Quaternion&);




// Implementation

constexpr Quaternion::Quaternion(double w, const Vec3& v):
    w{w},
    v{v}
{
}

inline constexpr Quaternion Quaternion::zero()
{
    return Quaternion{};
}

constexpr Quaternion Quaternion::one()
{
    return Quaternion{1, Vec3::zero()};
}

inline double Quaternion::sq_sum() const
{
    return w*w + math::sq_sum(v);
}

inline double Quaternion::length() const
{
    return std::sqrt(sq_sum());
}

inline Quaternion Quaternion::operator+(const Quaternion& q) const
{
    Quaternion r{*this};
    return r += q;
}

inline Quaternion Quaternion::operator-(const Quaternion& q) const
{
    Quaternion r{*this};
    return r -= q;
}

inline Quaternion Quaternion::operator*(const Quaternion& q) const
{
    Quaternion r{*this};
    return r *= q;
}

inline Quaternion Quaternion::operator/(const Quaternion& q) const
{
    Quaternion r{*this};
    return r /= q;
}

inline Quaternion Quaternion::operator+(double f) const
{
    Quaternion q{*this};
    return q += f;
}

inline Quaternion Quaternion::operator-(double f) const
{
    Quaternion q{*this};
    return q -= f;
}

inline Quaternion Quaternion::operator*(double f) const
{
    Quaternion q{*this};
    return q *= f;
}

inline Quaternion Quaternion::operator/(double f) const
{
    Quaternion q{*this};
    return q /= f;
}

inline Quaternion Quaternion::operator-() const
{
    Quaternion q{*this};
    return q.neg();
}

inline Quaternion& Quaternion::operator+=(const Quaternion& q)
{
    w += q.w;
    v += q.v;
    return *this;
}

inline Quaternion& Quaternion::operator-=(const Quaternion& q)
{
    w -= q.w;
    v -= q.v;
    return *this;
}

inline Quaternion& Quaternion::operator/=(const Quaternion& q)
{
    *this *= math::inv(q);
    return *this;
}

inline Quaternion& Quaternion::operator+=(double f)
{
    w += f;
    return *this;
}

inline Quaternion& Quaternion::operator-=(double f)
{
    w -= f;
    return *this;
}

inline Quaternion& Quaternion::operator*=(double f)
{
    w *= f;
    v *= f;
    return *this;
}

inline Quaternion& Quaternion::operator/=(double f)
{
    w /= f;
    v /= f;
    return *this;
}

inline bool Quaternion::operator==(const Quaternion& q) const
{
    return w == q.w && v == q.v;
}

inline bool Quaternion::operator!=(const Quaternion& q) const
{
    return w != q.w || v != q.v;
}

inline Quaternion& Quaternion::neg()
{
    w = -w;
    v.neg();
    return *this;
}

inline Quaternion& Quaternion::normalize()
{
    return *this /= length();
}

inline Quaternion& Quaternion::conj()
{
    v.neg();
    return *this;
}

inline Quaternion& Quaternion::inv()
{
    *this /= sq_sum();
    v.neg();
    return *this;
}

inline Quaternion::Quaternion(const Rotation3& r):
    w{std::cos(r.angle / 2)},
    v{r.axis * std::sin(r.angle / 2)}
{
}

inline Quaternion& Quaternion::set_rotation(const Vec3& axis, double cos_of_angle)
{
    v = axis;
    double k = (cos_of_angle + 1)/2;
    v *= std::sqrt(k-cos_of_angle);
    w = std::sqrt(k);
    return *this;
}

inline Quaternion operator+(double f, const Quaternion& q)
{
    Quaternion r = q; return r += f;
}

inline Quaternion operator-(double f, const Quaternion& q)
{
    Quaternion r = q; return r -= f;
}

inline Quaternion operator*(double f, const Quaternion& q)
{
    Quaternion r = q; return r *= f;
}

inline Quaternion operator/(double f, const Quaternion& q)
{
    Quaternion r = q; return r /= f;
}

inline Quaternion normalize(const Quaternion& q)
{
    Quaternion r = q; return r.normalize();
}

inline Quaternion conj(const Quaternion& q)
{
    Quaternion r = q; return r.conj();
}

inline Quaternion inv(const Quaternion& q)
{
    Quaternion r = q; return r.inv();
}

inline Quaternion versor_from_proper_euler_angles(double alpha, double beta, double gamma)
{
    double ca = std::cos(0.5 * alpha), sa = std::sin(0.5 * alpha);
    double cb = std::cos(0.5 * beta),  sb = std::sin(0.5 * beta);
    double cg = std::cos(0.5 * gamma), sg = std::sin(0.5 * gamma);

    double cc = ca * cg;
    double ss = sa * sg;
    double sc = sa * cg;
    double cs = ca * sg;

    double w = (cc - ss) * cb;
    double x = (cc + ss) * sb;
    double y = (sc - cs) * sb;
    double z = (sc + cs) * cb;

    return Quaternion{w, Vec3{x,y,z}};
}

constexpr Quaternion horiz_to_vert(Quaternion q)
{
    return Quaternion{q.w, horiz_to_vert(q.v)};
}

constexpr Quaternion vert_to_horiz(Quaternion q)
{
    return Quaternion{q.w, vert_to_horiz(q.v)};
}

} // namespace math
} // namespace archon

#endif // ARCHON_MATH_QUATERNION_HPP
