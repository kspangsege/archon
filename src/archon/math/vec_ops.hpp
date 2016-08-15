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
 * Vector operations expressed in terms of iterators.
 */

#ifndef ARCHON_MATH_VEC_OPS_HPP
#define ARCHON_MATH_VEC_OPS_HPP

#include <iterator>
#include <functional>
#include <algorithm>
#include <numeric>
#include <ios>

#include <archon/core/stream_utils.hpp>
#include <archon/math/functions.hpp>


namespace archon
{
  namespace math
  {
    /**
     * Map the components of the vector through the specified unary
     * operation and assign the result back into the same vector.
     */
    template<class Iter, class Oper>
    inline void vec_unop_assign(Iter target, Iter target_end, Oper o)
    {
      std::transform(target, target_end, target, o);
    }


    /**
     * Combine the components of the target and the source vector
     * pairwise and in that order by using the specified binary
     * operator, and assign the result back into the target vector.
     */
    template<class Iter, class Iter2, class Oper>
    inline void vec_binop_assign(Iter target, Iter target_end, Iter2 source, Oper o)
    {
      std::transform(target, target_end, source, target, o);
    }


    /**
     * Add the source vector to the target vector.
     *
     * The calculations are done using the value type of the target
     * vector.
     */
    template<class Iter, class Iter2>
    inline void vec_add_assign(Iter target, Iter target_end, Iter2 source)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_binop_assign(target, target_end, source, std::plus<T>());
    }


    /**
     * Subtract the source vector from the target vector.
     *
     * The calculations are done using the value type of the target
     * vector.
     */
    template<class Iter, class Iter2>
    inline void vec_sub_assign(Iter target, Iter target_end, Iter2 source)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_binop_assign(target, target_end, source, std::minus<T>());
    }


    /**
     * Multiply the components of the source and the target vector
     * pairwise, and assign the result back into the target vector.
     *
     * The calculations are done using the value type of the target
     * vector.
     */
    template<class Iter, class Iter2>
    inline void vec_mul_assign(Iter target, Iter target_end, Iter2 source)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_binop_assign(target, target_end, source, std::multiplies<T>());
    }


    /**
     * Divide the components of the target vector by those at
     * corresponding position in the source vector, and assign the
     * result back into the target vector.
     *
     * The calculations are done using the value type of the target
     * vector.
     */
    template<class Iter, class Iter2>
    inline void vec_div_assign(Iter target, Iter target_end, Iter2 source)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_binop_assign(target, target_end, source, std::divides<T>());
    }


    /**
     * Negate the specified vector.
     */
    template<class Iter>
    inline void vec_neg_assign(Iter target, Iter target_end)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_unop_assign(target, target_end, std::negate<T>());
    }


    /**
     * Add the source vector to the target vector after scaling the
     * source vector by the specified factor.
     *
     * The calculations are done using the value type of the target
     * vector.
     */
    template<class Iter, class Iter2, class U>
    inline void vec_add_scale_assign(Iter target, Iter target_end, Iter2 source, U fact)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      vec_binop_assign(target, target_end, source, AddAlpha<T>(fact));
    }


    /**
     * Determine the dot product (a.k.a. inner product) of the two
     * specified vectors.
     *
     * The calculations are done using the value type of the
     * vector that acts as the left operand.
     */
    template<class Iter, class Iter2>
    inline typename std::iterator_traits<Iter>::value_type
    vec_dot(Iter left, Iter left_end, Iter2 right)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      return std::inner_product(left, left_end, right, T());
    }


    /**
     * Determine the squared distance between the two specified
     * vectors.
     *
     * The calculations are done using the value type of the
     * vector that acts as the left operand.
     */
    template<class Iter, class Iter2>
    inline typename std::iterator_traits<Iter>::value_type
    vec_sq_dist(Iter left, Iter left_end, Iter2 right)
    {
      typedef typename std::iterator_traits<Iter>::value_type T;
      return std::inner_product(left, left_end, right, T(), std::plus<T>(), SqDiff<T>());
    }


    /**
     * Print the specified vector onto the specified output stream.
     */
    template<class C, class T, class Iter>
    inline void vec_print(std::basic_ostream<C,T> &out, Iter source, Iter source_end)
    {
      C const left(out.widen('[')), comma(out.widen(',')), right(out.widen(']'));
      out << left;
      if (source != source_end) {
        out << *source;
        while(++source != source_end) out << comma << *source;
      }
      out << right;
    }


    /**
     * Attempt to read a vector from the specified input stream by
     * parsing a suitable prefix of its contents. If successful,
     * assign the result to the specified target vector.
     *
     * \note If parsing fails, the specified target vector may be in a
     * partially updated state when this method returns.
     */
    template<class C, class T, class Iter>
    inline void vec_parse(std::basic_istream<C,T> &in, Iter target, Iter target_end)
    {
      bool bad = false;
      C const left(in.widen('[')), comma(in.widen(',')), right(in.widen(']'));
      C ch;
      in >> ch;
      if(!in || ch != left) bad = true;
      else
      {
        core::BasicIosFormatResetter<C,T> sentry(in);
        in.setf(std::ios_base::skipws);
        if(target != target_end)
        {
          in >> *target;
          if(!in) bad = true;
          else
          {
            while(++target != target_end)
            {
              in >> ch >> *target;
              if(!in || ch != comma)
              {
                bad = true;
                break;
              }
            }
            if(!bad)
            {
              in >> ch;
              if(!in || ch != right) bad = true;
            }
          }
        }
      }
      if(bad) in.setstate(std::ios_base::badbit);
    }
  }
}

#endif // ARCHON_MATH_VEC_OPS_HPP
