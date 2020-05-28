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

#ifndef ARCHON_X_CORE_X_SUPER_INT_HPP
#define ARCHON_X_CORE_X_SUPER_INT_HPP

/// \file


#include <cstdint>
#include <limits>
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/integer_formatter.hpp>


namespace archon::core {


/// \brief Integer type capable of holding values of all fundamental integer types.
///
/// This class presents a signed integer type that is capable of representing the values of
/// all fundamental signed and unsigned integer types. It uses the two's complement
/// representation of negative values. The representations of results of arithmetic
/// operations are reduced modulo 2**(N+1) where N is the number of value bits (\ref
/// digits). The results of arithmetic operations that overflow are therefore well defined,
/// even when they involve negative values.
///
class SuperInt {
public:
    using uint_type = std::uintmax_t;

private:
    using uint_lim_type = std::numeric_limits<uint_type>;

public:
    /// \brief Number of value bits.
    ///
    /// This is the number of value bits (excluding the sign bit).
    ///
    static constexpr int digits = uint_lim_type::digits;

    /// \brief Number of value bits plus 1 for sign bit.
    ///
    /// This is the number of value bits plus one for the sign bit.
    ///
    static constexpr int width = digits + 1;

    static constexpr auto min() noexcept -> SuperInt;
    static constexpr auto max() noexcept -> SuperInt;

    constexpr SuperInt() noexcept = default;
    template<class T> explicit constexpr SuperInt(T value) noexcept;
    constexpr SuperInt(uint_type value, bool sign_bit) noexcept;

    template<class T> constexpr bool cast_has_overflow() const noexcept;

    template<class T> constexpr bool get_as(T&) const noexcept;

    constexpr auto operator+() const noexcept -> SuperInt;
    constexpr auto operator-() const noexcept -> SuperInt;
    constexpr auto operator~() const noexcept -> SuperInt;

