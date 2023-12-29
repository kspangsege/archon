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

#ifndef ARCHON_X_CORE_X_INTEGER_HPP
#define ARCHON_X_CORE_X_INTEGER_HPP

/// \file


#include <type_traits>
#include <limits>
#include <algorithm>
#include <utility>
#include <array>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer_traits.hpp>


namespace archon::core {


namespace impl {
template<class, class...> struct CommonIntType;
template<class T> struct IntDivMod;
} // namespace impl


/// \brief Whether type is integer.
///
/// This function returns `true` if, and only if \p T is an integer type. In this context,
/// "being an integer type" means conforming to \ref Concept_Archon_Core_Integer. `T` is an
/// integer type in this sense when, and only when `core::IntegerTraits<T>::is_specialized`
/// is `true`.
///
/// All fundamental integer types, including character types and `bool`, are considered, by
/// this function, to be integer types.
///
template<class T> constexpr bool is_integer() noexcept;


/// \brief Suitable common integer type for arithmetic.
///
/// This is a type that can generally be regarded as suitable for performing arithmetic that
/// involves objects of all of the specified integer types (\p T, \p U...). It plays a
/// similar role to `std::common_type_t`, but the two are generally not identical, even for
/// fundamental integer types. `core::common_int_type` generally provides a stronger
/// value-preservation guarantee than `std::common_type_t`, even for the fundamental integer
/// types (see below).
///
/// The specified types (\p T, \p U...) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// Since the availability of mixed-type arithmetic operators are not guaranteed by \ref
/// Concept_Archon_Core_Integer, "maximally" generic arithmetic should probably take the
/// following form:
///
/// \code{.cpp}
///
///    auto func(T a, T b, U c, U d)
///    {
///        using type = core::common_int_type<int, T, U>;
///        return (core::int_cast_a<type>(a) * core::int_cast_a<type>(c) +
///                core::int_cast_a<type>(b) * core::int_cast_a<type>(d));
///    }
///
/// \endcode
///
/// When two types, `A` and `B`, are specified, the common type is chosen as follows:
///
///   - If both `A` and `B` are unsigned, `core::common_int_type<A, B>` is the widest one,
///     or `A` if they are the same width (same number of value bits).
///
///   - If both `A` and `B` are signed, `core::common_int_type<A, B>` is the widest one. If
///     they have the same number of value bits, the one with the lowest minimum value is
///     chosen. If they have the same minimum value, `A` is chosen.
///
///   - If `A` is signed and `B` is unsigned, `core::common_int_type<A, B>` is `A` (the
///     signed type) if `A` has at least as many value bits as `B` (the unsigned
///     type). Otherwise, `core::common_int_type<A, B>` is `B`.
///
///   - If `A` is unsigned and `B` is signed, `core::common_int_type<A, B>` is the same type
///     as `core::common_int_type<B, A>`.
///
/// Note that `core::common_int_type<A, B>` is always either `A` or `B`.
///
/// When only one type, `A`, is specified, `core::common_int_type<A>` is `A`.
///
/// When more than two types, `A`, `B`, `C...`, are specified, `core::common_int_type<A, B,
/// C...>` is `core::common_int_type<A, core::common_int_type<B, C...>>`.
///
/// If `a` is an object of type `A`, `b` is an object of type `B`, and `C` is
/// `core::common_int_type<A, B>`, then `core::int_cast_a<C>(a)` and
/// `core::int_cast_a<C>(b)` are guaranteed to be a value preserving conversions if `a` and
/// `b` are not negative.
///
/// If `a` is negative, `C(a)` is still a value preserving conversion except when `A` is
/// signed, and `B` is unsigned, and `B` has more value bits than `A`. However, even in this
/// case, there is a guarantee of no information loss, because the conversion to `B` of the
/// negative value will fully preserve that value using two's complement representation in
/// `B`.
///
/// Similarly, if `b` is negative, `C(b)` is still a value preserving conversion except when
/// `B` is signed, and `A` is unsigned, and `A` has more value bits than `B`. However, as
/// before, even in this case, there is a guarantee of no information loss, because the
/// conversion to `A` of the negative value will fully preserve that value using two's
/// complement representation in `A`.
///
template<class T, class... U> using common_int_type = typename impl::CommonIntType<T, U...>::type;


/// \brief Type of result of promotion operation.
///
/// This is the type of the result of the promotion operation on a value of the specified
/// type (\p T). See \ref core::promote().
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> using promoted_type = decltype(+std::declval<T>());


/// \brief Type of result of strong promotion operation.
///
/// This is the type of the result of the strong promotion operation on a value of the
/// specified type (\p T). See \ref core::promote_strongly().
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> using strongly_promoted_type = core::common_int_type<int, core::promoted_type<T>>;


/// \brief Promote specified integer value.
///
/// This function returns the result of `+val`. For the standard integer types
/// (`std::is_integral`), this corresponds to the ordinary promotion operation. For standard
/// and non-standard integer types, it is a value-preserving operation. The return type is
/// `declval(+val)`.
///
/// Promotion is idempotent in the sense that if `val_2` is the result of `promote(val)` and
/// `val_3` is the result of `promote(val_2)`, then `val_3` has the same type as `val_2`.
///
/// When \p T is a standard integer type (`std::is_integral`), `core::promoted_type<T>` is
/// guaranteed to cover the entire value range of `int` if \p T is signed. If \p T is an
/// unsigned standard integer type, then `core::promoted_type<T>` is guaranteed to cover the
/// entire non-negative range of `int`. While, for regular promotion, these guarantees do
/// not extend to non-standard integer types, there is an alternative notion of promotion,
/// namely *strong promotion* (see \ref core::promote_strongly()), that does extend these
/// guarantees to all integer types that satisfy \ref Concept_Archon_Core_Integer.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// \sa \ref core::promote_strongly()
///
template<class T> constexpr auto promote(T val) noexcept -> core::promoted_type<T>;


/// \brief Strongly promote specified integer value.
///
/// This function returns the result of `core::int_cast_a<U>(val)` where `U` is
/// `core::common_int_type<int, core::promoted_type<T>>`. It is always a value-preserving
/// operation, and it is always idempotent in the sense that if `val_2` is the result of
/// `promote_strongly(val)` and `val_3` is the result of `promote_strongly(val_2)`, then
/// `val_3` has the same type as `val_2`.
///
/// A strongly promoted value is also regularly promoted (\ref core::promote()), so if
/// `val_2` is the result of `promote_strongly(val)` and `val_3` is the result of
/// `promote(val_2)`, then `val_3` has the same type as `val_2`.
///
/// If \p T is signed, `core::strongly_promoted_type<T>` is guaranteed to cover the entire
/// value range of `int`. If \p T is unsigned, `core::strongly_promoted_type<T>` is
/// guaranteed to cover the entire non-negative range of `int`. It follows, therefore, that
/// `core::strongly_promoted_type<T>` will cover the entire non-negative range of `int`
/// regardless of whether \p T is signed or unsigned.
///
/// Note that \ref core::promote() offers the same `int` coverage guarantees, but only when
/// \p T is one of the standard integer types (`std::is_integral`). Strong promotion differs
/// from regular promotion by extending these guarantees to non-standard integer types.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// \sa \ref core::promote()
///
template<class T> constexpr auto promote_strongly(T val) noexcept -> core::strongly_promoted_type<T>;


/// \brief Corresponding unsigned type for specified integer type.
///
/// This is the corresponding unsigned integer type for the specified integer type (\p
/// T). For fundamental integer types, the corresponding unsigned type is
/// `std::make_unsigned_t<T>`. In general, the corresponding unsigned type is specified by
/// the integer traits for \p T (see \ref core::IntegerTraits).
///
/// If \p T is unsigned, `core::unsigned_type<T>` is `T`. Otherwise, it is some unsigned
/// type with at least as many value bits as there are in \p T (\ref
/// core::num_value_bits()).
///
/// Note that the width of `core::unsigned_type<T>` can be lower (by one) than the width of
/// `T` (\ref core::int_width()).
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> using unsigned_type = typename core::IntegerTraits<T>::unsigned_type;


/// \brief Cast specified integer value to unsigned type.
///
/// This function casts the specified integer value to the corresponding unsigned type (\ref
/// core::unsigned_type). If the specified integer value has unsigned type, this function
/// returns the specified value unchanged, and without changing its type. For non-negative
/// values of signed type, this function is a value-preserving operation.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr auto to_unsigned(T val) noexcept -> core::unsigned_type<T>;


/// \brief Bit width of integer type.
///
/// Get the bit width of the specified integer type. The bit width is the number of value
/// bits (\ref core::num_value_bits()), plus one if the integer type is signed (\ref
/// core::is_signed()).
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// \sa \ref core::int_inner_width()
///
template<class T> constexpr int int_width() noexcept;


/// \brief Inner bit with of integer type.
///
/// This function returns the inner bit width of the specified integer type. The inner bit
/// width is the number of "fully covered" bits in the value range of the type. Formally, it
/// is floor(log2(N)) where N is the number of distinct representable values in \p T.
///
/// If \p T is unsigned, `int_inner_width<T>()` is equal to `core::int_width<T>()`. The same
/// is true if \p T is signed and uses two's complement representation for negative
/// values. If \p T uses ones' complement, or sign-magnitude representation for negative
/// values, `int_inner_width<T>()` is one less than `core::int_width<T>()`. See \ref
/// core::int_width().
///
/// Since C++20, if \p T is a standard or extended signed integer type,
/// `int_inner_width<T>()` is equal to `core::int_width<T>()` because those types are
/// required by the C++20 specification to use two's complement representation of negative
/// values.
///
template<class T> constexpr int int_inner_width() noexcept;


/// \brief Number of value bits in integer type.
///
/// This function returns the number of value bits in the specified integer type. For
/// fundamental types, this is the same as `std::numeric_limits<T>::digits`.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr int num_value_bits() noexcept;


/// \{
///
/// \brief Whether integer type is signed or unsigned.
///
/// `is_signed()` returns `true` if, and only if the specified integer type is signed. For
/// fundamental integer types, this is the same as `std::numeric_limits<T>::is_signed`.
///
/// `is_unsigned()` returns `true` if, and only if `is_signed()` returns `false`.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool is_signed() noexcept;
template<class T> constexpr bool is_unsigned() noexcept;
/// \}


/// \{
///
/// \brief Get minimum or maximum value for integer type.
///
/// Get the minimum, or maximum possible value for the specified integer type. These are
/// shorthands for `std::numeric_limits<T>::min()` and `std::numeric_limits<T>::max()`
/// respectively.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr auto int_min() noexcept -> T;
template<class T> constexpr auto int_max() noexcept -> T;
/// \}


/// \brief Determine if integer is power of two.
///
/// Determine, in a very efficient manner, whether the specified integer is a power of
/// two. No negative numbers are considered to be powers of two by this function.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool int_is_power_of_two(T val) noexcept;


/// \brief Find position of most significant bit.
///
/// This function returns the position of the most significant bit that is set in the
/// specified integer value, or -1 if the specified integer value is zero. The least
/// significant bit position is taken to be 0. For a signed integer type with N value bits,
/// the first N bit positions are taken to be the value bits. The position of the sign bit
/// is then one plus the position of the last value bit. This means that if \p val is
/// negative, the returned value is always N.
///
/// For values greater than, or equal to 1, this function can also be thought of as the
/// integer part of the base-2 logarithm.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// \sa \ref int_find_msd_pos().
///
template<class T> constexpr int int_find_msb_pos(T val) noexcept;


/// \brief Find position of most significant digit.
///
/// Find the position of the most significant digit in the specified value and with respect
/// to the specified base/radix. This works for negative values too. Returns -1 if the
/// specified value is zero. Otherwise it always returns a non-negative value.
///
/// For values greater than, or equal to 1, this function can also be thought of as the
/// integer part of the logarithm of the specified base.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// \sa \ref int_find_msb_pos().
///
/// \note If `v` is negative, `core::int_find_msd_pos(v, 2)` is generally **not** equal to
/// `core::int_find_msb_pos(v)`. On the other hand, if `v` is non-negative, the two are
/// definitely equal.
///
template<class T> constexpr int int_find_msd_pos(T val, int base = 10) noexcept;


/// \brief Find number of digits in integer value.
///
/// Find the number of digits needed to represent the specified value in the specified
/// base/radix. This function does not count the sign itself as a digit.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr int int_num_digits(T val, int base = 10) noexcept;


/// \brief Find maximum number of digits for integer type.
///
/// Find the maximum number of digits needed to represent any positive or negative value of
/// the specified integer type in the specified base/radix. This function does not count the
/// sign itself as a digit.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr int int_max_digits(int base) noexcept;


/// \brief Test for being zero.
///
/// This function return `true` when, and only when the specified value is zero. This
/// function can be useful when working with non-fundamental integer types.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool is_zero(T val) noexcept;


/// \brief Test for negativity.
///
/// This function allows you to test for a negative value in any integer type, even when the
/// type is unsigned. Normally, when the type is unsigned, such a test will produce a
/// compiler warning.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool is_negative(T val) noexcept;


/// \{
///
/// \brief Whether integer is even or odd.
///
/// `int_is_even()` returns `true` if, and only if the specified integer is even.
///
/// `int_is_odd()` returns `true` if, and only if the specified integer is odd. An integer
/// is always either even or odd, and never both.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool int_is_even(T val) noexcept;
template<class T> constexpr bool int_is_odd(T val) noexcept;
/// \}


/// \{
///
/// \brief Whether integer is equal to minimum, or maximum value for type.
///
/// `int_is_min()` returns `true` if, and only if the specified integer is equal to the
/// minimum value for its type, i.e., equal to `core::int_min<T>()`.
///
/// `int_is_max()` returns `true` if, and only if the specified integer is equal to the
/// maximum value for its type, i.e., equal to `core::int_max<T>()`.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr bool int_is_min(T val) noexcept;
template<class T> constexpr bool int_is_max(T val) noexcept;
/// \}


/// \brief Produce N-bit mask.
///
/// This function returns a value of the specified type in which the N least significant
/// bits are 1. Here, N is the smaller of \p num_bits and `std::numeric_limits<T>::digits`.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr auto int_mask(int num_bits) noexcept -> T;


/// \{
///
/// \brief Cast integer value without special treatment of `bool`.
///
/// `int_cast<T>(val)` has the same effect as `int_cast_a<T>(val)` except that it throws
/// `std::overflow_error` if the specified value can not be represented in `T` (see \ref
/// core::int_cast_a() and \ref core::can_int_cast()).
///
/// `int_cast(from, to)` has the same effect as `to = int_cast<T>(from)` if `T` is the type
/// of `to`.
///
/// The type of the specified value (\p F) and the target type (\p T) must both conform to
/// the integer concept (\ref Concept_Archon_Core_Integer).
///
/// \sa core::can_int_cast().
/// \sa core::try_int_cast().
/// \sa core::int_cast_a().
///
template<class T, class F> constexpr auto int_cast(F val) -> T;
template<class T, class F> constexpr void int_cast(F from, T& to);
/// \}


/// \brief Integer cast is value preserving.
///
/// This function returns true if, and only if the specified value can be represented in
/// `T`.
///
/// The type of the specified value (\p F) and the specified target type (\p T) must both
/// conform to the integer concept (\ref Concept_Archon_Core_Integer).
///
/// \sa core::int_cast().
///
template<class T, class F> constexpr bool can_int_cast(F val) noexcept;


/// \brief Try to cast integer value without special treatment of `bool`.
///
/// This function has the same effect as \ref core::int_cast_a() except that it performs the
/// conversion when, and only when the specified value can be represented in `T` (\ref
/// core::can_int_cast()). If it can, this function returns `true` and assigns the result to
/// \p to. Otherwise it returns `false` and leaves \p to untouched.
///
/// The type of \p from (\p F) and the type of \p to (\p T) must both conform to the integer
/// concept (\ref Concept_Archon_Core_Integer).
///
/// \sa core::int_cast().
///
template<class T, class F> constexpr bool try_int_cast(F from, T& to) noexcept;


/// \brief Cast to integer value without special treatment of `bool`.
///
/// This function operates just like a regular cast to \p T, except that it treats `bool` as
/// a regular unsigned integer type with one value bit, so `int_cast_a<bool>(2)` is zero,
/// not one. It also works for all integer types that conform to \ref
/// Concept_Archon_Core_Integer, and not just the fundamental integer types.
///
/// The types \p F and \p T must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// Just like for a regular cast, if the target integer type is signed, and the specified
/// value cannot be represented in that signed type, the result is implementation defined.
///
/// If the target integer type is unsigned, then, regardless of whether the target type is
/// `bool`, the result is the least unsigned value congruent to the specified value modulo
/// 2**N, where N is the number of value bits in the target type. Note how this deviates
/// from a regular cast in that it makes `int_cast_a<bool>(2)` equal to zero (or false),
/// whereas `bool(2)` is one (or true).
///
/// \sa core::int_cast().
///
template<class T, class F> constexpr auto int_cast_a(F val) noexcept -> T;


/// \{
///
/// \brief Cast integer value with clamping behavior.
///
/// If \p val is less than \p min, the result is \p min, otherwise, if \p val is greater
/// than \p max, the result is \p max, otherwise, the result is the same as
/// `int_cast_a<T>(val)`.
///
/// `int_cast_clamp(from, to, min, max)` has the same effect as `to =
/// int_cast_clamp<T>(from, min, max)` where `T` is the type of `to`.
///
/// `int_cast_clamp(from, to)` has the same effect as `int_cast_clamp(from, to,
/// core::int_min<T>(), core::int_max<T>())` where `T` is the type of `to`.
///
/// The type of the specified value (\p F) and the target type (\p T) must both conform to
/// the integer concept (\ref Concept_Archon_Core_Integer).
///
/// \sa core::int_cast().
/// \sa core::int_cast_a().
///
template<class T, class F> constexpr auto int_cast_clamp(F val, T min, T max) noexcept -> T;
template<class T, class F> constexpr void int_cast_clamp(F from, T& to, T min, T max) noexcept;
template<class T, class F> constexpr void int_cast_clamp(F from, T& to) noexcept;
/// \}


/// \{
///
/// \brief Convert integer to two's complement representation.
///
/// These functions have the same effect as \ref core::int_cast(), core::can_int_cast(),
/// \ref core::try_int_cast(), and \ref core::int_cast_a() respectively, except that if `T`
/// (the target type) is an unsigned integer type, these functions assume that `T` stores
/// negative values using two's complement representation.
///
/// Note in particular, that when \ref try_cast_to_twos_compl() returns `true`, the
/// conversion preserves the represented value, even when it is negative.
///
/// Note also that \ref cast_to_twos_compl_a() is effectively an alias for \ref
/// core::int_cast_a(). There is no difference between them. Additionally, if `T` is not
/// `bool`, \ref cast_to_twos_compl_a() is the same as a regular cast.
///
/// These functions make no assumptions about the platform except that it complies with at
/// least C++17.
///
template<class T, class F> constexpr auto cast_to_twos_compl(F val) -> T;
template<class T, class F> constexpr bool can_cast_to_twos_compl(F val) noexcept;
template<class T, class F> constexpr bool try_cast_to_twos_compl(F from, T& to) noexcept;
template<class T, class F> constexpr auto cast_to_twos_compl_a(F val) noexcept -> T;
/// \}


/// \{
///
/// \brief Convert integer from two's complement representation.
///
/// These functions have the same effect as \ref core::int_cast(), core::can_int_cast(),
/// \ref core::try_int_cast(), and \ref core::int_cast_a() respectively, except that if `F`
/// (the origin type) is an unsigned integer type, these functions assume that `F` stores
/// negative values using two's complement representation.
///
/// Note in particular, that when \ref try_cast_from_twos_compl() returns `true`, the
/// conversion preserves the represented value, even when it is negative.
///
/// These functions make no assumptions about the platform except that it complies with at
/// least C++17.
///
template<class T, class F> constexpr auto cast_from_twos_compl(F val) -> T;
template<class T, class F> constexpr bool can_cast_from_twos_compl(F val) noexcept;
template<class T, class F> constexpr bool try_cast_from_twos_compl(F from, T& to) noexcept;
template<class T, class F> constexpr auto cast_from_twos_compl_a(F val) noexcept -> T;
/// \}


/// \{
///
/// \brief Convert integer between two's complement representations.
///
/// These functions have the same effect as \ref core::int_cast(), core::can_int_cast(),
/// \ref core::try_int_cast(), and \ref core::int_cast_a() respectively, except that if `F`
/// (the origin type) is an unsigned integer type, these functions assume that `F` stores
/// negative values using two's complement representation. Similarly, if `T` (the target
/// type) is an unsigned integer type, these functions assume that `T` stores negative
/// values using two's complement representation.
///
/// Note in particular, that when \ref try_twos_compl_cast() returns `true`, the conversion
/// preserves the represented value, even when the value is negative.
///
/// These functions make no assumptions about the platform except that it complies with at
/// least C++17.
///
template<class T, class F> constexpr auto twos_compl_cast(F val) -> T;
template<class T, class F> constexpr bool can_twos_compl_cast(F val) noexcept;
template<class T, class F> constexpr bool try_twos_compl_cast(F from, T& to) noexcept;
template<class T, class F> constexpr auto twos_compl_cast_a(F val) noexcept -> T;
/// \}


/// \brief Sign-extend in two's complement representation.
///
/// This function sign-extends the specified value from a two's complement representation
/// using the specified number of bit (\p from_width) to a two's complement representation
/// using all of the value bits in `T`.
///
/// If \p from_width is greater than the number of value bits in `T`, this function returns
/// the specified value unmodified.
///
/// Sign extension occurs as follows: If the sign bit, i.e., the bit at position `from_width
/// - 1`, is 1, the N most significant bits will also be set. Here, `N` is the width of `T`
/// (\ref core::int_width()) minus \p from_width.
///
/// It is an error if `T` is not an unsigned integer type. It is also an error if \p
/// from_width is less than 1.
///
/// This function makes no assumptions about the platform except that it complies with at
/// least C++17.
///
template<class T> constexpr auto twos_compl_sign_extend(T val, int from_width) noexcept -> T;


/// \{
///
/// \brief Compare heterogeneously typed integer values.
///
/// Compare two integers of the same, or of different type, and produce the expected result
/// according to the natural interpretation of the operation.
///
/// Note that, in general, a standard comparison between a signed and an unsigned integer
/// type is unsafe, and it often generates a compiler warning. An example is a 'less than'
/// comparison between a negative value of type `int` and a small positive value of type
/// `unsigned`. In this case the negative value will be converted to `unsigned` producing a
/// large positive value, which, in turn, will lead to the counter intuitive result of
/// `false`.
///
/// The types of the specified values (\p A and \p B) must conform to the integer concept
/// (\ref Concept_Archon_Core_Integer). They need not be the same type, in particular, one
/// can be signed and the other one can be unsigned.
///
template<class A, class B> constexpr bool int_equal(A, B) noexcept;
template<class A, class B> constexpr bool int_not_equal(A, B) noexcept;
template<class A, class B> constexpr bool int_less(A, B) noexcept;
template<class A, class B> constexpr bool int_less_equal(A, B) noexcept;
template<class A, class B> constexpr bool int_greater(A, B) noexcept;
template<class A, class B> constexpr bool int_greater_equal(A, B) noexcept;
/// \}


/// \{
///
/// \brief Perform arithmetic operations on integer values values while checking for
/// overflow.
///
/// These functions are like \ref core::try_int_add(), \ref core::try_int_sub(), \ref
/// core::try_int_mul(), and \ref core::try_int_pos() respectively, except that they throw
/// `std::overflow_error` on positive or negative overflow instead of returning `false`.
///
template<class L, class R> constexpr void int_add(L& lval, R rval);
template<class L, class R> constexpr void int_sub(L& lval, R rval);
template<class L, class R> constexpr void int_mul(L& lval, R rval);
template<class L, class R> constexpr void int_pow(L& lval, R rval);
/// \}


/// \brief Perform arithmetic left-shift while checking for overflow.
///
/// This function is like \ref core::try_int_arith_shift_left() except that it throws
/// `std::overflow_error` on positive or negative overflow instead of returning a boolean.
///
template<class T> constexpr void int_arith_shift_left(T& lval, int i);


/// \brief Performing logical left-shift by any number of bits.
///
/// This function performs a logical bitwise shift to the left by any number of bit
/// positions. It works on any unsigned integer type. It does not check for overflow, unlike
/// \ref core::int_arith_shift_left(). \p i must be non-negative, but \p i can be greater
/// than, or equal to `std::numeric_limits<T>::digits`.
///
template<class T> constexpr void int_logic_shift_left(T& lval, int i) noexcept;


/// \brief Performing logical right-shift by any number of bits.
///
/// This function performs a logical bitwise shift to the right by any number of bit
/// positions. It works on any unsigned, or signed integer type, but the value must be
/// non-negative. \p i must be non-negative, but \p i can be greater than, or equal to
/// `std::numeric_limits<T>::digits`.
///
template<class T> constexpr void int_logic_shift_right(T& lval, int i) noexcept;


/// \{
///
/// \brief Whether sum, difference, or product of two values is representable in type of
/// first value.
///
/// These functions have the same affect as \ref core::try_int_add(), \ref
/// core::try_int_sub(), or \ref core::try_int_mul() respectively, except that the result of
/// the addition, subtraction, or multiplication is thrown away.
///
/// \sa \ref core::can_int_add(), \ref core::can_int_sub(), \ref core::can_int_mul()
/// \sa \ref core::try_int_add(), \ref core::try_int_sub(), \ref core::try_int_mul()
///
template<class L, class R> constexpr bool can_int_add(L lval, R rval) noexcept;
template<class L, class R> constexpr bool can_int_sub(L lval, R rval) noexcept;
template<class L, class R> constexpr bool can_int_mul(L lval, R rval) noexcept;
/// \}


/// \{
///
/// \brief Try to add, subtract, or multiply heterogeneously typed integer values while
/// checking for overflow.
///
/// These functions check for overflow in integer variable \p lval while adding integer
/// value \p rval to it, while subtracting integer value \p rval from it, or while
/// multiplying it with integer value \p rval. They return `false` on positive or negative
/// overflow. Otherwise, they return `true`. When they return `false`, \p lval will be left
/// unchanged.
///
/// Both types (\p L and \p R) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer). They need not be the same type, in particular, one can be
/// signed and the other one can be unsigned.
///
/// In terms of efficiency, these functions are especially well suited for cases where \p
/// rval is a compile-time constant.
///
/// \sa \ref core::can_int_add(), \ref core::can_int_sub(), \ref core::can_int_mul()
/// \sa \ref core::try_int_add(), \ref core::try_int_sub(), \ref core::try_int_mul()
///
template<class L, class R> constexpr bool try_int_add(L& lval, R rval) noexcept;
template<class L, class R> constexpr bool try_int_sub(L& lval, R rval) noexcept;
template<class L, class R> constexpr bool try_int_mul(L& lval, R rval) noexcept;
/// \}


/// \brief Try to perform exponentiation with heterogeneously typed integers.
///
/// If the mathematical result of the prior value of \p lval raised to the power of \p rval
/// is representable in \p lval, this function returns `true` after setting \p lval to that
/// result. Otherwise this function returns `false` and leaves \p lval unchanged.
///
/// If \p rval is negative, the result is the integer division of 1 by the mathematical
/// result of the prior value of `lval` raised to the power of `-rval`, which is always
/// either 0, +/- 1, or +/- infinity. If it is +/- infinity, this function returns
/// `false`. Otherwise, the result is representable in \p lval, so the function returns
/// `true`. The result is representable in this case because the mathematical result of the
/// division can only be -1 when the prior value of \p lval is -1.
///
template<class L, class R> constexpr bool try_int_pow(L& lval, R rval) noexcept;


/// \brief Try to perform arithmetic left-shift while checking for overflow.
///
/// This function checks for positive overflow while performing an arithmetic bitwise shift
/// to the left on a non-negative value of arbitrary integer type. It returns `false` on
/// overflow. Otherwise, it returns `true`.
///
/// \param lval Must not be negative. Must be of an integer type for which a specialization
/// of `std::numeric_limits` exists. Both signed and unsigned types can be used.
///
/// \param i Must be non-negative. Values larger than, or equal to
/// `std::numeric_limits<T>::digits` is allowed.
///
/// This function makes no assumptions about the platform except that it complies with at
/// least C++17.
///
template<class T> constexpr bool try_int_arith_shift_left(T& lval, int i) noexcept;


/// \brief Integer negation.
///
/// This function returns the negative of the specified integer value, which is always
/// `-val`, however, for values of unsigned type, some compilers will issue a warning when
/// seeing `-val`, and this function is a way to avoid such a warning. For an unsigned type,
/// the result is also equal to `~val + 1`.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr auto int_neg(T val) noexcept -> decltype(+val);


/// \brief Quotient and remainder of integer division.
///
/// This type is equivalent to one defined as follows:
///
/// \code{.cpp}
///
///   template<class T> struct IntDivMod {
///       T quot; // Quotient
///       T rem;  // Remainder
///   };
///
/// \endcode
///
/// \sa \ref core::int_divmod().
///
template<class T> using IntDivMod = typename impl::IntDivMod<T>::type;


/// \brief Perform combined division and modulo operations.
///
/// This function performs a combined division (\p a by \p b) and modulo operation. In some
/// cases, this is more efficient than performing them separately. This operation is
/// functionally equivalent to constructing a `core::IntDivMod<T>` object using `{
/// core::int_cast_a<T>(a / b), core::int_cast_a<T>(a % b) }`.
///
/// The type of the specified values (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> constexpr auto int_divmod(T a, T b) noexcept -> core::IntDivMod<T>;


/// \brief Integer division with upwards rounding.
///
/// This function divides \p a by \p b and returns the least integer that is greater than,
/// or equal to the true untruncated result. Both \p a and \p b must be non-negative
/// integers. If \p b is zero, the effect is the same as it would be for the expression `a /
/// b`.
///
/// The types of the specified values (\p T and \p U) must conform to the integer concept
/// (\ref Concept_Archon_Core_Integer).
///
template<class T, class U> constexpr auto int_div_round_up(T a, U b) noexcept -> T;


/// \brief Integer division with half-down rounding behavior.
///
/// This function divides \p a by \p b and returns the result rounded to the nearest
/// integer, or if the result is half way between two integers, rounded down. Both \p a and
/// \p b must be non-negative integers. If \p b is zero, the effect is the same as it would
/// be for the expression `a / b`.
///
/// The types of the specified values (\p T and \p U) must conform to the integer concept
/// (\ref Concept_Archon_Core_Integer).
///
template<class T, class U> constexpr auto int_div_round_half_down(T a, U b) noexcept -> T;


/// \brief Performs periodic modulo operation.
///
/// This function performs the periodic modulo operation, which means that it computes the
/// result of `a - floor(a / b) * b` where each operation should be thought of as occurring
/// in the reals (R). So long as \p b is no zero, the result is always an integer, and is
/// always representable in `U`. This function works correctly for all combinations of
/// integer types (\p T and \p U) and for all possible values of those integer
/// types. Behavior is undefined if \p b is zero.
///
/// The types of the specified values (\p T and \p U) must conform to the integer concept
/// (\ref Concept_Archon_Core_Integer).
///
/// Note that the periodic modulo operation is a periodic function in the first argument.
///
/// When \p T and \p U are the same type, \p a is non-negative, \p b is positive, and the
/// result of `a / b` is representable in `T`, this function is identical to the standard
/// modulo operation (`%`). Note that the standard modulo operation is not a periodic
/// function in the first argument, but is an odd function in the first argument.
///
/// FIXME: Should introduce heterogeneous `core::int_odd_mod(T a, U b) -> T` as a safe
/// version of `%`, which correctly handles the case where `a / b` is not representable in
/// `decltype(a / b)`. Such a function can probably be implemented in terms of homogeneous
/// `impl::int_odd_mod()` (see below). Once such a function is introduced, it might be
/// necessary to go through all uses of the `%` operator, and see whether they should be
/// replaced with `core::int_odd_mod()`.                                                      
///
template<class T, class U> constexpr auto int_periodic_mod(T a, U b) noexcept -> U;


/// \brief Integer square root.
///
/// This function computes the integer square root of the specified value. The specified
/// value must be a non-negative integer.
///
/// The type of the specified value (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
/// The current implementation uses Newton's method with a start guess based on the base-2
/// logarithm (see https://en.wikipedia.org/wiki/Integer_square_root).
///
template<class T> constexpr auto int_sqrt(T val) noexcept -> T;


/// \brief Copy integer bits.
///
/// Both types (\p T and \p U) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<bool sign_extend, class T, class U, std::size_t N, std::size_t M>
constexpr void int_bit_copy(const std::array<T, N>& parts_1, std::array<U, M>& parts_2) noexcept;








// Implementation


namespace impl {


template<class S, class U> struct CommonIntType1 {
    static_assert(core::is_signed<S>());
    static_assert(!core::is_signed<U>());
    using type = std::conditional_t<core::num_value_bits<S>() >= core::num_value_bits<U>(), S, U>;
};

template<class, class, bool, bool> struct CommonIntType2;

// Unsigned vs unsigned
template<class T, class U> struct CommonIntType2<T, U, false, false> {
    using type = std::conditional_t<core::num_value_bits<T>() >= core::num_value_bits<U>(), T, U>;
};

// Signed vs signed
template<class T, class U> struct CommonIntType2<T, U, true, true> {
    static constexpr bool cond = (core::num_value_bits<T>() > core::num_value_bits<U>() ||
                                  core::int_cast_a<U>(core::int_min<T>() + T(1)) <=
                                  core::int_min<U>() + U(1));
    using type = std::conditional_t<cond, T, U>;
};

// Unsigned vs signed
template<class T, class U> struct CommonIntType2<T, U, false, true>
    : impl::CommonIntType1<U, T> {
};

// Signed vs unsigned
template<class T, class U> struct CommonIntType2<T, U, true, false>
    : impl::CommonIntType1<T, U> {
};

template<class T, class U> struct CommonIntType3 {
    using type = typename impl::CommonIntType2<T, U, core::is_signed<T>(), core::is_signed<U>()>::type;
};

template<class T> struct CommonIntType<T> {
    using type = T;
};

template<class T, class U, class... V> struct CommonIntType<T, U, V...> {
    using type = typename impl::CommonIntType3<T, typename CommonIntType<U, V...>::type>::type;
};


} // namespace impl


template<class T> constexpr bool is_integer() noexcept
{
    using traits_type = core::IntegerTraits<T>;
    return traits_type::is_specialized;
}


template<class T> constexpr auto promote(T val) noexcept -> core::promoted_type<T>
{
    static_assert(core::is_integer<T>());
    return +val;
}


template<class T> constexpr auto promote_strongly(T val) noexcept -> core::strongly_promoted_type<T>
{
    using type = core::strongly_promoted_type<T>;
    return core::int_cast_a<type>(val);
}


template<class T> constexpr auto to_unsigned(T val) noexcept -> core::unsigned_type<T>
{
    static_assert(core::is_integer<T>());
    using unsigned_type = core::unsigned_type<T>;
    return core::int_cast_a<unsigned_type>(val);
}


template<class T> constexpr int num_value_bits() noexcept
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    return traits_type::num_value_bits;
}


