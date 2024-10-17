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

#ifndef ARCHON_X_CORE_X_MUL_PREC_INT_HPP
#define ARCHON_X_CORE_X_MUL_PREC_INT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <concepts>
#include <algorithm>
#include <array>
#include <string_view>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer_traits.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/as_int.hpp>


namespace archon::core {


/// \brief Multiple precision integer type.
///
/// This class offers a multiple precision integer type that conforms to \ref
/// Concept_Archon_Core_Integer.
///
/// A value of this type is constructed from a fixed number of parts. The type of the parts
/// is specified by \p T and the number of parts is specified by \p N. Each part is an
/// unsigned integer that contributes all its value bits to the state of the multiple
/// precision integer type. The first part contributes the least significant bits (little
/// endian).
///
/// The multiple precision integer type can be signed or unsigned. It is signed if `true` is
/// passed for \p S. Otherwise it is unsigned.
///
/// Formatting and parsing is supported through use of the stream output and input operators
/// (`operator<<` of `std::basic_ostream` and `operator>>` of `std::basic_istream`). These
/// use \ref core::BasicIntegerFormatter and \ref core::BasicIntegerParser
/// respectively. Robust value parsing through \ref core::BasicValueParser is also
/// supported.
///
/// \p T must be one of the fundamental unsigned integer types, and it cannot be `bool`.
///
template<class T, int N, bool S> class MulPrecInt {
public:
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_unsigned_v<T>);
    static_assert(N > 0);
    static_assert(N <= core::int_max<int>() / core::int_width<T>());

    using part_type = T;

    static constexpr int num_parts = N;
    static constexpr bool is_signed = S;

    /// \brief Number of value bits plus one if signed.
    ///
    /// This is the number of value bits plus one for the sign bit if `is_signed` is true.
    ///
    static constexpr int width = num_parts * core::int_width<part_type>();

    /// \brief Number of value bits.
    ///
    /// This is the number of value bits (excluding the sign bit).
    ///
    static constexpr int digits = width - (is_signed ? 1 : 0);

    using parts_type = std::array<part_type, num_parts>;

    constexpr MulPrecInt() noexcept = default;
    constexpr explicit MulPrecInt(parts_type parts) noexcept;
    template<std::integral I> constexpr explicit MulPrecInt(I val) noexcept;

    template<std::integral I> constexpr explicit operator I() const noexcept;

    constexpr auto get_parts() const noexcept -> parts_type;

    constexpr bool is_nonneg() const noexcept;

    constexpr auto operator+() const noexcept -> MulPrecInt;
    constexpr auto operator-() const noexcept -> MulPrecInt;
    constexpr auto operator~() const noexcept -> MulPrecInt;

