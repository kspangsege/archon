// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__INTEGER_HPP
#define ARCHON__BASE__INTEGER_HPP

#include <type_traits>
#include <limits>
#include <algorithm>
#include <utility>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>


namespace archon::base {


/// \brief Promote the specified integer.
///
/// This function converts the specified integer to its promoted type, which is
/// `decltype(std::declval<T>() + 0)`.
///
template<class T> constexpr auto promote(T value) noexcept;


/// \brief Cast specified integer value to unsigned type.
///
/// This function casts the specified integer value to the unsigned type
/// produced by `std::make_unsigned_t<T>`. Note that this is guaranteed to be a
/// value preserving operation if the specified value is known to be
/// non-negative.
///
template<class T> constexpr auto to_unsigned(T value) noexcept;


/// \brief Get width of integer type.
///
/// Get the width of the specified integer type. The width is the number of
/// value bits (`std::numeric_limits<T>::digits`), plus one if the type is
/// signed.
///
template<class T> constexpr int get_int_width() noexcept;


/// \brief Determine if integer is power of two.
///
/// Determine, in a very efficient manner, whether the specified integer is a
/// power of two. No negative numbers are considered to be powers of two by this
/// function.
///
template<class T> constexpr bool int_is_power_of_two(T value) noexcept;


/// \brief Find position of most significant digit.
///
/// Find the position of the most significant digit in the specified value and
/// with respect to the specified base/radix. This works for negative values
/// too. Returns -1 if the specified value is zero. Otherwise it always returns
/// a nonnegative value.
///
template<class T> constexpr int int_find_msd_pos(T value, int base) noexcept;


/// \brief Find number of digits in integer value.
///
/// Find the number of digits needed to represent the specified value in the
/// specified base/radix. This function does not count the sign itself as a
/// digit.
///
template<class T> constexpr int int_digits(T value, int base) noexcept;


/// \brief Find maximum number of digits for integer type.
///
/// Find the maximum number of digits needed to represent any positive or
/// negative value of the specified integer type in the specified
/// base/radix. This function does not count the sign itself as a digit.
///
template<class T> constexpr int int_max_digits(int base) noexcept;


/// \brief Test for negativity.
///
/// This function allows you to test for a negative value in any numeric type,
/// even when the type is unsigned. Normally, when the type is unsigned, such a
/// test will produce a compiler warning.
///
template<class T> constexpr bool is_negative(T value) noexcept;


/// \brief Produce N-bit mask.
///
/// This function returns a value of the specified type in which the N least
/// significant bits are 1. Here, N is the smaller of \p num_bits and
/// `std::numeric_limits<T>::digits`.
///
template<class T> constexpr T int_mask(int num_bits) noexcept;


/// \brief Cast integer value without special treatment of `bool`.
///
/// This function operates just like a regular cast, except that it treats
/// `bool` as a regular unsigned integer type with one value bit, so
/// `int_cast<bool>(2)` is zero, not one.
///
/// Just like for a regular cast, if the target type is signed, and the
/// specified value cannot be represented in that signed type, the result is
/// implementation defined.
///
/// If the target type is unsigned, then, regardles of whether the target type
/// is `bool`, the result is the least unsigned value congruent to the specified
/// value modulo 2**N, where N is the number of value bits in the target
/// type. Note how this deviates from a regular cast in that it makes
/// `int_cast<bool>(2)` equal to zero (or false), whereas `bool(2)` is one (or
/// true).
///
/// It is an error if `T` or `F` is anything other than an integer type
/// (`std::is_integral`).
///
/// This function makes no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T, class F> constexpr T int_cast(F value) noexcept;


/// \brief Integer cast is value preserving.
///
/// This function returns true if, and only if the specified value can be
/// represented in `T`.
///
/// It is an error if `T` or `F` is anything other than an integer type
/// (`std::is_integral`).
///
/// This function makes no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T, class F> constexpr bool can_int_cast(F value) noexcept;


/// \brief Try to cast integer value without special treatment of `bool`.
///
/// This function has the same effect as \ref int_cast() except that it performs
/// the conversion when, ond only when the specified value can be represented in
/// `T` (\ref can_int_cast()). If it can, this function returns `true` and
/// assigns the result to \p to. Otherwise it returns `false` and leaves \p to
/// untouched.
///
template<class T, class F> constexpr bool try_int_cast(F from, T& to) noexcept;


/// \{
///
/// \brief Convert integer to two's complement representation.
///
/// These functions have the same effect as \ref int_cast(), can_int_cast(), and
/// \ref try_int_cast() respectively, except that if `T` (the target type) is an
/// unsigned integer type, these functions assume that `T` stores negative
/// values using two's complement representation.
///
/// Note in particular, that when \ref try_cast_fto_twos_compl() returns `true`,
/// the conversion preserves the represented value, even when it is negative.
///
/// Note also that \ref cast_to_twos_compl() is effectively an alias for \ref
/// int_cast(). There is no diffrence between them. Additionally, if `T` is not
/// `bool`, \ref cast_to_twos_compl() is the same as a regular cast.
///
/// These functions make no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T, class F> constexpr T cast_to_twos_compl(F value) noexcept;
template<class T, class F> constexpr bool can_cast_to_twos_compl(F value) noexcept;
template<class T, class F> constexpr bool try_cast_to_twos_compl(F from, T& to) noexcept;
/// \}


/// \{
///
/// \brief Convert integer from two's complement representation.
///
/// These functions have the same effect as \ref int_cast(), can_int_cast(), and
/// \ref try_int_cast() respectively, except that if `F` (the origin type) is an
/// unsigned integer type, these functions assume that `F` stores negative
/// values using two's complement representation.
///
/// Note in particular, that when \ref try_cast_from_twos_compl() returns
/// `true`, the conversion preserves the represented value, even when it is
/// negative.
///
/// These functions make no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T, class F> constexpr T cast_from_twos_compl(F value) noexcept;
template<class T, class F> constexpr bool can_cast_from_twos_compl(F value) noexcept;
template<class T, class F> constexpr bool try_cast_from_twos_compl(F from, T& to) noexcept;
/// \}


/// \{
///
/// \brief Convert integer between two's complement representations.
///
/// These functions have the same effect as \ref int_cast(), can_int_cast(), and
/// \ref try_int_cast() respectively, except that if `F` (the origin type) is an
/// unsigned integer type, these functions assume that `F` stores negative
/// values using two's complement representation. Similarly, if `T` (the target
/// type) is an unsigned integer type, these functions assume that `T` stores
/// negative values using two's complement representation.
///
/// Note in particular, that when \ref try_twos_compl_cast() returns `true`, the
/// conversion preserves the represented value, even when it is negative.
///
/// These functions make no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T, class F> constexpr T twos_compl_cast(F value) noexcept;
template<class T, class F> constexpr bool can_twos_compl_cast(F value) noexcept;
template<class T, class F> constexpr bool try_twos_compl_cast(F from, T& to) noexcept;
/// \}


/// \brief Sign-extend in two's complement representation.
///
/// This function sign-extends the specified value from a two's complement
/// representation using the specified number of bit (\p from_width) to a two's
/// complement representation using all of the value bits in `T`.
///
/// If \p from_width is greater than the number of value bits in `T`, this
/// function returns the specified value unmodified.
///
/// Sign extension occurs as follows: If the sign bit, i.e., the bit at position
/// `from_width - 1`, is 1, the N most significant bits will also be set. Here,
/// `N` is the width of `T` (\ref get_int_width()) minus \p from_width.
///
/// It is an error if `T` is not an unsigned integer type. It is also an error
/// if \p from_width is less than 1.
///
/// This function makes no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T> constexpr T twos_compl_sign_extend(T value, int from_width) noexcept;


/// \{
///
/// \brief Compare heterogeneously typed integer values.
///
/// Compare two integers of the same, or of different type, and produce the
/// expected result according to the natural interpretation of the operation.
///
/// Note that in general a standard comparison between a signed and an unsigned
/// integer type is unsafe, and it often generates a compiler warning. An
/// example is a 'less than' comparison between a negative value of type 'int'
/// and a small positive value of type 'unsigned'. In this case the negative
/// value will be converted to 'unsigned' producing a large positive value
/// which, in turn, will lead to the counter intuitive result of 'false'.
///
/// Please note that these operation incur absolutely no overhead when the two
/// types have the same signedness.
///
/// These functions check at compile time that both types have valid
/// specializations of std::numeric_limits<> and that both are indeed integers.
///
/// These functions make no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class A, class B> constexpr bool int_equal_to(A, B) noexcept;
template<class A, class B> constexpr bool int_not_equal_to(A, B) noexcept;
template<class A, class B> constexpr bool int_less_than(A, B) noexcept;
template<class A, class B> constexpr bool int_less_than_or_equal(A, B) noexcept;
template<class A, class B> constexpr bool int_greater_than(A, B) noexcept;
template<class A, class B> constexpr bool int_greater_than_or_equal(A, B) noexcept;
/// \}


/// \{
///
/// \brief Add and subtract heterogeneously typed integer values while checking
/// for overflow.
///
/// These functions are like \ref try_int_add() and \ref try_int_sub()
/// respectively except that they throw `std::overflow_error` on positive or
/// negative overflow instead of returning a boolean.
///
template<class L, class R> constexpr void int_add(L& lval, R rval);
template<class L, class R> constexpr void int_sub(L& lval, R rval);
/// \}


/// \{
///
/// \brief Multiply heterogeneously typed integer values while checking for
/// overflow.
///
/// This function is like \ref try_int_mul() except that it throws
/// `std::overflow_error` on positive or negative overflow instead of returning
/// a boolean.
///
template<class L, class R> constexpr void int_mul(L& lval, R rval);
/// \}


/// \{
///
/// \brief Performing bitwise left-shift while checking for overflow.
///
/// This function is like \ref try_int_shift_left() except that it throws
/// `std::overflow_error` on positive or negative overflow instead of returning
/// a boolean.
///
template<class T> constexpr void int_shift_left(T& lval, int i);
/// \}


/// \{
///
/// \brief Add and subtract heterogeneously typed integer values while checking
/// for overflow.
///
/// These functions check for overflow in integer variable \p lval while adding
/// integer \p rval to it, or while subtracting integer \p rval from it. They
/// return `false` on positive or negative overflow. Otherwise, they return
/// `true`.
///
/// Both \p lval and \p rval must be of an integer type for which a
/// specialization of `std::numeric_limits` exists. The two types need not be
/// the same, in particular, one can be signed and the other one can be
/// unsigned.
///
/// In terms of efficiency, these functions are especially well suited for cases
/// where \p rval is a compile-time constant.
///
/// These functions make no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class L, class R> constexpr bool try_int_add(L& lval, R rval) noexcept;
template<class L, class R> constexpr bool try_int_sub(L& lval, R rval) noexcept;
/// \}


/// \brief Multiply heterogeneously typed integer values while checking for
/// overflow.
///
/// This function checks for positive overflow while multiplying two positive
/// integers of the same, or of different type. It returns `false` on
/// overflow. Otherwise, it returns `true`.
///
/// Both \p lval and \p rval must be of an integer type for which a
/// specialization of `std::numeric_limits` exists. The two types need not be
/// the same, in particular, one can be signed and the other one can be
/// unsigned.
///
/// \param lval Must not be negative. Both signed and unsigned types can be
/// used.
///
/// \param rval Must be stricly greater than zero. Both signed and unsigned
/// types can be used.
///
/// In terms of efficiency, this function is especially well suited for cases
/// where \p rval is a compile-time constant.
///
/// This function makes no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class L, class R> constexpr bool try_int_mul(L& lval, R rval) noexcept;


/// \brief Performing bitwise left-shift while checking for overflow.
///
/// This function checks for positive overflow while performing a bitwise shift
/// to the left on a non-negative value of arbitrary integer type. It returns
/// `false` on overflow. Otherwise, it returns `true`.
///
/// \param lval Must not be negative. Must be of an integer type for which a
/// specialization of `std::numeric_limits` exists. Both signed and unsigned
/// types can be used.
///
/// \param i Must be non-negative and such that `L(1) >> i` has a value that is
/// defined by the C++17 standard. In particular, the value of \p i must be
/// strictly less than `std::numeric_limits<T>::digits`.
///
/// This function makes no assumptions about the platform except that it
/// complies with at least C++17.
///
template<class T> constexpr bool try_int_shift_left(T& lval, int i) noexcept;








// Implementation


template<class T> constexpr auto promote(T value) noexcept
{
    static_assert(std::is_integral_v<T>);
    return value + 0;
}


template<class T> constexpr auto to_unsigned(T value) noexcept
{
    using uint_type = std::make_unsigned_t<T>;
    return uint_type(value);
}


template<class T> constexpr int get_int_width() noexcept
{
    static_assert(std::is_integral_v<T>);
    int digits = std::numeric_limits<T>::digits;
    if constexpr (std::is_signed_v<T>)
        ++digits;
    return digits;
}


template<class T> constexpr bool int_is_power_of_two(T value) noexcept
{
    static_assert(std::is_integral_v<T>);
    return ((value > 0) && ((value & (value - 1)) == 0));
}


template<class T> constexpr int int_find_msd_pos(T value, int base) noexcept
{
    static_assert(std::is_integral_v<T>);
    return (value == 0 ? -1 : 1 + int_find_msd_pos(T(value / base), base));
}


template<class T> constexpr int int_digits(T value, int base) noexcept
{
    return 1 + int_find_msd_pos(value, base);
}


template<class T> constexpr int int_max_digits(int base) noexcept
{
    static_assert(std::is_integral_v<T>);
    using lim = std::numeric_limits<T>;
    static_assert(lim::is_specialized);
    static_assert(lim::is_integer);
    return 1 + std::max(int_find_msd_pos(lim::min(), base), int_find_msd_pos(lim::max(), base));
}


namespace detail {


template<class L, class R, bool l_signed, bool r_signed> class SafeIntBinopsImpl {};


// (unsigned, unsigned) (all size combinations)
//
// This implementation utilizes the fact that overflow in unsigned arithmetic is
// guaranteed to be handled by reduction modulo 2**N where N is the number of
// bits in the common unsigned type. Also, this implementation uses the fact
// that if modular addition overflows, then the result must be a value that is
// less than both operands. Also, if modular subtraction overflows, then the
// result must be a value that is greater than the first operand.
//
template<class L, class R> class SafeIntBinopsImpl<L, R, false, false> {
public:
    using common_type = decltype(std::declval<L>() + std::declval<R>());
    using common_unsigned = typename std::make_unsigned_t<common_type>;

    static constexpr bool equal(L l, R r) noexcept
    {
        return (common_unsigned(l) == common_unsigned(r));
    }

    static constexpr bool less(L l, R r) noexcept
    {
        return (common_unsigned(l) < common_unsigned(r));
    }

    static constexpr bool try_add(L& lval, R rval) noexcept
    {
        L lval_2 = int_cast<L>(lval + rval);
        bool overflow = (common_unsigned(lval_2) < common_unsigned(rval));
        if (ARCHON_LIKELY(!overflow)) {
            lval = lval_2;
            return true;
        }
        return false;
    }

    static constexpr bool try_sub(L& lval, R rval) noexcept
    {
        common_unsigned lval_2 = common_unsigned(lval) - common_unsigned(rval);
        bool overflow = (lval_2 > common_unsigned(lval));
        if (ARCHON_LIKELY(!overflow)) {
            lval = int_cast<L>(lval_2);
            return true;
        }
        return false;
    }
};


// (unsigned, signed) (all size combinations)
//
template<class L, class R> class SafeIntBinopsImpl<L, R, false, true> {
public:
    using common_type = decltype(std::declval<L>() + std::declval<R>());
    using common_unsigned = typename std::make_unsigned_t<common_type>;

    using lim_l  = std::numeric_limits<L>;
    using lim_r  = std::numeric_limits<R>;
    using lim_cu = std::numeric_limits<common_unsigned>;

    static constexpr bool equal(L l, R r) noexcept
    {
        if constexpr (lim_l::digits > lim_r::digits)
            return (r >= 0 && l == int_cast<L>(r));
        return (R(l) == r);
    }

    static constexpr bool less(L l, R r) noexcept
    {
        if constexpr (lim_l::digits > lim_r::digits)
            return (r >= 0 && l < int_cast<L>(r));
        return (R(l) < r);
    }

    static constexpr bool try_add(L& lval, R rval) noexcept
    {
        common_unsigned lval_2 = lval + common_unsigned(rval);
        bool overflow = false;
        if constexpr (lim_cu::digits > lim_l::digits && lim_cu::digits > lim_r::digits) {
            overflow = (lval_2 > common_unsigned(lim_l::max()));
        }
        else if (ARCHON_LIKELY(rval >= 0)) {
            overflow = (lval_2 < common_unsigned(lval) || lval_2 > common_unsigned(lim_l::max()));
        }
        else {
            overflow = (lval_2 >= common_unsigned(lval));
        }
        if (ARCHON_LIKELY(!overflow)) {
            lval = int_cast<L>(lval_2);
            return true;
        }
        return false;
    }

    static constexpr bool try_sub(L& lval, R rval) noexcept
    {
        common_unsigned lval_2 = lval - common_unsigned(rval);
        bool overflow = false;
        if constexpr (lim_cu::digits > lim_l::digits && lim_cu::digits > lim_r::digits) {
            overflow = (lval_2 > common_unsigned(lim_l::max()));
        }
        else if (ARCHON_LIKELY(rval >= 0)) {
            overflow = (lval_2 > common_unsigned(lval));
        }
        else {
            overflow = (lval_2 <= common_unsigned(lval) || lval_2 > common_unsigned(lim_l::max()));
        }
        if (ARCHON_LIKELY(!overflow)) {
            lval = int_cast<L>(lval_2);
            return true;
        }
        return false;
    }
};


// (signed, unsigned) (all size combinations)
//
template<class L, class R> class SafeIntBinopsImpl<L, R, true, false> {
public:
    using common_type = decltype(std::declval<L>() + std::declval<R>());
    using common_unsigned = typename std::make_unsigned_t<common_type>;

    using lim_l  = std::numeric_limits<L>;
    using lim_r  = std::numeric_limits<R>;
    using lim_cu = std::numeric_limits<common_unsigned>;

    static constexpr bool equal(L l, R r) noexcept
    {
        if constexpr (lim_l::digits < lim_r::digits)
            return (l >= 0 && int_cast<R>(l) == r);
        return (l == L(r));
    }

    static constexpr bool less(L l, R r) noexcept
    {
        if constexpr (lim_l::digits < lim_r::digits)
            return (l < 0 || int_cast<R>(l) < r);
        return (l < L(r));
    }

    static_assert(lim_cu::digits >= lim_l::digits);
    static_assert(lim_cu::digits != lim_l::digits || lim_r::digits <= lim_l::digits);

    static constexpr bool try_add(L& lval, R rval) noexcept
    {
        if constexpr (lim_cu::digits == lim_l::digits)
            return SafeIntBinopsImpl<L, L, true, true>::try_add(lval, L(rval));
        common_unsigned max_add = common_unsigned(lim_l::max()) - common_unsigned(lval);
        bool overflow = (common_unsigned(rval) > max_add);
        if (ARCHON_LIKELY(!overflow)) {
            lval = cast_from_twos_compl<L>(common_unsigned(lval) + rval);
            return true;
        }
        return false;
    }

    static constexpr bool try_sub(L& lval, R rval) noexcept
    {
        if constexpr (lim_cu::digits == lim_l::digits)
            return SafeIntBinopsImpl<L, L, true, true>::try_sub(lval, L(rval));
        common_unsigned max_sub = common_unsigned(lval) - common_unsigned(lim_l::min());
        bool overflow = (common_unsigned(rval) > max_sub);
        if (ARCHON_LIKELY(!overflow)) {
            lval = cast_from_twos_compl<L>(common_unsigned(lval) - rval);
            return true;
        }
        return false;
    }
};


// (signed, signed) (all size combinations)
//
template<class L, class R> class SafeIntBinopsImpl<L, R, true, true> {
public:
    using lim_l = std::numeric_limits<L>;

    static constexpr bool equal(L l, R r) noexcept
    {
        return (l == r);
    }

    static constexpr bool less(L l, R r) noexcept
    {
        return (l < r);
    }

    static constexpr bool try_add(L& lval, R rval) noexcept
    {
        // Note that both subtractions below occur in a signed type that is at
        // least as wide as both of the two types. Note also that any signed
        // type guarantees that there is no overflow when subtracting two
        // negative values or two non-negative value. See C11 (adopted as subset
        // of C++14) section 6.2.6.2 "Integer types" paragraph 2.
        if (rval < 0) {
            if (ARCHON_UNLIKELY(lval < lim_l::min() - rval))
                return false;
        }
        else {
            if (ARCHON_UNLIKELY(lval > lim_l::max() - rval))
                return false;
        }
        // The following statement has exactly the same effect as
        // `lval += rval`.
        lval = L(lval + rval);
        return true;
    }

    static constexpr bool try_sub(L& lval, R rval) noexcept
    {
        // Note that both additions below occur in a signed type that is at
        // least as wide as both of the two types. Note also that there can be
        // no overflow when adding a negative value to a non-negative value, or
        // when adding a non-negative value to a negative one. See C11 (adopted
        // as subset of C++14) section 6.2.6.2 "Integer types" paragraph 2.
        if (rval < 0) {
            if (ARCHON_UNLIKELY(lval > lim_l::max() + rval))
                return false;
        }
        else {
            if (ARCHON_UNLIKELY(lval < lim_l::min() + rval))
                return false;
        }
        // The following statement has exactly the same effect as `lval -=
        // rval`.
        lval = L(lval - rval);
        return true;
    }
};


template<class L, class R> class SafeIntBinops :
        public SafeIntBinopsImpl<L, R, std::numeric_limits<L>::is_signed,
                                 std::numeric_limits<R>::is_signed> {
public:
    using lim_l = std::numeric_limits<L>;
    using lim_r = std::numeric_limits<R>;

    static_assert(lim_l::is_specialized && lim_r::is_specialized);
    static_assert(lim_l::is_integer && lim_r::is_integer);
};


} // namespace detail


template<class T> constexpr bool is_negative(T value) noexcept
{
    if constexpr (std::is_signed_v<T>) {
        return (value < 0);
    }
    else {
        static_cast<void>(value);
        return false;
    }
}


template<class T> constexpr T int_mask(int num_bits) noexcept
{
    static_assert(std::is_integral_v<T>);
    using lim = std::numeric_limits<T>;
    static_assert(lim::is_specialized);
    static_assert(lim::is_integer);
    if (ARCHON_LIKELY(num_bits > 0)) {
        int num_extra_bits = 0;
        if (ARCHON_LIKELY(num_bits <= lim::digits))
            num_extra_bits = lim::digits - num_bits;
        return T(lim::max() >> num_extra_bits);
    }
    return 0;
}


template<class T, class F> constexpr T int_cast(F value) noexcept
{
    if constexpr (std::is_same_v<T, bool>) {
        return bool(unsigned(value) & 1);
    }
    else {
        return T(value);
    }

}


template<class T, class F> constexpr bool can_int_cast(F value) noexcept
{
    using lim_t = std::numeric_limits<T>;
    return (int_greater_than_or_equal(value, lim_t::min()) &&
            int_less_than_or_equal(value, lim_t::max()));
}


template<class T, class F> constexpr bool try_int_cast(F from, T& to) noexcept
{
    if (ARCHON_LIKELY(can_int_cast<T>(from))) {
        to = T(from);
        return true;
    }
    return false;
}


template<class T, class F> constexpr T cast_to_twos_compl(F value) noexcept
{
    return int_cast<T>(value);
}


template<class T, class F> constexpr bool can_cast_to_twos_compl(F value) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if constexpr (std::is_unsigned_v<T>) {
        static_assert(T(~lim_t::max()) == 0);
        auto max = to_unsigned(promote(lim_t::max()) >> 1);
        if constexpr (std::is_unsigned_v<F>) {
            return (value <= max);
        }
        else {
            if (ARCHON_LIKELY(value >= 0)) {
                // Non-negative value
                return (to_unsigned(promote(value)) <= max);
            }
            // Negative value
            return (to_unsigned(-1 - value) <= max);
        }
    }
    return can_int_cast<T>(value);
}


