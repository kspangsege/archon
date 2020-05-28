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

#ifndef ARCHON_X_BASE_X_SUPER_INT_HPP
#define ARCHON_X_BASE_X_SUPER_INT_HPP

/// \file


#include <cstdint>
#include <limits>
#include <utility>
#include <array>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/stream_output.hpp>
#include <archon/base/integer_formatter.hpp>


namespace archon::base {


/// \brief Integer type capable of holding values of all integer types.
///
/// This class presents a signed integer type that is capable of uniquely
/// representing the values of all fundamental signed and unsigned integer
/// types. It uses the two's complement representation of negative values. The
/// representations of results of arithmetic operations are reduced modulo
/// 2**(N+1) where N is the number of value bits (\ref digits). The results of
/// arithmetic operations that overflow are therefore well defined, even when
/// they involve negative values.
///
class SuperInt {
private:
    using val_uint = std::uintmax_t;
    using lim_uint = std::numeric_limits<val_uint>;

public:
    /// \brief Number of value bits.
    ///
    /// Number of value bits (excluding the sign bit).
    ///
    static constexpr int digits = lim_uint::digits;

    static constexpr SuperInt min() noexcept;
    static constexpr SuperInt max() noexcept;

    constexpr SuperInt() noexcept = default;
    template<class T> explicit constexpr SuperInt(T value) noexcept;

    template<class T> constexpr bool cast_has_overflow() const noexcept;

    template<class T> constexpr bool get_as(T&) const noexcept;

    constexpr SuperInt operator+(SuperInt) const noexcept;
    constexpr SuperInt operator-(SuperInt) const noexcept;
    constexpr SuperInt operator*(SuperInt) const noexcept;
    constexpr SuperInt operator/(SuperInt) const noexcept;

    constexpr SuperInt& operator+=(SuperInt) noexcept;
    constexpr SuperInt& operator-=(SuperInt) noexcept;
    constexpr SuperInt& operator*=(SuperInt) noexcept;
    constexpr SuperInt& operator/=(SuperInt) noexcept;

    constexpr bool operator==(SuperInt) const noexcept;
    constexpr bool operator!=(SuperInt) const noexcept;
    constexpr bool operator<(SuperInt) const noexcept;
    constexpr bool operator<=(SuperInt) const noexcept;
    constexpr bool operator>(SuperInt) const noexcept;
    constexpr bool operator>=(SuperInt) const noexcept;

    constexpr bool add_with_overflow_detect(SuperInt) noexcept;
    constexpr bool subtract_with_overflow_detect(SuperInt) noexcept;
    constexpr bool multiply_with_overflow_detect(SuperInt) noexcept;
    constexpr bool divide_with_overflow_detect(SuperInt) noexcept;

    /// \brief Format integer value.
    ///
    /// Formatting occurs as in the C locale, irrespective ofthe actual
    /// locale. The formatting process ignores all formatting flags (e.g., it
    /// ignores `std::ios_base::hex`).
    ///
    /// Field width specifications in the target stream are respected.
    ///
    template<class C, class T>
    friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>&, SuperInt);

private:
    // Value bits (not including the sign bit) of the two's complement
    // representation of the stored value.
    //
    val_uint m_value = 0;

    // When false, the represented value is `m_value`. When true, the
    // represented value is `m_value - 2**N`, where `N` is the number of value
    // bits in `val_uint`.
    //
    bool m_sign_bit = false;
};








// Implementation


constexpr SuperInt SuperInt::min() noexcept
{
    SuperInt i;
    i.m_sign_bit = true;
    return i;
}


constexpr SuperInt SuperInt::max() noexcept
{
    SuperInt i;
    i.m_value = lim_uint::max();
    return i;
}


