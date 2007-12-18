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

#include <stdexcept>
#include <iostream>

#include <archon/core/unique_ptr.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace Archon::Core;

namespace
{
  struct A
  {
    A() { cerr << "A" << endl; }
    virtual ~A() { cerr << "~A" << endl; }
    virtual void release() { delete this; }
  };

  struct B: A {};

  struct MyDel
  {
    void operator()(A *p) const { if(p) p->release(); }
  };

  typedef UniquePtr<A, MyDel> MyPtrA;
  typedef UniquePtr<B, MyDel> MyPtrB;

  MyPtrA h()
  {
    MyPtrA p(new A);
    return p;
  }

  MyPtrA h2()
  {
    MyPtrA p(h().release());
    return p;
  }

  struct C
  {
    C(MyPtrA b): a(b.release()) {}
    C(MyPtrB b): a(b.release()) {}
    MyPtrA a;
  };

  struct D: C
  {
    D(MyPtrB b): C(b) {}
  };


  void test_1()
  {
    {
      UniquePtr<int> p(new int);
      TEST(p);
      UniquePtr<int> q(p);
      TEST(!p);
      TEST(q);
    }
    {
      UniquePtr<int> p(new int), q;
      TEST(p);
      TEST(!q);
      q = p;
      TEST(!p);
      TEST(q);
    }
  }


  void test_2()
  {
    MyPtrA p(h2().release()), q;

    q.reset(new A);

    swap(p,q);

    q.reset(new A);

    if(q) cerr << "YES" << endl;

    p = q;

    cerr << "CLICK" << endl;

    MyPtrB r(new B);

    q = r;

    MyPtrA s(r);

    D d(r);
  }
}


int main() throw()
{
  test_1();
  test_2();

  return 0;
}
