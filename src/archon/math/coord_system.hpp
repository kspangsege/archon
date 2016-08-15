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

#ifndef ARCHON_MATH_COORD_SYSTEM_HPP
#define ARCHON_MATH_COORD_SYSTEM_HPP

#include <archon/math/matrix.hpp>

namespace archon
{
  namespace Math
  {
    template<int M, int N, class T> struct BasicCoordSystem;



    template<int M, int N, class T> struct CoordSystemBase
    {
      BasicMat<M,N,T> basis;
      BasicVec<M,T> origin;
    };



    template<int M, int N, class T> struct CoordSystemSpec: CoordSystemBase<M,N,T> {};

    /**
     * This specialization provides methods for coordinate systems
     * with square base matrices.
     */
    template<int N, class T> struct CoordSystemSpec<N,N,T>: CoordSystemBase<N,N,T>
    {
      /**
       * Get the identity coordinate system description. That is, the
       * one whose basis is the identity matrix, and whose origin is
       * zero.
       *
       * \return A reference to a statically allocated identity
       * coordinate system.
       */
      static BasicCoordSystem<N,N,T> const &identity();


      /**
       * Invert this coordinate system description. That is, transform
       * it such that it becomes a description of the reference system
       * relative to the local one that was originally described by
       * this object.
       *
       * If the original basis matrix was singular, the result is
       * undefined.
       */
      BasicCoordSystem<N,N,T> &inv()
      {
	this->basis.inv();
	this->origin = this->basis * this->origin;
	this->origin.neg();
        return static_cast<BasicCoordSystem<N,N,T> &>(*this);
      }
    };



    /**
     * Describes the embedding of an N-dimensional rectilinear
     * coordinate system within an implicit M-dimensional reference
     * coordinate system. The basis of the new coordinate system is
     * described by the contained matrix whos columns define the unit
     * axis vectors of the new system, expressed in the coordinates of
     * the reference system. The origin of the new coordinte system is
     * described by the contained vector. Thus, an instance of this
     * class imediately describes the transformation from the new
     * coordinate system into the reference system.
     *
     * \note The phrase "reference coordinate system" is uses to refer
     * to the coordinate system against which the new coordinate
     * system is described. Frequently, one must work with a hierarchy
     * of several nested coordinate systems, and in this case all but
     * the inner-most system acts as a reference coordinate system for
     * some other. The outer-most reference coordinate system (ie. the
     * reference coordinate system of the outer-most coordinate system
     * description) is a purely mental entity. Although it does not
     * have a description we assume its existence (it is kind of like
     * the universe...)
     */
    template<int M, int N = M, class T = double> struct BasicCoordSystem: CoordSystemSpec<M,N,T>
    {
      /**
       * The default constructor does not initialize the basis nor the
       * origin.
       */
      BasicCoordSystem() {}

      /**
       * \param o The origin of this new coordinate system described
       * as a point in the reference coordinate system.
       */
      BasicCoordSystem(BasicMat<M,N,T> const &b, BasicVec<M,T> const &o)
      {
        this->basis  = b;
        this->origin = o;
      }

      /**
       * Combine with the specified coordinate system such that (*this
       * * s) * v = *this * (s * v).
       */
      BasicCoordSystem &operator*=(BasicCoordSystem<N,N,T> const &s)
      {
        this->origin += this->basis * s.origin;
        this->basis *= s.basis;
        return *this;
      }

      /**
       * Map this coordinate system through the specified one such
       * that (s * *this) * v = s * (*this * v).
       */
      BasicCoordSystem &pre_mult(BasicCoordSystem<M,M,T> const &s)
      {
        this->origin = s.origin + s.basis * this->origin;
        this->basis = s.basis * this->basis;
        return *this;
      }

      /**
       * Map the specified locally described vector into one that is
       * described relative to the reference coordinate system.
       */
      template<class R, class E> BasicVec<M> operator*(VecVal<N,T,R,E> const &v) const
      {
        return this->origin + this->basis * v;
      }

      /**
       * Map the specified locally described vector into one that is
       * described relative to the reference coordinate system.
       */
      template<class R, class E> BasicVec<M> operator()(VecVal<N,T,R,E> const &v) const
      {
        return *this * v;
      }

      /**
       * Translate this coordinate system by the specified locally
       * described vector.
       */
      template<class R, class E> void translate(VecVal<N,T,R,E> const &v)
      {
	this->origin += this->basis * v;
      }
    };



    typedef BasicCoordSystem<2> CoordSystem2;
    typedef BasicCoordSystem<3> CoordSystem3;
    typedef BasicCoordSystem<4> CoordSystem4;

    typedef BasicCoordSystem<2,2> CoordSystem2x2;
    typedef BasicCoordSystem<2,3> CoordSystem2x3;
    typedef BasicCoordSystem<2,4> CoordSystem2x4;

    typedef BasicCoordSystem<3,2> CoordSystem3x2;
    typedef BasicCoordSystem<3,3> CoordSystem3x3;
    typedef BasicCoordSystem<3,4> CoordSystem3x4;

    typedef BasicCoordSystem<4,2> CoordSystem4x2;
    typedef BasicCoordSystem<4,3> CoordSystem4x3;
    typedef BasicCoordSystem<4,4> CoordSystem4x4;


    typedef BasicCoordSystem<2,2, float> CoordSystem2F;
    typedef BasicCoordSystem<3,3, float> CoordSystem3F;
    typedef BasicCoordSystem<4,4, float> CoordSystem4F;

    typedef BasicCoordSystem<2,2, float> CoordSystem2x2F;
    typedef BasicCoordSystem<2,3, float> CoordSystem2x3F;
    typedef BasicCoordSystem<2,4, float> CoordSystem2x4F;

    typedef BasicCoordSystem<3,2, float> CoordSystem3x2F;
    typedef BasicCoordSystem<3,3, float> CoordSystem3x3F;
    typedef BasicCoordSystem<3,4, float> CoordSystem3x4F;

    typedef BasicCoordSystem<4,2, float> CoordSystem4x2F;
    typedef BasicCoordSystem<4,3, float> CoordSystem4x3F;
    typedef BasicCoordSystem<4,4, float> CoordSystem4x4F;


    typedef BasicCoordSystem<2,2, long double> CoordSystem2L;
    typedef BasicCoordSystem<3,3, long double> CoordSystem3L;
    typedef BasicCoordSystem<4,4, long double> CoordSystem4L;

    typedef BasicCoordSystem<2,2, long double> CoordSystem2x2L;
    typedef BasicCoordSystem<2,3, long double> CoordSystem2x3L;
    typedef BasicCoordSystem<2,4, long double> CoordSystem2x4L;

    typedef BasicCoordSystem<3,2, long double> CoordSystem3x2L;
    typedef BasicCoordSystem<3,3, long double> CoordSystem3x3L;
    typedef BasicCoordSystem<3,4, long double> CoordSystem3x4L;

    typedef BasicCoordSystem<4,2, long double> CoordSystem4x2L;
    typedef BasicCoordSystem<4,3, long double> CoordSystem4x3L;
    typedef BasicCoordSystem<4,4, long double> CoordSystem4x4L;






    // Implementation

    template<int N, class T> BasicCoordSystem<N,N,T> const &CoordSystemSpec<N,N,T>::identity()
    {
      static BasicCoordSystem<N,N,T> const i(BasicMat<N,N,T>::identity(), BasicVec<N,T>::zero());
      return i;
    }
  }
}

#endif // ARCHON_MATH_COORD_SYSTEM_HPP
