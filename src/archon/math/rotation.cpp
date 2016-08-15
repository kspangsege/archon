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

#include <archon/math/quaternion.hpp>


namespace archon
{
  namespace math
  {
    Rotation3 &Rotation3::combine_with(const Rotation3 &r)
    {
      Quaternion q1, q2;
      q1.set_rotation(*this);
      q2.set_rotation(r);
      q2 *= q1;
      q2.get_rotation(*this);
      return *this;
    }


    /**
     * The rotation matrix may be decomposed as follows (Rodrigues
     * rotation formula):
     *
     * <pre>
     *
     *           [ 1 0 0 ]                                 [ 0 -z  y ]
     *   (cos a) [ 0 1 0 ] + (1 - cos a) V * V^T + (sin a) [ z  0 -x ]
     *           [ 0 0 1 ]                                 [-y  x  0 ]
     *
     * </pre>
     *
     * where <tt>a</tt> is the angle, <tt>V</tt> is the axis, and
     * <tt>x</tt>, <tt>y</tt>, and <tt>z</tt> are the scalar
     * components of the axis. The last matrix expresses the
     * cross-product with the axis of rotation. Thus, a rotation is a
     * linear combination of the vector itself, the projection of the
     * vector onto the axis, and the cross-product between the vector
     * and the axis.
     *
     * In 2-D this would look like:
     *
     * <pre>
     *
     *           [ 1  0 ]                                 [ 0 -1 ]
     *   (cos v) [      ] + (1 - cos v) 0 * 0^T + (sin v) [      ]
     *           [ 0  1 ]                                 [ 1  0 ]
     *
     * </pre>
     *
     * That is, the 3rd term vanishes and the last matrix now
     * expresses the concept of "perpendicular vector".
     *
     * This is easily seen by setting the axis to [ 0 0 1 ]^T and then
     * discarding the 3rd dimention.
     *
     * In 4-D we should compute the double-axis (or plane) rotation
     * matrix. But how does the last matrix look in 4-D? Somehow it
     * ought to reflect the generalization of the right hand rule
     * (RHR) in 4-D. Again we should expect that when inserting [ 0 0
     * 0 1 ]^T for the second axis and discarding the 4th dimention,
     * we should end up with the 3-D formula above.
     *
     * Observations:
     *
     * 1) The last matric has rank 2 in both cases.
     *
     * 2) In both cases the last matrix is antisymmetric.
     *
     * 3) The last matrix appears to be a basis of the orthogonal
     *    complement of the rotation axis, which is also the
     *    null-space of the basis of the axis. In the 2-D case, the
     *    rotation axis degenerates to the zero space {0}, which
     *    contains only the zero vector.
     *
     * The orthogonal complement of a plane spanned by vectors [a b c
     * d] and [e f g h] is equal to the span of
     *
     * <pre>
     *
     *   [ bh-df  de-ah    0    af-be ]
     *   [ bg-cf  ce-ag  af-be    0   ]
     *   [   0      0      0      0   ]
     *   [   0      0      0      0   ]
     *
     * </pre>
     *
     * This looks a lot like second minors of the rowspace basis of
     * the rotation plane:
     *
     * <pre>
     *
     *   [ d13  d30   0   d01 ]
     *   [ d12  d20  d01   0  ]
     *   [  0    0    0    0  ]
     *   [  0    0    0    0  ]
     *
     * </pre>
     *
     * Projected onto R^3, we get:
     *
     * <pre>
     *
     *   [ bh-df  de-ah  0  0 ]
     *   [ bg-cf  ce-ag  0  0 ]
     *   [   0      0    0  0 ]
     *   [   0      0    0  0 ]
     *
     * </pre>
     *
     * This is in error!
     *
     * See http://www.geometrictools.com/Documentation/RotationsFromPowerSeries.pdf
     *
     * Using geometric algebra, we can decompose a vector V as follows:
     *
     *   V  =  dot(V,A) * A  +  wedge(V,A) * A
     *
     * where A is an axis vector of unit length, the first term is the
     * projection onto that axis, and the second term is the
     * corresponding rejection.
     *
     * The interim conclusion is that the quest to generalize this
     * representation of a rotation to any number of dimensions is
     * unproductive. In general, simple rotations where only two
     * degrees of freedom interact, are most conveniently represented
     * as simple bivectors. Simple bivectors can always be represented
     * by two ordinary vectors of which the bivector is a
     * wedge-product. Thus, representing a rotation by an angle and an
     * N-2 dimensional subspace is only productive in three
     * dimensions.
     */
    void Rotation3::get_matrix(Mat3 &mat) const
    {
      const double ca = cos(angle);
      const double sa = sin(angle);

      const double cx = (1 - ca) * axis[0];
      const double cy = (1 - ca) * axis[1];
      const double cz = (1 - ca) * axis[2];

      const double cxy = cx * axis[1];
      const double cyz = cy * axis[2];
      const double czx = cz * axis[0];

      const double sx = axis[0] * sa;
      const double sy = axis[1] * sa;
      const double sz = axis[2] * sa;

      mat.row(0).set(ca + cx * axis[0], cxy - sz, czx + sy);
      mat.row(1).set(cxy + sz, ca + cy * axis[1], cyz - sx);
      mat.row(2).set(czx - sy, cyz + sx, ca + cz * axis[2]);
    }
  }
}
