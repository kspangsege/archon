// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_UTIL_X_COLOR_COMP_HPP
#define ARCHON_X_UTIL_X_COLOR_COMP_HPP

/// \file


#include <type_traits>

#include <archon/core/integer.hpp>
#include <archon/util/bit_medium.hpp>
#include <archon/util/unit_frac.hpp>


namespace archon::util {


/// \brief Convert packed color component value between integer-based representations.
///
/// This function converts a component value from one integer-based representation to
/// another integer-base representation. The specified value (\p val) is treated as an M-bit
/// component value packed into a bit medium of width M (\ref
/// util::is_bit_medium_of_width). The returned value is the scaled N-bit component value
/// packed into a bit medium of width N. The scaling from an M-bit to an N-bit integer-based
/// representation is performed by \ref util::unit_frac::int_to_int().
///
/// \tparam J The type of the specified value. This must be a bit medium of width M.
///
/// \tparam I The type of the returned value. This must be a bit medium of width N.
///
template<int M, class I, int N, class J> constexpr auto color_comp_int_to_int(J val) noexcept -> I;



/// \brief Convert packed N-bit integer color component value to floating point type.
///
/// This function converts a component value from an integer-based representation to a
/// floating-point based representation. The specified value (\p val) is treated as an N-bit
/// integer component value packed into a bit medium of width N (\ref
/// util::is_bit_medium_of_width). The conversion to floating-point type is performed by
/// \ref util::unit_frac::int_to_flt().
///
/// \tparam I The type of the specified value. This must be one of the standard integer
/// types (`std::is_integral`) that is a bit medium of width N.
///
/// \tparam F The type of the returned value. This must be a standard floating-point type
/// (`std::is_floating_point`).
///
template<int N, class F, class I> constexpr auto color_comp_int_to_float(I val) noexcept -> F;



/// \brief Convert color component value of floating point type to packed N-bit integer.
///
/// This function converts a component value of floating-point type(\p val) to its packed
/// N-bit integer representation. The returned value is an N-bit integer component value
/// packed into a bit medium of width N (\ref util::is_bit_medium_of_width). The conversion
/// from floating-point type is performed by \ref util::unit_frac::flt_to_int().
///
/// \tparam F The type of the specified value. This must be a standard floating-point type
/// (`std::is_floating_point`).
///
/// \tparam I The type of the returned value. This must be one of the standard integer types
/// (`std::is_integral`) that is a bit medium of width N.
///
template<class I, int N, class F> constexpr auto color_comp_float_to_int(F val) noexcept -> I;








// Implementation


template<int M, class I, int N, class J> constexpr auto color_comp_int_to_int(J val) noexcept -> I
{
    using type_1 = util::unpacked_type<J, M>;
    using type_2 = util::unpacked_type<I, N>;
    using type_3 = core::common_int_type<type_1, type_2>;
    namespace uf = util::unit_frac;
    type_1 val_2 = util::unpack_int<M>(val);
    type_3 val_3 = uf::int_to_int<M, N>(type_3(val_2));
    return util::pack_int<I, N>(val_3);
}


template<int N, class F, class I> constexpr auto color_comp_int_to_float(I val) noexcept -> F
{
    // Conversion to floating point is only guaranteed to work if I is a standard integer
    // type and F is a standard floating-point type
    static_assert(std::is_integral_v<I>);
    static_assert(std::is_floating_point_v<F>);
    using type = util::unpacked_type<I, N>;
    type val_2 = util::unpack_int<N>(val);
    constexpr type max = core::int_mask<type>(N);
    namespace uf = util::unit_frac;
    return uf::int_to_flt<F>(val_2, max);
}


template<class I, int N, class F> constexpr auto color_comp_float_to_int(F val) noexcept -> I
{
    // Conversion from floating point is only guaranteed to work if I is a standard integer
    // type and F is a standard floating-point type
    static_assert(std::is_integral_v<I>);
    static_assert(std::is_floating_point_v<F>);
    using type = util::unpacked_type<I, N>;
    constexpr type max = core::int_mask<type>(N);
    namespace uf = util::unit_frac;
    return util::pack_int<I, N>(uf::flt_to_int<type>(val, max));
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_COLOR_COMP_HPP
