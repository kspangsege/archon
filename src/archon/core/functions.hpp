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
 * A collection of generic functions working on numbers.
 */

#ifndef ARCHON_CORE_FUNCTIONS_HPP
#define ARCHON_CORE_FUNCTIONS_HPP

#include <cstdlib>
#include <cmath>
#include <limits>
#include <algorithm>
#include <functional>

#include <archon/core/assert.hpp>


namespace archon
{
  namespace core
  {

    // FIXME: Consider this one: How to get the average of two integers
/*
    template<class T> T midpoint(T a, T b)
    {
      return a < b ? a + (b-a)/2 : b + (a-b)/2;
    }
*/


    // FIXME: Consider these
/*
    // Truncation towards negative infinity
    // Assumes 0<b
    template<class Int> Int int_div_neg(Int a, Int b)
    {
      return a<0 ? -((b-a-1)/b) : a/b;
    }

    // Truncation towards positive infinity
    // Assumes 0<b
    template<class Int> Int int_div_pos(Int a, Int b)
    {
      return a<0 ? -(-a/b) : (b-1+a)/b;
    }

    // Truncation towards zero
    // Assumes 0<b
    template<class Int> Int int_div_zero(Int a, Int b)
    {
      return 0<=a ? 0<=b ? a/b : b == numeric_limits<Int>::min() ? 0 : a/-b : 0<=b ?  ----------  a<0 ? -(-a/b) : a/b;
    }
*/



    /**
     * Reliable comparison of two integers of possibly differing
     * types. This is especially relevant when the two types have
     * different signedness, that is, if one is signed and the other
     * is not.
     *
     * \return True if, and only if <tt>a</tt> is less than
     * <tt>b</tt>.
     */
    template<class A, class B> bool int_less_than(A a, B b);

    /**
     * Same idea as <tt>int_less_than</tt>.
     *
     * \return True if, and only if <tt>a</tt> is less than or equal
     * to <tt>b</tt>.
     */
    template<class A, class B> bool int_less_than_equal(A a, B b);

    /**
     * Same idea as <tt>int_less_than</tt>.
     *
     * \return True if, and oonly if <tt>a</tt> is greater than
     * <tt>b</tt>.
     */
    template<class A, class B> bool int_greater_than(A a, B b);

    /**
     * Same idea as <tt>int_less_than</tt>.
     *
     * \return True if, and only if <tt>a</tt> is greater than or
     * equal to <tt>b</tt>.
     */
    template<class A, class B> bool int_greater_than_equal(A a, B b);

    /**
     * Same idea as <tt>int_less_than</tt>.
     *
     * \return True if, and only if <tt>a</tt> is equal to <tt>b</tt>.
     */
    template<class A, class B> bool int_equal(A a, B b);

    /**
     * Same idea as <tt>int_less_than</tt>.
     *
     * \return True if, and only if <tt>a</tt> is not equal to
     * <tt>b</tt>.
     */
    template<class A, class B> bool int_not_equal(A a, B b);



    /**
     * Clamp a value to a certain range.
     *
     * \param v The value to clamp.
     *
     * \param l The lowest allowable value.
     *
     * \param h The highest allowable value.
     *
     * \return The clamped value.
     */
    template<class T> constexpr T clamp(T v, T l, T h)
    {
      return v < l ? l : h < v ? h : v;
    }



    /**
     * Calculate y = x mod m such that x = y (mod m).
     *
     * That is, find y in the half open interval [0,m) such that the
     * difference between x and y is an integer multiple of m.
     *
     * Note: For T=double and non-negative values of x this function
     * is identical to the standard library function 'fmod', whereas
     * for negative values they differ by m. that is modulo(x) =
     * fmod(x) + m for x<0.
     *
     * Note also that this function is periodic but neither "even" nor
     * "odd" where as the standard library function 'fmod' is an "odd"
     * function and not periodic.
     *
     * A typical type of argument would be angels which are cyclic by
     * nature and are often needed in the interval [0,360). Another
     * example is texture coordinates for tiled texture mapping.
     *
     * \param m Must be a strictly positive value.
     */
    template<class T> T modulo(T x, T m);

    /**
     * Like modulo(T, T) except that this function also tells you the
     * index of the module in which the argument resides. That is, n
     * is the integer value such that x = n * m + modulo(x, m).
     *
     * \param n The index of the module in which the argument
     * resides. '0' indicates the principal module [0,m). 1 indicates
     * the next module [m,2m), -1 indicates the previous module [-m;0)
     * and so on.
     */
    template<class T> T modulo(T x, T m, long &n);



