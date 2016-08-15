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

#ifndef ARCHON_UTIL_STATISTICS_HPP
#define ARCHON_UTIL_STATISTICS_HPP

#include <algorithm>


namespace archon
{
  namespace util
  {
    /**
     * This class provides a means of maintaining the weighted moving
     * average of a sequence of data points. Each time the add()
     * method is called, the 'window' is moved one position ahead in
     * the sequence. With a window of N points the last added point
     * has weight N, the second last has weight N−1, etc.
     */
    template<class T, int N> struct WeightedMovingAverage
    {
      void add(T v)
      {
        numerator += N*v - total;
        total += v - memory[cursor];
        memory[cursor] = v;
        if(++cursor == N) cursor = 0;
      }

      T get() const { return numerator / denominator; }

      WeightedMovingAverage(T start_value = 0):
        denominator(N*(N+1)/2), total(N*start_value),
        numerator(denominator*total), cursor(0)
      {
        std::fill(memory, memory+N, start_value);
      }

    private:
      T const denominator;
      T total, numerator, memory[N];
      int cursor;
    };




    /**
     * On-line algorithm for variance calculation.
     *
     * \sa Donald E. Knuth (1998). The Art of Computer Programming,
     * volume 2: Seminumerical Algorithms, 3rd edn., p. 232. Boston:
     * Addison-Wesley
     */
    template<class T> struct Variance
    {
      void add(T v)
      {
        T const delta = v - mean;
        mean += delta / ++n;
        m2 += delta * (v-mean);
      }

      T get_variance() const { return m2 / (n-1); }

      T get_arith_mean() const { return mean; }

      Variance(): n(0), mean(0), m2(0) {}

    private:
      T n, mean, m2;
    };
  }
}

#endif // ARCHON_UTIL_STATISTICS_HPP