    constexpr auto operator+(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator-(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator*(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator/(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator&(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator|(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator^(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator%(SuperInt) const noexcept -> SuperInt;
    constexpr auto operator<<(int) const noexcept -> SuperInt;
    constexpr auto operator>>(int) const noexcept -> SuperInt;

    constexpr auto operator+=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator-=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator*=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator/=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator%=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator&=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator|=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator^=(SuperInt) noexcept -> SuperInt&;
    constexpr auto operator<<=(int) noexcept -> SuperInt&;
    constexpr auto operator>>=(int) noexcept -> SuperInt&;

    constexpr bool operator==(SuperInt) const noexcept;
    constexpr bool operator!=(SuperInt) const noexcept;
    constexpr bool operator< (SuperInt) const noexcept;
    constexpr bool operator<=(SuperInt) const noexcept;
    constexpr bool operator> (SuperInt) const noexcept;
    constexpr bool operator>=(SuperInt) const noexcept;

    constexpr bool add_with_overflow_detect(SuperInt) noexcept;
    constexpr bool subtract_with_overflow_detect(SuperInt) noexcept;
    constexpr bool multiply_with_overflow_detect(SuperInt) noexcept;
    constexpr bool divide_with_overflow_detect(SuperInt) noexcept;

    constexpr auto get_value() const noexcept -> uint_type;
    constexpr bool get_sign_bit() const noexcept;

    /// \brief Format integer value.
    ///
    /// Formatting occurs as in the C locale, irrespective of the actual locale. The
    /// formatting process ignores all formatting flags (e.g., it ignores
    /// `std::ios_base::hex`).
    ///
    /// A field width specification in the target stream is respected
    /// (`std::ios_base::width()`).
    ///
    template<class C, class T>
    friend auto operator<<(std::basic_ostream<C, T>&, SuperInt) -> std::basic_ostream<C, T>&;

private:
    // Value bits (not including the sign bit) of the two's complement representation of the
    // stored value.
    //
    uint_type m_value = 0;

    // When false, the represented value is `m_value`. When true, the represented value is
    // `m_value - 2**N`, where `N` is the number of value bits in `uint_type`.
    //
    bool m_sign_bit = false;
};








// Implementation


constexpr auto SuperInt::min() noexcept -> SuperInt
{
    SuperInt i;
    i.m_sign_bit = true;
    return i;
}


constexpr auto SuperInt::max() noexcept -> SuperInt
{
    SuperInt i;
    i.m_value = uint_lim_type::max();
    return i;
}


template<class T> constexpr SuperInt::SuperInt(T value) noexcept
{
    // C++11 (through its inclusion of C99) guarantees that the largest unsigned type has at
    // least as many value bits as any standard signed type (see C99+TC3 section 6.2.6.2
    // paragraph 2). This means that the following conversion to two's complement
    // representation can throw away at most the sign bit, which is fine, because we handle
    // the sign bit separately.
    //
    m_value = uint_type(value);
    using lim_t = std::numeric_limits<T>;
    if constexpr (lim_t::is_signed)
        m_sign_bit = (value < 0);
}


constexpr SuperInt::SuperInt(uint_type value, bool sign_bit) noexcept
    : m_value(value)
    , m_sign_bit(sign_bit)
{
}


template<class T> constexpr bool SuperInt::cast_has_overflow() const noexcept
{
    using lim_t = std::numeric_limits<T>;
    if (*this < SuperInt(lim_t::min()))
        return true;
    if (*this > SuperInt(lim_t::max()))
        return true;
    return false;
}


template<class T> constexpr bool SuperInt::get_as(T& v) const noexcept
{
    // Ensure that the value represented by `*this` can also be represented in `T`.
    if (cast_has_overflow<T>())
        return false;
    if (m_sign_bit) {
        // The conversion from two's complement to the native representation of negative
        // values below requires no assumptions beyond what is guaranteed by C++11. The
        // unsigned result of `~m_value` is guaranteed to be representable as a non-negative
        // value in `T`, which is a signed type in this case, and therefore it is also
        // guaranteed to be representable in `promoted`.
        //
        // To see that `~m_value` is representable in `T`, first note that `*this >=
        // SuperInt(lim_t::min())` where `lim_t` is `std::numeric_limits<T>`, which implies
        // that `m_value >= uint_type(lim_t::min())` (see definition of `operator<(SuperInt,
        // SuperInt)`), which in turn implies that `~m_value <= uint_lim_type::max() -
        // uint_type(lim_t::min())`. From C99+TC3 section 6.2.6.2 paragraph 2, we know that
        // `uint_lim_type::max() - uint_type(lim_t::min()) <= lim_t::max()`, which then
        // implies that `~m_value <= lim_t::max()`.
        //
        using promoted = decltype(+std::declval<T>());
        v = T(-1 - promoted(~m_value));
    }
    else {
        v = T(m_value);
    }
    return true;
}


constexpr auto SuperInt::operator+() const noexcept -> SuperInt
{
    return *this;
}


constexpr auto SuperInt::operator-() const noexcept -> SuperInt
{
    return SuperInt() - *this;
}


constexpr auto SuperInt::operator~() const noexcept -> SuperInt
{
    return { uint_type(~m_value), !m_sign_bit };
}


constexpr auto SuperInt::operator+(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i += other;
    return i;
}


constexpr auto SuperInt::operator-(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i -= other;
    return i;
}


constexpr auto SuperInt::operator*(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i *= other;
    return i;
}


constexpr auto SuperInt::operator/(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i /= other;
    return i;
}


constexpr auto SuperInt::operator%(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i %= other;
    return i;
}


constexpr auto SuperInt::operator&(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i &= other;
    return i;
}


constexpr auto SuperInt::operator|(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i |= other;
    return i;
}


constexpr auto SuperInt::operator^(SuperInt other) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i ^= other;
    return i;
}


constexpr auto SuperInt::operator<<(int n) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i <<= n;
    return i;
}


constexpr auto SuperInt::operator>>(int n) const noexcept -> SuperInt
{
    SuperInt i = *this;
    i >>= n;
    return i;
}


constexpr auto SuperInt::operator+=(SuperInt other) noexcept -> SuperInt&
{
    SuperInt a = *this, b = other;
    m_value = a.m_value + b.m_value;
    bool carry = (m_value < a.m_value);
    m_sign_bit = ((a.m_sign_bit != b.m_sign_bit) != carry);
    return *this;
}


constexpr auto SuperInt::operator-=(SuperInt other) noexcept -> SuperInt&
{
    SuperInt a = *this, b = other;
    m_value = a.m_value - b.m_value;
    bool borrow = (m_value > a.m_value);
    m_sign_bit = ((a.m_sign_bit != b.m_sign_bit) != borrow);
    return *this;
}


constexpr auto SuperInt::operator*=(SuperInt other) noexcept -> SuperInt&
{
    SuperInt a = *this, b = other;
    int msb_pos = SuperInt::digits - 1;
    uint_type a_1 = uint_type(a.m_value & 1);
    uint_type b_1 = uint_type(b.m_value & 1);
    uint_type a_2 = (uint_type(a.m_sign_bit) << msb_pos) | (a.m_value >> 1);
    uint_type b_2 = (uint_type(b.m_sign_bit) << msb_pos) | (b.m_value >> 1);
    uint_type v = ((a_2 * b_2) << 1) + a_2 * b_1 + a_1 * b_2;
    m_value = (v << 1) | (a_1 * b_1);
    m_sign_bit = (v >> msb_pos != 0);
    return *this;
}


constexpr auto SuperInt::operator/=(SuperInt other) noexcept -> SuperInt&
{
    if (ARCHON_LIKELY(!divide_with_overflow_detect(other)))
        return *this;
    // Overflow can only happen with two negative values, and only when the dividend is
    // equal to \ref min() and the divisor is -1. In this case, the result, whose
    // representation is reduced modulo 2**N, is the maximally negative value, where N is
    // the numbner of value bits.
    ARCHON_ASSERT(*this == min());
    ARCHON_ASSERT(other == SuperInt(-1));
    *this = min();
    return *this;
}


constexpr auto SuperInt::operator%=(SuperInt other) noexcept -> SuperInt&
{
    SuperInt v = *this;
    if (ARCHON_LIKELY(!v.divide_with_overflow_detect(other)))
        return *this -= v * other;
    // Overflow can only happen with two negative values, and only when the dividend is
    // equal to \ref min() and the divisor is -1. In this case, the result is zero, as
    // division by -1 can leave no remainder.
    ARCHON_ASSERT(*this == min());
    ARCHON_ASSERT(other == SuperInt(-1));
    *this = {};
    return *this;
}


constexpr auto SuperInt::operator&=(SuperInt other) noexcept -> SuperInt&
{
    m_value &= other.m_value;
    m_sign_bit = (m_sign_bit && other.m_sign_bit);
    return *this;
}


constexpr auto SuperInt::operator|=(SuperInt other) noexcept -> SuperInt&
{
    m_value |= other.m_value;
    m_sign_bit = (m_sign_bit || other.m_sign_bit);
    return *this;
}


constexpr auto SuperInt::operator^=(SuperInt other) noexcept -> SuperInt&
{
    m_value ^= other.m_value;
    m_sign_bit = (m_sign_bit != other.m_sign_bit);
    return *this;
}


constexpr auto SuperInt::operator<<=(int n) noexcept -> SuperInt&
{
    if (ARCHON_LIKELY(n > 0)) {
        m_sign_bit = bool((m_value >> (digits - n)) & 1);
        m_value <<= n;
    }
    return *this;
}


constexpr auto SuperInt::operator>>=(int n) noexcept -> SuperInt&
{
    if (ARCHON_LIKELY(n > 0)) {
        m_value >>= n;
        m_value |= uint_type(m_sign_bit) << (digits - n);
        m_sign_bit = false;
    }
    return *this;
}


constexpr bool SuperInt::operator==(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    return (a.m_value == b.m_value && a.m_sign_bit == b.m_sign_bit);
}


constexpr bool SuperInt::operator!=(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    return !(a == b);
}


constexpr bool SuperInt::operator<(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    if (a.m_sign_bit > b.m_sign_bit)
        return true;
    if (a.m_sign_bit == b.m_sign_bit) {
        if (a.m_value < b.m_value)
            return true;
    }
    return false;
}


constexpr bool SuperInt::operator<=(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    return !(b < a);
}


constexpr bool SuperInt::operator>(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    return (b < a);
}


constexpr bool SuperInt::operator>=(SuperInt other) const noexcept
{
    SuperInt a = *this, b = other;
    return !(a < b);
}


constexpr bool SuperInt::add_with_overflow_detect(SuperInt other) noexcept
{
    SuperInt sum = *this + other;
    bool carry = (sum.m_value < m_value);
    bool overflow = (m_sign_bit == other.m_sign_bit && m_sign_bit != carry);
    if (ARCHON_LIKELY(!overflow)) {
        *this = sum;
        return false;
    }
    return true;
}


constexpr bool SuperInt::subtract_with_overflow_detect(SuperInt other) noexcept
{
    SuperInt diff = *this - other;
    bool borrow = (diff.m_value > m_value);
    bool overflow = (m_sign_bit != other.m_sign_bit && m_sign_bit == borrow);
    if (ARCHON_LIKELY(!overflow)) {
        *this = diff;
        return false;
    }
    return true;
}


constexpr bool SuperInt::multiply_with_overflow_detect(SuperInt other) noexcept
{
    // Some useful facts:
    //
    // In general, from a mathematical point of view, if X is an integer, then -X = ~X + 1
    // (mod 2**N) for any non-negative integer N (number of bits).
    //
    // Now, let A and B be two super integers (instances of `SuperInt`). Then let `a` and
    // `b` refer to the `m_value` properties of A and B respectively. Finally, let `max` be
    // the maximum value for the type of `a` and `b`.
    //
    // If A is non-negative, then the value of A is equal to `a`.
    //
    // If A is negative, then the negative of the value of A is equal to `~a + 1`, and the
    // sum does not overflow.
    //
    // It follows then, that if A is non-negative and B is negative, the product is
    // representable as a super integer if, and only if `a * (~b + 1) <= max + 1`. We can
    // rearrange this as `~b <= (max - (a - 1)) / a` in order to avoid overflow, provided
    // that A is not zero.
    //
    // In a similar fashion, if both A and B are negative, the product is representable as a
    // super integer if, and only if `(~a + 1) * (~b + 1) <= max`. We can rearrange this as
    // `~a <= max - 1 && ~b <= max - 1 - ~a && ~a <= (max - 1 - ~a - ~b) / ~b` in order to
    // avoid overflow, provided that B is not equal to -1.
    //
    uint_type max = uint_lim_type::max();
    uint_type a = m_value;
    uint_type b = other.m_value;
    bool good = false;
    if (ARCHON_LIKELY(!m_sign_bit)) {
        if (ARCHON_LIKELY(!other.m_sign_bit)) {
            // Case: non-negative * non-negative
            good = (b == 0 || +a <= max / b);
        }
        else {
            // Case: non-negative * negative
            good = (a == 0 || ~b <= (max - (a - 1)) / a);
        }
    }
    else {
        if (ARCHON_LIKELY(!other.m_sign_bit)) {
            // Case: negative * non-negative
            good = (b == 0 || ~a <= (max - (b - 1)) / b);
        }
        else {
            // Case: negative * negative
            if (ARCHON_LIKELY(~b != 0)) {
                good = (~a <= max - 1 && ~b <= max - 1 - ~a && ~a <= (max - 1 - ~a - ~b) / ~b);
            }
            else {
                good = (a != 0);
            }
        }
    }
    if (ARCHON_LIKELY(good)) {
        *this *= other;
        return false; // No overflow
    }
    return true; // Overflow
}


constexpr bool SuperInt::divide_with_overflow_detect(SuperInt other) noexcept
{
    SuperInt a = *this, b = other;
    if (ARCHON_LIKELY(!a.m_sign_bit)) {
        if (ARCHON_LIKELY(!b.m_sign_bit)) {
            // Non-negative dividend, non-negative divisor
            m_value = uint_type(a.m_value / b.m_value);
            return false;
        }
        // Non-negative dividend, negative divisor
        if (ARCHON_LIKELY(b.m_value > 0)) {
            uint_type result = uint_type(a.m_value / (~b.m_value + 1));
            m_value = uint_type(~result + 1);
            m_sign_bit = (result > 0);
            return false;
        }
        // Maximally negative divisor
        m_value = 0;
        return false;
    }
    uint_type max_uint = uint_lim_type::max();
    if (ARCHON_LIKELY(!b.m_sign_bit)) {
        // Negative dividend, non-negative divisor
        if (ARCHON_LIKELY(a.m_value > 0)) {
            uint_type result = uint_type((~a.m_value + 1) / b.m_value);
            m_value = uint_type(~result + 1);
            m_sign_bit = (result > 0);
            return false;
        }
        // Maximally negative dividend
        uint_type result = uint_type(max_uint / b.m_value);
        if (max_uint - result * b.m_value >= b.m_value - 1) {
            if (result == max_uint) {
                m_sign_bit = true;
                return false;
            }
            ++result;
        }
        m_value = uint_type(~result + 1);
        m_sign_bit = (result > 0);
        return false;
    }
    // Negative dividend, negative divisor
    if (ARCHON_LIKELY(a.m_value > 0)) {
        if (ARCHON_LIKELY(b.m_value > 0)) {
            m_value = uint_type((~a.m_value + 1) / (~b.m_value + 1));
            m_sign_bit = false;
            return false;
        }
        // Maximally negative divisor
        m_value = 0;
        m_sign_bit = false;
        return false;
    }
    if (ARCHON_LIKELY(b.m_value > 0)) {
        // Maximally negative dividend
        uint_type b_2 = uint_type(~b.m_value + 1);
        uint_type result = uint_type(max_uint / b_2);
        if (max_uint - result * b_2 >= b_2 - 1) {
            if (result == max_uint)
                return true; // Overflow
            ++result;
        }
        m_value = result;
        m_sign_bit = false;
        return false;
    }
    // Maximally negative dividend and divisor
    m_value = 1;
    m_sign_bit = false;
    return false;
}


constexpr auto SuperInt::get_value() const noexcept -> uint_type
{
    return m_value;
}


constexpr bool SuperInt::get_sign_bit() const noexcept
{
    return m_sign_bit;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, SuperInt i) -> std::basic_ostream<C, T>&
{
    std::array<C, 64> seed_memory;
    return core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<C, T>& helper) {
        core::BasicCharMapper mapper(out); // Throws
        core::BasicIntegerFormatter formatter(mapper);
        using uint_type = SuperInt::uint_type;
        if (i.m_sign_bit) {
            uint_type max = uint_type(-1);
            int last_digit_1 = int(max % 10);
            uint_type other_digits_1 = max / 10;
            // Add one to max value
            ++last_digit_1;
            if (last_digit_1 == 10) {
                last_digit_1 = 0;
                ++other_digits_1;
            }
            int last_digit_2 = int(i.m_value % 10);
            uint_type other_digits_2 = i.m_value / 10;
            // Subtract m_value from max + 1
            last_digit_1 -= last_digit_2;
            other_digits_1 -= other_digits_2;
            if (last_digit_1 < 0) {
                last_digit_1 += 10;
                --other_digits_1;
            }
            helper.put(mapper.widen('-')); // Throws
            if (other_digits_1 > 0)
                helper.write(formatter.format_dec(other_digits_1)); // Throws
            helper.write(formatter.format_dec(last_digit_1)); // Throws
        }
        else {
            helper.write(formatter.format_dec(i.m_value)); // Throws
        }
    }, core::Span(seed_memory)); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SUPER_INT_HPP
