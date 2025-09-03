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

#ifndef ARCHON_X_CORE_X_FLOAT_WIDTH_HPP
#define ARCHON_X_CORE_X_FLOAT_WIDTH_HPP

/// \file


#include <limits>
#include <utility>

#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float_traits.hpp>
#include <archon/core/ext_int_type.hpp>


namespace archon::core {


/// \brief Approximate equivalent bit width of floating-point type.
///
/// This function computes the approximate equivalent bit width of the specified
/// floating-point type \p T, which can be any type that conform to the floating-point
/// concept, \ref Concept_Archon_Core_Float. In rough terms, it should be thought of as the
/// number of bits needed to hold the mantissa plus the number of bits needed to hold the
/// exponent.
///
/// Formally, the *approximate equivalent bit width* of the floating-point type \p T is
/// equal to *round(log2(r^d \* e))*, where *r* is `U::radix`, *d* is `U::digits`, *e* is
/// the size of the exponent range, `U::max_exponent - U::min_exponent + 1`, and `U` is
/// `core::FloatTraits<T>`.
///
/// This function returns 32 and 64 for the 32- and 64-bit IEEE floating-point types
/// respectively. Note that the exact match is a bit of a coincidence in that there is a
/// sign bit that is not counted by this function, but on the other hand, there is an
/// implicit mantissa bit that is counted in the value of \c std::numeric_limits<T>::digits
/// for the IEEE types.
///
/// \sa \ref core::FloatTraits
///
template<class T> constexpr int float_width() noexcept;








// Implementation


template<class T> constexpr int float_width() noexcept
{
    using float_type = T;
    using traits_type = core::FloatTraits<float_type>;

    constexpr int radix   = traits_type::radix;
    constexpr int digits  = traits_type::digits;
    constexpr int min_exp = traits_type::min_exponent;
    constexpr int max_exp = traits_type::max_exponent;

    // To compute round(log2(r^d * e)) where r is radix, d is digits, and e is the size of
    // the exponent range (max_exp - min_exp + 1), we can instead compute
    //
    //   floor(log2(r^d * e) + 0.5)
    //
    // but this can be rewritten as
    //
    //   floor(floor(log2((r^d * e)^2)) + 1) / 2)
    //
    // which can be computed exactly, so long as we use an integer type that is wide enough
    // to hold (r^d * e)^2.
    //
    // An upper bound on number of bits needed to hold (r^d * e)^2 can be trivially obtained
    // as 2 * (d * b(r) + b(e)) where b(i) = floor(log2(i)) + 1.

    constexpr unsigned exp_range_minus_one = unsigned(max_exp) - unsigned(min_exp);
    constexpr int exp_width = core::int_find_msb_pos(exp_range_minus_one) + 1;
    constexpr int max_width = 2 * (digits * (core::int_find_msb_pos(radix) + 1) + exp_width);

    using int_type = core::fast_unsigned_ext_int_type<max_width>;
    int_type val = int_type(radix);
    bool success = core::try_int_pow(val, digits);
    int_type val_2 = {};
    success = success && core::try_int_cast(exp_range_minus_one, val_2);
    success = success && core::try_int_add(val_2, 1);
    success = success && core::try_int_mul(val, val_2);
    success = success && core::try_int_pow(val, 2);
    ARCHON_ASSERT(success);

    return (core::int_find_msb_pos(val) + 1) / 2;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FLOAT_WIDTH_HPP