template<class T> constexpr bool is_signed() noexcept
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    return traits_type::is_signed;
}


template<class T> constexpr bool is_unsigned() noexcept
{
    return !core::is_signed<T>();
}


template<class T> constexpr int int_width() noexcept
{
    static_assert(int(core::is_signed<T>()) <= std::numeric_limits<int>::max() - core::num_value_bits<T>());
    return core::num_value_bits<T>() + int(core::is_signed<T>());
}


template<class T> constexpr int int_inner_width() noexcept
{
    int n = core::num_value_bits<T>();
    if constexpr (!core::is_unsigned<T>()) {
        bool include_sign_bit = (core::int_min<T>() + core::int_max<T>() < 0);
        n += int(include_sign_bit);
    }
    return n;
}


template<class T> constexpr auto int_min() noexcept -> T
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    return traits_type::min();
}


template<class T> constexpr auto int_max() noexcept -> T
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    return traits_type::max();
}


template<class T> constexpr bool int_is_power_of_two(T val) noexcept
{
    static_assert(core::is_integer<T>());
    auto val_2 = core::promote(val);
    using type = decltype(val_2);
    return ((val_2 > type(0)) && ((val_2 & (val_2 - 1)) == type(0)));
}


template<class T> constexpr int int_find_msb_pos(T val) noexcept
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    if constexpr (traits_type::has_find_msb_pos) {
        return traits_type::find_msb_pos(val);
    }
    else {
        if (ARCHON_LIKELY(!core::is_negative(val))) {
            auto v = core::to_unsigned(core::promote(val));
            using type = decltype(v);
            int i = 0;
            int j = core::num_value_bits<T>();
            for (;;) {
                int n = (j - i) / 2;
                if (ARCHON_LIKELY(n > 0)) {
                    type w = v >> n;
                    if (ARCHON_LIKELY(w != type(0))) {
                        i += n;
                        v = w;
                    }
                    else {
                        j = i + n;
                    }
                    continue;
                }
                return (v != type(0) ? i : i - 1);
            }
        }
        return core::num_value_bits<T>();
    }
}


