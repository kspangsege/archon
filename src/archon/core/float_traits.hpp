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

#ifndef ARCHON_X_CORE_X_FLOAT_TRAITS_HPP
#define ARCHON_X_CORE_X_FLOAT_TRAITS_HPP

/// \file


#include <cmath>
#include <type_traits>
#include <limits>


namespace archon::core {


/// \brief Traits of floating-point types.
///
/// This class template represents a homogeneous low-level interface for working with
/// different kinds of floating-point types that live up to a certain set of minimum
/// requirements.
///
/// Applications are allowed to specialize this class template for new floating-point types
/// according to the rules laid out below.
///
/// All specializations must define `is_specialized`, which must be a compile-time constant
/// of type `bool`. A particular specialization, `T`, for type, `F`, is allowed to set
/// `is_specialized` to `true` if all of the basic floating-point requirements are met for
/// `F` (see below), and all of the requirements for a valid specialization are met for `T`
/// (see below). A type, `F`, for which `IntegerTraits<F>::is_specialized` is true, conforms
/// to \ref Concept_Archon_Core_Float.
///
/// The standard floating-point types (those for which `std::is_floating_point_v` is `true`)
/// are covered by the primary template. By default, these types will not be assumed to
/// satisfy the basic requirements for floating-point types unless they are known to conform
/// to IEC 559 (IEEE 754), but see \ref ARCHON_ASSUME_VALID_STD_FLOAT for a way to override
/// that.
///
/// Below, A value `f`, that is representable in `F`, is *finite* if that value is in R (the
/// real numbers). As such, a finite value cannot be inifinty, and cannot be NaN (not a
/// number).
///
///
/// #### Basic requirements for floating-point types
///
/// Let `F` be a type, let `f` and `g` be values of that type, and let `r` be the radix
/// associated with the type (see `T::radix` under requirements for valid
/// specialization). Then the following are the *basic requirements for floating-point
/// types*:
///
/// 1.  Integers 0, 1, -1, and `r` can be represented exactly in `F`.
///
/// 2.  If `f` is not infinity, not negative infinity, and not NaN, it is finite. `F` may,
///     or may not be able to represent infinity, and it may, or may not be able to
///     represent NaN (not a number).
///
/// 3.  `F(f)` must be a valid non-throwing copy-construction that can be evaluated at
///     compile-time when the operand is compile-time constant.
///
/// 4.  If `f` is a non-const l-value, then `f = g` must be a valid non-throwing assignment
///     that can be evaluated at compile-time when the operands can be evaluated at
///     compile-time.
///
/// 5.  `f == g`, `f != g`, `f < g`, `f > g`, `f <= g`, and `f >= g` must all be valid
///     non-throwing comparisons that can be evaluated at compile-time when the operands are
///     compile-time constants. The result of each of these expressions must be exact if the
///     operands are finite. For example, `f < g` must be `true` when, and only when `f` is
///     less than `g`, so long as as both `f` and `g` are finite. If `F` is able to
///     represent infinity, infinity must compare greater than all finite values. If `F` is
///     able to represent negative infinity, negative infinity must compare less than all
///     finite values, and less that positive infinity. If `F` is able to represent NaN (not
///     a number), any comparison involving NaN must be `false`.
///
/// 6.  `f + g`, `f - g`, `f * g`, `f / g`, `+f`, and `-f` must all be valid non-throwing
///     arithmetic expressions that can be evaluated at compile-time when the operands are
///     compile-time constants. The result of each of these expressions must be exact if the
///     operands are finite, and when the result of the corresponding operation on the real
///     numbers (R) is representable in `F`. In general, the operations must be
///     approximations to the corresponding operations on the reals (normative).
///
/// 7.  If `f` is finite, then the values of `floor(f)`, `ceil(f)`, and `trunc(f)`, are
///     representable in `F`. Here, `floor` and `ceil` should be understood as the
///     well-known functions on the real numbers (R) rather than their namesakes in the
///     standard C++ library, and `trunc(f)` should be understood as equal to `floor(f)` for
///     non-negative `f`, and equal to `ceil(f)` for negative `f`.
///
/// 8.  `f` can be converted to integer type, `I`, using the expression, `I(f)`, so long as
///     `I` is one of the standard integer types, other than `bool`, specified by C++17. The
///     conversion must be a non-throwing expression. The standard integer types are those
///     for which `std::is_integral_v` is `true`. If `trunc(f)` is within the range
///     representable in `I`, the result of the conversion is equal to
///     `trunc(f)`. Otherwise, behavior is undefined.
///
/// 9.  An integer, `i`, of type, `I`, can be converted to floating-point type, `F`, using
///     the expression, `F(i)`, so long as `I` is one of the standard integer types
///     specified by C++17 (those for which `std::is_integral_v` is `true`). The conversion
///     must be a non-throwing expression. If `i` can be represented exactly in `F`, the
///     conversion is exact. Otherwise, if `i` is between two adjacent finite floating-point
///     values that are representable in `F`, the result of the conversion one of those two
///     floating-point values, but it is unspecified which one. Otherwise, behavior is
///     undefined.
///
/// 10. If `f` is greater than 1 or less than -1, and `f` is finite, then `f / r` is
///     representable in `F`. Here, the division should be understood as division in the
///     real numbers (R) rather than as the division that is offered by the C++
///     implementation.
///
/// 11. If `i` is a postive integer, that is not representable in `F`, and `f` is the lowest
///     representable finite floating-point value greater than `i`, or if `i` is a negative
///     integer, that is not representable in `F`, and `f` is the highest representable
///     finite floating-point value less than `i`, then `f / r` is an integer. Note that it
///     follows from requirement 1 that `i` is greater than or equal to 2, or less than or
///     equal to -2. Note also that it follows from requirement 7 that `f` itself is an
///     integer, because if it was not, then `trunc(f)` would have been a representable
///     value between `i` and `f`, or a representable value equal to `i`, both of which
///     would be immediate contradictions. Note finally that it follows from requirement 10
///     that `f / r` is represetnable in `F`.
///
/// Note that requirements 8 and 9 directly mirror the requirements for conversion between
/// standard floating-point, and standard integer types in C++17.
///
/// A floating-point type that conforms to IEC 559 (IEEE 754) automatically satisfies all
/// these requirements.
///
/// FIXME: Probably also require direct non-throwing convertability to and from standard floating-point types (`float`, `double`, and `long double`).                            
///
///
/// #### Requirements for valid specializations
///
/// A particular specialization, `T`, for type, `F`, is *valid* if all of the following
/// requirements are met:
///
/// - `T::radix` is the radix associated with `F`. It must be a compile-time constant of
///   type `int`, and its value must be greater than, or equal to 2.  For the standard
///   floating-point types, this is \c std::numeric_limits<F>::radix.
///
/// - `T::nextafter(from, to)` must be a valid non-throwing function invocation if `from`
///   and `to` are values of type `F`. The return type must be `F`. If `from` and `to` are
///   finite and not equal, then the function must return the finite representable value
///   closest to `from`, but not equal to `from` in the direction of `to`. If `from` and
///   `to` are finite and equal, then the function must return `from`. For the standard
///   floating-point types, this is the same as `std::nextafter(from, to)`.
///
/// FIXME: When moving to C++23, require that `T::nextafter(from, to)` can be evaluated at
/// compile-time when the arguments are compile-time constants.
///
template<class F> struct FloatTraits;


/// \def ARCHON_ASSUME_VALID_STD_FLOAT
///
/// \brief Assume that standard floating-point types meet basic requirements.
///
/// If this macro is set to a value that evaluates to `true` in a boolean context, all the
/// basic requirements for floating-point types (see \ref FloatTraits) will be assumed to be
/// satisfied by all the standard floating-point types, i.e., those types, `F`, for which
/// `std::is_floating_point_v<F>` is `true`.
///
/// On the other hand, if this macro is set to a value that evaluates to `false` in a
/// boolean context (the default), then the basic requirements for floating-point types will
/// be assumed to be satisfied for a particular standard floating-point type, `F`, if, and
/// only if `std::numeric_limits<F>::is_iec559` is `true`.
///
#if !defined ARCHON_ASSUME_VALID_STD_FLOAT
#  define ARCHON_ASSUME_VALID_STD_FLOAT 0
#endif








// Implementation


namespace impl {


template<class F, bool I> struct FloatTraits {
    static constexpr bool is_specialized = false;
};


template<class F> struct FloatTraits<F, true> {
    using float_type = F;

    using limits_type = std::numeric_limits<float_type>;

#if ARCHON_ASSUME_VALID_STD_FLOAT
    static constexpr bool is_specialized = true;
#else
    static constexpr bool is_specialized = limits_type::is_iec559;
#endif

    static constexpr int radix = limits_type::radix;

    static auto nextafter(float_type from, float_type to) noexcept -> float_type
    {
        return std::nextafter(from, to);
    }
};


} // namespace impl


template<class F> struct FloatTraits
    : impl::FloatTraits<F, std::is_floating_point_v<F>> {
};


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FLOAT_TRAITS_HPP
