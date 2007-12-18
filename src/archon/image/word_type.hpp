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

#ifndef ARCHON_IMAGE_WORD_TYPE_HPP
#define ARCHON_IMAGE_WORD_TYPE_HPP

#include <stdexcept>
#include <limits>
#include <string>

#include <archon/core/types.hpp>
#include <archon/core/functions.hpp>
#include <archon/core/enum.hpp>


namespace Archon
{
  namespace Imaging
  {
    /**
     * \note Word type descriptors can be compared, and it can be
     * relied on that <tt>A <= B</tt> if and only if
     * <tt>get_bits_per_word(A) <= get_bits_per_word(B)</tt> assuming
     * that either both types are integers or both are floating point.
     */
    enum WordType
    {
      word_type_UChar,  ///< Unsigned characters / bytes (same thing)
      word_type_UShort, ///< Unsigned low precision integers
      word_type_UInt,   ///< Unsigned normal precision integers
      word_type_ULong,  ///< Unsigned high precision integers
      word_type_Float,  ///< Low precision floating point numbers
      word_type_Double, ///< Normal precision floating point numbers
      word_type_LngDbl  ///< High precision floating point number
    };

    namespace _Impl { struct WordTypeEnumSpec { static Core::EnumAssoc map[]; }; }
    typedef Core::Enum<WordType, _Impl::WordTypeEnumSpec> WordTypeEnum;


    struct NoSuchWordTypeException;

    /**
     * Get the word type that corresponds with the template argument.
     */
    template<typename T> WordType get_word_type_by_type();

    /**
     * Get the word type corrsponding to the specified bit
     * width. Optionally, the smallest word type of at least the
     * specified bit width.
     *
     * \param width The width in bits of the desired word type.
     *
     * \param floating_point If true seach only among available
     * floating point types, otherwise search only among integer
     * types.
     *
     * \param at_least If true, find the smallest word type of at
     * least the specified bit width. Otherwise find a word type whose
     * width is exactly as specified.
     *
     * \return The word type descriptor of the word type that
     * satisfies the specified width. If multiple word types match,
     * you will get the one that is logically shortest, i.e. if \c int
     * and \c long have the same size, you will get <tt>int</tt>.
     *
     * \throw NoSuchWordTypeException If there is no word type on this
     * platform that satisfies the specified width.
     */
    WordType get_word_type_by_bit_width(int width, bool floating_point = false,
                                        bool at_least = false);

    /**
     * Get the smallest floating point type whose mantissa has at
     * least the specified number of bits, or if no such type exists,
     * get the one with the most mantissa bits.
     */
    WordType get_best_float_type_by_mantissa_bits(int width);

    template<typename T> WordType get_smallest_int_type_by_max_val(T max_val)
    {
      return get_word_type_by_bit_width(Core::find_most_sig_bit(max_val)+1, false, true);
    }

    /**
     * Get the word type bearing the specified name.
     *
     * \param name The name of the desired word type.
     *
     * \return The descriptor of the specified word type.
     *
     * \throw NoSuchWordTypeException If there is no word type with
     * the specified name.
     */
    WordType get_word_type_by_name(std::string name);

    int get_bytes_per_word(WordType);

    int get_bits_per_word(WordType);

    bool is_floating_point(WordType);

    WordType get_smallest_float_cover(WordType);
    WordType get_fastest_float_cover(WordType);


    std::string get_word_type_name(WordType);

    /**
     * Get the width of the widest known word type in bytes.
     */
    int get_max_bytes_per_word();

    /**
     * Get the number of known word types.
     *
     * \sa get_word_type_by_index
     */
    int get_num_word_types();

    /**
     * Get any one of the known word types. To get each known word
     * type in turn, do as follows:
     *
     * <pre>
     *
     *   int const num_word_types = get_num_word_types();
     *   for(int i=0; i<num_word_types; ++i)
     *   {
     *     WordType word_type = get_word_type_by_index(i);
     *
     *     // ...
     *   }
     *
     * </pre>
     *
     * \sa get_num_word_types
     */
    WordType get_word_type_by_index(int index);