template<class T, class F> constexpr bool try_cast_to_twos_compl(F from, T& to) noexcept
{
    if (ARCHON_LIKELY(can_cast_to_twos_compl<T>(from))) {
        to = T(from);
        return true;
    }
    return false;
}


template<class T, class F> constexpr T cast_from_twos_compl(F value) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if constexpr (std::is_unsigned_v<F>) {
        auto sign_bit = promote(value) & (promote(F(1)) << (get_int_width<F>() - 1));
        bool nonnegative = (sign_bit == 0);
        if constexpr (std::is_unsigned_v<T>) {
            return int_cast<T>(value);
        }
        else {
            if (ARCHON_LIKELY(nonnegative)) {
                // Non-negative value
                return T(value);
            }
            // Negative value
            auto val = promote(F(-1)) - promote(value);
            using signed_type = decltype(promote(T(0)));
            return T(-1 - signed_type(val));
        }
    }
    return int_cast<T>(value);
}


template<class T, class F> constexpr bool can_cast_from_twos_compl(F value) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if constexpr (std::is_unsigned_v<F>) {
        auto sign_bit = promote(value) & (promote(F(1)) << (get_int_width<F>() - 1));
        bool nonnegative = (sign_bit == 0);
        if constexpr (std::is_unsigned_v<T>) {
            return (nonnegative && value <= lim_t::max());
        }
        else {
            if (ARCHON_LIKELY(nonnegative)) {
                // Non-negative value
                return (value <= to_unsigned(promote(lim_t::max())));
            }
            // Negative value
            auto val = promote(F(-1)) - promote(value);
            auto max = promote(T(-1)) - promote(lim_t::min());
            return (to_unsigned(val) <= to_unsigned(max));
        }
    }
    return can_int_cast<T>(value);
}


