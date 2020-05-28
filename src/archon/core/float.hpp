// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_FLOAT_HPP
#define ARCHON_X_CORE_X_FLOAT_HPP

/// \file


#include <cmath>
#include <type_traits>
#include <limits>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float_traits.hpp>


namespace archon::core {


/// \brief Whether type is floating-point.
///
/// This function returns `true` if, and only if \p F is a floating-point type type, which,
/// in this context, means that `F` conforms to \ref Concept_Archon_Core_Float. `F` conforms
/// to \ref Concept_Archon_Core_Float precisely when `core::FloatTraits<F>::is_specialized`
/// is `true`.
///
/// The standard floating-point types do conform to \ref Concept_Archon_Core_Float if they
/// conform to IEC 559 (IEEE 754), which they usually do. If \ref
/// ARCHON_ASSUME_VALID_STD_FLOAT is nonzero, they will be assumed to conform in any case,
/// even if conformance to IEC 559 is not know For more on this, see \ref core::FloatTraits
/// and \ref ARCHON_ASSUME_VALID_STD_FLOAT.
///
template<class F> constexpr bool is_float() noexcept;


/// \{
///
/// \brief Reliably compare floating-point and integer values.
///
/// These functions perform reliable comparisons between floating-point and integer
/// values. In each case, the comparison is performed in such a way that the result is
/// always correct under the mathematical sense of that comparison. Note that this is not
/// trivial, because the core language (C++17) offers no way to directly compare a
/// floating-point value with an integer value.
///
/// The comparisons give the right result for all values of the floating-point type and all
/// values of the integer type. In particular, if the floating-point value is a quiet NaN,
/// the comparisons always return `false`.
///
/// The comparisons are carried out in a way that never invokes undefined behavior so long
/// as the finite range of the floating point type (\p F) completely covers the range of the
/// integer type (\p I). Here, the range is to be understood as an interval of real numbers
/// extending from the lowest to the highest representable value.
///
/// The type of the specified floating-point value (\p F) must conform to the floating-point
/// concept (\ref Concept_Archon_Core_Float).
///
/// The type of the specified integer value (\p I) must be one of the standard integer types
/// (those for which `std::is_integral_v` is `true`).
///
/// FIXME: Make constexpr in C++23
///
template<class F, class I> bool float_equal_int(F float_val, I int_val);
template<class F, class I> bool float_less_int(F float_val, I int_val);
template<class F, class I> bool float_greater_int(F float_val, I int_val);
template<class F, class I> bool float_less_equal_int(F float_val, I int_val);
template<class F, class I> bool float_greater_equal_int(F float_val, I int_val);
/// \}


/// \brief Convert floating-point value to integer value with clamping behavior.
///
/// This function converts the specified floating-point value (\p val) to an integer value
/// of the specified type (\p I). If the specified value is outside the range representable
/// in the integer type, it will be silently clamped to that range.
///
/// If the specified value is less than the minimum value representable in `I`, the result
/// of the conversion is the minimum value representable in `I`. Here, "less than" is to be
/// understood as \ref core::float_less_int().
///
/// If the specified value is greater than the maximum value representable in `I`, the
/// result of the conversion is the maximum value representable in `I`. As before, "greater
/// than" is to be understood as \ref core::float_greater_int().
///
/// If the specified value is greater than, or equal to the minimum value representable in
/// `I`; and less than, or equal to the maximum value representable in `I`, the result of
/// the conversion is `core::float_to_int_a<I>(val)` (see \ref core::float_to_int_a()).
///
/// If the specified value is a quiet NaN (see `std::numeric_limits::has_quiet_NaN`), the
/// result of the conversion is zero.
///
/// The type of the specified floating-point value (\p F) must conform to the floating-point
/// concept (\ref Concept_Archon_Core_Float).
///
/// The specified integer type (\p I) must be one of the standard integer types (those for
/// which `std::is_integral_v` is `true`).
///
/// \sa \ref core::float_to_int_a(), \ref core::try_float_to_int()
///
/// FIXME: Make constexpr in C++23
///
template<class I, class F> auto clamped_float_to_int(F val) -> I;


/// \brief Try to convert floating-point value to integer value.
///
/// If the specified value (\p float_val) lies between the minimum and maximum values
/// representable in the specified integer type (\p I), or if it is equal to either the
/// minimum or the maximum value, this function returns `true` after setting \p int_val to
/// `core::float_to_int_a<I>(float_val)` (see \ref core::float_to_int_a()). Otherwise this
/// function returns `false` and leaves `int_val` unchanged.
///
/// The comparisons between a floating-point and an integer value, indirectly referred to
/// above, are to be understood as \ref core::float_less_int() and \ref
/// core::float_greater_int() respectively..
///
/// If the specified value is a quiet NaN (see `std::numeric_limits::has_quiet_NaN`), this
/// function returns `false` and leaves `int_val` unchanged.
///
/// The type of the specified floating-point value (\p F) must conform to the floating-point
/// concept (\ref Concept_Archon_Core_Float).
///
/// The specified integer type (\p I) must be one of the standard integer types (those for
/// which `std::is_integral_v` is `true`).
///
/// \sa \ref core::float_to_int_a(), \ref core::clamped_float_to_int()
///
/// FIXME: Make constexpr in C++23
///
template<class F, class I> bool try_float_to_int(F float_val, I& int_val);


/// \brief Checked conversion of floating-point to integer.
///
/// `float_to_int(f, i)` has the same effect as `try_float_to_int(f, i)` except that the
/// former throws `std::overflow_error` when, and only when the latter returns `false`.
///
template<class F, class I> void float_to_int(F float_val, I& int_val);


/// \brief Cast floating-point value to integer, or boolean type.
///
/// This function casts the specified floating-point value (\p val) to an integer value of
/// the specified type (\p I). If the integer type (\p I) is not `bool`, the returned value
/// is `I(P(val))` where `P` is the type of the promotion of a value of type `I`. If the
/// integer type is `bool`, the returned value is `core::int_cast_a<I>(P(val))`, which means
/// that conversion occurs as if `bool` was an unsigned integer type with one value bit (see
/// \ref core::int_cast_a()).
///
/// CAUTION: The conversion, `J(val)`, for some integer type `J`, invokes undefined behavior
/// unless `std::trunc(val)` is representable in `J`. For that reason, this function should
/// generally not be used unless the caller is sure that `std::trunc(val)` is representable
/// in `I` (or at least in `P`). For alternative forms of the conversion, that are
/// guaranteed to not invoke undefined behavior, see \ref core::clamped_float_to_int() and
/// \ref core::try_float_to_int().
///
/// The specified integer type (\p I) must be one of the standard integer types (those for
/// which `std::is_integral_v` is `true`).
///
/// The type of the specified floating-point value (\p F) must either conform to the
/// floating-point concept (\ref Concept_Archon_Core_Float), or be one of the standard
/// floating-point types (those for which `std::is_floating_point_v` is `true`).
///
/// \sa \ref core::clamped_float_to_int(), \ref core::try_float_to_int()
///
template<class I, class F> constexpr auto float_to_int_a(F val) -> I;


/// \{
///
/// \brief Determine lowest and highest floating-point value inside integer range.
///
/// `min_float_for_int()` determines the lowest floating-point value that is not lower than
/// the lowest integer value representable in the specified integer type (\p I).
///
/// `max_float_for_int()` determines the highest floating-point value that is not higher
/// than the highest integer value representable in the specified integer type (\p I).
///
/// For a given floating-point value `f` of type `F`, `float_to_int_a<I>(f)` (think `I(f)`)
/// avoids unspecified behavior from the point of view of C++17 precisely when
/// `std::trunc(f) >= min_float_for_int<F, I>() && std::trunc(f) <= max_float_for_int<F,
/// I>()`.
///
/// The specified floating-point type (\p F) must conform to the floating-point concept
/// (\ref Concept_Archon_Core_Float).
///
/// The specified integer type (\p I) must be one of the standard integer types (those for
/// which `std::is_integral_v` is `true`).
///
/// \note It will be tempting to think of these values as the lowest and highest values
/// respectively for `f` for which `I(f)` avoids unspecified behavior from the point of view
/// of C++17, however, this is *not always correct*. For a small-range integer type, the
/// lowest and highest values for which `I(f)` avoids unspecified behavior will probably be
/// a little greater than `min_float_for_int<F, I>() - 1` and a little less than
/// `max_float_for_int<F, I>() + 1` respectively, but those values are not trivial to
/// compute generically.
///
/// FIXME: Make constexpr in C++23
///
template<class F, class I> auto min_float_for_int() -> F;
template<class F, class I> auto max_float_for_int() -> F;
/// \}


/// \{
///
/// \brief Determine important limits for conversion between floating-point and integer.
///
/// `min_float_not_below_nonpos_int()` determines the lowest floating-point value (most
/// negative) that is greater than, or equal to the specified non-positive integer value.
///
/// `max_float_not_above_nonneg_int()` determines the highest floating-point value that is
/// less than, or equal to the specified non-negative integer value.
///
/// The specified floating-point type (\p F) must conform to the floating-point concept
/// (\ref Concept_Archon_Core_Float).
///
/// The type of the specified integer (\p i) must be one of the standard integer types
/// (those for which `std::is_integral_v` is `true`).
///
/// Behavior is undefined if a non-positive integer is passed to
/// `max_float_not_above_nonneg_int()`, or if a non-negative integer is passed to
/// `min_float_not_below_nonpos_int()`.
///
/// Behavior is undefined if \p i is less than the lowest finite value representable in \p
/// F, or if \p i is greater than the highest finite value representable in \p F.
///
/// FIXME: Make constexpr in C++23
///
template<class F, class I> auto min_float_not_below_nonpos_int(I i) -> F;
template<class F, class I> auto max_float_not_above_nonneg_int(I i) -> F;
/// \}








// Implementation


template<class F> constexpr bool is_float() noexcept
{
    using traits_type = core::FloatTraits<F>;
    return traits_type::is_specialized;
}


template<class F, class I> inline bool float_equal_int(F float_val, I int_val)
{
    if (ARCHON_LIKELY(float_val >= (core::min_float_for_int<F, I>()) &&
                      float_val <= (core::max_float_for_int<F, I>()))) // Throws
        return (std::trunc(float_val) == float_val &&
                core::float_to_int_a<I>(float_val) == int_val); // Throws
    return false;
}


template<class F, class I> inline bool float_less_int(F float_val, I int_val)
{
    if (ARCHON_UNLIKELY(float_val < F(0))) {
        if constexpr (core::is_signed<I>()) {
            F float_val_2 = std::nextafter(float_val, F(0)); // Throws
            return (float_val < core::min_float_for_int<F, I>() ||
                    core::float_to_int_a<I>(float_val_2) <= int_val); // Throws
        }
        return (float_val < F(0)); // Catch NaN
    }
    return (float_val <= core::max_float_for_int<F, I>() &&
            core::float_to_int_a<I>(float_val) < int_val); // Throws
}


template<class F, class I> inline bool float_greater_int(F float_val, I int_val)
{
    if (ARCHON_LIKELY(float_val > F(0))) {
        F float_val_2 = std::nextafter(float_val, F(0)); // Throws
        return (float_val > core::max_float_for_int<F, I>() ||
                core::float_to_int_a<I>(float_val_2) >= int_val); // Throws
    }
    if constexpr (core::is_signed<I>()) {
        return (float_val >= core::min_float_for_int<F, I>() &&
                core::float_to_int_a<I>(float_val) > int_val); // Throws
    }
    return (float_val > F(0)); // Catch NaN
}


template<class F, class I> inline bool float_less_equal_int(F float_val, I int_val)
{
    if (ARCHON_UNLIKELY(float_val <= F(0))) {
        if constexpr (core::is_signed<I>()) {
            return (float_val < core::min_float_for_int<F, I>() ||
                    core::float_to_int_a<I>(float_val) <= int_val); // Throws
        }
        return (float_val <= F(0)); // Catch NaN
    }
    F float_val_2 = std::nextafter(float_val, F(0)); // Throws
    return (float_val <= core::max_float_for_int<F, I>() &&
            core::float_to_int_a<I>(float_val_2) < int_val); // Throws
}


template<class F, class I> inline bool float_greater_equal_int(F float_val, I int_val)
{
    if (ARCHON_LIKELY(float_val >= F(0)))
        return (float_val > core::max_float_for_int<F, I>() ||
                core::float_to_int_a<I>(float_val) >= int_val); // Throws
    if constexpr (core::is_signed<I>()) {
        F float_val_2 = std::nextafter(float_val, F(0)); // Throws
        return (float_val >= core::min_float_for_int<F, I>() &&
                core::float_to_int_a<I>(float_val_2) > int_val); // Throws
    }
    return (float_val >= F(0)); // Catch NaN
}


template<class I, class F> inline auto clamped_float_to_int(F val) -> I
{
    // Making sure comparisons are done such that NaN is converted to zero.
    if (ARCHON_LIKELY(val >= (core::min_float_for_int<F, I>()))) { // Throws
        if (ARCHON_LIKELY(val <= (core::max_float_for_int<F, I>()))) // Throws
            return core::float_to_int_a<I>(val); // Throws
        return core::int_max<I>();
    }
    if (ARCHON_LIKELY(val < (core::min_float_for_int<F, I>()))) // Throws
        return core::int_min<I>();
    return I(0); // NaN
}


template<class F, class I> inline bool try_float_to_int(F float_val, I& int_val)
{
    // Making sure comparisons are done such that NaN is rejected.
    if (ARCHON_LIKELY(float_val >= (core::min_float_for_int<F, I>()) &&
                      float_val <= (core::max_float_for_int<F, I>()))) { // Throws
        int_val = core::float_to_int_a<I>(float_val); // Throws
        return true;
    }
    return false;
}


template<class F, class I> inline void float_to_int(F float_val, I& int_val)
{
    if (ARCHON_LIKELY(core::try_float_to_int(float_val, int_val)))
        return;
    throw std::overflow_error("Floating-point to integer conversion");
}


template<class I, class F> constexpr auto float_to_int_a(F val) -> I
{
    using int_type   = I;
    using float_type = F;

    static_assert(std::is_integral_v<int_type>);
    static_assert(std::is_floating_point_v<float_type> || core::is_float<float_type>());

    using promoted_int_type = core::promoted_type<int_type>;
    return core::int_cast_a<int_type>(promoted_int_type(val));
}


template<class F, class I> inline auto min_float_for_int() -> F
{
    I min_int = std::numeric_limits<I>::min();
    return core::min_float_not_below_nonpos_int<F>(min_int); // Throws
}


template<class F, class I> inline auto max_float_for_int() -> F
{
    I max_int = std::numeric_limits<I>::max();
    return core::max_float_not_above_nonneg_int<F>(max_int); // Throws
}


template<class F, class I> inline auto min_float_not_below_nonpos_int(I i) -> F
{
    // The proof for the correct behavior of this function proceeds exactly as
    // for `max_float_not_above_nonneg_int()` except for sign inversion.

    using float_type = F;
    using int_type   = I;

    static_assert(core::is_float<float_type>());
    static_assert(std::is_integral_v<int_type>);

    ARCHON_ASSERT(core::int_less_equal(i, 0));

    using float_traits_type = core::FloatTraits<float_type>;
    using promoted_int_type = core::promoted_type<int_type>;

    float_type f = float_type(i);
    constexpr int r = float_traits_type::radix;
    bool not_below = (core::int_greater_equal(i, -1) || promoted_int_type(f / r) >= core::promote(i) / r);
    if (ARCHON_LIKELY(not_below))
        return f;
    return float_traits_type::nextafter(f, float_type(0)); // Throws
}


template<class F, class I> inline auto max_float_not_above_nonneg_int(I i) -> F
{
    // A proof for the correct behavior of this function:
    //
    // We know that if `f` (see code below) is not exactly equal to `i`, then `f` is either
    // the lowest representable value greater than `i` or the highest representable value
    // less that `i` [^1]. Therefore, what this function needs to do, is to determine
    // whether `f` is less than, or equal to `i` (`not_above`), and if so, return
    // `f`. Otherwise, it must return the next representable value from `f` in the direction
    // of `i` which is also the direction of zero, i.e., it must return `nextafter(f, 0)`.
    //
    // To see that `not_above` is `true` precisely when `f` is less than, or equal to `i`,
    // let us first look at the case where `i` is 0 or 1. In these two cases, `not_above`
    // clearly becomes `true`, which is correct, because `f` is guaranteed to be equal to
    // `i` [^2].
    //
    // Next, let us look at the case where `i` is greater than, or equal to 2, and `f` is
    // less than or equal to `i`. In that case, we know that `f` must be an integer, because
    // if `f` was not an integer, `f` could not be equal to `i`, so `f` would have to have
    // been strictly less than `i`, however, `ceil(f)` must be representable in `F` [^3],
    // and would have had to be less than, or equal to `i`, but that is impossible because
    // no representable value can come between `f` and `i` [^1]. Since `f` is an integer,
    // `p(f / r)` must be equal to `p(f) / r` [^4][^5], where `p` is `promoted_int_type`,
    // and where the latter division is integer division. Because `f` is an integer and less
    // than, or equal to `i`, `p(f) / r` must be less than, or equal to `i / r` (both are
    // integer divisions). This proves that `not_above` is `true` in this case.
    //
    // Finally, let us look at the case where `i` is greater than, or equal to 2, and `f` is
    // greater than `i`. In this case `f / r` must be representable in `F` [^6] and be an
    // integer [^7], therefore, `p(f / r)` must be equal to `f / r` [^8]. Additionally,
    // because `f` is greater than `i`, `f / r` must be greater than 'i' divided by `r`
    // (division on the real numbers), which, in turn, must be greater than, or equal to `i
    // / r` (integer division). This proves that `not_above` is `false` in this case.
    //
    // [^1]: This follows from requirement 7 of the "Basic requirements for floating-point
    //       types" in \ref core::FloatTraits, i.e., the one about conversion from integer
    //       type.
    //
    // [^2]: This follows from requirements 1 and 7 of the "Basic requirements for
    //       floating-point types" in \ref core::FloatTraits, i.e., the one about
    //       representability of certain small integers, and the one about conversion from
    //       integer type.
    //
    // [^3]: This follows from requirement 5 of the "Basic requirements for floating-point
    //       types" in \ref core::FloatTraits, i.e., the one about representability of
    //       `floor(f)`, `ceil(f)`, and `trunc(f)`.
    //
    // [^4]: Note that `f / r` is representable in `F` because of requirements 1 and 8 of
    //       the "Basic requirements for floating-point types" in \ref core::FloatTraits,
    //       i.e., the one about representability of certain small integers, and the one
    //       about representability of `f / r` (`f` is greater than 1).
    //
    // [^5]: Note that neither `p(f / r)` nor `p(f) / r` invokes undefined behavior, because
    //       `f` is less than, or equal to `i`, which clearly is less than, or equal the the
    //       maximum representable integer. See requirement 6 of the "Basic requirements for
    //       floating-point types" in \ref core::FloatTraits, i.e., the one about conversion
    //       to integer type.
    //
    // [^6]: This follows from requirement 8 of the "Basic requirements for floating-point
    //       types" in \ref core::FloatTraits, i.e., the one about representability of `f /
    //       r` (note that `f` is greater than 1).
    //
    // [^7]: This follows from requirement 9 of the "Basic requirements for floating-point
    //       types" in \ref core::FloatTraits, i.e., the one about `f / r` being an integer
    //       under certain circumstances.
    //
    // [^8]: Note that `p(f / r)` does not invoke undefined behavior so long as `trunc(f /
    //       r)` is less than, or equal to the maximum representable integer, but `f / r`
    //       must be less than, or equal to `i`, because if it was not, `f` would not be the
    //       lowest representable value greater than `i`, and that would be a
    //       contradiction. See also requirement 6 of the "Basic requirements for
    //       floating-point types" in \ref core::FloatTraits, i.e., the one about conversion
    //       to integer type.

    using float_type = F;
    using int_type   = I;

    static_assert(core::is_float<float_type>());
    static_assert(std::is_integral_v<int_type>);

    ARCHON_ASSERT(core::int_greater_equal(i, 0));

    using float_traits_type = core::FloatTraits<float_type>;
    using promoted_int_type = core::promoted_type<int_type>;

    float_type f = float_type(i);
    constexpr int r = float_traits_type::radix;
    bool not_above = (core::int_less_equal(i, 1) || promoted_int_type(f / r) <= core::promote(i) / r);
    if (ARCHON_LIKELY(not_above))
        return f;
    return float_traits_type::nextafter(f, float_type(0)); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FLOAT_HPP
