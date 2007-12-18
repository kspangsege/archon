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
 * Various utilities for working with arthmetic types.
 */

#ifndef ARCHON_CORE_TYPES_HPP
#define ARCHON_CORE_TYPES_HPP

#include <limits>

#include <archon/core/meta.hpp>


namespace Archon
{
  namespace Core
  {
    namespace _Impl { template<typename T> struct ToNum; }


    /**
     * If the type of the argument is <tt>char</tt>, <tt>signed
     * char</tt>, or <tt>unsigned char</tt>, convert it to a type that
     * will be written out as a numeral on an STL output
     * stream. Otherwise the argument is passed through without
     * conversion.
     */
    template<typename T> typename _Impl::ToNum<T>::type to_num(T);




    /**
     * Choose the fastest signed integer type with at least the
     * specified number of bits, or if no type is wide enough, choose
     * the widest signed type available on this platform.
     */
    template<int n> struct FastestSignedWithBits
    {
    private:
      static int const m = std::numeric_limits<unsigned char>::digits;
      static bool const cond = n <= sizeof(int) * m;

    public:
      typedef typename CondType<cond, int, long>::type type;
    };


    /**
     * Choose the fastest unsigned integer type with at least the
     * specified number of bits, or if no type is wide enough, choose
     * the widest unsigned type available on this platform.
     */
    template<int n> struct FastestUnsignedWithBits
    {
    private:
      static int const m = std::numeric_limits<unsigned char>::digits;
      static bool const cond = n <= sizeof(unsigned) * m;

    public:
      typedef typename CondType<cond, unsigned, unsigned long>::type type;
    };


    /**
     * Choose the smallest signed integer type with at least the
     * specified number of bits, or if no type is wide enough, choose
     * the widest signed type available on this platform.
     */
    template<int n> struct SmallestSignedWithBits
    {
    private:
      static int const m = std::numeric_limits<unsigned char>::digits;
      static bool const cond1 = n <= sizeof(signed char) * m;
      static bool const cond2 = n <= sizeof(short)       * m;
      typedef typename FastestSignedWithBits<n>::type type3;
      typedef typename CondType<cond2, short, type3>::type type2;

    public:
      typedef typename CondType<cond1, signed char, type2>::type type;
    };


    /**
     * Choose the smallest unsigned integer type with at least the
     * specified number of bits, or if no type is wide enough, choose
     * the widest unsigned type available on this platform.
     */
    template<int n> struct SmallestUnsignedWithBits
    {
    private:
      static int const m = std::numeric_limits<unsigned char>::digits;
      static bool const cond1 = n <= sizeof(unsigned char)  * m;
      static bool const cond2 = n <= sizeof(unsigned short) * m;
      typedef typename FastestUnsignedWithBits<n>::type type3;
      typedef typename CondType<cond2, unsigned short, type3>::type type2;

    public:
      typedef typename CondType<cond1, unsigned char, type2>::type type;
    };




    /**
     * Choose the fastest unsigned integer type that is wide enough to
     * hold the value specified as template argument.
     */
    template<unsigned long v> struct FastestUnsignedWithValue
    {
      typedef typename CondType<unsigned(v) == v, unsigned,
                                unsigned long>::type type;
    };


    /**
     * Choose the smallest unsigned integer type that is wide enough to
     * hold the value specified as template argument.
     */
    template<unsigned long v> struct SmallestUnsignedWithValue
    {
    private:
      typedef typename FastestUnsignedWithValue<v>::type t1;
      typedef typename CondType<static_cast<unsigned short>(v) == v,
                                unsigned short, t1>::type t2;

    public:
      typedef typename CondType<static_cast<unsigned char>(v) == v,
                                unsigned char, t2>::type type;
    };




    typedef SmallestSignedWithBits<8>::type  IntMin8;      // C++0x: int_least8_t.  Always 'signed char'
    typedef SmallestSignedWithBits<16>::type IntMin16;     // C++0x: int_least16_t
    typedef SmallestSignedWithBits<32>::type IntMin32;     // C++0x: int_least32_t

    typedef FastestSignedWithBits<8>::type  IntFast8;      // C++0x: int_fast8_t.  Always 'int'
    typedef FastestSignedWithBits<16>::type IntFast16;     // C++0x: int_fast16_t. Always 'int'
    typedef FastestSignedWithBits<32>::type IntFast32;     // C++0x: int_fast32_t

    typedef SmallestUnsignedWithBits<8>::type  UIntMin8;   // C++0x: uint_least8_t. Always 'unsigned char'.
    typedef SmallestUnsignedWithBits<16>::type UIntMin16;  // C++0x: uint_least16_t
    typedef SmallestUnsignedWithBits<32>::type UIntMin32;  // C++0x: uint_least32_t

    typedef FastestUnsignedWithBits<8>::type  UIntFast8;   // C++0x: uint_fast8_t.  Always 'unsigned int'
    typedef FastestUnsignedWithBits<16>::type UIntFast16;  // C++0x: uint_fast16_t. Always 'unsigned int'
    typedef FastestUnsignedWithBits<32>::type UIntFast32;  // C++0x: uint_fast32_t