template<class T, class F> constexpr bool try_cast_from_twos_compl(F from, T& to) noexcept
{
    using lim_f = std::numeric_limits<F>;
    using lim_t = std::numeric_limits<T>;
    static_assert(lim_f::is_specialized && lim_t::is_specialized);
    static_assert(lim_f::is_integer && lim_t::is_integer);
    if (std::is_unsigned_v<F>) {
        auto sign_bit = promote(from) & (promote(F(1)) << (get_int_width<F>() - 1));
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
                if (ARCHON_LIKELY(from <= to_unsigned(promote(lim_t::max())))) {
                    to = T(from);
                    return true;
                }
                return false;
            }
            // Negative value
            auto val = promote(F(-1)) - promote(from);
            auto max = promote(T(-1)) - promote(lim_t::min());
            if (ARCHON_LIKELY(to_unsigned(val) <= to_unsigned(max))) {
                using signed_type = decltype(promote(T(0)));
                to = T(-1 - signed_type(val));
                return true;
            }
            return false;
        }
    }
    return try_int_cast(from, to);
}


template<class T, class F> constexpr T twos_compl_cast(F value) noexcept
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = get_int_width<T>();
        constexpr int width_f = get_int_width<F>();
        if constexpr (width_t >= width_f)
            return twos_compl_sign_extend(T(value), width_f);
        return T(value);
    }
    if constexpr (std::is_unsigned_v<T>)
        return cast_to_twos_compl<T>(value);
    if constexpr (std::is_unsigned_v<F>)
        return cast_from_twos_compl<T>(value);
    return int_cast<T>(value);
}


