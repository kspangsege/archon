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
 * Testing the fraction representation conversion functions.
 */

#include <stdexcept>
#include <iostream>
#include <limits>

#include <archon/core/types.hpp>
#include <archon/core/cxx.hpp>
#include <archon/core/text.hpp>
#include <archon/util/unit_frac.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;
using namespace archon::Util;


namespace
{
 /*
   * Test that the shift based conversion agrees with the floating
   * point base conversion.
   */
  template<typename Int> void test_adjust_bit_width_2(int n, int m)
  {
    TEST_MSG(0 <= n && n <= numeric_limits<Int>::digits, "Bad source width");
    TEST_MSG(0 <= m && m <= numeric_limits<Int>::digits, "Bad target width");

    int const extra_bits = 2; // Require two extra bit of the float type
    typedef typename FastestFloatCover<Int, extra_bits>::type Float;
    int const int_bits = numeric_limits<Int>::digits;
    int const float_bits = numeric_limits<Float>::radix == 2 ? numeric_limits<Float>::digits :
      numeric_limits<Float>::digits * log(numeric_limits<Float>::radix) / log(2);
    int const fuzzy_bits = max(int_bits - (float_bits - extra_bits), 0);
    Int const fuzzy_factor = 1 << fuzzy_bits;


    Int denom1 = 0, denom2 = 0;
    if(n < numeric_limits<Int>::digits) { denom1 = 1; denom1 <<= n; }
    if(m < numeric_limits<Int>::digits) { denom2 = 1; denom2 <<= m; }
    Int max1 = denom1 ? denom1 - 1 : numeric_limits<Int>::max();
    unsigned long max2 = 1<<21 - 1;
    typedef typename WidestType<Int, unsigned long>::type Widest;
    unsigned long num_iters = min<Widest>(max1, max2) + 1;
    for(unsigned long i=0; i<num_iters; ++i)
    {
      Float v = i;
      v /= (num_iters-1);
      v *= denom1 ? denom1 : Float(numeric_limits<Int>::max()) + 1;
      Int const w = Float(max1) <= v ? max1 : Int(v);
      Int const a = frac_adjust_bit_width(w,n,m);
      Int const b = frac_float_to_int<Float, Int>(frac_int_to_float<Int, Float>(w, denom1), denom2);
      if((a < b ? b - a : a - b) / fuzzy_factor == 0) continue;
      TEST_MSG(false, "Mismatch for "+Text::print(to_num(w))+": "
               ""+Text::print(n)+" bits -> "+Text::print(m)+" bits"", "
               "int type = '"+Cxx::type<Int>()+"', float type = '"+Cxx::type<Float>()+"', "
               "fuzzy factor = "+Text::print(fuzzy_factor)+", "
               ""+Text::print(to_num(a))+" (shift) != "+Text::print(to_num(b))+" (float)");
    }
  }

  template<typename Int> void test_adjust_bit_width_1()
  {
    int bit_widths[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 30, 31, 32, 33, 62, 63, 64 };
    int n = sizeof(bit_widths)/sizeof(*bit_widths);
    int m = 0;
    while(m < n && bit_widths[m] <= numeric_limits<Int>::digits) ++m;
    for(int i=0; i<m; ++i)
      for(int j=0; j<m; ++j)
        test_adjust_bit_width_2<Int>(bit_widths[i], bit_widths[j]);
  }
}


int main() throw()
{
  test_adjust_bit_width_1<char>();
  test_adjust_bit_width_1<signed   char>();
  test_adjust_bit_width_1<unsigned char>();
  test_adjust_bit_width_1<signed   short>();
  test_adjust_bit_width_1<unsigned short>();
  test_adjust_bit_width_1<signed   int>();
  test_adjust_bit_width_1<unsigned int>();
  test_adjust_bit_width_1<signed   long>();
  test_adjust_bit_width_1<unsigned long>();

  cout << "OK" << endl;

  return 0;
}
