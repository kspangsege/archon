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


#include <limits>

#include <archon/core/features.hpp>
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


/// \{
///
/// \brief Convert fraction of unity from integer-based to floating-point representation.
///
/// These functions produces a floating-point value that corresponds to the fraction of
/// unity represented by the specified integer value. There are two available models for
/// mapping a fraction of unity between floating-point and integer representations (see
/// "Mapping models" below for a detailed discussion):
///
/// `int_to_flt<F>(int_val, max_int)` computes `F(int_val) / max_int`.
///
/// `int_to_flt_a<F>(int_val, max_int)` computes `F(int_val + 0.5) / (max_int + 1)`.
///
/// While the nominal range for the specified integer value (\p int_val) is zero -> \p
/// max_int, these functions do not require the specified value to be in this range, nor do
/// they clamp it to the range. If the specified value is outside the range, the returned
/// value will be less than zero or greater than one.
///
/// \tparam F A standard floating-point type (std::is_floating_point).
///
/// \tparam I A standard integer type (std::is_integral).
///
/// \sa \ref unit_frac::int_to_flt(), \ref unit_frac::int_to_flt_a()
/// \sa \ref unit_frac::flt_to_int(), \ref unit_frac::flt_to_int_a()
/// \sa \ref unit_frac::int_to_int(), \ref unit_frac::int_to_int_a(), \ref unit_frac::change_bit_width()
///
///
/// ### Mapping models
///
/// 1. Default model: Point-Sampled Quantization
///
///    Int to float: `F(int_val) / max_int`
///
///    Float to int: `round_half_even(flt_val * max_int)`
///
///    Functions: \ref unit_frac::int_to_flt(), \ref unit_frac::flt_to_int(), \ref
///    unit_frac::int_to_int()
///
///    Domain: Computer Graphics, GPUs (Vulkan/OpenGL/DirectX), and Image Processing.
///
///    Behavior: Treats integers as dimensionless point-samples mapped to exactly `int_val /
///    max_int`. The "bins" for zero and \p max_int are exactly half the width of interior
///    bins (see diagram below). This ensures that a smooth gradient correctly reaches the
///    absolute endpoints without statistical bias. Rounds to the even integer in half-way
///    cases during conversion from floating-point to integer representation. This ensures
///    reasonable numeric stability across multiple round-trip conversions. This is the
///    industry standard for color conversion and alpha blending.
///
/// 2. Alternative model: Uniform Binning / Uniform Quantization
///
///    Int to float: `F(int_val + 0.5) / (max_int + 1)`
///
///    Float to int: `min(trunc(flt_val * (max_int + 1)), max_int)`
///
///    Functions: \ref unit_frac::int_to_flt_a(), \ref unit_frac::flt_to_int_a(), \ref
///    unit_frac::int_to_int_a(), \ref unit_frac::change_bit_width()
///
///    Domain: DSP, Statistics, Spatial Subdivision, and hardware bit-replication.
///
///    Behavior: Divides the floating-point unit range into `max_int + 1` intervals of equal
///    size (see diagram below). Every integer receives an exact, equal share of the
///    floating-point domain.
///
///    Warning: Applying Uniform Binning to point-sampled image data (like standard 8-bit
///    PNGs) will result in subtle color shifting and off-by-one errors during round-trip
///    conversions. Use only for statistical binning or pure mathematical scaling.
///
/// Illustration of bin layout for the two models in a case where \p max_int is 3:
///
///              0                                               1
///     ---------|-----------------------------------------------|--------->
///              .                                               .
///      |<----- 0 ----->|<----- 1 ----->|<----- 2 ----->|<----- 3 ----->|  Default model
///              .                                               .
///              |<--- 0 --->|<--- 1 --->|<--- 2 --->|<--- 3 --->|          Alternative model
///
template<class F, class I>
constexpr auto int_to_flt(I int_val, core::type<I> max_int = core::int_max<I>()) noexcept -> F;
template<class F, class I>
constexpr auto int_to_flt_a(I int_val, core::type<I> max_int = core::int_max<I>()) noexcept -> F;
/// \}


/// \{
///
/// \brief Convert fraction of unity from floating-point to integer-based representation.
///
/// These functions convert the specified floating-point representation of a fraction of
/// unity to the corresponding integer representation. The two functions do this in slightly
/// different ways (see "Mapping models" under \ref unit_frac::int_to_flt() for a detailed
/// discussion):
///
/// `flt_to_int(flt_val, max_int)` computes `round_half_even(flt_val * max_int)` when \p
/// flt_val is between zero and one.
///
/// `flt_to_int_a(flt_val, max_int)` computes `min(trunc(flt_val * (max_int + 1)), max_int)`
/// when \p flt_val is between zero and one.
///
/// Both functions have clamping behavior, which means that they return zero when \p flt_val
/// is less than zero and \p max_int when \p flt_val is greater than one.
///
/// These functions convert NaN to zero.
///
/// \tparam I A standard integer type (std::is_integral).
///
/// \tparam F A standard floating-point type (std::is_floating_point).
///
/// \sa \ref unit_frac::flt_to_int(), \ref unit_frac::flt_to_int_a()
/// \sa \ref unit_frac::int_to_flt(), \ref unit_frac::int_to_flt_a()
///
template<class I, class F>
constexpr auto flt_to_int(F flt_val, core::type<I> max_int = core::int_max<I>()) noexcept -> I;
template<class I, class F>
constexpr auto flt_to_int_a(F flt_val, core::type<I> max_int = core::int_max<I>()) noexcept -> I;
/// \}


