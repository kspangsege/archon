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

#include <archon/util/prime.hpp>
#include <archon/util/ticker.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;


int main() throw()
{
  TEST_MSG(get_prime_not_under(0) == 1, "Bad prime for zero");
  TEST_MSG(get_prime_not_under(2147483647) == 2147483647, "Bad prime for 2147483647");
  TEST_MSG(get_prime_not_under(numeric_limits<long>::min()) == 1, "Bad min prime");
  TEST_MSG(get_prime_not_under(numeric_limits<long>::max()) == 2147483647, "Bad max prime");

  {
    int const n = 0x1000000;
    ProgressBar progress;
    ProgressTicker ticker(&progress, n);
    long prev = 0;
    for(int j=0; j<n; ++j)
    {
      long base = j<<7;
      for(int i = 0<j ? 0 : 1; i < 0x80; ++i)
      {
        long const num = base + i;
        long const prim = get_prime_not_under(num);
        TEST_MSG(num <= prim, "Prime under arg");
        TEST_MSG(prev <= prim, "Not monotonic");
        TEST_MSG(prev == prim || prev < num, "Not consistent");
        double const inc = double(prim-num) / num;
        bool const over_inc = (170 < num ? 0.12 : 24 < num ? 0.16 : 0.38) < inc;
        TEST_MSG(!over_inc, "Excessive increase");
        prev = prim;
      }
      ticker.tick();
    }
  }

  cerr << "OK" << endl;

  return 0;
}
