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
 * Testing the vector implementation.
 */

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <archon/math/vector.hpp>

using namespace std;
using namespace archon::math;

namespace
{
  int sq_sum(int n)
  {
    return n*(n-1)*(2*n-1)/6;
  }

  template<int N, typename T> void test0()
  {
    {
      BasicVec<N,T> v(9);
      BasicVec<N,T> const w(7);
      // Methods of _VecImpl::Base
      if(!v._is_lval || !v.template slice<0>()._is_lval ||
         !w._is_lval || !w.template slice<0>()._is_lval ||
         (v+w)._is_lval || (v+w).template slice<0>()._is_lval ||
         N != v.size() || N != w.size() ||
         N != v.end() - v.begin() || N != v.rend() - v.rbegin() ||
         N != w.end() - w.begin() || N != w.rend() - w.rbegin())
        throw runtime_error("Fail");

      // Methods of _VecImpl::Basic
      if(abs(w.get() - v.get()) < N)
        throw runtime_error("Fail");
    }
    {
      BasicVec<N,T> v;
      BasicVec<N,T> w;

      // Methods of _VecImpl::Base
      v.set(8);
      if(sq_sum(v) != N*64)
        throw runtime_error("Fail");
      T a[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
      w.set(a);
      if(sq_sum(w) != sq_sum(N))
        throw runtime_error("Fail");
      w.set(v+w);
      if(sq_sum(w) != sq_sum(8+N)-sq_sum(8))
        throw runtime_error("Fail");
      v /= 2;
      w = v+w;
      if(sq_sum(w) != sq_sum(12+N)-sq_sum(12))
        throw runtime_error("Fail");
      v *= 2;
      v.map(std::negate<T>());
      w -= v;
      if(sq_sum(w) != sq_sum(20+N)-sq_sum(20))
        throw runtime_error("Fail");
      v.neg();
      w += v;
      if(sq_sum(w) != sq_sum(28+N)-sq_sum(28))
        throw runtime_error("Fail");

      // Global functions
      //cerr << dot(v, w) + sq_sum(v) << endl;
      //cerr << proj(v, w) << endl;
      v = BasicVec<N,T>::zero();
      if(len(v) != 0)
        throw runtime_error("Fail");
    }
  }

  template<int N, typename T> void test1()
  {
    test0<N,T>();

    BasicVec<N,T> v(2);
    BasicVec<N,T> const w(4);

    // Methods of _VecImpl::Base
    if(v.sub(0) != 2 || v[N-1] != 2 || len(v.template slice<1>()) != 2 ||
       w.sub(0) != 4 || w[N-1] != 4 || len(w.template slice<1>()) != 4)
      throw runtime_error("Fail");

    v.set(0.0);
    v[0] = 4;
    v.unit();
    if(v[0] != 1 || len(v) != 1)
      throw runtime_error("Fail");

    BasicVec<N,T> u(4);
    u.proj(v);
    if(u[0] != 4 || len(u) != 4)
      throw runtime_error("Fail");
  }

  template<typename T> void test_len()
  {
    //test0<0,T>();
    test1<1,T>();
    test1<2,T>();
    test1<3,T>();
    test1<4,T>();
    test1<5,T>();

    {
      BasicVec<2,T> v(7,9);
      if(v[0] != 7 || v[1] != 9)
        throw runtime_error("Fail");
      v.set(3,5);
      v.perp();
      if(v[0] != -5 || v[1] != 3)
        throw runtime_error("Fail");
    }
    {
      BasicVec<3,T> v, w(4,5,6);
      v.set(1,2,3);
      v *= w;
      if(v[0] != -3 || v[1] != 6 || v[2] != -3)
        throw runtime_error("Fail");
    }
    {
      BasicVec<4,T> v(0,1,2,3);
      if(v[0] != 0 || v[1] != 1 || v[2] != 2 || v[3] != 3)
        throw runtime_error("Fail");
      v.set(3,2,1,0);
      if(v[0] != 3 || v[1] != 2 || v[2] != 1 || v[3] != 0)
        throw runtime_error("Fail");
    }
  }
}

int main() throw()
{
  //test_len<int>();
  test_len<float>();
  test_len<double>();
  test_len<long double>();

  Vec2 x(1,1);

  cerr << x << endl;

  cerr << ang(x) << endl;
  cerr << perp(x) << endl;

  return 0;
}
