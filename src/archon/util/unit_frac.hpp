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

#ifndef ARCHON_X_UTIL_X_UNIT_FRAC_HPP
#define ARCHON_X_UTIL_X_UNIT_FRAC_HPP

/// \file


#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/ext_int_type.hpp>


/// \brief Convert fractions of unity between representations.
///
/// This namespace contain a number of functions that deal with the conversion of a fraction
/// of unity (a value between 0 and 1) between different representations, especially
/// representations involving integers.
///
namespace archon::util::unit_frac {


/// \brief Convert fraction of unity from integer-based to floating-point representation.
///
/// This function produces a floating-point value that corresponds to the fraction of unity
/// represented by the specified integer value. The returned value is `F(int_val) /
/// max_int`.
///
/// There are two different mental models for what this function is doing; causing each of
/// \ref util::flt_to_int() and \ref util::flt_to_int_a() to be a natural inverse operation.
///
/// Default model, where \ref util::flt_to_int() is the natural inverse operation: The range
/// 0 -> 1 is divided into N equally sized intervals, where N is the number of available
/// integer values, i.e., \p max_int + 1, and the size of each interval is 1/N. The I'th
/// interval corresponds to the I'th integer value. The floating-point value returned by
/// this function for some integer value, does not generally fall in the center of the
/// corresponding interval. Instead, in order to ensure that integer values 0 and N-1 map to
/// floating-point values 0 and 1 respectively, a linearly varying shift is introduced such
/// that, at integer value 0, it pushes the floating point value from 1/(2N) to 0; and, at
/// integer value N-1, it pushes the floating point value from 1-1/(2N) to 1.
///
/// Alternate model, where \ref util::flt_to_int_a() is the natural inverse operation: The
/// floating-point range -S/2 -> 1+S/2 is divided into N equally sized intervals, where N is
/// the number of available integer values, i.e., \p max_int + 1, and S = 1/(N-1) is the
/// interval size. The I'th interval corresponds to the I'th integer value. The
/// floating-point value produced for a particular integer value lies in the center of the
/// corresponding interval.
///
/// If N was 4 (2 bit integer), the intervals would be as follows:
///
///              0                                               1
///     ---------|-----------------------------------------------|--------->
///              .                                               .
///              |<--- 0 --->|<--- 1 --->|<--- 2 --->|<--- 3 --->|          Default model
///              .                                               .
///      |<----- 0 ----->|<----- 1 ----->|<----- 2 ----->|<----- 3 ----->|  Alternate model
///
/// For a integer value `i` of type `I` and a floating-point type `F`, the round trip
/// `flt_to_int_a<I>(int_to_flt<F>(i))` (alternate model) is numerically more stable than
/// `flt_to_int<I>(int_to_flt<F>(i))` (default model). The additional stability in the
/// alternate model stems from the produced floating-point values being centered in the
/// respective intervals.
///
/// There are two reasons why the alternate model is not the default model. The first reason
/// is that under the alternate model, all integer values are not represented by an equal
/// share of the unit interval (0 -> 1) which can lead to bias in some usage scenarios. The
/// second reason is that the operation of \ref util::change_bit_width() is consistent with
/// the model that is the default mode, and inconsistent with the alternate model.
///
/// While the nominal range for the specified integer value (\p int_val) is 0 -> \p max_int,
/// this function does not require the specified value to be in this range, nor does it
/// clamp it to the range. If the specified value is outside the range, the returned value
/// will be less than 0 or greater than 1. Note however, that under the default model, due
/// to the shifting, passing an integer value outside the nominal range will produced a
/// nonsensical result (unstable round trip).
///
/// \sa \ref util::flt_to_int(), \ref util::flt_to_int_a()
///
template<class F, class I>
constexpr auto int_to_flt(I int_val, core::Type<I> max_int = core::int_max<I>()) noexcept -> F;


/// \{
///
/// \brief Convert fraction of unity from floating-point to integer-based representation.
///
/// These functions convert the specified floating-point representation of a fraction of
/// unity to the corresponding integer representation. The two functions does it in slightly
/// different ways.
///
/// `flt_to_int(flt_val, max_int)` returns `trunc(flt_val * (max_int + 1))` when `0 <=
/// flt_val < 1`. The computation is done in a way that allows for \p max_int to be the
/// maximum value for the type. When flt_val is 1, it returns `max_int`.
///
/// `flt_to_int_a(flt_val, max_int)` returns `round(flt_val * max_int)` when `0 <= flt_val
/// <= 1`.
///
/// See \ref unit_frac::int_to_flt() for further information on, and motivation for the two
/// alternative forms.
///
/// These functions have clamping behavior, meaning that if the specified floating-point
/// representation is out of bounds (less than zero, or greater than 1), the returned value
/// is the one that corresponds to the boundary point nearest to the specified value (0 or
/// 1).
///
/// These functions convert NaN to zero.
///
/// \sa \ref util::int_to_flt()
///
template<class I, class F>
constexpr auto flt_to_int(F flt_val, core::Type<I> max_int = core::int_max<I>()) noexcept -> I;
template<class I, class F>
constexpr auto flt_to_int_a(F flt_val, core::Type<I> max_int = core::int_max<I>()) noexcept -> I;
/// \}


/// \brief Convert fraction of unity between integer-based representations of different bit
/// width.
///
/// This function converts a fraction of unity from an integer representation of width \p m
/// to another integer representation of width \p n.
///
/// Given an integer value `i` of type `I` and a (real or imaginary) floating-point type `F`
/// of sufficient width, `change_bit_width(i, m, n)` produces the same result as
/// `flt_to_int<I>(int_to_flt<F>(i, p), q)` where `p` is `core::int_mask<I>(m)` and `q` is
/// `core::int_mask<I>(n)`. On the other hand, it does not always produce the same result as
/// `flt_to_int_a<I>(int_to_flt<F>(i, p), q)` (alternate model).
///
/// Behavior is undefined if \p v is out of range, i.e., if it is less than zero, or greater
/// than, or equal to `2^m`. So long as \p v is in range, the result is guaranteed to also
/// be in range, i.e, greater than, or equal to zero, and less than `2^n`.
///
template<class T> constexpr auto change_bit_width(T v, int m, int n) noexcept -> T;


/// \brief Convert fraction of unity between integer-based representations.
///
/// This function converts a fraction of unity from one integer representation to
/// another. Both the origin and target representations are specified in terms of their
/// maximum value (\p max_1, \p max_2).
///
/// This conversion is an infinitely precise alternative to first converting to a floating
/// point representation using \ref unit_frac::int_to_flt() and then to the target integer
/// representation using \ref unit_frac::flt_to_int_a().
///
/// \tparam N A limit on the number of value bits needed to represent \p max_1. In general,
/// a lower value leads to a more efficient conversion operation.
///
/// \tparam M A limit on the number of value bits needed to represent \p max_2. In general,
/// a lower value leads to a more efficient conversion operation.
///
/// \param val A value of the origin representation to be converted to the target
/// representation. The result is unspecified if this is negative or greater than \p max_1.
///
/// \param max_1 The maximum value for the origin representation. Behavior is undefined if
/// this is negative or zero.
///
/// \param max_2 The maximum value for the target representation. Behavior is undefined if
/// this is negative or zero.
///
template<int N, int M, class I, class J> auto int_to_int_a(I val, I max_1, J max_2) -> J;








// Implementation


namespace impl {


template<class I, bool alt, class F>
constexpr auto flt_to_int(F flt_val, core::Type<I> max_int) noexcept -> I
{
    // Max value for type is must be odd, so that we can be sure below that it is possible
    // to add 1 when the value is even.
    static_assert((core::int_max<I>() & 1) == 1);
    F v = flt_val;
    if (ARCHON_LIKELY(v >= F(0))) {
        if constexpr (!alt) {
            // Model: Default
            // Intuition: v = floor(v * (max_int + 1))
            v *= ((max_int & 1) == 1 ? F(2 * F(+max_int / 2 + 1)) : F(max_int + 1));
        }
        else {
            // Model: Alternate
            // Intuition: v = round(v * max_int)
            v *= F(max_int);
            v += F(1) / 2;
        }
        // Avoid undefined behavior in the conversion from floating point to integer.
        F max_flt = core::max_float_for_int<F, I>();
        if (ARCHON_LIKELY(v <= max_flt)) {
            I w = core::float_to_int_a<I>(v);
            if (ARCHON_LIKELY(w <= max_int))
                return w;
        }
        return max_int;
    }
    return I(0);
}


} // namespace impl


template<class F, class I> constexpr auto int_to_flt(I int_val, core::Type<I> max_int) noexcept -> F
{
    return F(F(int_val) / F(max_int));
}


template<class I, class F> constexpr auto flt_to_int(F flt_val, core::Type<I> max_int) noexcept -> I
{
    constexpr bool alt = false;
    return impl::flt_to_int<I, alt>(flt_val, max_int);
}


template<class I, class F> constexpr auto flt_to_int_a(F flt_val, core::Type<I> max_int) noexcept -> I
{
    constexpr bool alt = true;
    return impl::flt_to_int<I, alt>(flt_val, max_int);
}


template<class T> constexpr auto change_bit_width(T v, int n, int m) noexcept -> T
{
    ARCHON_ASSERT(n > 0 && n <= core::int_width<T>());
    ARCHON_ASSERT(m > 0 && m <= core::int_width<T>());
    auto v_2 = core::promote(v);
    using type = decltype(v_2);
    if (m < n)
        return core::int_cast_a<T>(v_2 >> (n - m)); // Shrink
    // Expand
    int n_2 = n;
    for (;;) {
        int n_3 = n_2 << 1;
        if (m < n_3) {
            int r = m - n_2;
            return core::int_cast_a<T>(r != 0 ? v_2 << r | v_2 >> (n_2 - r) : v_2);
        }
        // Double the bit sequence when possible, since this is particularly easy
        v_2 *= (type(1) << n_2) + 1;
        n_2 = n_3;
    }
}


template<int N, int M, class I, class J> auto int_to_int_a(I val, I max_1, J max_2) -> J
{
    using type = core::fast_unsigned_ext_int_type<N + M>;
    type val_2 = core::int_cast_a<type>(val) * core::int_cast_a<type>(max_2);
    return core::int_cast_a<J>(core::int_div_round_half_down(val_2, max_1));
}


} // namespace archon::util::unit_frac

#endif // ARCHON_X_UTIL_X_UNIT_FRAC_HPP
