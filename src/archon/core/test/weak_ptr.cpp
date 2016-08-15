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
#include <sstream>
#include <iostream>

#include <archon/core/weak_ptr.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;


namespace
{
  int n = 0;

  struct A
  {
    ~A() { ++n; }
  };

  // The following shows how one can use a weak_ptr to allow a member
  // function to contruct a SharedPtr from 'this' under the assumption
  // that 'this' is already managed by a SharedPtr.
  struct B
  {
    static SharedPtr<B> make()
    {
      SharedPtr<B> b(new B);
      b->weak_self = b;
      return b;
    }

    SharedPtr<B> get_shared_ptr()
    {
      return SharedPtr<B>(weak_self);
    }

    ~B() { ++n; }

  private:
    B() {}
    WeakPtr<B> weak_self;
  };
}

int main() throw()
{
  TEST(n == 0);
  {
    SharedPtr<A> a(new A);
    TEST(n == 0);
    WeakPtr<A> b(a);
    TEST(n == 0);
    TEST(b.lock().get() != 0);
    {
      SharedPtr<A> c(b);
    }
    a.reset();
    TEST(n == 1);
    TEST(b.lock().get() == 0);
    try
    {
      SharedPtr<A> c(b);
      TEST(false);
    }
    catch(BadWeakPtr &) {}
  }
  TEST(n == 1);

  {
    SharedPtr<B> b = B::make();
    TEST(n == 1);
    {
      SharedPtr<B> c = b->get_shared_ptr();
      TEST(n == 1);
    }
    TEST(n == 1);
  }
  TEST(n == 2);

  cout << "OK" << endl;

  return 0;
}
