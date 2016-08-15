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
 * Testing the bit cast function.
 */

#include <stdexcept>
#include <limits>
#include <iostream>

#include <archon/core/functions.hpp>
#include <archon/core/random.hpp>
#include <archon/core/text.hpp>
#include <archon/core/cxx.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;


namespace
{
  template<class T> bool bit(T v, int i)
  {
    return v & T(1) << i;
  }

  Random random;

  template<class Source, class Target> void test()
  {
    for(long i=0; i<100000; ++i)
    {
      Source v;

      for(size_t j=0; j<sizeof(Source); ++j)
        reinterpret_cast<unsigned char *>(&v)[j] = random.get_uniform() * 256;

      Target const w = bit_cast<Target>(v);

      int const source_bits = sizeof(Source)*numeric_limits<unsigned char>::digits;
      int const target_bits = sizeof(Target)*numeric_limits<unsigned char>::digits;
      try
      {
        for(int j=0; j<target_bits; ++j)
          TEST(bit(w,j) == (j < source_bits && bit(v,j)));
      }
      catch(...)
      {
        Text::format_binary(cout << "Source: ", v, true, true) << " : " << cxx::type<Source>() << endl;
        Text::format_binary(cout << "Target: ", w, true, true) << " : " << cxx::type<Target>() << endl;
        throw;
      }
    }
  }

  template<class Source> void test()
  {
    test<Source, char>();
    test<Source, signed char>();
    test<Source, unsigned char>();
    test<Source, short>();
    test<Source, unsigned short>();
    test<Source, int>();
    test<Source, unsigned int>();
    test<Source, long>();
    test<Source, unsigned long>();
    test<Source, wchar_t>();
  }
}

int main() throw()
{
  test<char>();
  test<signed char>();
  test<unsigned char>();
  test<short>();
  test<unsigned short>();
  test<int>();
  test<unsigned int>();
  test<long>();
  test<unsigned long>();
  test<wchar_t>();

  cout << "OK" << endl;
  return 0;
}
