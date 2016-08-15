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

#ifndef ARCHON_UTIL_PERMUTATION_HPP
#define ARCHON_UTIL_PERMUTATION_HPP

#include<utility>
#include<algorithm>


namespace archon
{
  namespace Util
  {
    struct Parity
    {
      static Parity const even;
      static Parity const odd;
      Parity() {}
      explicit Parity(bool v): val(v) {}
      explicit Parity(int v): val(v&1) {}
      Parity &operator++()    { val ^= true; return *this; }
      Parity  operator++(int) { bool const v = val; val ^= true; return Parity(v); }
      Parity &operator+=(Parity const &p) { val ^= p.val; return *this; }
      Parity  operator+ (Parity const &p) const { return Parity(bool(val ^ p.val)); }
      bool    operator==(Parity const &p) const { return val == p.val; }
      bool    operator!=(Parity const &p) const { return val != p.val; }
    private:
      bool val;
    };

    Parity const Parity::even = Parity(false);
    Parity const Parity::odd  = Parity(true);


    /**
     * Bring the two specified sequences into a common order, and
     * determine the parity of the required permutation.
     *
     * Note that in general the permutation is not unique, but the
     * permutation theorem tells us that the parities of all the
     * possible permutations are indeed the same.
     *
     * The two sequences are expected to contain the same elements but
     * generally in different orders. In that case the position of the
     * returned iterator is one plus the iterator position of the last
     * element in the second sequence, and the elements of the first
     * sequence will be permuted such that they occur in the same
     * order as they do in the second sequence.
     *
     * If the two sequences do not contain the same elements, the
     * returned iterator will point to the first element of the
     * second sequence which do not have a match in the first
     * sequence. Assume that the first such element is at begin2+N,
     * then, upon return, the elements of the first sequence will have
     * been reordered by some permutation that brings the two
     * sequences into agreement on the first N positions, and the
     * returned parity will reflect the that permutation.
     *
     * The sequences may contain duplicate elements.
     *
     * The elements must be totally ordered an be of an order
     * comparable type.
     */
    template<class RanInOut, class FwdIn>
    std::pair<Parity, FwdIn> get_parity_of_permutation(RanInOut begin, RanInOut end, FwdIn begin2);







    // Implementation:

    template<class RanInOut, class FwdIn> inline std::pair<Parity, FwdIn>
    get_parity_of_permutation(RanInOut begin, RanInOut end, FwdIn begin2)
    {
      Parity parity = Parity::even;
      while(begin != end)
      {
        RanInOut const current = begin++;
        typename std::iterator_traits<FwdIn>::value_type const elem = *begin2;
        if(elem != *current)
        {
          RanInOut const iter = std::find(begin, end, elem);
          if(iter == end) break;
          std::swap(*iter, *current);
          ++parity;
        }
        ++begin2;
      }
      return std::make_pair(parity, begin2);
    }
  }
}

#endif // ARCHON_UTIL_PERMUTATION_HPP