template<class T> constexpr int int_find_msd_pos(T val, int base) noexcept
{
    static_assert(core::is_integer<T>());
    auto val_2 = core::promote_strongly(val);
    using type = decltype(val_2);
    if (ARCHON_LIKELY(type(base) <= core::int_max<type>())) {
        int i = -1;
        while (val_2 != type(0)) {
            val_2 /= type(base);
            ++i;
        }
        return i;
    }
    return (val_2 != type(0) ? 0 : -1);
}


template<class T> constexpr int int_num_digits(T val, int base) noexcept
{
    return 1 + core::int_find_msd_pos(val, base);
}


template<class T> constexpr int int_max_digits(int base) noexcept
{
    return 1 + std::max(core::int_find_msd_pos(core::int_min<T>(), base),
                        core::int_find_msd_pos(core::int_max<T>(), base));
}


template<class T> constexpr bool is_zero(T val) noexcept
{
    return (val == T(0));
}


template<class T> constexpr bool is_negative(T val) noexcept
{
    if constexpr (core::is_signed<T>()) {
        return (val < T(0));
    }
    else {
        static_cast<void>(val);
        return false;
    }
}


template<class T> constexpr bool int_is_even(T val) noexcept
{
    auto val_2 = core::promote_strongly(val);
    using type = decltype(val_2);
    return core::is_zero(val_2 & type(1));
}