    template<class T> inline T min3(T a, T b, T c)
    {
      return std::min(std::min(a,b), c);
    }

    template<class T> inline T max3(T a, T b, T c)
    {
      return std::max(std::max(a,b), c);
    }



    template<class T> void sort3(T x0, T x1, T x2, int &i0, int &i1, int &i2)
    {
      if(x1 > x0)
      {
        if(x2 > x1)      { i0 = 2; i1 = 1; i2 = 0; }
        else if(x2 > x0) { i0 = 1; i1 = 2; i2 = 0; }
        else             { i0 = 1; i1 = 0; i2 = 2; }
      }
      else
      {
        if(x2 > x0)      { i0 = 2; i1 = 0; i2 = 1; }
        else if(x2 > x1) { i0 = 0; i1 = 2; i2 = 1; }
        else             { i0 = 0; i1 = 1; i2 = 2; }
      }
    }



    /**
     * Emulate the behavious of C99's <tt>round</tt>.
     */
    inline float archon_round(float v)
    {
      return v < 0 ? std::ceil(v - 0.5F) : std::floor(v + 0.5F);
    }

    /**
     * Emulate the behavious of C99's <tt>round</tt>.
     */
    inline double archon_round(double v)
    {
      return v < 0 ? std::ceil(v - 0.5) : std::floor(v + 0.5);
    }

    /**
     * Emulate the behavious of C99's <tt>round</tt>.
     */
    inline long double archon_round(long double v)
    {
      return v < 0 ? std::ceil(v - 0.5L) : std::floor(v + 0.5L);
    }



    /**
     * Convert the specified floating point value to an integer. The
     * result is the representable integer that is closest to the
     * integer part of the floating point value.
     */
    template<class Int, class Float> Int clamp_float_to_int(Float f);

    /**
     * Convert the specified value to some arbitrary other type. Any
     * value that is greater than the maximum value of the target
     * type, or less that the minimum value of the target type, is
     * clamped.
     */
    template<class Target, class Source> Target clamp_any_to_any(Source v);




    /**
     * Get the index of the least significant bit in the argument. The
     * argument type must be a signed or an unsigned integer. Negative
     * values give correct results only if the platform represents
     * them in two's complement form.
     *
     * This is a terribly slow implementation considdering the fact that a
     * BITSCAN maschine instruction can answer in a few clock cycles.
     *
     * \param i The integer to scan.
     *
     * \return The index of the least significant bit in the argument
     * counting from the position of least significance. If no bits
     * are found, -1 is returned.
     */
    template<class Int> int find_least_sig_bit(Int i);


    /**
     * Get the index of the most significant bit in the argument. The
     * argument type must be a signed or an unsigned integer. Negative
     * values give correct results only if the platform represents
     * them in two's complement form.
     *
     * This is a terribly slow implementation considdering the fact that a
     * BITSCAN machine instruction can answer in a few clock cycles.
     *
     * \param i The integer to scan.
     *
     * \return The index of the most significant bit in the argument
     * counting from the position of least significance. If no bits
     * are found, -1 is returned.
     *
     * \note For positive non-zero arguments the returned value is
     * equal (in theory) to the integer part of <tt>log2(i)</tt>.
     *
     * \note <tt>find_most_sig_bit(v) + 1</tt> is the number of
     * bits required to hold the value <tt>i</tt>.
     */
    template<class Int> int find_most_sig_bit(Int i);


    /**
     * Copy bit pattern from one integral type to another.
     *
     * Let \c bit(v,i) be a function that returns 1 if the bit at
     * position \c i of \c v is set, and 0 if it is not set. Positions
     * are counted starting from the position of least significance.
     *
     * Also let \c n be the number of bits in the source type
     * <tt>U</tt>, then this function is defined as follows:
     *
     * <pre>
     *
     *   bit(bit_cast<T,U>(v), i) = bit(v, i) if i < n, else 0
     *
     * </pre>
     *
     * Note that this is unlike ordinary casting, which attempts to be
     * value preserving rather than bit pattern preserving, and also
     * has undefined behaviour for many combinations involving signed
     * types.
     *
     * Both source and destination types must be one of the
     * following: <tt>char</tt>, <tt>signed char</tt>, <tt>unsigned
     * char</tt>, <tt>short</tt>, <tt>unsigned short</tt>,
     * <tt>int</tt>, <tt>unsigned int</tt>, <tt>long</tt>,
     * <tt>unsigned long</tt>, <tt>long long</tt>, <tt>unsigned long
     * long</tt>, <tt>wchar_t</tt>.
     *
     * This method assumes that the platform uses two's complement
     * represenation of negative values. Note that this is not
     * guaranteed by standard C++.
     *
     * \todo FIXME: Should be changed to be based on reintepret_cast
     * on lvalues.
     */
    template<class T, class U> T bit_cast(U v);


