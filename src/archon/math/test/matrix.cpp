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
 *
 * Testing the matrix implementation.
 */

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <archon/core/cxx.hpp>
#include <archon/core/random.hpp>
#include <archon/math/matrix.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Math;

namespace
{
  void test_basic()
  {
    BasicMat<7, 9, double> M;
    BasicVec<9> v;
    BasicVec<7> w;
    w = M*v;
    BasicMat<9, 9, double> M2;
    M *= M2;
  }


  Random g;

  template<int N, typename T> void test_square()
  {
    BasicMat<N,N,T> m;
    for(int i=0; i<N; ++i) for(int j=0; j<N; ++j) m(i,j) = g.get_uniform()+0.1;

    BasicMat<N,N,T> m2 = inv(m);

    BasicMat<N,N,T> m3 = m*m2;
    for(int i=0; i<N; ++i) for(int j=0; j<N; ++j) if(abs(m3(i,j)) < 1e-14) m3(i,j) = 0;

    cout << m3 << endl;
  }

  void test_inverse()
  {
    long double d[] =
      {
        +0.299000, +0.587000, +0.114000,
        -0.168736, -0.331264, +0.500000,
        +0.500000, -0.418688, -0.081312
      };

    Mat3L m(d);

    cout << m*inv(m) << endl;
  }
}


int main() throw()
{
  test_basic();
  test_square<50, long double>();
  test_inverse();

  return 0;
}
