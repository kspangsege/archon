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

#ifndef ARCHON_MATH_ROTATION_HPP
#define ARCHON_MATH_ROTATION_HPP

#include <archon/math/matrix.hpp>


namespace archon
{
  namespace math
  {
    /**
     * This kind of rotation can be described as an interaction
     * between two degees of freedom, governed by a single scalar
     * angle. The remaining degrees of freedom form an N-2 dimensional
     * subspace which is unaffected by the rotation. In 3-D this is
     * the rotation axis. In general, to describe the rotation
     * uniquely, one must specify a particular orthonormal basis for
     * the N-2 dimensional subspace.
     *
     * Description of a rotatation in 3-D space around an arbitrary
     * origin based axis, or a 1-D subspace if you like. The angle is
     * specified in radians.
     *
     * The axis must be a unit vector.
     *
     * This kind of rotation could be generalized to N dimensions in
     * which case the axis would be an N by N-2 matrix. In 4-D it would
     * be a rotation around a 2-D subspace, or a plane if you like. In
     * 2-D the rotation would be around a 0-D subspace (ie. the
     * origin) and thus it would be a degenerate case where the axis
     * would vanish completely.
     *
     * Another posibility would be to integrate the angle into the
     * length of the axis vector.
     *
     * In several contexts it would be beneficial to be able to
     * produce a single axial rotation that when applied to the basis
     * vectors of a coordinate system will orient it such that its
     * negative z-axis is directed towards p and its x-axis is
     * parallel with the x-z-place of the reference coordinate system.
     *
     * A sollution always exists but is never unique. As is always the
     * case for angles we have an inifinite set of sollutions in that
     * v is as good as 2pi-v and as good as 2pi+v. This is not a
     * problem, we just choose the smalles one in an absolute
     * sence. if the angle is pi exactly there is no unique smallest
     * angle. In this case we choose the positive alternative.
     *
     * Another case that generates even more results is when the
     * resulting direction of the local z-axis becomes parallel to the
     * reference y-axis. In this case the local x-axis will always be
     * parallel to the x-z-place of the reference system and we need
     * another way to constraint the result. A sensible choice in this
     * case would be the rotation leading to the shortest angle.
     *
     * Again there is a special case where there is no unique rotation
     * with a shortest angle - if you are looking staight down and you
     * are asked to look straight up. In this case we should choose
     * one of the two rotations that avoid yaw.
     *
     * Please note that in cases such as when pi and -pi both are
     * sollutions it could make a big difference which one is chosen
     * if the result is used in animation.
     *
     * So, how can it be computed:
     *
     * Let:
     *
     *   O  be the local origin.
     *
     *   a  be the original direction of the local negative z-axis.
     *
     *   b  be the direction from O to p.
     *
     *   P  be a plane spanned by a and b and coincident with the local
     *   origin.
     *
     *   Q  be a plane that is perpendicular to P, coincident with O
     *   and dividing the angle between a and b in half.
     *
     * The primary constraint was to align a with b. Now, this can be
     * achieved with any rotation-axis lying in Q.
     *
     * For any such axis the proper rotation angle is determined as
     * the smallest angle between the projections of a and b onto a
     * plane perpendicular to the chosen rotation axis.
     *
     * But, how do we choose among the rotation axes in Q?
     *
     * Let:
     *
     *   c  be the original direction of the local x-axis.
     *
     *   d  be the new direction of the local x-axis after rotation.
     *
     *   A  be the chosen rotation axis in Q coincident with O.
     *
     *   R  be a plane perpendicular to A.
     *
     *   r  be the direction of A, |r| = 1.
     *
     * We must choose an axis in Q that at the same time it takes a to
     * b, it takes c to d.
     *
     * Let:
     *
     *   M(r, v)  be the rotation matrix corresponing with the sought
     *   axial rotation.
     *
     *   y  be the direction of the reference y axis.
     *
     * Then:
     *
     *   b = M(r, v) a
     *   
     *   d = M(r, v) c
     *
     *   d·y = 0
     *   
     *   (M(r, v) c)·u = 0
     *
     * (UNFINISHED!!!)
     *
     *
     * Please note, it would also be possible to determine a sequence
     * of simpler rotations and then combine them through quaternion
     * math, but I belive this would be bad for efficiency.
     *
     * \sa Quaternion
     */
    struct Rotation3
    {
      Vec3 axis; ///< The 1-D sub space around which we are rotating.
      double angle; ///< The angle of rotation in radians.

      Rotation3() {}

      Rotation3(Vec3 const &axis, double angle): axis(axis), angle(angle) {}

      static Rotation3 const &zero();

      Rotation3 operator-() const;

      bool operator==(const Rotation3 &r) const;

      bool operator!=(const Rotation3 &r) const;

      /**
       * Negate/invert this rotation.
       *
       * \return A reference to this rotation.
       */
      Rotation3 &neg() { angle = -angle; return *this; }

      /**
       * Produce the combined rotation corresponding to a sequence of
       * two rotation, the first one described by this object, and the
       * second one passed as argument.
       */
      Rotation3 &combine_with(Rotation3 const &);

      void get_matrix(Mat3 &) const;
    };






    // Implementation:

    inline Rotation3 const &Rotation3::zero()
    {
      static Rotation3 r(Vec3::zero(), 0);
      return r;
    }

    inline Rotation3 Rotation3::operator-() const
    {
      return Rotation3(axis, -angle);
    }

    inline bool Rotation3::operator==(const Rotation3 &r) const
    {
      return axis == r.axis && angle == r.angle;
    }

    inline bool Rotation3::operator!=(const Rotation3 &r) const
    {
      return axis != r.axis || angle != r.angle;
    }


    /**
     * Print the specified rotation onto the specified output stream.
     */
    template<class C, class T>
    inline std::basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, Rotation3 const &r)
    {
      C const left(out.widen('[')), comma(out.widen(',')),
        semicolon(out.widen(';')), right(out.widen(']'));
      out << left << r.axis[0] << comma << r.axis[1] << comma << r.axis[2] << semicolon <<
        r.angle << right;
      return out;
    }


    /**
     * Parse characters from the specified input stream as a
     * rotation. If successful, assign the result to the specified
     * target rotation.
     */
    template<class C, class T>
    inline std::basic_istream<C,T> &operator>>(std::basic_istream<C,T> &in, Rotation3 &r)
    {
      Rotation3 s;
      C const left(in.widen('[')), comma(in.widen(',')),
        semicolon(in.widen(';')), right(in.widen(']'));
      C ch;
      bool bad = false;
      in >> ch; if(ch != left) bad  = true;
      core::BasicIosFormatResetter<C,T> sentry(in);
      in >> s.axis[0] >> ch; if(ch != comma)     bad  = true; 
      in >> s.axis[1] >> ch; if(ch != comma)     bad  = true; 
      in >> s.axis[2] >> ch; if(ch != semicolon) bad  = true; 
      in >> s.angle   >> ch; if(ch != right)     bad  = true; 
      if(bad) in.setstate(std::ios_base::badbit);
      if(in) r = s;
      return in;
    }
  }
}

#endif // ARCHON_MATH_ROTATION_HPP