    /**
     * Produce a value with \a width consecutive bits, and with least
     * significant bit at position \a offset (i.e., value of least
     * significant bit is 2**\a offset).
     *
     * The result is undefined if either argument is negative, or if
     * their sum is greater than the number of value bits in \a T.
     *
     * \tparam T Any integer type for which std::numeric_limits is
     * properly specialized.
     */
    template<class T> T bit_range(int width, int offset = 0);


    /**
     * Shift the bit pattern of the specified integral value to the
     * right by the specified number of positions.
     *
     * The result has a bit set on position \c i if, and only if \c v
     * has a bit set on position <tt>i+n</tt>, which of course, it
     * only has if \c i+n refers to a valid position in
     * <tt>T</tt>. Positions are counted starting from the position of
     * least significance.
     *
     * Note that this is unlike ordinary shifting, which attempts to
     * be value preserving rather than bit pattern preserving, and
     * also has undefined behaviour for many combinations involving
     * signed types.
     *
     * \param n The number of positions to shift to the right. This may
     * be a negative value, in which case the bits will be shifted to
     * the left. If \c abs(n) is greater than, or equal to the number
     * of bits in <tt>T</tt>, then the result will be zero.
     *
     * \tparam T Must be one of the following: <tt>char</tt>,
     * <tt>signed char</tt>, <tt>unsigned char</tt>, <tt>short</tt>,
     * <tt>unsigned short</tt>, <tt>int</tt>, <tt>unsigned int</tt>,
     * <tt>long</tt>, <tt>unsigned long</tt>, <tt>long long</tt>,
     * <tt>unsigned long long</tt>, <tt>wchar_t</tt>.
     *
     * This method assumes that the platform uses two's complement
     * represenation of negative values. Note that this is not
     * guaranteed by standard C++.
     */
    template<class T> T bit_shift_right(T v, int n);

    /**
     * Shift the bit pattern of the specified integral value to the
     * right by the specified number of positions.
     *
     * The result is equal to <tt>bit_shift_right(v, -n)</tt>.
     */
    template<class T> T bit_shift_left(T v, int n);





    // Function objects

    template<class T> struct Min: std::binary_function<T,T,T>
    {
      T operator()(T v, T w) const { return std::min(v,w); }
    };

    template<class T> struct Max: std::binary_function<T,T,T>
    {
      T operator()(T v, T w) const { return std::max(v,w); }
    };

    template<class T> struct Clamp: std::unary_function<T,T>
    {
      T operator()(T v) const { return clamp(v,l,h); }
      Clamp(T l, T h): l(l), h(h) {}
      T const l, h;
    };

    template<class T> struct Identity: std::unary_function<T,T>
    {
      T       &operator()(T       &x) const { return x; }
      T const &operator()(T const &x) const { return x; }
    };

    template<class Pair>
    struct SelectFirst: std::unary_function<Pair, typename Pair::first_type>
    {
      typename Pair::first_type       &operator()(Pair       &p) const { return p.first; }
      typename Pair::first_type const &operator()(Pair const &p) const { return p.first; }
    };









    // Inline implementations:

    namespace _impl
    {
      // This one is used when A and B are both signed or both unsigned
      template<class A, class B, bool signedA, bool signedB, int op>
      struct IntCompare
      {
        static bool cmp(A a, B b)
        {
          switch(op)
          {
          case 0: return a <  b;
          case 1: return a <= b;
          case 2: return a == b;
          }
        }
      };

      // This one is used when A is unsigned and B is signed
      template<class A, class B, int op>
      struct IntCompare<A, B, false, true, op>
      {
        static bool cmp(A a, B b)
        {
          B const a2 = a;
          A const b2 = b;
          switch(op)
          {
          case 0: return sizeof(A) < sizeof(B) ? a2 <   b : 0 <  b && a <  b2;
          case 1: return sizeof(A) < sizeof(B) ? a2 <=  b : 0 <= b && a <= b2;
          case 2: return sizeof(A) < sizeof(B) ? a2 ==  b : 0 <= b && a == b2;
          }
        }
      };