/// \{
///
/// \brief Convert fraction of unity between integer-based representations.
///
/// These functions convert a fraction of unity from one integer representation to
/// another. Both the origin and target representations are specified in terms of their
/// maximum value (\p max_1, \p max_2). There are two available mapping models (see "Mapping
/// models" under \ref unit_frac::int_to_flt() for a detailed discussion):
///
/// In the case of `int_to_int()`, the conversion is an infinitely precise alternative to
/// first converting to floating-point representation using \ref unit_frac::int_to_flt() and
/// then to the target integer representation using \ref unit_frac::flt_to_int().
///
/// In the case of `int_to_int_a()`, the conversion is an infinitely precise alternative to
/// first converting to floating-point representation using \ref unit_frac::int_to_flt()
/// and then to the target integer representation using \ref unit_frac::flt_to_int_a().
///
/// The overloads that only take one argument are shorthands for passing
/// `core::int_mask<I>(M)` for `max_1` and `core::int_mask<I>(N)` for `max_2`.
///
/// For the special case where both \p max_1 and \p max_2 are one minus a power of two, see
/// \ref change_bit_width().
///
/// \tparam M A limit on the number of value bits needed to represent \p max_1. In general,
/// a lower value leads to a more efficient conversion operation.
///
/// \tparam N A limit on the number of value bits needed to represent \p max_2. In general,
/// a lower value leads to a more efficient conversion operation.
///
/// \tparam I, J These must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
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
/// \sa \ref unit_frac::int_to_int(), \ref unit_frac::int_to_int_a(), \ref unit_frac::change_bit_width()
/// \sa \ref unit_frac::int_to_flt(), \ref unit_frac::int_to_flt_a()
/// \sa \ref unit_frac::flt_to_int(), \ref unit_frac::flt_to_int_a()
///
template<int M, int N, class I>          constexpr auto int_to_int(I val) noexcept -> I;
template<int M, int N, class I>          constexpr auto int_to_int_a(I val) noexcept -> I;
template<int M, int N, class I, class J> constexpr auto int_to_int(I val, I max_1, J max_2) noexcept -> J;
template<int M, int N, class I, class J> constexpr auto int_to_int_a(I val, I max_1, J max_2) noexcept -> J;
/// \}


/// \brief Convert fraction of unity between integer-based representations of different bit
/// width.
///
/// This function converts a fraction of unity from an integer representation of width \p m
/// to another integer representation of width \p n.
///
/// This function is functionally equivalent to `unit_frac::int_to_int_a<p, p>(v,
/// core::int_mask<T>(m), core::int_mask<T>(n))` where `p = core::int_width<T>()`, but it is
/// generally more efficient to compute.
///
/// Behavior is undefined if \p v is out of range, i.e., if it is less than zero, or greater
/// than, or equal to `2^m`. So long as \p v is in range, the result is guaranteed to also
/// be in range, i.e, greater than, or equal to zero, and less than `2^n`.
///
/// \tparam T Must conform to the integer concept (\ref Concept_Archon_Core_Integer).
///
/// \sa \ref unit_frac::int_to_int_a()
///
template<class T> constexpr auto change_bit_width(T v, int m, int n) noexcept -> T;








// Implementation


template<class F, class I> constexpr auto int_to_flt(I int_val, core::type<I> max_int) noexcept -> F
{
    return F(F(int_val) / F(max_int));
}


template<class F, class I> constexpr auto int_to_flt_a(I int_val, core::type<I> max_int) noexcept -> F
{
    static_assert(core::int_is_odd(core::int_max<I>())); // Needed below
    auto max_int_2 = core::promote(max_int);
    return (F(int_val) + F(0.5)) / (core::int_is_odd(max_int_2) ? 2 * F(max_int_2 / 2 + 1) : F(max_int_2 + 1));
}


