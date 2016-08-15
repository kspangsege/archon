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

#include <archon/core/shared_ptr.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;


namespace
{
  int n = 0;

  struct A
  {
    virtual ~A() { ++n; }
  };

  struct B: A {};
}

int main() throw()
{
  TEST(n == 0);
  {
    SharedPtr<A> b(new B);
    TEST(n == 0);
  }
  TEST(n == 1);
  {
    SharedPtr<A> a;
    TEST(n == 1);
    {
      SharedPtr<B> b(new B);
      TEST(n == 1);
      a = b;
    }
    TEST(n == 1);
  }
  TEST(n == 2);

  {
    n = 0;
    SharedPtr<A> a(new A), b(new B);
    SharedPtr<A> c(a);
    TEST(a == c);
    TEST(a != b);
    TEST(a < b || b < a);
    TEST(n == 0);
    swap(b, c);
    TEST(a == b);
    TEST(a != c);
    TEST(n == 0);
    static_pointer_cast<B>(c);
    TEST(n == 0);
    TEST(dynamic_pointer_cast<B>(a) == 0);
    TEST(dynamic_pointer_cast<B>(c) != 0);
    ostringstream o1, o2;
    o1 << a;
    o2 << c;
    TEST(o1.str() != o2.str());
  }
  TEST(n == 2);

  cout << "OK" << endl;

  return 0;
}