      // This one is used when A is signed and B is unsigned
      template<class A, class B, int op>
      struct IntCompare<A, B, true, false, op>
      {
        static bool cmp(A a, B b)
        {
          switch(op)
          {
          case 0: return !IntCompare<B, A, false, true, 1>::cmp(b,a);
          case 1: return !IntCompare<B, A, false, true, 0>::cmp(b,a);
          case 2: return  IntCompare<B, A, false, true, 2>::cmp(b,a);
          }
        }
      };
    }


    template<class A, class B> inline bool int_less_than(A a, B b)
    {
      return _impl::IntCompare<A, B, std::numeric_limits<A>::is_signed,
        std::numeric_limits<B>::is_signed, 0>::cmp(a,b);
    }

    template<class A, class B> inline bool int_less_than_equal(A a, B b)
    {
      return _impl::IntCompare<A, B, std::numeric_limits<A>::is_signed,
        std::numeric_limits<B>::is_signed, 1>::cmp(a,b);
    }

    template<class A, class B> inline bool int_greater_than(A a, B b)
    {
      return int_less_than(b,a);
    }

    template<class A, class B> inline bool int_greater_than_equal(A a, B b)
    {
      return int_less_than_equal(b,a);
    }

    template<class A, class B> inline bool int_equal(A a, B b)
    {
      return _impl::IntCompare<A, B, std::numeric_limits<A>::is_signed,
        std::numeric_limits<B>::is_signed, 2>::cmp(a,b);
    }

    template<class A, class B> inline bool int_not_equal(A a, B b)
    {
      return !int_equal(a,b);
    }



    template<> inline short modulo(short x, short m)
    {
      if(x < 0 || m <= x) { x %= m; if(x < 0) x += m; }
      return x;
    }

    template<> inline int modulo(int x, int m)
    {
      if(x < 0 || m <= x) { x %= m; if(x < 0) x += m; }
      return x;
    }

    template<> inline long modulo(long x, long m)
    {
      if(x < 0 || m <= x) { x %= m; if(x < 0) x += m; }
      return x;
    }

    template<> inline float modulo(float x, float m)
    {
      if(0 <= x && x < m) return x;
      x = std::fmod(x, m);
      if(x < 0) x += m;
      // Prevent numeric instability from breaking 0 <= x < m.
      return 0 <= x && x < m ? x : 0;
    }

    template<> inline double modulo(double x, double m)
    {
      if(0 <= x && x < m) return x;
      x = std::fmod(x, m);
      if(x < 0) x += m;
      // Prevent numeric instability from breaking 0 <= x < m.
      return 0 <= x && x < m ? x : 0;
    }

    template<> inline long double modulo(long double x, long double m)
    {
      if(0 <= x && x < m) return x;
      x = std::fmod(x, m);
      if(x < 0) x += m;
      // Prevent numeric instability from breaking 0 <= x < m.
      return 0 <= x && x < m ? x : 0;
    }

    template<> inline short modulo(short x, short m, long &n)
    {
      if(x<0 || x>=m)
      {
        std::div_t d = std::div(x, m);
        x = d.rem;
        n = d.quot;
        if(x<0)
        {
          x+=m;
          --n;
        }
      }
      else n = 0;
      return x;
    }

    template<> inline int modulo(int x, int m, long &n)
    {
      if(x<0 || x>=m)
      {
        std::div_t d = std::div(x, m);
        x = d.rem;
        n = d.quot;
        if(x<0)
        {
          x+=m;
          --n;
        }
      }
      else n = 0;
      return x;
    }

    template<> inline long modulo(long x, long m, long &n)
    {
      if(x<0 || x>=m)
      {
        std::ldiv_t d = std::div(x, m);
        x = d.rem;
        n = d.quot;
        if(x<0)
        {
          x+=m;
          --n;
        }
      }
      else n = 0;
      return x;
    }

    template<> inline float modulo(float x, float m, long &n)
    {
      // Be sure not to break  0 <= x < m.
      float y = modulo(x, m);
      n = static_cast<long>((x-y)/m + 0.5);
      return y;
    }

    template<> inline double modulo(double x, double m, long &n)
    {
      // Be sure not to break  0 <= x < m.
      double y = modulo(x, m);
      n = static_cast<long>((x-y)/m + 0.5);
      return y;
    }

    template<> inline long double modulo(long double x, long double m, long &n)
    {
      // Be sure not to break  0 <= x < m.
      long double y = modulo(x, m);
      n = static_cast<long>((x-y)/m + 0.5);
      return y;
    }



