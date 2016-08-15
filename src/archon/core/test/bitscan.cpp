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
 * Testing the bit-scan functions.
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
  template<class A, class B> B change_type(A v)
  {
    return *reinterpret_cast<B *>(&v);
  }

  template<class S, class U> void test()
  {
    int const n = sizeof(S) * numeric_limits<unsigned char>::digits;
    TEST_MSG(find_least_sig_bit(change_type<U,S>(0)) == -1 &&
             find_most_sig_bit(change_type<U,S>(0))  == -1, "signed zero");
    TEST_MSG(find_least_sig_bit(change_type<U,U>(0)) == -1 &&
             find_most_sig_bit(change_type<U,U>(0))  == -1, "unsigned zero");

    U all = ~(U(0));

    int i;
    U one, above, below;
    try
    {
      for(i=0; i<n; ++i)
      {
        one   = U(1) << i;
        above = all << i;
        below = all >> i;
        TEST_MSG(find_least_sig_bit(change_type<U,S>(one))   == i &&
                 find_most_sig_bit(change_type<U,S>(one))    == i,   "signed one");
        TEST_MSG(find_least_sig_bit(change_type<U,U>(one))   == i &&
                 find_most_sig_bit(change_type<U,U>(one))    == i,   "unsigned one");
        TEST_MSG(find_least_sig_bit(change_type<U,S>(above)) == i &&
                 find_most_sig_bit(change_type<U,S>(above))  == n-1, "signed above");
        TEST_MSG(find_least_sig_bit(change_type<U,U>(above)) == i &&
                 find_most_sig_bit(change_type<U,U>(above))  == n-1, "unsigned above");
        if(i)
        {
          TEST_MSG(find_least_sig_bit(change_type<U,S>(below)) == 0 &&
                   find_most_sig_bit(change_type<U,S>(below))  == n-1-i, "signed below");
          TEST_MSG(find_least_sig_bit(change_type<U,U>(below)) == 0 &&
                   find_most_sig_bit(change_type<U,U>(below))  == n-1-i, "unsigned below");
        }
      }
    }
    catch(...)
    {
      cout << "Type: signed = '" << Cxx::type<S>() << "', unsigned = '" << Cxx::type<U>() << "'" << endl;
      cout << "Position: " << i << endl;
      cerr << "(signed least, signed most), (unsigned least, unsigned most)" << endl;
      Text::format_binary(cout << "One:   ", one, true) << "  "
        "(" << find_least_sig_bit(change_type<U,S>(one)) << ", " << find_most_sig_bit(change_type<U,S>(one)) << "), "
        "(" << find_least_sig_bit(change_type<U,U>(one)) << ", " << find_most_sig_bit(change_type<U,U>(one)) << ")" << endl;
      Text::format_binary(cout << "Above: ", above, true) << "  "
        "(" << find_least_sig_bit(change_type<U,S>(above)) << ", " << find_most_sig_bit(change_type<U,S>(above)) << "), "
        "(" << find_least_sig_bit(change_type<U,U>(above)) << ", " << find_most_sig_bit(change_type<U,U>(above)) << ")" << endl;
      Text::format_binary(cout << "Below: ", below, true) << "  "
        "(" << find_least_sig_bit(change_type<U,S>(below)) << ", " << find_most_sig_bit(change_type<U,S>(below)) << "), "
        "(" << find_least_sig_bit(change_type<U,U>(below)) << ", " << find_most_sig_bit(change_type<U,U>(below)) << ")" << endl;
      throw;
    }

    // Random tests
    Random random;
    for(int j=0; j<10000; ++j)
    {
      U v = 0;
      for(int i=0; i<n-1; ++i)
      {
        v *= 2;
        if(0.5 < random.get_uniform()) v += 1;
      }

      // Sanity
      TEST(find_most_sig_bit<U>(v) < n-1);

      for(int i=0; i<n-1; ++i)
      {
        U above = 2*v+1 << i;
        U below = (U(1)<<n-1)+v >> i;
        TEST_MSG(find_least_sig_bit(change_type<U,S>(above)) == i,     "random signed above "   + Text::print(i));
        TEST_MSG(find_least_sig_bit(change_type<U,U>(above)) == i,     "random unsigned above " + Text::print(i));
        TEST_MSG(find_most_sig_bit(change_type<U,S>(below))  == n-1-i, "random signed below "   + Text::print(i));
        TEST_MSG(find_most_sig_bit(change_type<U,U>(below))  == n-1-i, "random unsigned below " + Text::print(i));
      }
    }
  }
}

int main() throw()
{
  test<signed char,  unsigned char>();
  test<signed short, unsigned short>();
  test<signed int,   unsigned int>();
  test<signed long,  unsigned long>();

  cout << "OK" << endl;
  return 0;
}