    /**
     * Convert a number of memory consecutive words from one type to
     * another. The types are implicit, that is, given at the time the
     * converter was acquired.
     *
     * \param n The number of words to convert.
     */
    typedef void (*WordTypeConverter)(void const *source, void *target, size_t n);


    /**
     * Get a converter that assumes both types encode fractions of
     * unity in the most efficient way. For floating point types,
     * there is no special interpretation, the value expresses the
     * fraction directly. For integer types, however, the value \c v
     * represents the fraction <tt>v / max</tt> where \c max is the
     * maximum value allowed by the integer type.
     *
     * When converting from floating point values to integers, source
     * values are clamped to the range [0,1] before they are
     * converted.
     *
     * \sa Util::fracAnyToAny
     */
    WordTypeConverter get_word_type_frac_converter(WordType source, WordType target);

    /**
     * Get a converter that preserves the source value when the source
     * value can be represented in the target type. When it cannot, it
     * is clamped to the range allowed by the target type.
     */
    WordTypeConverter get_word_type_clamp_converter(WordType source, WordType target);


    template<template<typename, WordType> class Func,
             typename Arg, typename Res, bool only_floats = false> struct WordTypeSwitch;






    // Implementation

    struct NoSuchWordTypeException: std::runtime_error
    {
      NoSuchWordTypeException(std::string m): std::runtime_error(m) {}
    };

    template<typename T> inline WordType get_word_type_by_type()
    {
      throw std::runtime_error("Unexpected word type");
    }
    template<> inline WordType get_word_type_by_type<unsigned char>()  { return word_type_UChar;  }
    template<> inline WordType get_word_type_by_type<unsigned short>() { return word_type_UShort; }
    template<> inline WordType get_word_type_by_type<unsigned int>()   { return word_type_UInt;   }
    template<> inline WordType get_word_type_by_type<unsigned long>()  { return word_type_ULong;  }
    template<> inline WordType get_word_type_by_type<float>()          { return word_type_Float;  }
    template<> inline WordType get_word_type_by_type<double>()         { return word_type_Double; }
    template<> inline WordType get_word_type_by_type<long double>()    { return word_type_LngDbl; }

    inline int get_bits_per_word(WordType t)
    {
      return get_bytes_per_word(t) * std::numeric_limits<unsigned char>::digits;
    }

    namespace _Impl
    {
      template<template<typename, WordType> class F> struct WordTypeSwitchBase
      {
        typedef F<unsigned char,  word_type_UChar>  UChar;
        typedef F<unsigned short, word_type_UShort> UShort;
        typedef F<unsigned int,   word_type_UInt>   UInt;
        typedef F<unsigned long,  word_type_ULong>  ULong;
        typedef F<float,          word_type_Float>  Float;
        typedef F<double,         word_type_Double> Double;
        typedef F<long double,    word_type_LngDbl> LngDbl;
      };

      template<template<typename, WordType> class F, typename A, typename R, bool>
      struct WordTypeSwitch: WordTypeSwitchBase<F>
      {
        typedef WordTypeSwitchBase<F> B;
        struct Switch: B::UChar, B::UShort, B::UInt, B::ULong, B::Float, B::Double, B::LngDbl
        {
          R operator()(WordType t, A a) const
          {
            switch(t)
            {
            case word_type_UChar:  return static_cast<typename B::UChar  const &>(*this)(a);
            case word_type_UShort: return static_cast<typename B::UShort const &>(*this)(a);
            case word_type_UInt:   return static_cast<typename B::UInt   const &>(*this)(a);
            case word_type_ULong:  return static_cast<typename B::ULong  const &>(*this)(a);
            case word_type_Float:  return static_cast<typename B::Float  const &>(*this)(a);
            case word_type_Double: return static_cast<typename B::Double const &>(*this)(a);
            case word_type_LngDbl: return static_cast<typename B::LngDbl const &>(*this)(a);
            }
            throw std::runtime_error("Unexpected word type");
          }
        };
      };      

