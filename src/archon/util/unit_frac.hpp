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

#ifndef ARCHON_UTIL_UNIT_FRAC_HPP
#define ARCHON_UTIL_UNIT_FRAC_HPP

#include <limits>
#include <functional>
#include <algorithm>

#include <archon/core/types.hpp>


namespace archon
{
  namespace util
  {
    /**
     * Convert an integer representation of a fraction of unity from
     * one implied denominator to another. See
     * <tt>frac_float_to_int</tt> for more on the canonical integer
     * representation of fractions of unity.
     *
     * The result is achieved by first converting to immediate
     * floating point representation, and then to the target integer
     * representation. This implies that the result is the target
     * fraction range into which that canonical representative of the
     * source range falls. It ensures that zero maps to zero, and that
     * <tt>n-1</tt> maps to <tt>m-1</tt>.
     *
     * The result is exactly correct only if the 'long double' type
     * has enough mantissa bits. The general rule is that the number
     * of mantissa bits in 'long double' must be at least the number
     * of bits in the specified integer type plus 2. The
     * implementation is "wise" enough to choose a narrower floating
     * point type if that is sufficient.
     *
     * If both the source and the target denominators a powers of two,
     * you can achieve the same result, but more efficiently, by using
     * <tt>frac_adjust_bit_width</tt>. The latter also does not suffer
     * from potential precision problems.
     *
     * \param v The integer that selects a fraction range in the
     * source integer representation. It is automatically clamped to
     * the range <tt>[0;n-1]</tt>.
     *
     * \param n The implied denominator of the source integer
     * representation. A value of zero, will be taken to mean
     * <tt>2**k</tt> where <tt>k</tt> is the number of non-sign bits
     * in the selected integer type.
     *
     * \param m The implied denominator of the target
     * representation. A value of zero, will be taken to mean
     * <tt>2**k</tt> where <tt>k</tt> is the number of non-sign bits
     * in the selected integer type.
     *
     * \return An integer that represents a fraction range in the
     * target integer representation. It vill always be a vlaue in the
     * range <tt>[0;m-1]</tt>.
     */
    template<typename Int> Int frac_adjust_denom(Int v, Int n, Int m);


    /**
     * Convert an integer representation of a fraction of unity from
     * one bit width to another. See <tt>frac_float_to_int</tt> for
     * more on the canonical integer representation of fractions of
     * unity.
     *
     * When the conversion is narrowing, there is only one way it can
     * be done. That is because any representative of the source range
     * falls within the same target range.
     *
     * When the conversion is widening, however, there is no longer a
     * unique way to do it. This function chooses the target range in
     * which the canonical representative of the source range
     * falls. This has the advantage that zero maps to zero, and the
     * maximum source value maps to the maximum target value. Further
     * more, it produces a result that is consistent with the less
     * efficient (and less precise if the float type has too few bits)
     * computation:
     *
     * <pre>
     *
     *   float_to_int(int_to_float(v,n), m)
     *
     * </pre>
     *
     * \note If you want to convert from one integer type to another
     * using all bits in both type, you should consider using
     * <tt>frac_any_to_any</tt> instead.
     *
     * Interestingly it turns out that when the bit width is
     * increased, one gets the right result by first shifting the \c n
     * least significant bits left by \c m-n bit positions, then
     * filling the vacated bit positions with replica of the \c n bits
     * that were shifted, with the last replication possibly
     * truncated. A narrowing variant always reduces to a single shift
     * operation. These facts are used in the implementation.
     *
     * \param n The bit width of the original value/fraction.
     *
     * \param m The desired adjusted bit width.
     *
     * \note Both \c n and \c m must be less than or equal to the
     * number of non-sign bits in <tt>Int</tt>.
     *
     * \note A widening operation will fail if \c v has bits set above
     * the \c n least significant ones.
     */
    template<typename Int> Int frac_adjust_bit_width(Int v, int n, int m);


    /**
     * Produce the canonical integer representation of the specified
     * fraction <tt>v</tt> of unity. The canonical integer
     * representation is defined as follows:
     *
     * An integer value <tt>i</tt> in the range <tt>[0;n-1]</tt>
     * represents all fractions <tt>f</tt> in the range <tt>i/n <= f <
     * (i+1)/n</tt>, where <tt>n</tt> is the implied denominator of
     * the representation. Additionally <tt>i = 0</tt> representas all
     * fractional values less than zero, and <tt>i = n-1</tt>
     * representas all fractional values greater than or equal to one.
     *
     * This means that the result can be computed in the following
     * way:
     *
     * <pre>
     *
     *                       {  0               if f <  0
     *   float_to_int(f)  =  {  n-1             if 1 <= f
     *                       {  floor(f * n)    oterwise
     *
     * </pre>
     *
     * Please observe the counter intuitive fact that
     * <tt>float_to_int(1)</tt> with an implied denominator of 2
     * (halves) gives "1", not "2". So, the integer value of the
     * canonical integer representation must not be confused with the
     * numerator in the ordinary fraction notation.
     *
     * The reverse mapping, from integer representation to immediate
     * floating point representation is generally not unique, becuase
     * we could choose any value in the range represented by the
     * integer. However, it is generally desirable, that zero maps to
     * zero and that the maximum integer <tt>n-1</tt> maps to one. For
     * this reason we define the conanical floating point
     * representative of the interval as follows:
     *
     * <pre>
     *
     *                       {  0               if i   < 0
     *   int_to_float(i)  =  {  1               if n-1 < i
     *                       {  i / (n-1)       otherwise
     *
     * </pre>
     *
     * \param denom The implied denominator of the integer
     * representation of the fraction. If zero is specified, the
     * denominator is taken to be <tt>2**m</tt> where <tt>m</tt> is
     * the number of non-sign bits in the integer type. The result is
     * undefined if a negative value is specified.
     */
    template<typename Float, typename Int>
    inline Int frac_float_to_int(Float v, Int denom = 0);