template<class T> constexpr bool int_is_odd(T val) noexcept
{
    return !core::int_is_even(val);
}


template<class T> constexpr bool int_is_min(T val) noexcept
{
    return (val == core::int_min<T>());
}


template<class T> constexpr bool int_is_max(T val) noexcept
{
    return (val == core::int_max<T>());
}


template<class T> constexpr auto int_mask(int num_bits) noexcept -> T
{
    static_assert(core::is_integer<T>());
    if (ARCHON_LIKELY(num_bits > 0)) {
        int num_extra_bits = 0;
        if (ARCHON_LIKELY(num_bits <= core::num_value_bits<T>()))
            num_extra_bits = core::num_value_bits<T>() - num_bits;
        return core::int_cast_a<T>(core::int_max<T>() >> num_extra_bits);
    }
    return T(0);
}


template<class T, class F> constexpr auto int_cast(F val) -> T
{
    T val_2 = T();
    if (ARCHON_LIKELY(core::try_int_cast<T>(val, val_2)))
        return val_2;
    throw std::overflow_error("Integer cast");
}


template<class T, class F> constexpr void int_cast(F from, T& to)
{
    to = core::int_cast<T>(from); // Throws
}


template<class T, class F> constexpr bool can_int_cast(F val) noexcept
{
    return (core::int_greater_equal(val, core::int_min<T>()) && core::int_less_equal(val, core::int_max<T>()));
}