      template<template<typename, WordType> class F, typename R>
      struct WordTypeSwitch<F, void, R, false>: WordTypeSwitchBase<F>
      {
        typedef WordTypeSwitchBase<F> B;
        struct Switch: B::UChar, B::UShort, B::UInt, B::ULong, B::Float, B::Double, B::LngDbl
        {
          R operator()(WordType t) const
          {
            switch(t)
            {
            case word_type_UChar:  return static_cast<typename B::UChar  const &>(*this)();
            case word_type_UShort: return static_cast<typename B::UShort const &>(*this)();
            case word_type_UInt:   return static_cast<typename B::UInt   const &>(*this)();
            case word_type_ULong:  return static_cast<typename B::ULong  const &>(*this)();
            case word_type_Float:  return static_cast<typename B::Float  const &>(*this)();
            case word_type_Double: return static_cast<typename B::Double const &>(*this)();
            case word_type_LngDbl: return static_cast<typename B::LngDbl const &>(*this)();
            }
            throw std::runtime_error("Unexpected word type");
          }
        };
      };      

      template<template<typename, WordType> class F, typename A, typename R>
      struct WordTypeSwitch<F, A, R, true>: WordTypeSwitchBase<F>
      {
        typedef WordTypeSwitchBase<F> B;
        struct Switch: B::Float, B::Double, B::LngDbl
        {
          R operator()(WordType t, A a) const
          {
            switch(t)
            {
            case word_type_UChar:
            case word_type_UShort:
            case word_type_UInt:
            case word_type_ULong:  break;
            case word_type_Float:  return static_cast<typename B::Float  const &>(*this)(a);
            case word_type_Double: return static_cast<typename B::Double const &>(*this)(a);
            case word_type_LngDbl: return static_cast<typename B::LngDbl const &>(*this)(a);
            }
            throw std::runtime_error("Unexpected word type");
          }
        };
      };      

      template<template<typename, WordType> class F, typename R>
      struct WordTypeSwitch<F, void, R, true>: WordTypeSwitchBase<F>
      {
        typedef WordTypeSwitchBase<F> B;
        struct Switch: B::Float, B::Double, B::LngDbl
        {
          R operator()(WordType t) const
          {
            switch(t)
            {
            case word_type_UChar:
            case word_type_UShort:
            case word_type_UInt:
            case word_type_ULong:  break;
            case word_type_Float:  return static_cast<typename B::Float  const &>(*this)();
            case word_type_Double: return static_cast<typename B::Double const &>(*this)();
            case word_type_LngDbl: return static_cast<typename B::LngDbl const &>(*this)();
            }
            throw std::runtime_error("Unexpected word type");
          }
        };
      };      

      template<typename T, WordType> struct GetBytesPerWord
      {
        int operator()() const { return sizeof(T); }
      };

      template<typename T, WordType> struct IsFloatingPoint
      {
        bool operator()() const { return !std::numeric_limits<T>::is_integer; }
      };

      template<typename T, WordType> struct GetSmallestFloatCover
      {
        WordType operator()() const
        {
          return get_word_type_by_type<typename Core::SmallestFloatCover<T>::type>();
        }
      };

      template<typename T, WordType> struct GetFastestFloatCover
      {
        WordType operator()() const
        {
          return get_word_type_by_type<typename Core::FastestFloatCover<T>::type>();
        }
      };
    }


    template<template<typename, WordType> class F, typename A, typename R, bool only_floats>
    struct WordTypeSwitch: _Impl::WordTypeSwitch<F, A, R, only_floats>::Switch {};

    inline int get_bytes_per_word(WordType t)
    {
      return WordTypeSwitch<_Impl::GetBytesPerWord, void, int>()(t);
    }

    inline bool is_floating_point(WordType t)
    {
      return WordTypeSwitch<_Impl::IsFloatingPoint, void, bool>()(t);
    }

    inline WordType get_smallest_float_cover(WordType t)
    {
      return WordTypeSwitch<_Impl::GetSmallestFloatCover, void, WordType>()(t);
    }

    inline WordType get_fastest_float_cover(WordType t)
    {
      return WordTypeSwitch<_Impl::GetFastestFloatCover, void, WordType>()(t);
    }
  }
}

#endif // ARCHON_IMAGE_WORD_TYPE_HPP

