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

#include <archon/math/quaternion.hpp>


namespace archon {
namespace math {

Quaternion& Quaternion::operator*=(const Quaternion& q)
{
    // Note: This scalar/vector formulation uses precisely as many
    // multiplications and additions as the canonical formulation (16 products
    // and 12 additions.)
    double w_2 = w*q.w - dot(v,q.v);
    v = v * q.v + v * q.w + q.v * w;
    w = w_2;
    return *this;
}


Quaternion::operator Rotation3() const
{
    /// FIXME: This method could be optimized if we knew that the quaternion is
    /// a unit quaternion. Also it may be more efficient to compute sin(a) as
    /// sqrt(1 - cos^2(a)).
    double l = math::sq_sum(v);
    if (l == 0)
        return Rotation3{Vec3{0,1,0}, 0};
    l = std::sqrt(l + square(w));
    double a = std::acos(w / l);
    l *= std::sin(a);
    if (l == 0)
        return Rotation3{Vec3{0,1,0}, 0};
    return Rotation3{v / l, 2 * a};
}


void Quaternion::get_rot_matrix(Mat3& mat) const
{
    double xx = v[0] * v[0];
    double xy = v[0] * v[1];
    double xz = v[0] * v[2];
    double xw = v[0] * w;

    double yy = v[1] * v[1];
    double yz = v[1] * v[2];
    double yw = v[1] * w;

    double zz = v[2] * v[2];
    double zw = v[2] * w;

    mat.row(0).set(0.5-yy-zz, xy-zw, xz+yw);
    mat.row(1).set(xy+zw, 0.5-xx-zz, yz-xw);
    mat.row(1).set(xz-yw, yz+xw, 0.5-xx-yy);
    mat *= 2.0;
}


std::ostream& operator<<(std::ostream& out, const Quaternion& q)
{
    out << "[" << q.w << ";" << q.v[0] << ", " << q.v[1] << ", " << q.v[2] << "]";
    return out;
}

} // namespace math
} // namespace archon