template<class T> constexpr SuperInt::SuperInt(T value) noexcept
{
    // C++11 (through its inclusion of C99) guarantees that the
    // largest unsigned type has at least as many value bits as any
    // standard signed type (see C99+TC3 section 6.2.6.2 paragraph
    // 2). This means that the following conversion to two's
    // complement representation can throw away at most the sign bit,
    // which is fine, because we handle the sign bit separately.
    //
    m_value = val_uint(value);
    using lim_t = std::numeric_limits<T>;
    if constexpr (lim_t::is_signed)
        m_sign_bit = (value < 0);
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
    // Ensure that the value represented by `*this` can also be represented in
    // `T`.
    if (cast_has_overflow<T>())
        return false;
    if (m_sign_bit) {
        // The conversion from two's complement to the native representation of
        // negative values below requires no assumptions beyond what is
        // guaranteed by C++11. The unsigned result of `~m_value` is guaranteed
        // to be representable as a non-negative value in `T`, which is a signed
        // type in this case, and therefore it is also guaranteed to be
        // representable in `promoted`.
        //
        // To see that `~m_value` is representable in `T`, first note that
        // `*this >= SuperInt(lim_t::min())` where `lim_t` is
        // `std::numeric_limits<T>`, which implies that `m_value >=
        // val_uint(lim_t::min())` (see definition of `operator<(SuperInt,
        // SuperInt)`), which in turn implies that `~m_value <= lim_uint::max()
        // - val_uint(lim_t::min())`. From C99+TC3 section 6.2.6.2 paragraph 2,
        // we know that `lim_uint::max() - val_uint(lim_t::min()) <=
        // lim_t::max()`, which then implies that `~m_value <= lim_t::max()`.
        //
        using promoted = decltype(std::declval<T>() + 0);
        v = T(-1 - promoted(~m_value));
    }
    else {
        v = T(m_value);
    }
    return true;
}


constexpr SuperInt SuperInt::operator+(SuperInt other) const noexcept
{
    SuperInt i = *this;
    i += other;
    return i;
}


constexpr SuperInt SuperInt::operator-(SuperInt other) const noexcept
{
    SuperInt i = *this;
    i -= other;
    return i;
}


constexpr SuperInt SuperInt::operator*(SuperInt other) const noexcept
{
    SuperInt i = *this;
    i *= other;
    return i;
}


constexpr SuperInt SuperInt::operator/(SuperInt other) const noexcept
{
    SuperInt i = *this;
    i /= other;
    return i;
}


constexpr SuperInt& SuperInt::operator+=(SuperInt other) noexcept
{
    SuperInt a = *this, b = other;
    m_value = a.m_value + b.m_value;
    bool carry = (m_value < a.m_value);
    m_sign_bit = ((a.m_sign_bit != b.m_sign_bit) != carry);
    return *this;
}


constexpr SuperInt& SuperInt::operator-=(SuperInt other) noexcept
{
    SuperInt a = *this, b = other;
    m_value = a.m_value - b.m_value;
    bool borrow = (m_value > a.m_value);
    m_sign_bit = ((a.m_sign_bit != b.m_sign_bit) != borrow);
    return *this;
}


constexpr SuperInt& SuperInt::operator*=(SuperInt other) noexcept
{
    SuperInt a = *this, b = other;
    int msb_pos = SuperInt::digits - 1;
    val_uint a_1 = val_uint(a.m_value & 1);
    val_uint b_1 = val_uint(b.m_value & 1);
    val_uint a_2 = (val_uint(a.m_sign_bit) << msb_pos) | (a.m_value >> 1);
    val_uint b_2 = (val_uint(b.m_sign_bit) << msb_pos) | (b.m_value >> 1);
    val_uint v = ((a_2 * b_2) << 1) + a_2 * b_1 + a_1 * b_2;
    m_value = (v << 1) | (a_1 * b_1);
    m_sign_bit = (v >> msb_pos != 0);
    return *this;
}