template<class T, class F> constexpr bool can_twos_compl_cast(F value) noexcept
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = get_int_width<T>();
        constexpr int width_f = get_int_width<F>();
        if constexpr (width_t >= width_f) {
            return true;
        }
        else {
            static_assert(width_t < width_f);
            auto sign_bit = promote(value) & (promote(F(1)) << (width_t - 1));
            auto expected_sign_extension = ~((sign_bit << 1) - 1);
            auto actual_sign_extension = promote(value) & (promote(F(-1)) << width_t);
            return (actual_sign_extension == expected_sign_extension);
        }
    }
    if constexpr (std::is_unsigned_v<T>)
        return can_cast_to_twos_compl<T>(value);
    if constexpr (std::is_unsigned_v<F>)
        return can_cast_from_twos_compl<T>(value);
    return can_int_cast<T>(value);
}


template<class T, class F> constexpr bool try_twos_compl_cast(F from, T& to) noexcept
{
    if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<F>) {
        constexpr int width_t = get_int_width<T>();
        constexpr int width_f = get_int_width<F>();
        if constexpr (width_t >= width_f) {
            to = twos_compl_sign_extend(T(from), width_f);
            return true;
        }
        else {
            static_assert(width_t < width_f);
            auto sign_bit = promote(from) & (promote(F(1)) << (width_t - 1));
            auto expected_sign_extension = ~((sign_bit << 1) - 1);
            auto actual_sign_extension = promote(from) & (promote(F(-1)) << width_t);
            if (ARCHON_LIKELY(actual_sign_extension == expected_sign_extension)) {
                to = T(from);
                return true;
            }
            return false;
        }
    }
    if constexpr (std::is_unsigned_v<T>)
        return try_cast_to_twos_compl(from, to);
    if constexpr (std::is_unsigned_v<F>)
        return try_cast_from_twos_compl(from, to);
    return try_int_cast(from, to);
}


