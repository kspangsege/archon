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

#ifndef ARCHON_MATH_QUATERNION_HPP
#define ARCHON_MATH_QUATERNION_HPP

#include <cmath>
#include <algorithm>
#include <iostream>

#include <archon/math/rotation.hpp>


namespace archon
{
  namespace Math
  {
    struct Quaternion
    {
      double w;
      Vec3 v;

      Quaternion() {}
      Quaternion(double w, Vec3 const &v): w(w), v(v) {}

      static Quaternion const &zero();
      static Quaternion const &one();

      double sq_sum() const;
      double length() const;

      Quaternion operator+(Quaternion const &q) const;
      Quaternion operator-(Quaternion const &q) const;
      Quaternion operator*(Quaternion const &q) const;
      Quaternion operator*(double f) const;
      Quaternion operator/(double f) const;
      Quaternion operator-() const;

      Quaternion &operator+=(Quaternion const &q);
      Quaternion &operator-=(Quaternion const &q);

      /**
       * Multiply this quaternion with the one specified (Hamilton product.)
       *
       * For two quaternions <tt>q1 = (w1, V1)</tt> and <tt>q2 = (w1, V2)</tt> the
       * product is defined as:
       *
       * <pre>
       *
       *   (w1 * w2 - dot(V1,V2), V1 * V2 + w2 * V1 + w1 * V2)
       *
       * </pre>
       *
       * The quaternion product may be used to combine rotations, that
       * is, applying rotation \c q1 and then \c q2 is the same as
       * applying the rotation <tt>q2 * q1</tt>.
       *
       * \note The quaternion product is not commutative.
       *
       * \note The product of two unit quaternions is again a unit
       * quaternion.
       */
      Quaternion &operator*=(Quaternion const &q);

      Quaternion &operator/=(Quaternion const &q);

      Quaternion &operator*=(double f);
      Quaternion &operator/=(double f);

      bool operator==(Quaternion const &q) const;
      bool operator!=(Quaternion const &q) const;

      void set(Vec3 const &v, double w);
      void negate();
      void normalize();
      void conjugate();
      void inverse();

      /**
       * Create a normalized quaternion represention from the
       * specified axis rotation.
       *
       * \note Make sure 'r.axis' is a normalized vector.
       */
      void set_rotation(Rotation3 const &r);

      /**
       * Creates a normalized quaternion represention from the
       * specified rotation.
       *
       * \note Make sure \c axis is a normalized vector and that
       * \a cosine_of_angle is strictly in the range [-1;1].
       */
      void set_rotation(Vec3 const &axis, double cosine_of_angle);

      /**
       * Convert this quaternion to an axis-angle rotation. The
       * quaternion does not need to be normalized first.
       *
       * \todo This method may be optimized if we know that the
       * quaternion is a unit quaternion. Also it may be more
       * efficient to compute sin(a) as sqrt(1 - cos^2(a)).
       */
      void get_rotation(Rotation3 &r) const;

      void get_rot_matrix(Mat3 &mat) const;
    };



    inline Quaternion operator*(double f, const Quaternion &q)
    {
      Quaternion r = q; return r *= f;
    }

    inline Quaternion operator/(double f, const Quaternion &q)
    {
      Quaternion r = q; r.inverse(); return r *= f;
    }

    inline Quaternion normalize(const Quaternion &q)
    {
      Quaternion r = q; r.normalize(); return r;
    }

    inline Quaternion conjugate(const Quaternion &q)
    {
      Quaternion r = q; r.conjugate(); return r;
    }

    inline Quaternion inverse(const Quaternion &q)
    {
      Quaternion r = q; r.inverse(); return r;
    }

    std::ostream &operator<<(std::ostream &, const Quaternion &);





    // Inline definitions

    inline Quaternion const &Quaternion::zero()
    {
      static Quaternion q(0, Vec3::zero());
      return q;
    }

    inline Quaternion const &Quaternion::one()
    {
      static Quaternion q(1, Vec3::zero());
      return q;
    }

    inline double Quaternion::sq_sum() const
    {
      return w*w + Math::sq_sum(v);
    }

    inline double Quaternion::length() const
    {
      return std::sqrt(sq_sum());
    }

    inline Quaternion Quaternion::operator+(Quaternion const &q) const
    {
      Quaternion r(*this);
      return r += q;
    }

    inline Quaternion Quaternion::operator-(Quaternion const &q) const
    {
      Quaternion r(*this);
      return r -= q;
    }

    inline Quaternion Quaternion::operator*(Quaternion const &q) const
    {
      Quaternion r(*this);
      return r *= q;
    }

    inline Quaternion Quaternion::operator*(double f) const
    {
      Quaternion q(*this);
      return q *= f;
    }

    inline Quaternion Quaternion::operator/(double f) const
    {
      Quaternion q(*this);
      return q /= f;
    }

    inline Quaternion Quaternion::operator-() const
    {
      Quaternion q(*this);
      q.negate();
      return q;
    }

    inline Quaternion &Quaternion::operator+=(Quaternion const &q)
    {
      w += q.w;
      v += q.v;
      return *this;
    }

    inline Quaternion &Quaternion::operator-=(Quaternion const &q)
    {
      w -= q.w;
      v -= q.v;
      return *this;
    }

    inline Quaternion &Quaternion::operator/=(Quaternion const &q)
    {
      Quaternion r(q);
      r.inverse();
      *this *= r;
      return *this;
    }

    inline Quaternion &Quaternion::operator*=(double f)
    {
      w *= f;
      v *= f;
      return *this;
    }

    inline Quaternion &Quaternion::operator/=(double f)
    {
      w /= f;
      v /= f;
      return *this;
    }

    inline bool Quaternion::operator==(Quaternion const &q) const
    {
      return w == q.w && v == q.v;
    }

    inline bool Quaternion::operator!=(Quaternion const &q) const
    {
      return w != q.w || v != q.v;
    }

    inline void Quaternion::set(Vec3 const &v, double w)
    {
      this->w = w;
      this->v = v;
    }

    inline void Quaternion::negate()
    {
      w = -w; v.neg();
    }

    inline void Quaternion::normalize()
    {
      *this /= length();
    }

    inline void Quaternion::conjugate()
    {
      v.neg();
    }

    inline void Quaternion::inverse()
    {
      *this /= sq_sum();
      v.neg();
    }

    inline void Quaternion::set_rotation(Rotation3 const &r)
    {
      v = r.axis;
      double a = r.angle / 2;
      v *= std::sin(a);
      w = std::cos(a);
    }

    inline void Quaternion::set_rotation(Vec3 const &axis, double cos_of_angle)
    {
      v = axis;
      double k = (cos_of_angle + 1)/2;
      v *= std::sqrt(k-cos_of_angle);
      w = std::sqrt(k);
    }
  }
}

#endif // ARCHON_MATH_QUATERNION_HPP