    /**
     * Produce the canonical floating point representative of the
     * specified fraction range of the canonical integer
     * representation. Please see <tt>frac_float_to_int</tt> for more
     * on the canonical integer representation.
     *
     * If the floating point type has fewer bits than the integer
     * type, there may be no floating point value that falls within
     * the specified fraction range, in this case, the closest value
     * is returned.
     *
     * \param denom The implied denominator of the integer
     * representation of the fraction. If zero is specified, the
     * denominator is taken to be <tt>2**m</tt> where <tt>m</tt> is
     * the number of non-sign bits in the integer type. The result is
     * undefined if a negative value is specified.
     *
     * \sa frac_float_to_int
     */
    template<typename Int, typename Float>
    inline Float frac_int_to_float(Int v, Int denom = 0);


    /**
     * Same as <tt>frac_float_to_int</tt> except that the denominator
     * is specified indirectly as a number of bits with the following
     * correspondance: <tt>denom = 2**n</tt> where <tt>n</tt> is the
     * specified number of bits.
     *
     * \param int_bits Must be less than or equal to the number of
     * non-sign bits in <tt>Int</tt>.
     */
    template<typename Float, typename Int>
    inline Int frac_float_to_n_bit_int(Float v, int int_bits);


    /**
     * Same as <tt>frac_int_to_float</tt> except that the denominator
     * is specified indirectly as a number of bits with the following
     * correspondance: <tt>denom = 2**n</tt> where <tt>n</tt> is the
     * specified number of bits. Also, this variant offers to mask the
     * selected bits, in case the value of the rest of the bits must
     * be ignored.
     *
     * \param int_bits Must be less than or equal to the number of
     * non-sign bits in <tt>Int</tt>.
     */
    template<typename Int, typename Float>
    inline Float frac_n_bit_int_to_float(Int v, int int_bits, bool mask_input = false);


    /**
     * Convert a unit fraction from one representation to another. For
     * a floating point type, it is a value in the interval
     * <tt>[0;1]</tt>. For an integer type, it is a value in the
     * interval <tt>[0;2**n-1]</tt> where \c n is the number of
     * non-sign bits of that integer type.
     *
     * See <tt>frac_float_to_int</tt> for information on the integer
     * representation of fractions.
     */
    template<typename A, typename B> inline B frac_any_to_any(A v);


    /**
     * Convert a sequence of unit fractions from one representation to
     * another. Each fraction is converted the same way that the
     * single argument version of <tt>frac_any_to_any</tt> dioes it.
     */
    template<typename A, typename B> void frac_any_to_any(A const *source, B *target, size_t n);


    /**
     * The value that corresponds to one, unity, or full intensity for
     * the representation associated with the template type.
     */
    template<typename T> T frac_full();


    /**
     * Produce the complement of the specified fraction of unity. That
     * is, what remains after removing the specified fraction. In
     * floating point notation it is <tt>1-f</tt> where <tt>f</tt> is
     * the specified fraction.
     */
    template<typename T> T frac_complement(T v);


    /**
     * Produce the complement of a sequence of fractions of
     * unity. Each complement is produced the same way that the single
     * argument version of <tt>frac_complement</tt> dioes it.
     */
    template<typename T> void frac_complement(T const *source, T *target, size_t n);








    // Implementation:

    template<typename Int> inline Int frac_adjust_denom(Int v, Int n, Int m)
    {
      // Require two extra bits of the float type, if this cannot be
      // honored, the result will not be exactly correct.
      int const extra_bits = 2;
      typedef typename core::FastestFloatCover<Int, extra_bits>::type Float;
      return frac_float_to_int<Float, Int>(frac_int_to_float<Int, Float>(v,n), m);
    }


    template<typename T> inline T frac_adjust_bit_width(T v, int n, int m)
    {
      if (m < n) return v >> (n-m); // Shrink
      if (m == n) return v; // Noop
      for (;;) { // Expand
        int n2 = n << 1;
        if (m < n2) {
          m -= n;
          return m ? v << m | v >> (n-m) : v;
        }
        // Double the bit sequence when possible, since this is
        // particularly easy
        v *= (T(1) << n) + 1;
        n = n2;
      }
    }