template<class T> constexpr T twos_compl_sign_extend(T value, int from_width) noexcept
{
    static_assert(std::is_unsigned_v<T>);
    ARCHON_ASSERT(from_width > 0);
    if (get_int_width<T>() > from_width) {
        auto sign_bit = promote(value) & (promote(T(1)) << (from_width - 1));
        auto sign_extension = ~((sign_bit << 1) - 1);
        return T(sign_extension | promote(value));
    }
    return value;
}


template<class A, class B> constexpr bool int_equal_to(A a, B b) noexcept
{
    return detail::SafeIntBinops<A, B>::equal(a, b);
}


template<class A, class B> constexpr bool int_not_equal_to(A a, B b) noexcept
{
    return !detail::SafeIntBinops<A, B>::equal(a, b);
}


template<class A, class B> constexpr bool int_less_than(A a, B b) noexcept
{
    return detail::SafeIntBinops<A, B>::less(a, b);
}


template<class A, class B> constexpr bool int_less_than_or_equal(A a, B b) noexcept
{
    return !detail::SafeIntBinops<B, A>::less(b, a); // Not greater than
}


template<class A, class B> constexpr bool int_greater_than(A a, B b) noexcept
{
    return detail::SafeIntBinops<B, A>::less(b, a);
}


template<class A, class B> constexpr bool int_greater_than_or_equal(A a, B b) noexcept
{
    return !detail::SafeIntBinops<A, B>::less(a, b); // Not less than
}


