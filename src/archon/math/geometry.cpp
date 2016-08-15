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

#include <limits>

#include <archon/math/geometry.hpp>


using namespace std;


namespace archon
{
  namespace math
  {
    /**
     * \todo FIXME: This can probably be generalized to N-D space as
     * sum(i,1,N,p(i).dist*sgn(j,1,N,(j+i-1)%N+1)*cross(j,1,N-1,p((j+i-1)%N+1).normal))/d where
     * p(i) is the i'th hyperplane, sgn() is the permutation signature, cross() is
     * the generalized N-D cross product taking N-1 arguments, and d is the determinant of
     * the square matrix whose i'th row (or column) is p(i).normal.
     *
     * \sa http://local.wasp.uwa.edu.au/~pbourke/geometry/3planes/
     *
     * \sa http://www.ittc.co.jp/us/cadl/us_cadl.htm
     *
     * \sa http://geometryalgorithms.com/Archive/algorithm_0104/algorithm_0104.htm
     */
    Vec3 intersect(Hyperplane3 const &p1, Hyperplane3 const &p2, Hyperplane3 const &p3)
    {
      Vec3 const v1 = p2.normal * p3.normal;
      double bunbo = dot(p1.normal, v1);
      if(bunbo == 0) bunbo = numeric_limits<double>::min(); // Just prevent division by zero
      return (v1 * p1.dist - (p2.normal * p3.dist - p3.normal * p2.dist) * p1.normal) / bunbo;
    }
  }
}
