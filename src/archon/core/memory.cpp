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

#include <algorithm>

#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>

using namespace std;


namespace
{
  vector<bool> detect_native_endianness()
  {
    ARCHON_STATIC_ASSERT(1 < sizeof (archon::UIntMax), "Unexpected size of maxint type");
    archon::UIntMax v = 1;
    char *p = reinterpret_cast<char *>(&v);
    // The bit pattern of the index of the non-zero byte describes the
    // native endianness in exactly the way described above.
    int i;
    for(i=0; i<static_cast<int>(sizeof(v)); ++i) if(p[i]) break;

    vector<bool> r;
    // Determine the number of bits in the endianness descriptor
    int n = 0;
    int w = sizeof(v) >> 1;
    while(w)
    {
      r.push_back(i&w);
      w >>= 1;
      ++n;
    }
    reverse(r.begin(), r.end());
    return r;
  }
}


namespace archon
{
  namespace Core
  {
    bool compare_endianness(vector<bool> const &a, vector<bool> const &b,
                            int levels)
    {
      if(levels == 0 || a.empty() && b.empty()) return true;
      vector<bool> const &c = a.empty() ? native_endianness : a;
      vector<bool> const &d = b.empty() ? native_endianness : b;

      int const m = c.size(), n = d.size();
      if(levels < 0) levels = max(m,n);
      for(int i=0; i<levels; ++i) if(c[i < m ? i : m-1] != d[i < n ? i : n-1]) return false;

      return true;
    }


    vector<bool> const native_endianness(detect_native_endianness());

    bool const is_little_endian = compare_endianness(vector<bool>(1,false));
    bool const is_big_endian    = compare_endianness(vector<bool>(1,true));
    bool const is_clean_endian  = is_little_endian || is_big_endian;

    vector<bool> const little_endianness =
      is_little_endian ? vector<bool>() : vector<bool>(1,false);
    vector<bool> const big_endianness =
      is_big_endian ? vector<bool>() : vector<bool>(1,true);


    vector<int> compute_byte_perm(vector<bool> const &endianness, int levels)
    {
      // Construct identity map
      int n = 1<<levels;
      vector<int> permutation(n);
      for(int i=0; i<n; ++i) permutation[i] = i;
      if(endianness.empty()) return permutation;

      // For each level
      for(int i=0; i<levels; ++i)
      {
	// Swap only if we have endianness disagreement on this level
	if(native_endianness[i < static_cast<int>(native_endianness.size()) ?
                             i : native_endianness.size()-1] ==
	   endianness[i < static_cast<int>(endianness.size()) ?
                      i : endianness.size()-1]) continue;

	int swapBlockSize = 1 << i+1;
	int numberOfSwapBlocks = n / swapBlockSize;
	int numberOfSwapsPerBlock = swapBlockSize / 2;

	// For each block that needs to be swapped
	for(int j=0; j<numberOfSwapBlocks; ++j)
	{
	  // For each swap
	  for(int k=0; k<numberOfSwapsPerBlock; ++k)
	  {
	    int offset = j * swapBlockSize + k;
            std::swap(permutation[offset],
                      permutation[offset+numberOfSwapsPerBlock]);
	  }
	}
      }

      return permutation;
    }
  }
}
