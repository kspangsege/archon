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

#ifndef ARCHON_MATH_INTERVAL_HPP
#define ARCHON_MATH_INTERVAL_HPP

#include <cmath>
#include <algorithm>


namespace archon
{
  namespace Math
  {
    /**
     * An abstract interval.
     */
    template<class T> struct BasicInterval
    {
      T begin, end;

      /**
       * Set beginning and end to zero.
       */
      BasicInterval(): begin(T()), end(T()) {}

      BasicInterval(T const &b, T const &e): begin(b), end(e) {}

      /**
       * Construct an origin centered interval of the specified size.
       */
      explicit BasicInterval(T const &size): begin(-0.5*size), end(0.5*size) {}


      BasicInterval &set(T const &b, T const &e) { begin = b; end = e; return *this; }


      /**
       * Get the center of this interval.
       */
      T get_center() const { return 0.5*(begin + end); }


      /**
       * Get the length of this interval.
       */
      T get_length() const;


      /**
       * Translate this interval by the specified amount. This does not
       * change the length of the interval, only its location.
       */
      void translate(T const &v);


      /**
       * Reflect this interval about the origin.
       */
      void reflect();


      /**
       * Expand this interval just enough to cover the specified one.
       *
       * That is, make this interval the least interval that includes
       * both itself and the specified interval.
       */
      void include(BasicInterval const &i);


      /**
       * Scale this interval by the specified scaling factor.
       */
      BasicInterval &operator*=(T const &v) { begin *= v; end *= v; return *this; }


      bool operator==(BasicInterval const &i) const { return begin == i.begin && end == i.end; }
      bool operator!=(BasicInterval const &i) const { return begin != i.begin || end != i.end; }
    };


    typedef BasicInterval<double>      Interval;
    typedef BasicInterval<float>       IntervalF;
    typedef BasicInterval<long double> IntervalL;





    // Implementation:

    template<class T> inline T BasicInterval<T>::get_length() const
    {
      using std::abs;
      return abs(end - begin);
    }

    template<class T> inline void BasicInterval<T>::translate(T const &v)
    {
      begin += v;
      end   += v;
    }

    template<class T> inline void BasicInterval<T>::reflect()
    {
      using std::swap;
      swap(begin, end);
      begin = -begin;
      end   = -end;
    }

    template<class T> inline void BasicInterval<T>::include(BasicInterval<T> const &i)
    {
      using std::min;
      using std::max;
      begin = min(begin, i.begin);
      end   = max(end,   i.end);
    }
  }
}

#endif // ARCHON_MATH_INTERVAL_HPP
