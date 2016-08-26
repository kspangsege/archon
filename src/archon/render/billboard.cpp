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

#include <cmath>

#include <GL/gl.h>

#include <archon/core/functions.hpp>
#include <archon/math/functions.hpp>
#include <archon/math/quaternion.hpp>
#include <archon/render/billboard.hpp>


using namespace archon::core;
using namespace archon::math;


namespace archon {
namespace render {
namespace billboard {

double rotate(const Vec3& rot_axis)
{
    // Extract OpenGl's current modelview coordinate system
    GLdouble m[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, m);
    Mat3 basis;
    basis.col(0).set(m + 0);
    basis.col(1).set(m + 4);
    basis.col(2).set(m + 8);
    Vec3 origin(m + 12);

    Rotation3 rot;
    calculate_rotation(basis, origin, rot_axis, rot);

    glRotatef(rot.angle*(180/M_PI), rot.axis[0], rot.axis[1], rot.axis[2]);

    return ang(origin.slice<2>());
}


void calculate_rotation(const Mat3& subframe_basis,
                        const Vec3& subframe_origin,
                        const Vec3& rot_axis,
                        Rotation3& rot)
{
    Mat3 inv_basis = inv(subframe_basis);
    Vec3 n = rot_axis;
    Vec3 o = subframe_origin;
    if (o.is_zero())
        o.set(0, 0, -1);
    Vec3 e = inv_basis * -o; // Position of eye in local coordinate system
    if (n.is_zero()) {
        // viewer-alignment

        // First get rotation on plane of eye and local z-axis as (axis_1, ca_1)
        Vec3 axis_1;
        double k = sq_sum(e.slice<2>()); // <1
        double ca_1 = e[2]/std::sqrt(k + square(e[2])); // >0 (e[2])
        if (k == 0) {
            // z-axis already coincides with direction towards eye
            axis_1.set(0,1,0);
        }
        else {
            k = std::sqrt(k); // <1
            // The projection of the eye position onto the
            // x-y-plane and then turned 90 degrees anticlockwise.
            axis_1.set(-e[1]/k, e[0]/k, 0); // 0, 1, 0
        }

        // Then rotate about updated z-axis to align y-axis with viewers notion
        // of upwards
        const Vec3& up = inv_basis.col(1);
        Vec3 x = e * up;
        Vec3 y = x * e; // Desired final direction of local y-axis
        // y is now perpendicular to e and in in the plane spanned by e and up.
        k = sq_sum(y);
        Vec3 axis_2;
        double ca_2;
        if (k == 0) {
            // The local origin lies somewhere on the y-axis of the
            // eye-coordinate system.
            axis_2.set(0,1,0);
            // We might want to choose a more reasonable rotation in this
            // special case.
            ca_2 = 0;
        }
        else {
            y /= std::sqrt(k);
            // axis_2 is first set to be the rotation of the y-axis by (axis_1,
            // ca_1). This makes axis_2 perpendicular to 'e'.
            k = (1 - ca_1) * axis_1[1]; // <1 (1-e[2])
            axis_2.set(k * axis_1[0],
                       k * axis_1[1] + ca_1,
                       axis_1[0] * std::sqrt(1 - ca_1*ca_1));
            ca_2 = dot(axis_2, y);
            axis_2 = unit(0 < dot(axis_2, x) ? -e : e);
        }

        // Combine the two rotations through the use of quaternions
        Quaternion q_1, q_2;
        q_1.set_rotation(axis_1, clamp(ca_1, -1.0, 1.0));
        q_2.set_rotation(axis_2, clamp(ca_2, -1.0, 1.0));
        rot = Rotation3{q_2 * q_1};
        return;
    }


    e *= n;
    double l_1 = len(e);
    if (l_1 == 0) {
        // The axis of rotation is coincident with the direction towards the
        // eye. In this case every rotation angle is as good as any other.
        rot.axis = n;
        rot.angle = 0;
        return;
    }
    n *= Vec3{0, 0, -1};
    double l_2 = len(n);
    if (l_2 == 0) {
        // The axis of rotation is coincident with the direction defining the
        // front of the object. In this case every rotation angle is as good as
        // any other.
        rot.axis = n;
        rot.angle = 0;
        return;
    }
    double p = dot(e, n) / (l_1 * l_2);
    if (1 <= p) {
        // We already face the front as much as possible.
        rot.axis = n;
        rot.angle = 0;
        return;
    }
    if (p <= -1) {
        // We face the back as much as possible.
        rot.axis = n;
        rot.angle = M_PI;
        return;
    }

    rot.axis  = unit(n * e);
    rot.angle = std::acos(p);
}

} // namespace billboard
} // namespace render
} // namespace archon
