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

#ifndef ARCHON_MATH_GEOMETRY_HPP
#define ARCHON_MATH_GEOMETRY_HPP

#include <cmath>

#include <archon/math/coord_system.hpp>


namespace archon
{
  namespace math
  {
    /**
     * Oriented line with origin, aka a ray or a 1-D subspace.
     *
     * A point \c p is on the line iff there exits a scalar value \c d
     * such that <tt>p = origin + d * direction</tt>. The part of the
     * line where <tt>d < 0</tt> is called the negative part, and
     * the rest is the positive part.
     *
     * \note The default constructor leaves the constituent vectors
     * uninitialized.
     *
     * \note The length of the direction vector must never be zero.
     */
    template<int N, class T> struct BasicLine
    {
      typedef BasicVec<N,T> VectorType;

      VectorType origin;
      VectorType direction;

      BasicLine() {}

      BasicLine(VectorType const &origin, VectorType const &direction):
        origin(origin), direction(direction) {}

      /**
       * Map this locally described line through the specified
       * coordinate system to obtain a description relative to the
       * implicit reference system.
       */
      BasicLine &pre_mult(BasicCoordSystem<N,N,T> const &s)
      {
        origin = s * origin;
        direction = s.basis * direction;
        return *this;
      }
    };

    typedef BasicLine<2, double> Line2;
    typedef BasicLine<3, double> Line3;
    typedef BasicLine<4, double> Line4;


    /**
     * Minimum distance between point and line.
     */
    template<class T> T dist(BasicVec<3,T> const &v, BasicLine<3,T> const &l)
    {
      using std::sqrt;
      return sqrt(sq_sum(l.direction * (l.origin-v)) / sq_sum(l.direction));
    }



    /**
     * An N-1 dimensional oriented hyperplane that divides
     * N-dimensional space into a 'front' and a 'back' section. The
     * front section is in the direction of the normal.
     *
     * The normal is expected to be a unit vector, however, it may in
     * some situations be usefull to set it to a non-unit vector, so
     * the effects of doing that, are documented below. Functions that
     * take a hyperplane as argument, should document whether they
     * require the normal to be a unit vector or not.
     *
     * When the normal is restricted to be of unit length, then this
     * is a minimal and unique description.
     *
     * In fact, it is possible to specify the same object using only N
     * scalars in stead of the N+1 used here, for example using N-1
     * angles and one signed minimal distance to the origin, however
     * such a description is often harder to work with.
     *
     * A point \c p is on the plane (assuming 3-D) iff <tt>p * normal
     * = dist</tt>. If \c normal is of unit length, then \c abs(dist)
     * is the smallest distance from the origin to the plane.
     *
     * \note The default constructor leaves the constituents
     * uninitialized.
     *
     * \note The length of the normal must never be zero.
     */
    template<int N, class T> struct BasicHyperplane
    {
      typedef T             ScalarType;
      typedef BasicVec<N,T> VectorType;

      /**
       * The direction determines which side of the hyperplane is the
       * front side. That is, if the normal is drawn from some point
       * on the hyperplane, then it points to a location in front of
       * the plane.
       *
       * This should normally be a unit vector, but the effects of
       * setting is to a non-unit vector are documented in the
       * relevant places.
       */
      VectorType normal;

      /**
       * Assuming that the normal is a unit vector, this is the
       * distance from the origin to the plane along the normal. That
       * is, the distance along a line parallel with the normal and
       * running through the origin. If the origin is behind the
       * hyperplane, \c dist is positive.
       *
       * If the normal is not a unit vector, the actual distance is
       * <tt>dist / len(normal)</tt>.
       */
      ScalarType dist;

      BasicHyperplane() {}

      BasicHyperplane(VectorType const &normal, ScalarType dist):
        normal(normal), dist(dist) {}

      BasicHyperplane(VectorType const &normal, VectorType const &point):
        normal(normal), dist(dot(normal, point)) {}

      /**
       * Assuming that the normal is a unit vector, this method
       * returns the height of the specified point over the
       * hyperplane. If the point is behind the hyperplane, the height
       * is negative. The minimum distance from the point to the
       * hyperplane, is the absolute value of the returned height.
       *
       * If the normal is not a unit vector, the actual height is
       * <tt>height(p) / len(normal)</tt>.
       */
      T height(VectorType const &p) const { return dot(normal, p) - dist; }
    };