template<class L, class R> constexpr void int_add(L& lval, R rval)
{
    if (ARCHON_LIKELY(try_int_add(lval, rval)))
        return;
    throw std::overflow_error("Integer addition");
}


template<class L, class R> constexpr void int_sub(L& lval, R rval)
{
    if (ARCHON_LIKELY(try_int_sub(lval, rval)))
        return;
    throw std::overflow_error("Integer subtraction");
}


template<class L, class R> constexpr void int_mul(L& lval, R rval)
{
    if (ARCHON_LIKELY(try_int_mul(lval, rval)))
        return;
    throw std::overflow_error("Integer multiplication");
}


template<class T> constexpr void int_shift_left(T& lval, int i)
{
    if (ARCHON_LIKELY(try_int_shift_left(lval, i)))
        return;
    throw std::overflow_error("Bitwise left-shift");
}


template<class L, class R> constexpr bool try_int_add(L& lval, R rval) noexcept
{
    return detail::SafeIntBinops<L, R>::try_add(lval, rval);
}


template<class L, class R> constexpr bool try_int_sub(L& lval, R rval) noexcept
{
    return detail::SafeIntBinops<L, R>::try_sub(lval, rval);
}


template<class L, class R> constexpr bool try_int_mul(L& lval, R rval) noexcept
{
    // FIXME: Check if the following optimizes better (assuming it works at
    // all):
    //
    //   L lval_2 = L(lval * rval);
    //   bool overflow  =  rval != 0  &&  (lval_2 / rval) != lval;
    //
    using lim_l = std::numeric_limits<L>;
    using lim_r = std::numeric_limits<R>;
    static_assert(lim_l::is_specialized && lim_r::is_specialized);
    static_assert(lim_l::is_integer && lim_r::is_integer);
    ARCHON_ASSERT(int_greater_than_or_equal(lval, 0));
    ARCHON_ASSERT(int_greater_than(rval, 0));
    if (ARCHON_LIKELY(int_greater_than_or_equal(promote(lim_l::max()) / promote(rval), lval))) {
        auto product = promote(lval) * promote(rval);
        lval = L(product);
        return true;
    }
    return false;
}


template<class T> constexpr bool try_int_shift_left(T& lval, int i) noexcept
{
    using lim = std::numeric_limits<T>;
    static_assert(lim::is_specialized);
    static_assert(lim::is_integer);
    ARCHON_ASSERT(int_greater_than_or_equal(lval, 0));
    if (ARCHON_LIKELY((lim::max() >> i) >= lval)) {
        lval <<= i;
        return true;
    }
    return false;
}


} // namespace archon::base

#endif // ARCHON__BASE__INTEGER_HPP
