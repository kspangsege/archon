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
 * This file can bee seen as an extension to the standard library
 * header <numeric>. It provides a number of generic functions that
 * deal with numerical calculations.
 */

#ifndef ARCHON_CORE_NUMERIC_HPP
#define ARCHON_CORE_NUMERIC_HPP

#include <iterator>


namespace archon
{
  namespace core
  {
    /**
     * This function is similar to std::partial_sum() but allows one
     * to specify a initial value for the summation. This extension is
     * generic enough that it really ought to be in the STL. As a
     * matter of fact, without this extension, \c partial_sum is a far
     * less usefull function.
     */
    template<class In, class Out, class BinOp, class T>
    Out partial_sum(In begin, In end, Out result, BinOp binop, T init)
    {
      typedef typename std::iterator_traits<In>::value_type ValueType;

      ValueType v = init;
      while(begin != end)
      {
        v = binop(v, *begin++);
        *result++ = v;
      }
      return result;
    }


    /**
     * Same as partial_sum() except that this one returns the final sum.
     */
    template<class In, class Out, class BinOp, class T>
    T partial_sum_alt(In begin, In end, Out result, BinOp binop, T init)
    {
      typedef typename std::iterator_traits<In>::value_type ValueType;

      ValueType v = init;
      while(begin != end)
      {
        v = binop(v, *begin++);
        *result++ = v;
      }
      return v;
    }
  }
}

#endif // ARCHON_CORE_NUMERIC_HPP