constexpr SuperInt& SuperInt::operator/=(SuperInt other) noexcept
{
    if (ARCHON_LIKELY(!divide_with_overflow_detect(other)))
        return *this;
    // Overflow can only happen with two negative values, and only when the
    // dividend is equal to \ref min() and the divisor is -1. In this case, the
    // result, whose representation is reduced modulo 2**N, is the maximally
    // negative value, where N is the numbner of value bits.
    ARCHON_ASSERT(*this == min());
    ARCHON_ASSERT(other == SuperInt(-1));
    *this = min();
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
    val_uint max_uint = lim_uint::max();
    val_uint a = m_value;
    val_uint b = other.m_value;
    if (ARCHON_LIKELY(!m_sign_bit)) {
        if (ARCHON_LIKELY(!other.m_sign_bit)) {
            if (ARCHON_UNLIKELY(a > max_uint / b ))
                return true;
        }
        else {
            if (ARCHON_UNLIKELY(b > 0 && b > (max_uint - b + 1) / ~a))
                return true;
        }
    }
    else {
        if (ARCHON_LIKELY(!other.m_sign_bit)) {
            if (ARCHON_UNLIKELY(a > 0 && a > (max_uint - a + 1) / ~b))
                return true;
        }
        else {
            val_uint a_2 = ~a;
            val_uint b_2 = ~b;
            val_uint acc = 1;
            if (ARCHON_UNLIKELY(a_2 > max_uint - acc))
                return true;
            acc += a_2;
            if (ARCHON_UNLIKELY(b_2 > max_uint - acc))
                return true;
            acc += b_2;
            if (ARCHON_UNLIKELY(a_2 > (max_uint - acc) / b_2))
                return true;
        }
    }
    *this *= other;
    return false;
}


constexpr bool SuperInt::divide_with_overflow_detect(SuperInt other) noexcept
{
    SuperInt a = *this, b = other;
    if (ARCHON_LIKELY(!a.m_sign_bit)) {
        if (ARCHON_LIKELY(!b.m_sign_bit)) {
            // Non-negative dividend, non-negative divisor
            m_value = val_uint(a.m_value / b.m_value);
            return false;
        }
        // Non-negative dividend, negative divisor
        if (ARCHON_LIKELY(b.m_value > 0)) {
            val_uint result = val_uint(a.m_value / (~b.m_value + 1));
            m_value = val_uint(~result + 1);
            m_sign_bit = (result > 0);
            return false;
        }
        // Maximally negative divisor
        m_value = 0;
        return false;
    }
    val_uint max_uint = lim_uint::max();
    if (ARCHON_LIKELY(!b.m_sign_bit)) {
        // Negative dividend, non-negative divisor
        if (ARCHON_LIKELY(a.m_value > 0)) {
            val_uint result = val_uint((~a.m_value + 1) / b.m_value);
            m_value = val_uint(~result + 1);
            m_sign_bit = (result > 0);
            return false;
        }
        // Maximally negative dividend
        val_uint result = val_uint(max_uint / b.m_value);
        if (max_uint - result * b.m_value >= b.m_value - 1) {
            if (result == max_uint) {
                m_sign_bit = true;
                return false;
            }
            ++result;
        }
        m_value = val_uint(~result + 1);
        m_sign_bit = (result > 0);
        return false;
    }
    // Negative dividend, negative divisor
    if (ARCHON_LIKELY(a.m_value > 0)) {
        if (ARCHON_LIKELY(b.m_value > 0)) {
            val_uint result = val_uint((~a.m_value + 1) / (~b.m_value + 1));
            m_value = val_uint(~result + 1);
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
        val_uint b_2 = val_uint(~b.m_value + 1);
        val_uint result = val_uint(max_uint / b_2);
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


template<class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out, SuperInt i)
{
    std::array<C, 64> seed_memory;
    return base::ostream_sentry(out, [&](base::BasicStreamOutputHelper<C, T>& helper) {
        base::BasicCharMapper mapper(out); // Throws
        base::BasicIntegerFormatter formatter(mapper);
        using val_uint = SuperInt::val_uint;
        if (i.m_sign_bit) {
            val_uint max = val_uint(-1);
            int last_digit_1 = int(max % 10);
            val_uint other_digits_1 = max / 10;
            // Add one to max value
            ++last_digit_1;
            if (last_digit_1 == 10) {
                last_digit_1 = 0;
                ++other_digits_1;
            }
            int last_digit_2 = int(i.m_value % 10);
            val_uint other_digits_2 = i.m_value / 10;
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
    }, base::Span(seed_memory)); // Throws
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_SUPER_INT_HPP