template<class T, class F> constexpr bool try_int_cast(F from, T& to) noexcept
{
    if (ARCHON_LIKELY(core::can_int_cast<T>(from))) {
        to = core::int_cast_a<T>(from);
        return true;
    }
    return false;
}


template<class T, class F> constexpr auto int_cast_a(F val) noexcept -> T
{
    static_assert(core::is_integer<F>());
    static_assert(core::is_integer<T>());
    using traits_type_1 = core::IntegerTraits<F>;
    using traits_type_2 = core::IntegerTraits<T>;
    if constexpr (std::is_same_v<F, T>) {
        return val;
    }
    else if constexpr (std::is_integral_v<F> && std::is_integral_v<T>) {
        if constexpr (std::is_same_v<T, bool>) {
            return bool(unsigned(val) & 1);
        }
        else {
            return T(val);
        }
    }
    else {
        using part_type_1 = typename traits_type_1::part_type;
        using part_type_2 = typename traits_type_2::part_type;
        constexpr int num_parts_1 = traits_type_1::num_parts;
        constexpr int num_parts_2 = traits_type_2::num_parts;
        using parts_type_1 = std::array<part_type_1, num_parts_1>;
        using parts_type_2 = std::array<part_type_2, num_parts_2>;
        parts_type_1 parts_1 = traits_type_1::get_parts(val);
        parts_type_2 parts_2 = {};
        core::int_bit_copy<traits_type_1::is_signed>(parts_1, parts_2);
        return traits_type_2::from_parts(parts_2);
    }
}


template<class T, class F> constexpr auto int_cast_clamp(F val, T min, T max) noexcept -> T
{
    if (ARCHON_LIKELY(core::int_greater_equal(val, min))) {
        if (ARCHON_LIKELY(core::int_less_equal(val, max)))
            return core::int_cast_a<T>(val);
        return max;
    }
    return min;
}


template<class T, class F> constexpr void int_cast_clamp(F from, T& to, T min, T max) noexcept
{
    to = core::int_cast_clamp<T>(from, min, max);
}


template<class T, class F> constexpr void int_cast_clamp(F from, T& to) noexcept
{
    core::int_cast_clamp(from, to, core::int_min<T>(), core::int_max<T>());
}


template<class T, class F> constexpr auto cast_to_twos_compl(F val) -> T
{
    T val_2 = {};
    if (ARCHON_LIKELY(core::try_cast_to_twos_compl<T>(val, val_2)))
        return val_2;
    throw std::overflow_error("Cast to two's complement");
}


template<class T, class F> constexpr bool can_cast_to_twos_compl(F val) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if constexpr (std::is_unsigned_v<T>) {
        static_assert(T(~lim_t::max()) == 0);
        auto max = core::to_unsigned(core::promote(lim_t::max()) >> 1);
        if constexpr (std::is_unsigned_v<F>) {
            return (val <= max);
        }
        else {
            if (ARCHON_LIKELY(val >= 0)) {
                // Non-negative value
                return (core::to_unsigned(core::promote(val)) <= max);
            }
            // Negative value
            return (core::to_unsigned(-1 - val) <= max);
        }
    }
    return core::can_int_cast<T>(val);
}


template<class T, class F> constexpr bool try_cast_to_twos_compl(F from, T& to) noexcept
{
    if (ARCHON_LIKELY(core::can_cast_to_twos_compl<T>(from))) {
        to = T(from);
        return true;
    }
    return false;
}


template<class T, class F> constexpr auto cast_to_twos_compl_a(F val) noexcept -> T
{
    return core::int_cast_a<T>(val);
}


template<class T, class F> constexpr auto cast_from_twos_compl(F val) -> T
{
    T val_2 = {};
    if (ARCHON_LIKELY(core::try_cast_from_twos_compl<T>(val, val_2)))
        return val_2;
    throw std::overflow_error("Cast from two's complement");
}


template<class T, class F> constexpr bool can_cast_from_twos_compl(F val) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if constexpr (std::is_unsigned_v<F>) {
        auto sign_bit = core::promote(val) & (core::promote(F(1)) << (core::int_width<F>() - 1));
        bool nonnegative = (sign_bit == 0);
        if constexpr (std::is_unsigned_v<T>) {
            return (nonnegative && val <= lim_t::max());
        }
        else {
            if (ARCHON_LIKELY(nonnegative)) {
                // Non-negative value
                return (val <= core::to_unsigned(core::promote(lim_t::max())));
            }
            // Negative value
            auto val_2 = core::promote(F(-1)) - core::promote(val);
            auto max = core::promote(T(-1)) - core::promote(lim_t::min());
            return (core::to_unsigned(val_2) <= core::to_unsigned(max));
        }
    }
    return core::can_int_cast<T>(val);
}


template<class T, class F> constexpr bool try_cast_from_twos_compl(F from, T& to) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if (std::is_unsigned_v<F>) {
        auto sign_bit = core::promote(from) & (core::promote(F(1)) << (core::int_width<F>() - 1));
        bool nonnegative = (sign_bit == 0);
        if constexpr (std::is_unsigned_v<T>) {
            if (ARCHON_LIKELY(nonnegative && from <= lim_t::max())) {
                to = T(from);
                return true;
            }
            return false;
        }
        else {
            if (ARCHON_LIKELY(nonnegative)) {
                // Non-negative value
                if (ARCHON_LIKELY(from <= core::to_unsigned(core::promote(lim_t::max())))) {
                    to = T(from);
                    return true;
                }
                return false;
            }
            // Negative value
            auto val = core::promote(F(-1)) - core::promote(from);
            auto max = core::promote(T(-1)) - core::promote(lim_t::min());
            if (ARCHON_LIKELY(core::to_unsigned(val) <= core::to_unsigned(max))) {
                using signed_type = core::promoted_type<T>;
                to = T(-1 - signed_type(val));
                return true;
            }
            return false;
        }
    }
    return core::try_int_cast(from, to);
}


template<class T, class F> constexpr auto cast_from_twos_compl_a(F val) noexcept -> T
{
    if constexpr (!core::is_signed<F>() && core::is_signed<T>()) {
        if constexpr (!std::is_integral_v<F> && std::is_integral_v<T>) {
            auto sign_bit = core::promote(val) & (core::promote(F(1)) << (core::int_width<F>() - 1));
            bool is_negative = !core::is_zero(sign_bit);
            if (ARCHON_LIKELY(!is_negative)) {
                // Non-negative value
                return T(val);
            }
            // Negative value
            auto val_2 = core::promote(F(-1)) - core::promote(val);
            using signed_type = core::promoted_type<T>;
            return T(-1 - signed_type(val_2));
        }
        else {
            using traits_type_1 = core::IntegerTraits<F>;
            using traits_type_2 = core::IntegerTraits<T>;
            using part_type_1 = typename traits_type_1::part_type;
            using part_type_2 = typename traits_type_2::part_type;
            constexpr int num_parts_1 = traits_type_1::num_parts;
            constexpr int num_parts_2 = traits_type_2::num_parts;
            using parts_type_1 = std::array<part_type_1, num_parts_1>;
            using parts_type_2 = std::array<part_type_2, num_parts_2>;
            parts_type_1 parts_1 = traits_type_1::get_parts(val);
            parts_type_2 parts_2 = {};
            // Sign extend
            constexpr int width = core::int_width<F>();
            constexpr int part_width = core::int_width<part_type_1>();
            constexpr int sign_bit_pos = (width - 1) % part_width;
            constexpr auto ext = ~core::int_mask<part_type_1>(sign_bit_pos + 1);
            auto sign_bit = parts_1[num_parts_1 - 1] >> sign_bit_pos;
            parts_1[num_parts_1 - 1] |= sign_bit * ext;
            constexpr bool from_signed = true;
            core::int_bit_copy<from_signed>(parts_1, parts_2);
            return traits_type_2::from_parts(parts_2);
        }
    }
    else {
        return core::int_cast_a<T>(val);
    }
}