template<class I, class F> constexpr auto flt_to_int(F flt_val, core::type<I> max_int) noexcept -> I
{
    // Intuition: round_half_even(flt_val * max_int) plus clamping behavior
    if (ARCHON_LIKELY(flt_val >= F(0))) {
        // The following multiplication involves a rounding to the nearest representable
        // floating-point value. This is the only place where loss of precision can occur in
        // this function in a way that affects the result.
        F flt_val_2 = flt_val * F(max_int);
        // Avoid undefined behavior in the conversion from floating point to integer.
        F max_flt = core::max_float_for_int<F, I>();
        if (ARCHON_LIKELY(flt_val_2 <= max_flt)) {
            using int_type = core::promoted_type<I>;
            int_type int_val = int_type(flt_val_2);
            if (ARCHON_LIKELY(int_val < int_type(max_int))) {
                using lim_type = std::numeric_limits<F>;
                if constexpr (core::int_is_even(lim_type::radix) && ARCHON_ENABLE_PLATFORM_OPTIMIZATIONS) {
                    F diff = flt_val_2 - F(int_val);
                    F half = 0.5; // Exactly representable
                    if (diff < half)
                        return I(int_val);
                    if (diff > half)
                        return I(int_val + 1);
                }
                else {
                    // Note on precision: While `flt_val_2 - F(int_val)` is always computed
                    // exactly (due to Sterbenz's Lemma and catastrophic cancellation of the
                    // integer bits), `F(int_val + 1) - flt_val_2` can suffer from precision
                    // loss if `flt_val_2` is very close to `int_val`. This happens because
                    // aligning the exponents forces the tiny fractional bits off the right
                    // edge of the mantissa.
                    //
                    // However, this precision loss is mathematically harmless. It only
                    // occurs when `flt_val_2` is extremely close to the floor, meaning
                    // `dist_down` will be orders of magnitude smaller than `dist_up`. The
                    // comparisons will trivially succeed despite the rounding error.
                    //
                    // Conversely, when `flt_val_2` approaches the exact halfway point
                    // (where precision is critical to resolve the tie-breaker), the
                    // exponents of the two operands align perfectly. In that region, the
                    // subtraction is guaranteed to be mathematically exact, ensuring
                    // flawless Banker's Rounding.
                    //
                    F dist_down = flt_val_2 - F(int_val);
                    F dist_up   = F(int_val + 1) - flt_val_2;
                    if (dist_down < dist_up)
                        return I(int_val);
                    if (dist_down > dist_up)
                        return I(int_val + 1);
                }
                if (core::int_is_even(int_val))
                    return I(int_val);
                return I(int_val + 1);
            }
        }
        return max_int;
    }
    return I(0);
}


template<class I, class F> constexpr auto flt_to_int_a(F flt_val, core::type<I> max_int) noexcept -> I
{
    // Intuition: min(floor(flt_val * (max_int + 1)), max_int) plus clamping behavior
    if (ARCHON_LIKELY(flt_val >= F(0))) {
        // The max value for the type is necessarily odd. This is needed below where 1 is
        // added when the value is even.
        static_assert((core::int_max<I>() & 1) == 1);
        F flt_val_2 = flt_val * (core::int_is_odd(max_int) ? F(2 * F(+max_int / 2 + 1)) : F(max_int + 1));
        // Avoid undefined behavior in the conversion from floating point to integer.
        F max_flt = core::max_float_for_int<F, I>();
        if (ARCHON_LIKELY(flt_val_2 <= max_flt)) {
            using int_type = core::promoted_type<I>;
            int_type int_val = int_type(flt_val_2);
            if (ARCHON_LIKELY(int_val <= int_type(max_int)))
                return I(int_val);
        }
        return max_int;
    }
    return I(0);
}


template<int M, int N, class I> constexpr auto int_to_int(I val) noexcept -> I
{
    if constexpr (M != 0 && N % M == 0)
        return unit_frac::change_bit_width(val, M, N);
    return unit_frac::int_to_int<M, N>(val, core::int_mask<I>(M), core::int_mask<I>(N));
}


template<int M, int N, class I> constexpr auto int_to_int_a(I val) noexcept -> I
{
    return unit_frac::change_bit_width(val, M, N);
}


template<int M, int N, class I, class J> constexpr auto int_to_int(I val, I max_1, J max_2) noexcept -> J
{
    using type = core::fast_unsigned_ext_int_type<M + N>;
    type val_2 = core::int_cast_a<type>(val) * core::int_cast_a<type>(max_2);
    return core::int_cast_a<J>(core::int_div_round_half_even(val_2, max_1));
}


template<int M, int N, class I, class J> constexpr auto int_to_int_a(I val, I max_1, J max_2) noexcept -> J
{
    if (ARCHON_LIKELY(val < max_1)) {
        using type = core::fast_unsigned_ext_int_type<M + N>;
        type val_2 = core::int_cast_a<type>(val) * (core::int_cast_a<type>(max_2) + type(1));
        return core::int_cast_a<J>(core::int_div_round_down(val_2, max_1));
    }
    return max_2;
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
        v_2 *= (type(1) << n_2) + type(1);
        n_2 = n_3;
    }
}


} // namespace archon::util::unit_frac

#endif // ARCHON_X_UTIL_X_UNIT_FRAC_HPP