    constexpr auto operator+(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator-(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator*(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator/(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator%(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator&(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator|(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator^(MulPrecInt) const noexcept -> MulPrecInt;
    constexpr auto operator<<(int) const noexcept -> MulPrecInt;
    constexpr auto operator>>(int) const noexcept -> MulPrecInt;

    constexpr auto operator+=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator-=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator*=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator/=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator%=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator&=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator|=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator^=(MulPrecInt) noexcept -> MulPrecInt&;
    constexpr auto operator<<=(int) noexcept -> MulPrecInt&;
    constexpr auto operator>>=(int) noexcept -> MulPrecInt&;

    constexpr bool operator==(MulPrecInt) const noexcept;
    constexpr bool operator!=(MulPrecInt) const noexcept;
    constexpr bool operator< (MulPrecInt) const noexcept;
    constexpr bool operator<=(MulPrecInt) const noexcept;
    constexpr bool operator> (MulPrecInt) const noexcept;
    constexpr bool operator>=(MulPrecInt) const noexcept;

    struct DivMod {
        MulPrecInt quot;
        MulPrecInt rem;
    };

    constexpr auto divmod(MulPrecInt) const noexcept -> DivMod;

    static constexpr auto min() noexcept -> MulPrecInt;
    static constexpr auto max() noexcept -> MulPrecInt;

    constexpr int find_msb_pos() const noexcept;

private:
    parts_type m_parts = {};

    static constexpr int s_part_width = core::int_width<part_type>();
    static constexpr int s_subpart_width = s_part_width / 2;
    static_assert(s_subpart_width > 0);
    static constexpr int s_num_subparts = core::int_div_round_up(width, s_subpart_width);
    static constexpr part_type s_subpart_mask = core::int_mask<part_type>(s_subpart_width);

    using subparts_type = std::array<T, s_num_subparts>;

    constexpr auto unsigned_div(MulPrecInt other) const noexcept -> DivMod;
    constexpr bool unsigned_less(MulPrecInt) const noexcept;

    static constexpr auto scatter(parts_type) noexcept -> subparts_type;
    static constexpr auto gather(subparts_type) noexcept -> parts_type;

    static constexpr bool partial_add(part_type& lval, part_type rval, bool carry) noexcept;
    static constexpr bool partial_sub(part_type& lval, part_type rval, bool carry) noexcept;
    static constexpr auto unsigned_short_div(subparts_type& a, part_type b) noexcept -> part_type;
    static constexpr auto unsigned_long_div(subparts_type& a, const part_type* b, int n) noexcept -> subparts_type;
};


/// \brief Format multiple precision integer value.
///
/// Formatting is carried out as if by `out << core::as_int(val)`.
///
template<class C, class T, class U, int N, bool S>
auto operator<<(std::basic_ostream<C, T>& out, const core::MulPrecInt<U, N, S>& val) -> std::basic_ostream<C, T>&;


/// \brief Parse multiple precision integer value.
///
/// Parsing is carried out as if by `in >> core::as_int(var)`.
///
template<class C, class T, class U, int N, bool S>
auto operator>>(std::basic_istream<C, T>& in, core::MulPrecInt<U, N, S>& var) -> std::basic_istream<C, T>&;


/// \brief Parse multiple precision integer value.
///
/// Parsing is carried out as if by `src.delegate(core::as_int(var))`.
///
template<class C, class T, class U, int N, bool S>
bool parse_value(core::BasicValueParserSource<C, T>& src, core::MulPrecInt<U, N, S>& var);


template<class T, int N, bool S> struct IntegerTraits<core::MulPrecInt<T, N, S>> {
    using int_type      = core::MulPrecInt<T, N, S>;
    using unsigned_type = core::MulPrecInt<T, N, false>;

    static constexpr bool is_specialized = true;
    static constexpr int  num_value_bits = int_type::digits;
    static constexpr bool is_signed      = int_type::is_signed;

    static constexpr auto min() noexcept -> int_type;
    static constexpr auto max() noexcept -> int_type;

    using part_type = core::unsigned_type<core::promoted_type<typename int_type::part_type>>;
    static constexpr int num_parts = core::int_div_round_up(int_type::width, core::int_width<part_type>());
    using parts_type = std::array<part_type, num_parts>;
    static constexpr auto get_parts(int_type) noexcept -> parts_type;
    static constexpr auto from_parts(parts_type) noexcept -> int_type;

    static constexpr bool has_divmod = true;
    using DivMod = typename int_type::DivMod;
    static constexpr auto divmod(int_type, int_type) noexcept -> DivMod;

    static constexpr bool has_find_msb_pos = true;
    static constexpr int find_msb_pos(int_type) noexcept;
};








// Implementation


template<class T, int N, bool S>
constexpr MulPrecInt<T, N, S>::MulPrecInt(parts_type parts) noexcept
    : m_parts(parts)
{
}


template<class T, int N, bool S>
template<std::integral  I> constexpr MulPrecInt<T, N, S>::MulPrecInt(I val) noexcept
    : MulPrecInt(core::int_cast_a<MulPrecInt>(val))
{
}


template<class T, int N, bool S>
template<std::integral I> constexpr MulPrecInt<T, N, S>::operator I() const noexcept
{
    return core::int_cast_a<I>(*this);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::get_parts() const noexcept -> parts_type
{
    return m_parts;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::is_nonneg() const noexcept
{
    if constexpr (is_signed) {
        return (m_parts[num_parts - 1] >> (s_part_width - 1)) == 0;
    }
    else {
        return true;
    }
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator+() const noexcept -> MulPrecInt
{
    return *this;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator-() const noexcept -> MulPrecInt
{
    return MulPrecInt() - *this;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator~() const noexcept -> MulPrecInt
{
    MulPrecInt res;
    for (int i = 0; i < num_parts; ++i)
        res.m_parts[i] = part_type(~m_parts[i]);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator+(MulPrecInt other) const noexcept -> MulPrecInt
{
    MulPrecInt res = *this;
    bool carry = false;
    for (int i = 0; i < num_parts; ++i)
        carry = partial_add(res.m_parts[i], other.m_parts[i], carry);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator-(MulPrecInt other) const noexcept -> MulPrecInt
{
    MulPrecInt res = *this;
    bool carry = false;
    for (int i = 0; i < num_parts; ++i)
        carry = partial_sub(res.m_parts[i], other.m_parts[i], carry);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator*(MulPrecInt other) const noexcept -> MulPrecInt
{
    subparts_type a = scatter(m_parts);
    subparts_type b = scatter(other.m_parts);
    subparts_type res = {};
    for (int i = 0; i < s_num_subparts; ++i) {
        part_type carry = 0;
        for (int j = 0; j < s_num_subparts - i; ++j) {
            auto v = a[i] * b[j] + res[i + j] + carry;
            res[i + j] = part_type(v & s_subpart_mask);
            carry = part_type(v >> s_subpart_width);
        }
    }
    return MulPrecInt(gather(res));
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator/(MulPrecInt other) const noexcept -> MulPrecInt
{
    return divmod(other).quot;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator%(MulPrecInt other) const noexcept -> MulPrecInt
{
    return divmod(other).rem;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator&(MulPrecInt other) const noexcept -> MulPrecInt
{
    MulPrecInt res;
    for (int i = 0; i < num_parts; ++i)
        res.m_parts[i] = part_type(m_parts[i] & other.m_parts[i]);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator|(MulPrecInt other) const noexcept -> MulPrecInt
{
    MulPrecInt res;
    for (int i = 0; i < num_parts; ++i)
        res.m_parts[i] = part_type(m_parts[i] | other.m_parts[i]);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator^(MulPrecInt other) const noexcept -> MulPrecInt
{
    MulPrecInt res;
    for (int i = 0; i < num_parts; ++i)
        res.m_parts[i] = part_type(m_parts[i] ^ other.m_parts[i]);
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator<<(int n) const noexcept -> MulPrecInt
{
    int m = n % width;
    int a = m / s_part_width;
    int b = m % s_part_width;
    MulPrecInt res;
    if (ARCHON_LIKELY(b != 0)) {
        res.m_parts[a] = part_type(m_parts[0] << b);
        for (int i = 1; i < num_parts - a; ++i)
            res.m_parts[a + i] = part_type((m_parts[i] << b) | (m_parts[i - 1] >> (s_part_width - b)));
    }
    else {
        for (int i = 0; i < num_parts - a; ++i)
            res.m_parts[a + i] = m_parts[i];
    }
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator>>(int n) const noexcept -> MulPrecInt
{
    int m = n % width;
    int a = m / s_part_width;
    int b = m % s_part_width;
    MulPrecInt res;
    if (ARCHON_LIKELY(b != 0)) {
        for (int i = 0; i < (num_parts - 1) - a; ++i)
            res.m_parts[i] = part_type((m_parts[a + i] >> b) | (m_parts[a + i + 1] << (s_part_width - b)));
        res.m_parts[(num_parts - 1) - a] = part_type(m_parts[num_parts - 1] >> b);
    }
    else {
        for (int i = 0; i < num_parts - a; ++i)
            res.m_parts[i] = m_parts[i + a];
    }
    return res;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator+=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this + other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator-=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this - other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator*=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this * other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator/=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this / other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator%=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this % other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator&=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this & other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator|=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this | other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator^=(MulPrecInt other) noexcept -> MulPrecInt&
{
    return (*this = *this ^ other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator<<=(int n) noexcept -> MulPrecInt&
{
    return (*this = *this << n);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::operator>>=(int n) noexcept -> MulPrecInt&
{
    return (*this = *this >> n);
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator==(MulPrecInt other) const noexcept
{
    for (int i = 0; i < num_parts; ++i) {
        if (ARCHON_LIKELY(m_parts[(num_parts - 1) - i] == other.m_parts[(num_parts - 1) - i]))
            continue;
        return false;
    }
    return true;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator!=(MulPrecInt other) const noexcept
{
    return !(*this == other);
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator<(MulPrecInt other) const noexcept
{
    bool a = unsigned_less(other);
    bool b = is_nonneg() == other.is_nonneg();
    return a == b;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator<=(MulPrecInt other) const noexcept
{
    return !(*this > other);
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator>(MulPrecInt other) const noexcept
{
    return other < *this;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::operator>=(MulPrecInt other) const noexcept
{
    return !(*this < other);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::divmod(MulPrecInt other) const noexcept -> DivMod
{
    if (is_nonneg()) {
        if (other.is_nonneg())
            return unsigned_div(other);
        auto res = unsigned_div(-other);
        return { -res.quot, res.rem };
    }
    auto neg = -*this;
    if (other.is_nonneg()) {
        auto res = neg.unsigned_div(other);
        return { -res.quot, -res.rem };
    }
    auto res = neg.unsigned_div(-other);
    return { res.quot, -res.rem };
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::min() noexcept -> MulPrecInt
{
    parts_type parts = {};
    if constexpr (is_signed)
        parts[num_parts - 1] = part_type(part_type(1) << (s_part_width - 1));
    return MulPrecInt(parts);
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::max() noexcept -> MulPrecInt
{
    parts_type parts = {};
    part_type max = core::int_max<part_type>();
    for (int i = 0; i < num_parts; ++i)
        parts[i] = max;
    if constexpr (is_signed)
        parts[num_parts - 1] = part_type(max / 2);
    return MulPrecInt(parts);
}


template<class T, int N, bool S>
constexpr int MulPrecInt<T, N, S>::find_msb_pos() const noexcept
{
    for (int i = 0; i < num_parts; ++i) {
        int j = num_parts - 1 - i;
        part_type part = m_parts[j];
        if (ARCHON_LIKELY(part != 0))
            return j * s_part_width + core::int_find_msb_pos(part);
    }
    return -1;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::unsigned_div(MulPrecInt other) const noexcept -> DivMod
{
    subparts_type subparts = scatter(m_parts);
    subparts_type other_subparts = scatter(other.m_parts);
    subparts_type rem = {};
    int n = s_num_subparts;
    while (n > 0 && other_subparts[n - 1] == 0)
        --n;
    if (ARCHON_LIKELY(n > 1)) {
        rem = unsigned_long_div(subparts, other_subparts.data(), n);
    }
    else {
        rem[0] = unsigned_short_div(subparts, other_subparts[0]);
    }
    return { MulPrecInt(gather(subparts)),  MulPrecInt(gather(rem)) };
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::unsigned_short_div(subparts_type& a, part_type b) noexcept -> part_type
{
    part_type rem = 0;
    for (int i = 0; i < s_num_subparts; ++i) {
        int j = (s_num_subparts - 1) - i;
        part_type dividend = part_type((rem << s_subpart_width) | a[j]);
        a[j] = dividend / b;
        rem = dividend % b;
    }
    return rem;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::unsigned_long_div(subparts_type& a, const part_type* b,
                                                      int n) noexcept -> subparts_type
{
    ARCHON_ASSERT(n > 1);
    ARCHON_ASSERT(n <= s_num_subparts);

    // The following is an implementation of "Algorithm D" in Knuth's TAOCP (section 4.3.1
    // "The Classical Algorithms" of section 4.3 "Multiple-Precision Arithmetic" in volume 2
    // "Seminumerical Algorithms" 3rd edition (Addison-Wesley, 1997) of "The Art of Computer
    // Programming" by Donald. E. Knuth).

    // Normalize
    int shift = (s_subpart_width - 1) - core::int_find_msb_pos(b[n - 1]);
    std::array<part_type, s_num_subparts + 1> u = {};
    for (int i = 0; i < s_num_subparts; ++i) {
        int j = s_num_subparts - i;
        u[j] = part_type(u[j] | (a[j - 1] >> (s_subpart_width - shift)));
        u[j - 1] = part_type((a[j - 1] << shift) & s_subpart_mask);
    }
    subparts_type v = {};
    v[n - 1] = part_type(b[n - 1] << shift);
    for (int i = 0; i < n - 1; ++i) {
        int j = n - 1 - i;
        v[j] = part_type(v[j] | (b[j - 1] >> (s_subpart_width - shift)));
        v[j - 1] = part_type((b[j - 1] << shift) & s_subpart_mask);
    }

    // Main loop
    int m = s_num_subparts - n;
    int j = m;
    subparts_type q = {};
    for (;;) {
        // Estimate next digit of quotient
        part_type e = part_type((u[j + n] << s_subpart_width) | u[j + n - 1]);
        part_type f = v[n - 1];
        part_type q_hat = e / f;
        part_type r_hat = e % f;
        part_type base = part_type(s_subpart_mask + 1);
        for (;;) { // Support for poor man's goto (goto not allowed in constexpr)
            if (ARCHON_LIKELY(q_hat < base && q_hat * v[n - 2] <= base * r_hat + u[j + n - 2]))
                break;
            --q_hat;
            r_hat += f;
            if (ARCHON_LIKELY(r_hat >= base))
                break;
            if (ARCHON_LIKELY(q_hat < base && q_hat * v[n - 2] <= base * r_hat + u[j + n - 2]))
                break;
            --q_hat;
            break;
        }

        // Multiply and subtract
        bool nonnegative = false;
        {
            std::array<part_type, s_num_subparts + 1> w = {};
            part_type carry = 0;
            for (int i = 0; i < n; ++i) {
                auto a = q_hat * v[i] + w[i] + carry;
                w[i] = part_type(a & s_subpart_mask);
                carry = part_type(a >> s_subpart_width);
            }
            w[n] = carry;
            carry = 0;
            for (int i = 0; i < n + 1; ++i) {
                auto a = u[j + i] - w[i] - carry;
                u[j + i] = part_type(a & s_subpart_mask);
                carry = part_type((a >> s_subpart_width) & 1);
            }
            nonnegative = (carry == 0);
        }

        // Add back
        for (;;) { // Support for poor man's goto (goto not allowed in constexpr)
            if (ARCHON_LIKELY(nonnegative))
                break;
            --q_hat;
            part_type carry = 0;
            for (int i = 0; i < n; ++i) {
                auto a = u[j + i] + v[i] + carry;
                u[j + i] = part_type(a & s_subpart_mask);
                carry = part_type(a >> s_subpart_width);
            }
            u[j + n] += carry;
            break;
        }

        q[j] = q_hat;

        if (ARCHON_LIKELY(j > 0)) {
            --j;
            continue;
        }
        break;
    }

    // Unnormalize
    a = q;
    subparts_type r = {};
    for (int i = 0; i < n - 1; ++i)
        r[i] = part_type((u[i] >> shift) | ((u[i + 1] << (s_subpart_width - shift)) & s_subpart_mask));
    r[n - 1] = part_type(u[n - 1] >> shift);
    return r;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::unsigned_less(MulPrecInt other) const noexcept
{
    for (int i = 0; i < num_parts; ++i) {
        if (ARCHON_LIKELY(m_parts[(num_parts - 1) - i] == other.m_parts[(num_parts - 1) - i]))
            continue;
        return m_parts[(num_parts - 1) - i] < other.m_parts[(num_parts - 1) - i];
    }
    return false;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::scatter(parts_type parts) noexcept -> subparts_type
{
    subparts_type subparts = {};
    int part_index = 0;
    part_type part = parts[0];
    int offset = 0;
    for (int i = 0; i < s_num_subparts; ++i) {
        part_type bits = part_type(part >> offset);
        offset += s_subpart_width;
        if (offset >= s_part_width) {
            part_index += 1;
            offset -= s_part_width;
            ARCHON_ASSERT(part_index < num_parts || i == s_num_subparts - 1);
            if (ARCHON_LIKELY(part_index < num_parts)) {
                part = parts[part_index];
                bits |= part << (s_subpart_width - offset);
            }
        }
        subparts[i] = part_type(bits & core::int_mask<part_type>(s_subpart_width));
    }
    return subparts;
}


template<class T, int N, bool S>
constexpr auto MulPrecInt<T, N, S>::gather(subparts_type subparts) noexcept -> parts_type
{
    parts_type parts = {};
    int part_index = 0;
    part_type part = 0;
    int offset = 0;
    for (int i = 0; i < s_num_subparts; ++i) {
        T subpart = subparts[i];
        part |= subpart << offset;
        offset += s_subpart_width;
        if (offset >= s_part_width) {
            ARCHON_ASSERT(part_index < num_parts);
            parts[part_index] = part;
            part_index += 1;
            offset -= s_part_width;
            part = part_type(subpart >> (s_subpart_width - offset));
        }
    }
    return parts;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::partial_add(part_type& lval, part_type rval, bool carry) noexcept
{
    part_type lval_2 = lval + rval;
    part_type lval_3 = lval_2 + carry;
    bool carry_2 = (lval_2 < lval || lval_3 < lval_2);
    lval = lval_3;
    return carry_2;
}


template<class T, int N, bool S>
constexpr bool MulPrecInt<T, N, S>::partial_sub(part_type& lval, part_type rval, bool carry) noexcept
{
    part_type lval_2 = lval - rval;
    part_type lval_3 = lval_2 - carry;
    bool carry_2 = (lval_2 > lval || lval_3 > lval_2);
    lval = lval_3;
    return carry_2;
}


template<class C, class T, class U, int N, bool S>
inline auto operator<<(std::basic_ostream<C, T>& out, const core::MulPrecInt<U, N, S>& val) ->
    std::basic_ostream<C, T>&
{
    return out << core::as_int(val); // Throws
}


template<class C, class T, class U, int N, bool S>
inline auto operator>>(std::basic_istream<C, T>& in, core::MulPrecInt<U, N, S>& var) -> std::basic_istream<C, T>&
{
    return in >> core::as_int(var); // Throws
}


template<class C, class T, class U, int N, bool S>
inline bool parse_value(core::BasicValueParserSource<C, T>& src, core::MulPrecInt<U, N, S>& var)
{
    return src.delegate(core::as_int(var)); // Throws
}


template<class T, int N, bool S>
constexpr auto IntegerTraits<core::MulPrecInt<T, N, S>>::min() noexcept -> int_type
{
    return int_type::min();
}


template<class T, int N, bool S>
constexpr auto IntegerTraits<core::MulPrecInt<T, N, S>>::max() noexcept -> int_type
{
    return int_type::max();
}


template<class T, int N, bool S>
constexpr auto IntegerTraits<core::MulPrecInt<T, N, S>>::get_parts(int_type val) noexcept -> parts_type
{
    if constexpr (std::is_same_v<part_type, typename int_type::part_type>) {
        return val.get_parts();
    }
    else {
        typename int_type::parts_type parts_1 = val.get_parts();
        parts_type parts_2 = {};
        constexpr bool sign_extend = is_signed;
        core::int_bit_copy<sign_extend>(parts_1, parts_2);
        return parts_2;
    }
}


template<class T, int N, bool S>
constexpr auto IntegerTraits<core::MulPrecInt<T, N, S>>::from_parts(parts_type parts) noexcept -> int_type
{
    if constexpr (std::is_same_v<part_type, typename int_type::part_type>) {
        return int_type(parts);
    }
    else {
        typename int_type::parts_type parts_2 = {};
        constexpr bool sign_extend = is_signed;
        core::int_bit_copy<sign_extend>(parts, parts_2);
        return int_type(parts_2);
    }
}


template<class T, int N, bool S>
constexpr int IntegerTraits<core::MulPrecInt<T, N, S>>::find_msb_pos(int_type val) noexcept
{
    return val.find_msb_pos();
}


template<class T, int N, bool S>
constexpr auto IntegerTraits<core::MulPrecInt<T, N, S>>::divmod(int_type a, int_type b) noexcept -> DivMod
{
    return a.divmod(b);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MUL_PREC_INT_HPP