template<class T, class F> constexpr auto twos_compl_cast(F val) -> T
{
    T val_2 = {};
    if (ARCHON_LIKELY(core::try_twos_compl_cast<T>(val, val_2)))
        return val_2;
    throw std::overflow_error("Two's complement cast");
}


template<class T, class F> constexpr bool can_twos_compl_cast(F val) noexcept
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = core::int_width<T>();
        constexpr int width_f = core::int_width<F>();
        if constexpr (width_t >= width_f) {
            return true;
        }
        else {
            static_assert(width_t < width_f);
            auto sign_bit = core::promote(val) & (core::promote(F(1)) << (width_t - 1));
            auto expected_sign_extension = ~((sign_bit << 1) - 1);
            auto actual_sign_extension = core::promote(val) & (core::promote(F(-1)) << width_t);
            return (actual_sign_extension == expected_sign_extension);
        }
    }
    if constexpr (std::is_unsigned_v<T>)
        return core::can_cast_to_twos_compl<T>(val);
    if constexpr (std::is_unsigned_v<F>)
        return core::can_cast_from_twos_compl<T>(val);
    return core::can_int_cast<T>(val);
}


template<class T, class F> constexpr bool try_twos_compl_cast(F from, T& to) noexcept
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = core::int_width<T>();
        constexpr int width_f = core::int_width<F>();
        if constexpr (width_t >= width_f) {
            to = core::twos_compl_sign_extend(T(from), width_f);
            return true;
        }
        else {
            static_assert(width_t < width_f);
            auto sign_bit = core::promote(from) & (core::promote(F(1)) << (width_t - 1));
            auto expected_sign_extension = ~((sign_bit << 1) - 1);
            auto actual_sign_extension = core::promote(from) & (core::promote(F(-1)) << width_t);
            if (ARCHON_LIKELY(actual_sign_extension == expected_sign_extension)) {
                to = T(from);
                return true;
            }
            return false;
        }
    }
    if constexpr (std::is_unsigned_v<T>)
        return core::try_cast_to_twos_compl(from, to);
    if constexpr (std::is_unsigned_v<F>)
        return core::try_cast_from_twos_compl(from, to);
    return core::try_int_cast(from, to);
}


template<class T, class F> constexpr auto twos_compl_cast_a(F val) noexcept -> T
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = core::int_width<T>();
        constexpr int width_f = core::int_width<F>();
        if constexpr (width_t >= width_f)
            return core::twos_compl_sign_extend(T(val), width_f);
        return T(val);
    }
    if constexpr (std::is_unsigned_v<T>)
        return core::cast_to_twos_compl_a<T>(val);
    if constexpr (std::is_unsigned_v<F>)
        return core::cast_from_twos_compl_a<T>(val);
    return core::int_cast_a<T>(val);
}


template<class T> constexpr auto twos_compl_sign_extend(T val, int from_width) noexcept -> T
{
    static_assert(std::is_unsigned_v<T>);
    ARCHON_ASSERT(from_width > 0);
    if (core::int_width<T>() > from_width) {
        auto sign_bit = core::promote(val) & (core::promote(T(1)) << (from_width - 1));
        auto sign_extension = ~((sign_bit << 1) - 1);
        return T(sign_extension | core::promote(val));
    }
    return val;
}


template<class A, class B> constexpr bool int_equal(A a, B b) noexcept
{
    using type = core::promoted_type<core::common_int_type<A, B>>;
    if constexpr (!core::is_signed<A>()) {
        if constexpr (!core::is_signed<B>()) {
            // Unsigned vs unsigned
            return (core::int_cast_a<type>(a) == core::int_cast_a<type>(b));
        }
        else {
            // Unsigned vs signed
            return (!core::is_negative(b) && core::int_cast_a<type>(a) == core::int_cast_a<type>(b));
        }
    }
    else {
        if constexpr (!core::is_signed<B>()) {
            // Signed vs unsigned
            return (!core::is_negative(a) && core::int_cast_a<type>(a) == core::int_cast_a<type>(b));
        }
        else {
            // Signed vs signed
            return (core::int_cast_a<type>(a) == core::int_cast_a<type>(b));
        }
    }
}


template<class A, class B> constexpr bool int_not_equal(A a, B b) noexcept
{
    return !core::int_equal(a, b); // Not equal
}


template<class A, class B> constexpr bool int_less(A a, B b) noexcept
{
    using type = core::promoted_type<core::common_int_type<A, B>>;
    if constexpr (!core::is_signed<A>()) {
        if constexpr (!core::is_signed<B>()) {
            // Unsigned vs unsigned
            return (core::int_cast_a<type>(a) < core::int_cast_a<type>(b));
        }
        else {
            // Unsigned vs signed
            return (!core::is_negative(b) &&
                    core::int_cast_a<type>(a) < core::int_cast_a<type>(b));
        }
    }
    else {
        if constexpr (!core::is_signed<B>()) {
            // Signed vs unsigned
            return (core::is_negative(a) ||
                    core::int_cast_a<type>(a) < core::int_cast_a<type>(b));
        }
        else {
            // Signed vs signed
            return (core::int_cast_a<type>(a) < core::int_cast_a<type>(b));
        }
    }
}


template<class A, class B> constexpr bool int_less_equal(A a, B b) noexcept
{
    return !core::int_less(b, a); // Not greater than
}


template<class A, class B> constexpr bool int_greater(A a, B b) noexcept
{
    return core::int_less(b, a);
}


template<class A, class B> constexpr bool int_greater_equal(A a, B b) noexcept
{
    return !core::int_less(a, b); // Not less than
}


template<class L, class R> constexpr void int_add(L& lval, R rval)
{
    if (ARCHON_LIKELY(core::try_int_add(lval, rval)))
        return;
    throw std::overflow_error("Integer addition");
}


template<class L, class R> constexpr void int_sub(L& lval, R rval)
{
    if (ARCHON_LIKELY(core::try_int_sub(lval, rval)))
        return;
    throw std::overflow_error("Integer subtraction");
}


template<class L, class R> constexpr void int_mul(L& lval, R rval)
{
    if (ARCHON_LIKELY(core::try_int_mul(lval, rval)))
        return;
    throw std::overflow_error("Integer multiplication");
}


template<class L, class R> constexpr void int_pow(L& lval, R rval)
{
    if (ARCHON_LIKELY(core::try_int_pow(lval, rval)))
        return;
    throw std::overflow_error("Integer exponentiation");
}


template<class T> constexpr void int_arith_shift_left(T& lval, int i)
{
    if (ARCHON_LIKELY(core::try_int_arith_shift_left(lval, i)))
        return;
    throw std::overflow_error("Arithmetic left-shift");
}


template<class T> constexpr void int_logic_shift_left(T& lval, int i) noexcept
{
    static_assert(std::is_unsigned_v<T>);
    ARCHON_ASSERT(i >= 0);
    if (ARCHON_LIKELY(i < std::numeric_limits<core::promoted_type<T>>::digits)) {
        lval <<= i;
    }
    else {
        lval = 0;
    }
}


template<class T> constexpr void int_logic_shift_right(T& lval, int i) noexcept
{
    ARCHON_ASSERT(core::int_greater_equal(lval, 0));
    ARCHON_ASSERT(i >= 0);
    if (ARCHON_LIKELY(i < std::numeric_limits<core::promoted_type<T>>::digits)) {
        lval >>= i;
    }
    else {
        lval = 0;
    }
}


template<class L, class R> constexpr bool can_int_add(L lval, R rval) noexcept
{
    return core::try_int_add(lval, rval);
}


template<class L, class R> constexpr bool can_int_sub(L lval, R rval) noexcept
{
    return core::try_int_sub(lval, rval);
}


template<class L, class R> constexpr bool can_int_mul(L lval, R rval) noexcept
{
    return core::try_int_mul(lval, rval);
}