    typedef BasicHyperplane<2, double> Hyperplane2;
    typedef BasicHyperplane<3, double> Hyperplane3;
    typedef BasicHyperplane<4, double> Hyperplane4;


    /**
     * Assuming that the normal of the hyperplane is a unit vector,
     * this function returns the minimum distance between the point
     * and the hyperplane.
     *
     * If the normal is not a unit vector, the actual distance is
     * <tt>dist(v,d) / len(d.normal)</tt>.
     */
    template<int N, class T> T dist(BasicVec<N,T> const &v, BasicHyperplane<N,T> const &d)
    {
      using std::abs;
      return abs(d.height(v));
    }



    /**
     * Axis-aligned rectangular box.
     *
     * \note The default constructor leaves the constituents
     * uninitialized.
     */
    template<int N, class T> struct BasicBox
    {
      typedef BasicVec<N,T> VectorType;
      typedef BasicBox<N,T> BoxType;

      VectorType lower; ///< Lower-left-back corner in 3D
      VectorType upper; ///< Upper-right-front corner in 3D

      BasicBox() {}

      BasicBox(VectorType const &l, VectorType const &u): lower(l), upper(u) {}

      /**
       * Construct an origin centered box of the specified size.
       */
      explicit BasicBox(VectorType const &size): lower(-size/T(2)), upper(size/T(2)) {}


      /**
       * The the position of the center point of this box.
       */
      VectorType get_center() const { return (lower + upper)/T(2); }


      /**
       * Get the size of this box.
       */
      VectorType get_size() const
      {
        VectorType v = lower - upper;
        for(int i=0; i<=N; ++i) if(v[i] < 0) v[i] = -v[i];
        return v;
      }


      /**
       * Translate the box by the specified vector. This does not
       * change the size of the box, only its location in the
       * N-dimensional space.
       */
      void translate(VectorType const &v)
      {
        lower += v;
        upper += v;
      }


      /**
       * Reflect this box about each axis.
       */
      void reflect()
      {
        lower.swap(upper);
        lower.neg();
        upper.neg();
      }


      /**
       * Expand this box just enough to cover the specified box.
       *
       * That is, make this box the least axis-aligned bounding box
       * that includes both the specified box and the original version
       * of itself.
       */
      void include(BoxType const &b)
      {
        for(int i=0; i<=N; ++i)
        {
          T lower = b.lower[i], upper = b.upper[i];
          if(lower < lower[i]) lower[i] = lower;
          if(upper[i] < upper) upper[i] = upper;
        }
      }
    };

    typedef BasicBox<2, double> Box2;
    typedef BasicBox<3, double> Box3;
    typedef BasicBox<4, double> Box4;

    typedef BasicBox<2, float> Box2F;
    typedef BasicBox<3, float> Box3F;
    typedef BasicBox<4, float> Box4F;

    typedef BasicBox<2, long double> Box2L;
    typedef BasicBox<3, long double> Box3L;
    typedef BasicBox<4, long double> Box4L;



    /**
     * An arbitrary N dimensional sphere.
     *
     * \note The default constructor leaves the constituents
     * uninitialized.
     */
    template<int N, class T> struct BasicSphere
    {
      typedef T             ScalarType;
      typedef BasicVec<N,T> VectorType;

      VectorType center;
      ScalarType radius;

      BasicSphere() {}

      BasicSphere(VectorType const &center, ScalarType radius):
        center(center), radius(radius) {}

      /**
       * Construct an origin centered sphere of the specified radius.
       */
      BasicSphere(ScalarType radius = 1):
        center(Vec3::zero()), radius(radius) {}
    };

    typedef BasicSphere<2, double> Sphere2;
    typedef BasicSphere<3, double> Sphere3;
    typedef BasicSphere<4, double> Sphere4;



    /**
     * Find the intersection point between three planes.
     */
    Vec3 intersect(Hyperplane3 const &, Hyperplane3 const &, Hyperplane3 const &);
  }
}

#endif // ARCHON_MATH_GEOMETRY_HPP