    /**
     * Compare the precision of two numeric types. For integers, the
     * precision is the number of bits not including the sign bit. For
     * floating point types, the precision is the number of bits in
     * the significand including any non-explicit bits. In all cases,
     * if the precision is the least number N such that the type can
     * encode all values in the range [0;2^N-1] exactly.
     *
     * The result is true if, and only if the precision of the first
     * type is less than or equal to the precision of the second.
     *
     * \tparam extra_bits The number of extra bits to require from the
     * second type. That is, the result is true if, and only if
     * <tt>prec(First) + extra_bits <= prec(Second)</tt>.
     */
    template<typename First, typename Second, int extra_bits = 0> struct ComparePrecision
    {
      // Due to the fact that constant expressions cannot handle
      // floating point arithmetic, we need to use integer arithmetic
      // with an implicit scaling factor.
      //
      // The scaling factor is determined such that any value can be
      // represented in a 32 bit unsigned integer, which is the
      // guaranteed minimum width of 'unsigned long'.
      //
      // max_real_bits   = 1024   (assumption)
      // max_extra_bits  = 64     (choice)
      // max_bits = max_real_bits + max_extra_bits
      // scaling_factor = 2^32/(max_bits * ln 2) = 5695154.42897011084738198732162

    private:
      static unsigned long const log2  =  3947580; // floor(scaling_factor * ln  2)
      static unsigned long const log10 = 13113577; // floor(scaling_factor * ln 10)
      typedef std::numeric_limits<First>  F;
      typedef std::numeric_limits<Second> S;
      static unsigned long const first =
        F::is_integer || F::radix == 2 ? F::digits * log2 : F::digits10 * log10;
      static unsigned long const second =
        S::is_integer || S::radix == 2 ? S::digits * log2 : S::digits10 * log10;

    public:
      static bool const result = first + extra_bits*log2 <= second;
    };




    /**
     * Choose the type with the lowest precision. The precision of a
     * type is the number of bits in the significand. See
     * ComparePrecision for further details.
     */
    template<typename First, typename Second> struct NarrowestType
    {
      typedef typename CondType<ComparePrecision<First, Second>::result,
                                First, Second>::type type;
    };


    /**
     * Choose the type with the highest precision. The precision of a
     * type is the number of bits in the significand. See
     * ComparePrecision for further details.
     */
    template<typename First, typename Second> struct WidestType
    {
      typedef typename CondType<ComparePrecision<First, Second>::result,
                                Second, First>::type type;
    };




    /**
     * Choose the fastest signed integer type whose precision is at
     * least as high as the numeric argument type, or if no such
     * integer type exists, choose the widest one available. The
     * precision of a type is the number of bits in the
     * significand. See ComparePrecision for further details.
     *
     * \tparam extra_bits The number of extra bits of precision that
     * must be offered by the result type.
     */
    template<class T, int extra_bits = 0> struct FastestIntCover
    {
      typedef typename CondType<ComparePrecision<T, int, extra_bits>::result,
                                int, long>::type type;
    };


    /**
     * Choose the narrowest signed integer type whose precision is at
     * least as high as the numeric argument type, or if no such
     * integer type exists, choose the widest one available. The
     * precision of a type is the number of bits in the
     * significand. See ComparePrecision for further details.
     *
     * \tparam extra_bits The number of extra bits of precision that
     * must be offered by the result type.
     */
    template<typename T, int extra_bits = 0> struct SmallestIntCover
    {
    private:
      static bool const cond1 = ComparePrecision<T, signed char, extra_bits>::result;
      static bool const cond2 = ComparePrecision<T, short,       extra_bits>::result;
      typedef typename FastestIntCover<T, extra_bits>::type type3;
      typedef typename CondType<cond2, short, type3>::type type2;

    public:
      typedef typename CondType<cond1, signed char, type2>::type type;
    };




    /**
     * Choose the fastest floating point type whose precision is at
     * least as high as the numeric argument type, or if no such
     * floating point type exists, choose the widest one
     * available. The precision of a type is the number of bits in the
     * significand. See ComparePrecision for further details.
     *
     * \tparam extra_bits The number of extra bits of precision that
     * must be offered by the result type.
     */
    template<typename T, int extra_bits = 0> struct FastestFloatCover
    {
      typedef typename CondType<ComparePrecision<T, double, extra_bits>::result,
                                double, long double>::type type;
    };


    /**
     * Choose the narrowest floating point type whose precision is at
     * least as high as the numeric argument type, or if no such
     * floating point type exists, choose the widest one
     * available. The precision of a type is the number of bits in the
     * significand. See ComparePrecision for further details.
     *
     * \tparam extra_bits The number of extra bits of precision that
     * must be offered by the result type.
     */
    template<typename T, int extra_bits = 0> struct SmallestFloatCover
    {
      typedef typename CondType<ComparePrecision<T, float, extra_bits>::result, float,
                                typename FastestFloatCover<T, extra_bits>::type>::type type;
    };







    // Implementation

    namespace _Impl
    {
      template<typename T> struct ToNum
      {
        typedef T type;
        T operator()(T v) { return v; }
      };
      template<> struct ToNum<signed char>
      {
        typedef int type;
        int operator()(signed char v) { return v; }
      };
      template<> struct ToNum<unsigned char>
      {
        typedef unsigned type;
        unsigned operator()(unsigned char v) { return v; }
      };

      template<bool isSigned> struct ToNumChar {};
      template<> struct ToNumChar<true>:  ToNum<signed   char> {};
      template<> struct ToNumChar<false>: ToNum<unsigned char> {};

      template<> struct ToNum<char>: ToNumChar<std::numeric_limits<char>::is_signed> {};
    }

    template<typename T> inline typename _Impl::ToNum<T>::type to_num(T v)
    {
      return _Impl::ToNum<T>()(v);
    }
  }
}

#endif // ARCHON_CORE_TYPES_HPP