template<class L, class R> constexpr bool try_int_add(L& lval, R rval) noexcept
{
    if constexpr (!core::is_signed<L>()) {
        if constexpr (!core::is_signed<R>()) {
            // Case: Unsigned plus unsigned
            using type = core::common_int_type<unsigned, L, R>;
            static_assert(!core::is_signed<core::promoted_type<type>>());
            type lval_2 = core::int_cast_a<type>(lval);
            type rval_2 = core::int_cast_a<type>(rval);
            bool overflow = (rval_2 > core::int_cast_a<type>(core::int_max<L>()) - lval_2);
            if (ARCHON_LIKELY(!overflow)) {
                lval = core::int_cast_a<L>(lval_2 + rval_2);
                return true;
            }
            return false;
        }
        else {
            // Case: Unsigned plus signed
            if (core::is_negative(rval)) {
                using type = core::common_int_type<unsigned, L, core::unsigned_type<R>>;
                static_assert(!core::is_signed<core::promoted_type<type>>());
                type lval_2 = core::int_cast_a<type>(lval);
                bool overflow = (core::int_cast_a<type>(R(-1) - rval) >= lval_2);
                if (ARCHON_LIKELY(!overflow)) {
                    lval = core::int_cast_a<L>(lval_2 + core::int_cast_a<type>(rval));
                    return true;
                }
                return false;
            }
            return try_int_add(lval, core::to_unsigned(rval));
        }
    }
    else {
        if constexpr (!core::is_signed<R>()) {
            // Case: Signed plus unsigned
            if constexpr (core::num_value_bits<L>() >= core::num_value_bits<R>()) {
                return try_int_add(lval, core::int_cast_a<L>(rval));
            }
            else {
                using type = core::common_int_type<unsigned, R>;
                static_assert(!core::is_signed<core::promoted_type<type>>());
                type lval_2 = core::int_cast_a<type>(lval);
                type rval_2 = core::int_cast_a<type>(rval);
                type max_add = core::int_cast_a<type>(core::int_max<L>()) - lval_2;
                bool overflow = (rval_2 > max_add);
                if (ARCHON_LIKELY(!overflow)) {
                    lval = core::cast_from_twos_compl_a<L>(lval_2 + rval_2);
                    return true;
                }
                return false;
            }
        }
        else {
            // Case: Signed plus signed
            using type = core::common_int_type<L, R>;
            type lval_2 = core::int_cast_a<type>(lval);
            type rval_2 = core::int_cast_a<type>(rval);
            // Note that both subtractions below occur in the widest of the two signed
            // types. Note also that any signed type guarantees that there is no overflow
            // when subtracting two negative values or two non-negative value.
            bool overflow = (core::is_negative(rval_2) ?
                             lval_2 < core::int_cast_a<type>(core::int_min<L>()) - rval_2 :
                             lval_2 > core::int_cast_a<type>(core::int_max<L>()) - rval_2);
            if (ARCHON_LIKELY(!overflow)) {
                lval = core::int_cast_a<L>(lval_2 + rval_2);
                return true;
            }
            return false;
        }
    }
}


template<class L, class R> constexpr bool try_int_sub(L& lval, R rval) noexcept
{
    if constexpr (!core::is_signed<L>()) {
        if constexpr (!core::is_signed<R>()) {
            // Case: Unsigned minus unsigned
            using type = core::common_int_type<unsigned, L, R>;
            static_assert(!core::is_signed<core::promoted_type<type>>());
            type lval_2 = core::int_cast_a<type>(lval);
            type rval_2 = core::int_cast_a<type>(rval);
            bool overflow = (rval_2 > lval_2);
            if (ARCHON_LIKELY(!overflow)) {
                lval = core::int_cast_a<L>(lval_2 - rval_2);
                return true;
            }
            return false;
        }
        else {
            // Case: Unsigned minus signed
            if (core::is_negative(rval)) {
                using type = core::common_int_type<unsigned, L, core::unsigned_type<R>>;
                static_assert(!core::is_signed<core::promoted_type<type>>());
                type lval_2 = core::int_cast_a<type>(lval);
                bool overflow = (core::int_cast_a<type>(R(-1) - rval) >=
                                 core::int_cast_a<type>(core::int_max<L>()) - lval_2);
                if (ARCHON_LIKELY(!overflow)) {
                    lval = core::int_cast_a<L>(lval_2 - core::int_cast_a<type>(rval));
                    return true;
                }
                return false;
            }
            return try_int_sub(lval, core::to_unsigned(rval));
        }
    }
    else {
        if constexpr (!core::is_signed<R>()) {
            // Case: Signed minus unsigned
            if constexpr (core::num_value_bits<L>() >= core::num_value_bits<R>()) {
                return try_int_sub(lval, core::int_cast_a<L>(rval));
            }
            else {
                using type = core::common_int_type<unsigned, R>;
                static_assert(!core::is_signed<core::promoted_type<type>>());
                type lval_2 = core::int_cast_a<type>(lval);
                type rval_2 = core::int_cast_a<type>(rval);
                type max_sub = lval_2 - core::int_cast_a<type>(core::int_min<L>());
                bool overflow = (rval_2 > max_sub);
                if (ARCHON_LIKELY(!overflow)) {
                    lval = core::cast_from_twos_compl_a<L>(lval_2 - rval_2);
                    return true;
                }
                return false;
            }
        }
        else {
            // Case: Signed minus signed
            using type = core::common_int_type<L, R>;
            type lval_2 = core::int_cast_a<type>(lval);
            type rval_2 = core::int_cast_a<type>(rval);
            // Note that both additions below occur in the widest of the two signed
            // types. Note also that any signed type guarantees that there is no overflow
            // when adding a non-negative and a negative value.
            bool overflow = (core::is_negative(rval_2) ?
                             lval_2 > core::int_cast_a<type>(core::int_max<L>()) + rval_2 :
                             lval_2 < core::int_cast_a<type>(core::int_min<L>()) + rval_2);
            if (ARCHON_LIKELY(!overflow)) {
                lval = core::int_cast_a<L>(lval_2 - rval_2);
                return true;
            }
            return false;
        }
    }
}


template<class L, class R> constexpr bool try_int_mul(L& lval, R rval) noexcept
{
    static_assert(core::is_integer<L>());
    static_assert(core::is_integer<R>());
    if (ARCHON_LIKELY(!core::is_zero(rval))) {
        if (ARCHON_LIKELY(!core::is_negative(rval))) {
            if (ARCHON_LIKELY(!core::is_negative(lval))) {
                // Case: Non-negative times positive
                using type = core::common_int_type<int, L, R>;
                type lval_2 = core::int_cast_a<type>(lval);
                type rval_2 = core::int_cast_a<type>(rval);
                type max = core::int_cast_a<type>(core::int_max<L>()) / rval_2;
                if (ARCHON_LIKELY(lval_2 <= max)) {
                    lval = core::int_cast_a<L>(lval_2 * rval_2);
                    return true;
                }
            }
            else {
                // Case: Negative times positive
                using type = core::common_int_type<int, L, R>;
                type rval_2 = core::int_cast_a<type>(rval);
                if constexpr (core::is_signed<R>()) {
                    type lval_2 = core::int_cast_a<type>(lval);
                    type min = core::int_cast_a<type>(core::int_min<L>()) / rval_2;
                    if (ARCHON_LIKELY(lval_2 >= min)) {
                        lval = core::int_cast_a<L>(lval_2 * rval_2);
                        return true;
                    }
                }
                else {
                    if (ARCHON_LIKELY(rval_2 <= core::int_cast_a<type>(core::int_max<L>()))) {
                        type lval_2 = core::int_cast_a<type>(lval);
                        type min = core::int_cast_a<type>(core::promote(core::int_min<L>()) /
                                                          core::promote(core::int_cast_a<L>(rval)));
                        if (ARCHON_LIKELY(lval_2 >= min)) {
                            lval = core::int_cast_a<L>(lval_2 * rval_2);
                            return true;
                        }
                    }
                    else if (ARCHON_LIKELY(rval_2 - type(1) <= core::int_cast_a<type>(L(-1) - core::int_min<L>()))) {
                        if (ARCHON_LIKELY(lval == L(-1))) {
                            lval = core::int_min<L>();
                            return true;
                        }
                    }
                }
            }
        }
        else {
            if (ARCHON_LIKELY(!core::is_negative(lval))) {
                // Case: Non-negative times negative
                if constexpr (core::is_signed<L>()) {
                    if (ARCHON_LIKELY(rval != R(-1))) {
                        using type = core::common_int_type<int, L, R>;
                        type lval_2 = core::int_cast_a<type>(lval);
                        type rval_2 = core::int_cast_a<type>(rval);
                        type max = core::int_cast_a<type>(core::int_min<L>()) / rval_2;
                        if (ARCHON_LIKELY(lval_2 <= max)) {
                            lval = core::int_cast_a<L>(lval_2 * rval_2);
                            return true;
                        }
                    }
                    else {
                        lval = core::int_cast_a<L>(-lval);
                        return true;
                    }
                }
                else {
                    if (ARCHON_LIKELY(core::is_zero(lval))) {
                        return true;
                    }
                }
            }
            else {
                // Case: Negative times negative
                using type = core::common_int_type<int, L, R>;
                type lval_2 = core::int_cast_a<type>(lval);
                type rval_2 = core::int_cast_a<type>(rval);
                type min = core::int_cast_a<type>(core::int_max<L>()) / rval_2;
                if (ARCHON_LIKELY(lval_2 >= min)) {
                    lval = core::int_cast_a<L>(lval_2 * rval_2);
                    return true;
                }
            }
        }
        return false;
    }
    lval = L(0);
    return true;
}


template<class L, class R> constexpr bool try_int_pow(L& lval, R rval) noexcept
{
    auto base = core::promote_strongly(lval);
    auto exp  = core::promote_strongly(rval);
    using base_type = decltype(base);
    base_type result = base_type(1);
    if (ARCHON_LIKELY(!core::is_negative(exp))) {
        for (;;) {
            if (core::int_is_odd(exp)) {
                if (ARCHON_UNLIKELY(!core::try_int_mul(result, base)))
                    return false;
            }
            exp >>= 1;
            if (ARCHON_UNLIKELY(core::is_zero(exp)))
                break;
            if (ARCHON_UNLIKELY(!core::try_int_mul(base, base)))
                return false;
        }
    }
    else {
        if (ARCHON_UNLIKELY(core::is_zero(base)))
            return false;
        if (ARCHON_UNLIKELY(core::int_equal(base, -1) && core::int_is_even(exp)))
            base = 1;
        result /= base;
    }
    return core::try_int_cast<L>(result, lval);
}