    // FIXME: This implementation appears to be incorrect. We may get
    // std::numeric_limits<I>::max() even though f + n <
    // std::numeric_limits<I>::max() (where comparison and addition
    // are assumed to promote its operands to arbitrary precision) for
    // various small non-negative integers n.
    template<class I, class F> inline I clamp_float_to_int(F f)
    {
      // When converting from float to int (in the conventional
      // sense), it is undefined what happens if the integer part of
      // the float cannot be represented in the integer type, for
      // example, because it is larger than the maximum value of the
      // integer type. Presumably some platforms will clamp, and
      // others will wrap. To get a consistently clamped result, we
      // must never ask for a conversion unless we know that the
      // integer part of the float can be represented in the integer
      // type.

      F const min = F(std::numeric_limits<I>::min());
      F const max = F(std::numeric_limits<I>::max());

      // Fortunately we know that a float value that is strictly less
      // than 'max', must have an integer part that is less than or
      // equal to the maximum integer value.

      // We also know, due to slightly different reasoning, that a
      // float value that is strictly greater than 'min', must have an
      // integer part that is greater than or equal to the minimum
      // integer value.

      return f <= min ? std::numeric_limits<I>::min() :
        max <= f ? std::numeric_limits<I>::max() : I(f);
    }

    namespace _impl
    {
      template<class S, bool sourceInt, class T, bool targetInt> struct ClampAnyToAny
      {
        static T cvt(S v) { return T(v); }
      };
      template<class S, class T> struct ClampAnyToAny<S, true, T, true>
      {
        static T cvt(S v)
        {
          T const min = std::numeric_limits<T>::min();
          T const max = std::numeric_limits<T>::max();
          return int_less_than(v, min) ? min : int_less_than(max, v) ? max : T(v);
        }
      };
      template<class S, class T> struct ClampAnyToAny<S, false, T, true>
      {
        static T cvt(S v) { return clamp_float_to_int<S,T>(v); }
      };
    }

    template<class T, class S> T clamp_any_to_any(S v)
    {
      return _impl::ClampAnyToAny<S, std::numeric_limits<S>::is_integer,
                                  T, std::numeric_limits<T>::is_integer>::cvt(v);
    }



    template<class T> inline int find_least_sig_bit(T v)
    {
      if(!v) return -1;

      int i;
      int const n = sizeof(T) * std::numeric_limits<unsigned char>::digits;

      // Negative values are assumed to be represented as two's
      // complement. This is not guaranteed by standard C++.
      if(int_less_than(v,0))
      {
        if(v == std::numeric_limits<T>::min()) return n - 1;
        v -= std::numeric_limits<T>::min(); // Guaranteed not to be negative
      }

      T const u = 1;

      // Handle unlikely widths of more than 128 bits by searching
      // iteratively from below for the 128 bit chunk containing the least
      // significant bit
      if(128 < n)
      {
        i = 0;
        while(!(v & ((u << 128%n) - u) << i)) i += 128;
        v >>= i;
      }
      else i = 0;

      // The modulo operations are there only to silence compiler
      // warnings. They will have no effect on the running program.
      if(64 < n && !(v & ((u << 64%n) - u))) { v >>= 64%n; i |= 64; }
      if(32 < n && !(v & ((u << 32%n) - u))) { v >>= 32%n; i |= 32; }
      if(16 < n && !(v & ((u << 16%n) - u))) { v >>= 16%n; i |= 16; }
      if( 8 < n && !(v & ((u <<  8%n) - u))) { v >>=  8%n; i |=  8; }
      if( 4 < n && !(v & ((u <<  4%n) - u))) { v >>=  4%n; i |=  4; }
      if( 2 < n && !(v & ((u <<  2%n) - u))) { v >>=  2%n; i |=  2; }
      if( 1 < n && !(v & ((u <<  1%n) - u))) {             i |=  1; }
      return i;
    }


