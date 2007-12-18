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

#ifndef ARCHON_CORE_RANDOM_HPP
#define ARCHON_CORE_RANDOM_HPP

#include <cstdlib>
#include <limits>
#include <algorithm>
#include <vector>

#include <archon/core/types.hpp>
#include <archon/core/functions.hpp>
#include <archon/core/unique_ptr.hpp>


namespace Archon
{
  namespace Core
  {
    /**
     * A generator of pseudo random numbers.
     *
     * Thread safety: The non-static \c get_uniform methods of this
     * class is not thread safe, so it is illegal for two threads to
     * simultaneously access a single instance. It is safe, however,
     * for two threads to access two different instances at the same
     * time, such that one thread accesses one instance and the other
     * thread accesses the other instance. All the static methods are
     * thread safe.
     */
    struct Random
    {
      /**
       * Generate a random floating-point value from the standard
       * uniform distribution, that is, a rectangular distribution
       * over the interval <tt>[0,1)</tt>.
       *
       * If you need a value that instead is distributed uniformly
       * over some other interval <tt>[a,b)</tt>, do
       * <tt>a+(b-a)*x</tt> where \c x is a value returned by this
       * method.
       *
       * You can also create a distribution object for the uniform
       * distribution (see <tt>UniformDistrib</tt>) which will provide
       * a bit more flexibility in terms of range.
       */
      double get_uniform() { return get_float(); }


      /**
       * Extract a random integer in the range [0;max]. All possible
       * integers will occur with equal probability (to the exent
       * possible).
       *
       * \param max The maximum integer value to allow.
       *
       * \tparam UInt Any unsigned integer type.
       */
      template<class UInt> UInt get_uint(UInt max);


      template<class UInt> UInt get_uint() { return get_uint(std::numeric_limits<UInt>::max()); }


      bool get_frac(int num, int denom) { return get_uint(denom-1) < num; }


      /**
       * Extract \a n random bits. All 2^n combinations of those
       * bits will occur with equal probability.
       *
       * \param n Number of bits to get. Must be less than or equal to
       * numeric_limits<UInt>::digits.
       *
       * \tparam UInt Any unsigned integer type.
       */
      template<class UInt> UInt get_bits(int n);


      template<class Out> void get_unit_vector(Out begin, Out end);


      /**
       * Initialize this random generator with an random seed.
       */
      Random();

      /**
       * Initialize this random generator with the specified seed.
       */
      Random(unsigned long seed);


      struct Distribution
      {
        virtual double get() = 0;

        virtual ~Distribution() {}
      };


      /**
       * A source of random values following a continuous
       * uniform distribution over the specified interval.
       */
      struct UniformDistrib: Distribution
      {
        double operator()() { return a+(b-a)*r.get_uniform(); }

        double get() { return (*this)(); }

        UniformDistrib(Random *r, double a=0, double b=1): r(*r), a(a), b(b) {}

      private:
        Random &r;
        double const a, b;
      };


      /**
       * Create a source of random values following a continuous
       * normal distribution of the specified mean and standard
       * deviation.
       */
      static UniquePtr<Distribution>
      get_normal_distrib(double mean = 0, double deviation = 1);


      /**
       * Create a source of random values following the discrete
       * poisson distribution with mean <tt>lambda</tt> and variance
       * <tt>lambda</tt>.
       */
      static UniquePtr<Distribution>
      get_poisson_distrib(double lambda);


      /**
       * Create a source of random values following the specified
       * discrete distribution. that is, a distribution that can take
       * on only a finite set of values <tt>[0,n-1]</tt>, where \c n
       * is the length of the specified vector.
       *
       * \param probs The probabilities of each of the possible values
       * that this distribution can take on. That is, the probability
       * of \c i is <tt>probs[i]</tt>. If the probabilities do not add
       * up to 1, each one will be divided by the sum.
       */
      static UniquePtr<Distribution>
      get_finite_distrib(std::vector<double> const &probs);


    private:
      static int const uint_bits = 32;
      typedef FastestUnsignedWithBits<uint_bits>::type UIntType;
      UIntType get_uint()  { return jrand48(xsubi); }
      double   get_float() { return erand48(xsubi); }

      void seed(unsigned long);

      unsigned short xsubi[3];
    };








    // Implementation:

    template<class UInt> inline UInt Random::get_bits(int n)
    {
      UInt v = get_uint();
      int m = uint_bits;
      while(m < n)
      {
        // The modulo is just to silence a compiler warning
        v <<= uint_bits % std::numeric_limits<UInt>::digits;
        v |= UInt(get_uint());
        m += uint_bits;
      }
      return v & bit_range<UInt>(n);
    }


    template<class UInt> inline UInt Random::get_uint(UInt max)
    {
      if(max == std::numeric_limits<UInt>::max())
        return get_bits<UInt>(std::numeric_limits<UInt>::digits);
      UInt const num_vals = max + 1;
      // If 'num_vals' is a power of two
      if((num_vals & max) == 0)
        return get_bits<UInt>(std::numeric_limits<UInt>::digits) & max;

      typedef typename WidestType<UIntType, UInt>::type UInt2;
      int const num_bits = std::max(uint_bits+0, std::numeric_limits<UInt>::digits);
      UInt2 const bulk = bit_range<UInt2>(num_bits);
      UInt2 const num_mods = bulk / num_vals; // Number of modules
      UInt2 const ceil = num_mods * num_vals; // Ceiling above which we must draw again (also if equal)
      for(;;)
      {
        UInt2 const val = get_bits<UInt2>(num_bits);
        if(val < ceil) return val / num_mods;
      }
    }


    template<class Out> inline void Random::get_unit_vector(Out begin, Out end)
    {
      for (;;) {
        double sum = 0;
        for (Out i = begin; i != end; ++i) {
          double const c = *i = 2*get_uniform() - 1;
          sum += c * c;
        }
        if (0.01 <= sum && sum < 1) {
          double const f = 1 / std::sqrt(sum);
          for (Out i = begin; i != end; ++i) *i *= f;
          return;
        }
      }
    }
  }
}

#endif // ARCHON_CORE_RANDOM_HPP
