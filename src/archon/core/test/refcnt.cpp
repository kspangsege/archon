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
 * Testing intrusive reference counting.
 */


#include <stdexcept>
#include <algorithm>
#include <iostream>

#include <archon/core/refcnt.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace Archon::Core;


namespace
{
  size_t n_a = 0, n_b = 0;

  struct A: CntRefObjectBase
  {
    A()  { ++n_a; }
    ~A() { --n_a; }
  };

  struct B: A
  {
    B()  { ++n_b; }
    ~B() { --n_b; }
  };


  void test_init()
  {
    TEST_MSG(n_a == 0 && n_b == 0, "init 1");
    {
      CntRef<A> a;
      TEST_MSG(n_a == 0 && n_b == 0 && !a, "init 2");
      CntRef<A> b((CntRefNullTag()));
      TEST_MSG(n_a == 0 && n_b == 0 && !b, "init 3");
      CntRef<A> c(0, CntRefSafeTag());
      TEST_MSG(n_a == 0 && n_b == 0 && !c, "init 4");
      A d;
      TEST_MSG(n_a == 1 && n_b == 0, "init 5");
      CntRef<A> e(&d, CntRefSafeTag());
      TEST_MSG(n_a == 1 && n_b == 0 && !e, "init 6");
    }
    TEST_MSG(n_a == 0 && n_b == 0, "init 7");
  }


  void test_equal()
  {
    TEST_MSG(n_a == 0 && n_b == 0, "equal 1");
    {
      CntRef<A> a, b;
      TEST_MSG(n_a == 0 && n_b == 0 && a == b && !(a != b), "equal 2");
      a.reset(new A);
      TEST_MSG(n_a == 1 && n_b == 0 && a != b && !(a == b), "equal 3");
      b.reset(a.get());
      TEST_MSG(n_a == 1 && n_b == 0 && a == b && !(a != b), "equal 4");
      a.reset(new B);
      TEST_MSG(n_a == 2 && n_b == 1 && a != b && !(a == b), "equal 5");
      a = b;
      TEST_MSG(n_a == 1 && n_b == 0 && a == b && !(a != b), "equal 6");
    }
    TEST_MSG(n_a == 0 && n_b == 0, "equal 7");
  }


  void test_swap()
  {
    TEST_MSG(n_a == 0 && n_b == 0, "swap 1");
    {
      CntRef<A> a(new A);
      TEST_MSG(n_a == 1 && n_b == 0, "swap 2");
      {
        CntRef<A> b(new B);
        TEST_MSG(n_a == 2 && n_b == 1, "swap 3");
        swap(a, b);
        TEST_MSG(n_a == 2 && n_b == 1, "swap 4");
      }
      TEST_MSG(n_a == 1 && n_b == 1, "swap 5");
    }
    TEST_MSG(n_a == 0 && n_b == 0, "swap 6");
  }


  void test_cast()
  {
    CntRef<B> b;
    CntRef<A const> a(b);
    a = b;
    TEST_MSG(a == b, "cast 2");
  }
}


int main() throw()
{
  test_init();
  test_equal();
  test_swap();
  test_cast();

  cout << "OK" << endl;
  return 0;
}