template<class T> constexpr bool try_int_arith_shift_left(T& lval, int i) noexcept
{
    using lim = std::numeric_limits<T>;
    static_assert(lim::is_specialized);
    static_assert(lim::is_integer);
    ARCHON_ASSERT(core::int_greater_equal(lval, 0));
    ARCHON_ASSERT(i >= 0);
    if (ARCHON_LIKELY(i < std::numeric_limits<core::promoted_type<T>>::digits)) {
        if (ARCHON_LIKELY((lim::max() >> i) >= lval)) {
            lval <<= i;
            return true;
        }
        return false;
    }
    return (lval == 0);
}


template<class T> constexpr auto int_neg(T val) noexcept -> decltype(+val)
{
    if constexpr (core::is_signed<T>()) {
        return -val;
    }
    else {
        return ~val + core::promote(T(1));
    }
}


namespace impl {


template<class T> struct IntFallbackDivMod {
    T quot;
    T rem;
};


template<class T> struct IntDivMod {
    using type = std::conditional_t<core::IntegerTraits<T>::has_divmod,
                                    typename core::IntegerTraits<T>::DivMod, IntFallbackDivMod<T>>;
};


} // namespace impl


template<class T> constexpr auto int_divmod(T a, T b) noexcept -> core::IntDivMod<T>
{
    static_assert(core::is_integer<T>());
    using traits_type = core::IntegerTraits<T>;
    if constexpr (core::IntegerTraits<T>::has_divmod) {
        return traits_type::divmod(a, b);
    }
    else {
        auto a_2 = core::promote_strongly(a);
        auto b_2 = core::promote_strongly(b);
        auto quot = a_2 / b_2;
        auto rem  = a_2 % b_2;
        return { core::int_cast_a<T>(quot), core::int_cast<T>(rem) };
    }
}


template<class T, class U> constexpr auto int_div_round_up(T a, U b) noexcept -> T
{
    ARCHON_ASSERT(!core::is_negative(a));
    ARCHON_ASSERT(!core::is_negative(b));
    using type = core::promoted_type<core::common_int_type<T, U>>;
    type a_2 = core::int_cast_a<type>(a);
    type b_2 = core::int_cast_a<type>(b);
    core::IntDivMod<type> res = core::int_divmod(a_2, b_2);
    if (ARCHON_LIKELY(res.rem != type(0)))
        return core::int_cast_a<T>(res.quot + type(1));
    return core::int_cast_a<T>(res.quot);
}


template<class T, class U> constexpr auto int_div_round_half_down(T a, U b) noexcept -> T
{
    ARCHON_ASSERT(!core::is_negative(a));
    ARCHON_ASSERT(!core::is_negative(b));
    using type = core::promoted_type<core::common_int_type<T, U>>;
    type a_2 = core::int_cast_a<type>(a);
    type b_2 = core::int_cast_a<type>(b);
    core::IntDivMod<type> res = core::int_divmod(a_2, b_2);
    if (ARCHON_LIKELY(res.rem <= type(b / 2)))
        return core::int_cast_a<T>(res.quot);
    return core::int_cast_a<T>(res.quot + type(1));
}


namespace impl {


template<class T> constexpr auto int_odd_mod(T a, T b) noexcept -> T
{
    static_assert(std::is_same_v<T, core::promoted_type<T>>);
    // Take care to not compute `a % b` when `a / b` might not be representable in `T`,
    // because in that case, `a % b` has unspecified behavior.
    if (ARCHON_LIKELY(core::int_not_equal(b, -1)))
         return a % b;
    return T(0);
}


template<class T> constexpr auto int_periodic_mod(T a, T b) noexcept -> T
{
    static_assert(std::is_same_v<T, core::promoted_type<T>>);
    T c = impl::int_odd_mod(a, b);
    if (ARCHON_LIKELY(b >= T(0))) {
        if (ARCHON_LIKELY(c >= T(0)))
            return c;
    }
    else {
        if (ARCHON_LIKELY(c <= T(0)))
            return c;
    }
    return b + c;
}


} // namespace impl


template<class T, class U> constexpr auto int_periodic_mod(T a, U b) noexcept -> U
{
    static_assert(core::is_integer<T>());
    static_assert(core::is_integer<U>());
    using type = core::promoted_type<core::common_int_type<T, U>>;
    type a_2 = core::int_cast_a<type>(a);
    type b_2 = core::int_cast_a<type>(b);

    if constexpr (std::is_unsigned_v<type>) {
        if (core::is_negative(a)) {
            // It follows that U is unsigned, and that U has more value bits than T
            return core::int_cast_a<U>(b_2 - type(1) - ~a_2 % b_2);
        }
        if (core::is_negative(b)) {
            // It follows that T is unsigned, and that T has more value bits than U
            type c = a_2 % core::int_neg(b_2);
            using type_2 = core::promoted_type<U>;
            return core::int_cast_a<U>(c != type(0) ? core::promote(b) + core::int_cast_a<type_2>(c) : type_2(0));
        }
    }

    return core::int_cast_a<U>(impl::int_periodic_mod(a_2, b_2));
}


template<class T> constexpr auto int_sqrt(T val) noexcept -> T
{
    ARCHON_ASSERT(!core::is_negative(val));
    auto v = core::promote(val);
    using type = decltype(v);
    if (ARCHON_LIKELY(v != type(0))) {
        type v_0 = type(1) << (core::int_find_msb_pos(core::to_unsigned(v)) / 2 + 1);
        type v_1 = (v_0 + v / v_0) >> 1;
        while (ARCHON_LIKELY(v_1 < v_0)) {
            v_0 = v_1;
            v_1 = (v_0 + v / v_0) >> 1;
        }
        return core::int_cast_a<T>(v_0);
    }
    return val;
}


template<bool from_signed, class T, class U, std::size_t N, std::size_t M>
constexpr void int_bit_copy(const std::array<T, N>& parts_1, std::array<U, M>& parts_2) noexcept
{
    using part_type_1 = T;
    using part_type_2 = U;
    static_assert(core::is_unsigned<part_type_1>());
    static_assert(core::is_unsigned<part_type_2>());
    using type = core::common_int_type<unsigned, part_type_1, part_type_2>;
    constexpr int num_parts_1 = N;
    constexpr int num_parts_2 = M;
    constexpr int part_width_1 = core::int_width<part_type_1>();
    constexpr int part_width_2 = core::int_width<part_type_2>();
    if constexpr (part_width_1 >= part_width_2) {
        // Scatter
        int part_index_1 = 0, part_index_2 = 0;
        type part_1 = core::int_cast_a<type>(parts_1[0]);
        int offset = 0;
        for (;;) {
            type part_2 = core::int_cast_a<type>(part_1 >> offset);
            if (ARCHON_LIKELY(offset < part_width_1 - part_width_2)) {
                offset += part_width_2;
            }
            else {
                offset -= part_width_1 - part_width_2;
                part_index_1 += 1;
                if (ARCHON_LIKELY(part_index_1 < num_parts_1)) {
                    part_1 = core::int_cast_a<type>(parts_1[part_index_1]);
                    part_2 |= (part_1 << 1) << (part_width_2 - offset - 1);
                }
                else {
                    // Sign extend
                    int sign_bit = 0;
                    if constexpr (from_signed)
                        sign_bit = int(parts_1[num_parts_1 - 1] >> (part_width_1 - 1));
                    type ext = type(-1 * sign_bit);
                    part_2 |= (ext << 1) << (part_width_2 - offset - 1);
                    parts_2[part_index_2] = core::int_cast_a<part_type_2>(part_2);
                    for (;;) {
                        part_index_2 += 1;
                        if (ARCHON_LIKELY(part_index_2 < num_parts_2)) {
                            parts_2[part_index_2] = core::int_cast_a<part_type_2>(ext);
                            continue;
                        }
                        break;
                    }
                    break;
                }
            }
            parts_2[part_index_2] = core::int_cast_a<part_type_2>(part_2);
            part_index_2 += 1;
            if (ARCHON_LIKELY(part_index_2 < num_parts_2))
                continue;
            break;
        }
    }
    else {
        // Gather
        int part_index_1 = 0, part_index_2 = 0;
        type part_2 = type(0);
        int offset = 0;
        for (;;) {
            type part_1 = core::int_cast_a<type>(parts_1[part_index_1]);
            part_2 |= part_1 << offset;
            if (ARCHON_LIKELY(offset < part_width_2 - part_width_1)) {
                offset += part_width_1;
            }
            else {
                parts_2[part_index_2] = core::int_cast_a<part_type_2>(part_2);
                part_index_2 += 1;
                if (part_index_2 < num_parts_2) {
                    offset -= part_width_2 - part_width_1;
                    part_2 = (part_1 >> 1) >> (part_width_1 - offset - 1);
                }
                else {
                    break;
                }
            }
            part_index_1 += 1;
            if (part_index_1 < num_parts_1)
                continue;
            // Sign extend
            int sign_bit = 0;
            if constexpr (from_signed)
                sign_bit = int(parts_1[num_parts_1 - 1] >> (part_width_1 - 1));
            type ext = type(-1 * sign_bit);
            part_2 |= ext << offset;
            parts_2[part_index_2] = core::int_cast_a<part_type_2>(part_2);
            for (;;) {
                part_index_2 += 1;
                if (ARCHON_LIKELY(part_index_2 < num_parts_2)) {
                    parts_2[part_index_2] = core::int_cast_a<part_type_2>(ext);
                    continue;
                }
                break;
            }
            break;
        }
    }
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INTEGER_HPP
