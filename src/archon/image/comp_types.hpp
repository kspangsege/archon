// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_IMAGE_X_COMP_TYPES_HPP
#define ARCHON_X_IMAGE_X_COMP_TYPES_HPP

/// \file


#include <cstdint>
#include <type_traits>

#include <archon/core/integer.hpp>
#include <archon/util/bit_medium.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/color_comp.hpp>
#include <archon/image/impl/comp_types.hpp>


namespace archon::image {


/// \{
///
/// \brief Numeric utility types for pixel components, memory words, and bit compounds.
///
/// These types are used by the Archon Image Library for various purposes. They are used
/// with component representation schemes (\ref image::CompRepr), as default memory word
/// types for various pixel storage schemes (e.g., \ref image::IntegerPixelFormat), and as
/// default types for bit compounds in various pixel storage schemes (e.g., \ref
/// image::PackedPixelFormat).
///
/// The integer types are each guaranteed to be bit media of particular widths (see \ref
/// util::is_bit_medium_of_width):
///
///   | Integer type | Unpacked type         | Guaranteed bit medium width
///   |--------------|-----------------------|-----------------------------
///   | `int8_type`  | `unpacked_int8_type`  |  8
///   | `int16_type` | `unpacked_int8_type`  | 16
///   | `int32_type` | `unpacked_int8_type`  | 32
///   | `int64_type` | `unpacked_int8_type`  | 64
///
/// \sa \ref image::unpacked_int8_type
///
using int8_type  = char;
using int16_type = std::int_least16_t;
using int32_type = std::int_least32_t;
using int64_type = std::int_least64_t;
using float_type = float;
/// \}



/// \{
///
/// \brief Unpacked versions of basic bit-media types.
///
/// These types are the unpacked versions of \ref image::int8_type, \ref image::int16_type,
/// \ref image::int32_type, and \ref image::int64_type respectively. That is,
/// `unpacked_int16_type`, for instance, is the return type of
/// `util::unpack_int<16>(image::int16_type())` (see \ref util::unpack_int()).
///
/// \sa \ref image::int8_type
/// \sa \ref util::unpack_int()
///
using unpacked_int8_type  = util::unpacked_type<image::int8_type, 8>;
using unpacked_int16_type = util::unpacked_type<image::int16_type, 16>;
using unpacked_int32_type = util::unpacked_type<image::int32_type, 32>;
using unpacked_int64_type = util::unpacked_type<image::int64_type, 64>;
/// \}



/// \brief Convert packed component value between integer-based representations.
///
/// This function is an alias for \ref util::color_comp_int_to_int().
///
template<int M, class I, int N, class J> constexpr auto int_to_int(J val) noexcept -> I;



/// \brief Convert packed N-bit integer component value to floating point type.
///
/// This function is an alias for \ref util::color_comp_int_to_float().
///
template<int N, class F, class I> constexpr auto int_to_float(I val) noexcept -> F;



/// \brief Convert component value of floating point type to packed N-bit integer.
///
/// This function is an alias for \ref util::color_comp_float_to_int().
///
template<class I, int N, class F> constexpr auto float_to_int(F val) noexcept -> I;



/// \brief Bit width of specified word type.
///
/// For integer types, this is the number of available bits in the type, or more precisely,
/// it is the largest width, N, such that \p T would be a bit medium of with N (see \ref
/// util::is_bit_medium_of_width). For unsigned types, this is always the number of value
/// bits in the type. For signed types, it is always either the number of value bits, or the
/// number of value bits plus one. Since C++20, if \p T is one of the standard or extended
/// signed integer types, this is the number of value bits plus one.
///
/// For floating-point types, this is the approximate equivalent bit width as returned by
/// \ref core::float_width().
///
/// The specified type (\p T) must either be an integer type that conforms to the integer
/// concept (\ref Concept_Archon_Core_Integer), or it must be a standard floating-point
/// (`std::is_floating_point`).
///
template<class T> constexpr int bit_width = impl::get_bit_width<T>();








// Implementation


template<int M, class I, int N, class J> constexpr auto int_to_int(J val) noexcept -> I
{
    return util::color_comp_int_to_int<M, I, N>(val);
}


template<int N, class F, class I> constexpr auto int_to_float(I val) noexcept -> F
{
    return util::color_comp_int_to_float<N, F>(val);
}


template<class I, int N, class F> constexpr auto float_to_int(F val) noexcept -> I
{
    return util::color_comp_float_to_int<I, N>(val);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_COMP_TYPES_HPP
