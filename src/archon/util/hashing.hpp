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

#ifndef ARCHON_UTIL_HASHING_HPP
#define ARCHON_UTIL_HASHING_HPP

#include <cmath>
#include <limits>
#include <iterator>
#include <string>

#include <archon/core/types.hpp>


namespace Archon
{
  namespace Util
  {
    /**
     * Fowler/Noll/Vo hash.
     *
     * \sa http://www.isthe.com/chongo/tech/comp/fnv/index.html
     */
    struct Hash_FNV_1a_32
    {
      /**
       * Digest the specified integer. The result is independant of
       * the platform.
       *
       * \tparam Int any signed or unsigned integer type. This includes any pointer type.
       *
       * \param v The integer value to be digested.
       */
      template<class Int> void add_int(Int v);


      /**
       * Digest the specified floating point number. The result is
       * independant of the platform.
       *
       * \tparam Float any one of the three standard floating point
       * types.
       *
       * \param v The floating point value to be digested.
       */
      template<class Float> void add_float(Float v);


      /**
       * Digest a sequence of bytes. The result is platform
       * independent.
       */
      void add_bytes(char const *data, size_t n);


      /**
       * Digest all characters of a string. The result is platform
       * independent.
       */
      template<class C, class T, class A> void add_string(std::basic_string<C,T,A> const &s);


      /**
       * Digest all elements of a sequence of integers. The result is
       * platform independent.
       */
      template<class Iter> void add_int_sequence(Iter begin, Iter end);


      /**
       * Digest all elements of a sequence of characters. The result
       * is platform independent.
       */
      template<class Iter> void add_char_sequence(Iter begin, Iter end);


      /**
       * Get hash code in the range <tt>[0;n-1]</tt>.
       */
      int get_hash(int n);


      Hash_FNV_1a_32(): h(2166136261) {}


    private:
      void add_octet(unsigned v);

      Core::UIntFast32 h;
    };







    // Implementation:

    template<class Int> inline void Hash_FNV_1a_32::add_int(Int v)
    {
      int const width = sizeof(Int)*std::numeric_limits<unsigned char>::digits;
      if(8 < width)
      {
        typedef typename Core::FastestUnsignedWithBits<width>::type UInt;
        UInt u = v;
        int n = width;
        while(n > 8)
        {
          add_octet(unsigned(u) & 0xFF);
          u >>= 8;
          n -= 8;
        }
        add_octet(u);
      }
      else add_octet(v);
    }


    template<class Float> inline void Hash_FNV_1a_32::add_float(Float v)
    {
      // Add exponent
      int exp;
      v = std::frexp(v, &exp);
      {
        unsigned u = exp;
        int n = sizeof(int)*std::numeric_limits<unsigned char>::digits;
        for(;;)
        {
          add_octet(u & 0xFF);
          u >>= 8;
          if(u == 0) break;
          n -= 8;
        }
      }

      // Add mantissa
      {
        int n = std::numeric_limits<Float>::digits;
        if(std::numeric_limits<Float>::radix != 2)
          n = (n+1) * (double(std::log(std::numeric_limits<Float>::radix))/std::log(2.0));
        for(;;)
        {
          typedef typename Core::SmallestIntCover<Float>::type Int;
          v = std::ldexp(v, std::numeric_limits<Int>::digits);
          Int const i = v;
          add_int(i);
          n -= std::numeric_limits<Int>::digits;
          if(n < 1) break;
          v -= i;
        }
      }
    }


    inline void Hash_FNV_1a_32::add_bytes(char const *data, size_t n)
    {
      char const *end = data + n;
      while(data != end) add_int(*data++);
    }


    template<class C, class T, class A>
    inline void Hash_FNV_1a_32::add_string(std::basic_string<C,T,A> const &s)
    {
      add_char_sequence(s.begin(), s.end());
    }


    template<class Iter> inline void Hash_FNV_1a_32::add_int_sequence(Iter begin, Iter end)
    {
      while(begin != end) add_int(*begin++);
    }


    template<class Iter> inline void Hash_FNV_1a_32::add_char_sequence(Iter begin, Iter end)
    {
      typedef typename std::iterator_traits<Iter>::value_type char_type;
      typedef typename std::char_traits<char_type> traits;
      while(begin != end) add_int(traits::to_int_type(*begin++));
    }


    inline int Hash_FNV_1a_32::get_hash(int n)
    {
      // Lazy mod mapping method
      return (h & 0xFFFFFFFF) % n;
    }


    inline void Hash_FNV_1a_32::add_octet(unsigned v)
    {
      // Xor the bottom bits with the incoming octet
      h ^= v;

      // Multiply by the 32 bit FNV magic prime mod 2^32
      h *= 16777619;
    }
  }
}

#endif // ARCHON_UTIL_HASHING_HPP
