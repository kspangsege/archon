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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <cmath>

#include <GL/gl.h>

#include <archon/core/functions.hpp>
#include <archon/math/functions.hpp>
#include <archon/math/quaternion.hpp>
#include <archon/render/billboard.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Math;


namespace Archon
{
  namespace Render
  {
    namespace Billboard
    {
      double rotate(Vec3 const &rot_axis)
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



      void calculate_rotation(Mat3 const &subframe_basis, Vec3 const &subframe_origin,
                              Vec3 const &rot_axis, Rotation3 &rot)
      {
        Mat3 const inv_basis = inv(subframe_basis);
        Vec3 n = rot_axis;
        Vec3 o = subframe_origin;
        if(o.is_zero()) o.set(0, 0, -1);
        Vec3 e = inv_basis * -o; // Position of eye in local coordinate system
        if(n.is_zero())
        {
          // viewer-alignment

          // First get rotation on plane of eye and local z-axis as (axis1, ca1)
          Vec3 axis1;
          double k = sq_sum(e.slice<2>()); // <1
          double const ca1 = e[2]/sqrt(k + square(e[2])); // >0 (e[2])
          if(k == 0)
          {
            // z-axis already coincides with direction towards eye
            axis1.set(0,1,0);
          }
          else
          {
            k = sqrt(k); // <1
            // The projection of the eye position onto the
            // x-y-plane and then turned 90 degrees anticlockwise.
            axis1.set(-e[1]/k, e[0]/k, 0); // 0, 1, 0
          }

          // Then rotate about updated z-axis to align y-axis with viewers notion of upwards
          Vec3 const &up = inv_basis.col(1);
          Vec3 const x = e * up;
          Vec3 y = x * e; // Desired final direction of local y-axis
          // y is now perpendicular to e and in in the plane spanned by e and up.
          k = sq_sum(y);
          Vec3 axis2;
          double ca2;
          if(k == 0)
          {
            // The local origin lies somewhere on the y-axis of the
            // eye-coordinate system.
            axis2.set(0,1,0);
            ca2 = 0; // We might want to choose a more reasonable rotation in this special case
          }
          else
          {
            y /= sqrt(k);
            // axis2 is first set to be the rotation of the y-axis by (axis1, ca1). This makes axis2 perpendicular to 'e'.
            k = (1 - ca1) * axis1[1]; // <1 (1-e[2])
            axis2.set(k * axis1[0],
                      k * axis1[1] + ca1,
                      axis1[0] * sqrt(1 - ca1*ca1));
            ca2 = dot(axis2, y);
            axis2 = unit(0 < dot(axis2, x) ? -e : e);
          }

          // Combine the two rotations through the use of quaternions
          Quaternion q1, q2;
          q1.set_rotation(axis1, clamp(ca1, -1.0, 1.0));
          q2.set_rotation(axis2, clamp(ca2, -1.0, 1.0));
          q2 *= q1;
          q2.get_rotation(rot);
          return;
        }


        e *= n;
        double const l1 = len(e);
        if(l1 == 0)
        {
          // The axis of rotation is coincident with the direction
          // towards the eye. In this case every rotation angle is as
          // good as any other.
          rot.axis = n;
          rot.angle = 0;
          return;
        }
        n *= Vec3(0, 0, -1);
        double const l2 = len(n);
        if(l2 == 0)
        {
          // The axis of rotation is coincident with the direction
          // defining the front of the object. In this case every
          // rotation angle is as good as any other.
          rot.axis = n;
          rot.angle = 0;
          return;
        }
        double const p = dot(e, n) / (l1*l2);
        if(1 <= p)
        {
          // We already face the front as much as possible.
          rot.axis = n;
          rot.angle = 0;
          return;
        }
        if(p <= -1)
        {
          // We face the back as much as possible.
          rot.axis = n;
          rot.angle = M_PI;
          return;
        }

        rot.axis  = unit(n*e);
        rot.angle = acos(p);
      }
    }
  }
}
