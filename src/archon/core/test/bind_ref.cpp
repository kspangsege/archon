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
 * Testing the binding references.
 */


#include <stdexcept>
#include <algorithm>
#include <map>
#include <iostream>

#include <archon/core/bind_ref.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;


namespace
{
  map<int, int> counts;

  void inc(int i) { ++counts[i]; }
  void dec(int i) { --counts[i]; }
  bool inc_safe(int i)
  {
    if(0 < counts[i])
    {
      ++counts[i];
      return true;
    }
    return false;
  }

  struct Traits
  {
    static void bind(int src) throw() { inc(src); }
    static bool bind_safe(int src) throw() { return inc_safe(src); }
    static void unbind(int r) { dec(r); }
  };

  typedef BindRef<int, Traits> Ref;
}


int main() throw()
{
  TEST(counts.size() == 0);
  {
    Ref a;
    TEST(counts.size() == 0);

    {
      Ref b(3);
      TEST(counts.size() == 1);
      TEST(counts[3] == 1);
      a = b;
      TEST(counts.size() == 1);
      TEST(counts[3] == 2);
    }

    TEST(counts.size() == 1);
    TEST(counts[3] == 1);
  }

  TEST(counts.size() == 1);
  TEST(counts[3] == 0);

  cout << "OK" << endl;
  return 0;
}