    template<typename Float, typename Int>
    inline Int frac_float_to_int(Float v, Int denom)
    {
      if(v <= static_cast<Float>(0)) return static_cast<Int>(0);
      Int const max_int = denom ? denom - Int(1) : std::numeric_limits<Int>::max();
      v *= denom ? denom : Float(std::numeric_limits<Int>::max()) + Float(1);
      // Special care is taken here because the conversion from float
      // to int may produce a surprising result. When the number of
      // bits in the float is significantly less than the number of
      // bits in the int, it is possible for the expression
      // Int(Float(max_int)) to produce zero. This is because the
      // integer part of the closest float value to max_int may be
      // strictly greater than max_int, and thus unrepresentable in
      // the int type.
      return Float(max_int) <= v ? max_int : static_cast<Int>(v);
    }


    template<typename Int, typename Float>
    inline Float frac_int_to_float(Int v, Int denom)
    {
      if(v <= 0) return 0;
      Int const max_int = denom ? denom - Int(1) : std::numeric_limits<Int>::max();
      return max_int <= v ? 1 : static_cast<Float>(v) / static_cast<Float>(max_int);
    }


    template<typename Float, typename Int>
    inline Int frac_float_to_n_bit_int(Float v, int int_bits)
    {
      Int denom;
      // If the integer type is unsigned, we need to be carefull not
      // to attempt shifts by the total number of bits in the integer,
      // because that sometimes does not work. The following takes
      // care of signed types too.
      if(int_bits == std::numeric_limits<Int>::digits) denom = 0;
      else
      {
        denom = 1;
        denom <<= int_bits;
      }
      return frac_float_to_int<Float, Int>(v, denom);
    }


    template<typename Int, typename Float>
    inline Float frac_n_bit_int_to_float(Int v, int int_bits, bool mask_input)
    {
      Int denom;
      // If the integer type is unsigned, we need to be carefull not
      // to attempt shifts by the total number of bits in the integer,
      // because that sometimes does not work. The following takes
      // care of signed types too.
      if(int_bits == std::numeric_limits<Int>::digits) denom = 0;
      else
      {
        denom = 1;
        denom <<= int_bits;
      }
      if(mask_input && denom) v &= denom - Int(1);
      return frac_int_to_float<Int, Float>(v, denom);
    }


    namespace _impl
    {
      template<typename A, typename B, bool intA, bool intB> struct FracAnyToAny {};
      template<typename A, typename B> struct FracAnyToAny<A, B, false, false>
      {
        static B cvt(A v) { return v; }
      };
      template<typename A, typename B> struct FracAnyToAny<A, B, false, true>
      {
        static B cvt(A v) { return frac_float_to_int<A,B>(v); }
      };
      template<typename A, typename B> struct FracAnyToAny<A, B, true, false>
      {
        static B cvt(A v) { return frac_int_to_float<A,B>(v); }
      };
      template<typename A, typename B> struct FracAnyToAny<A, B, true, true>
      {
        static B cvt(A v)
        {
          // Bit-width adjustment must occur in the widest of the two
          // types
          return sizeof(A) < sizeof(B) ?
            frac_adjust_bit_width<B>(v, std::numeric_limits<A>::digits,
                                     std::numeric_limits<B>::digits) :
            frac_adjust_bit_width<A>(v, std::numeric_limits<A>::digits,
                                     std::numeric_limits<B>::digits);
        }
      };


      template<typename T, bool is_int> struct FracComplement {};
      template<typename T> struct FracComplement<T, false>
      {
        static T inv(T v) { return 1 - v; }
      };
      template<typename T> struct FracComplement<T, true>
      {
        static T inv(T v) { return std::numeric_limits<T>::max() - v; }
      };
    }


    template<typename A, typename B> inline B frac_any_to_any(A v)
    {
      return _impl::FracAnyToAny<A, B, std::numeric_limits<A>::is_integer,
        std::numeric_limits<B>::is_integer>::cvt(v);
    }


    template<typename A, typename B> inline void frac_any_to_any(A const *s, B *t, size_t n)
    {
      std::transform(s, s+n, t, std::ptr_fun(&frac_any_to_any<A,B>));
    }


    template<typename T> inline T frac_full()
    {
      return T(std::numeric_limits<T>::is_integer ? std::numeric_limits<T>::max() : 1);
    }


    template<typename T> inline T frac_complement(T v)
    {
      return _impl::FracComplement<T, std::numeric_limits<T>::is_integer>::inv(v);
    }


    template<typename T> inline void frac_complement(T const *s, T *t, size_t n)
    {
      std::transform(s, s+n, t, std::ptr_fun(&frac_complement<T>));
    }
  }
}

#endif // ARCHON_UTIL_UNIT_FRAC_HPP