    template<class T> inline int find_most_sig_bit(T v)
    {
      if(!v) return -1;

      int const n = sizeof(T) * std::numeric_limits<unsigned char>::digits;

      // Negative values are assumed to be represented as two's
      // complement. This is not guaranteed by standard C++.
      if(int_less_than(v,0)) return n - 1;

      T const u = 1;

      // Handle unlikely widths of more than 128 bits by searching
      // iteratively from above for the 128 bit chunk containing the most
      // significant bit
      int i;
      if(128 < n)
      {
        i = (n-1)/128*128;
        while(!(v & ((u << 128%n) - u) << i)) i -= 128;
        v >>= i;
      }
      else i = 0;

      // The modulo operations are there only to silence compiler
      // warnings. They will have no effect on the running program.
      if(64 < n && v & ((u << 64%n) - u) << 64%n) { v >>= 64%n; i |= 64; }
      if(32 < n && v & ((u << 32%n) - u) << 32%n) { v >>= 32%n; i |= 32; }
      if(16 < n && v & ((u << 16%n) - u) << 16%n) { v >>= 16%n; i |= 16; }
      if( 8 < n && v & ((u <<  8%n) - u) <<  8%n) { v >>=  8%n; i |=  8; }
      if( 4 < n && v & ((u <<  4%n) - u) <<  4%n) { v >>=  4%n; i |=  4; }
      if( 2 < n && v & ((u <<  2%n) - u) <<  2%n) { v >>=  2%n; i |=  2; }
      if( 1 < n && v & ((u <<  1%n) - u) <<  1%n) {             i |=  1; }
      return i;
    }



    template<class T, class U> inline T bit_cast(U v)
    {
      int const source_bits = sizeof(U)*std::numeric_limits<unsigned char>::digits;
      int const target_bits = sizeof(T)*std::numeric_limits<unsigned char>::digits;

      // The modulo operation is only to silence compiler warnings
      int const source_sign_pos = (source_bits - 1) % target_bits; // Meaningless for narrowing
      int const target_sign_pos = (target_bits - 1) % source_bits; // Meaningless for windening

      if(!std::numeric_limits<T>::is_signed)
      {
        // Target type is unsigned.
        if(sizeof(T) <= sizeof(U) || int_less_than_equal(0,v)) return v;
        // Widening from signed U, to unsigned T, and v is negative.
        return T(1) << source_sign_pos  |  v & std::numeric_limits<U>::max();
      }

      // Target type is signed
      if(sizeof(T) == sizeof(U))
      {
        if(std::numeric_limits<U>::is_signed) return v;
        // Unsigned U to signed T with same number of bits
        if(v & U(1) << target_sign_pos)
          return ~std::numeric_limits<T>::max()  |  T(v & std::numeric_limits<T>::max());
        return v;
      }

      if(sizeof(T) < sizeof(U))
      {
        // Narrowing
        if(v & U(1) << target_sign_pos)
          return ~std::numeric_limits<T>::max()  |  T(v & std::numeric_limits<T>::max());
        return v & std::numeric_limits<T>::max();
      }

      if(int_less_than_equal(0,v)) return v;

      // Widening from signed U, to signed T, and v is negative.
      // The result is guaranteed to be positive
      return T(1) << source_sign_pos  |  v & std::numeric_limits<U>::max();
    }



    template<class T> inline T bit_range(int width, int offset)
    {
        static_assert(std::numeric_limits<T>::is_integer, "Need integer type");
        int num_digits = std::numeric_limits<T>::digits;
        ARCHON_ASSERT(width >= 0);
        ARCHON_ASSERT(offset >= 0);
        ARCHON_ASSERT(width + offset <= num_digits);
        if (width == num_digits)
            return std::numeric_limits<T>::max();
        return T(((T(1) << width) - 1) << offset);
    }



    namespace _impl
    {
      // Low-level right shifting that assumes a positive shift
      template<class T> inline T bit_shift_right(T v, int n)
      {
        int const remain = sizeof(T) * std::numeric_limits<unsigned char>::digits - n;
        if(remain <= 0) return 0;
        if(int_less_than_equal(0,v)) return v >> n;
        if(n == 0) return v;
        T const max = std::numeric_limits<T>::max();
        return (v&max) >> n | T(1) << remain-1;
      }

      // Low-level right shifting that assumes a positive shift
      template<class T> inline T bit_shift_left(T v, int n)
      {
        if(int(sizeof(T) * std::numeric_limits<unsigned char>::digits) <= n) return 0;
        return bit_cast<T>(v << n);
      }
    }

    template<class T> inline T bit_shift_right(T v, int n)
    {
      return n < 0 ? _impl::bit_shift_left(v, -n) : _impl::bit_shift_right(v, n);
    }

    template<class T> inline T bit_shift_left(T v, int n)
    {
      return n < 0 ? _impl::bit_shift_right(v, -n) : _impl::bit_shift_left(v, n);
    }
  }
}

#endif // ARCHON_CORE_FUNCTIONS_HPP
