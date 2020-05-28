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


#include <array>

#include <archon/core/integer_traits.hpp>
#include <archon/core/endianness.hpp>
#include <archon/core/mul_prec_int.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


enum class TestEndianness { big, little, mixed };


template<TestEndianness E> struct Integer {
    using part_type = unsigned char;
    using mul_prec_type = core::MulPrecInt<part_type, 3, false>;
    using parts_type = mul_prec_type::parts_type;

    static constexpr TestEndianness endianness = E;

    parts_type m_parts = {};

    constexpr Integer() noexcept = default;

    explicit constexpr Integer(int val) noexcept
        : Integer(mul_prec_type(val))
    {
    }

    explicit constexpr operator int() const noexcept
    {
        return int(mul_prec_type(*this));
    }

    explicit constexpr Integer(mul_prec_type val) noexcept
    {
        parts_type parts = val.get_parts();
        switch (endianness) {
            case TestEndianness::big:
                m_parts[0] = parts[2];
                m_parts[1] = parts[1];
                m_parts[2] = parts[0];
                break;
            case TestEndianness::little:
                m_parts[0] = parts[0];
                m_parts[1] = parts[1];
                m_parts[2] = parts[2];
                break;
            case TestEndianness::mixed:
                m_parts[0] = parts[2];
                m_parts[1] = parts[0];
                m_parts[2] = parts[1];
                break;
        }
    }

    explicit constexpr operator mul_prec_type() const noexcept
    {
        parts_type parts_2 = {};
        switch (endianness) {
            case TestEndianness::big:
                parts_2[0] = m_parts[2];
                parts_2[1] = m_parts[1];
                parts_2[2] = m_parts[0];
                break;
            case TestEndianness::little:
                parts_2[0] = m_parts[0];
                parts_2[1] = m_parts[1];
                parts_2[2] = m_parts[2];
                break;
            case TestEndianness::mixed:
                parts_2[0] = m_parts[1];
                parts_2[1] = m_parts[2];
                parts_2[2] = m_parts[0];
                break;
        }
        return mul_prec_type(parts_2);
    }

    constexpr auto operator+() const noexcept -> Integer
    {
        return Integer(+mul_prec_type(*this));
    }

    constexpr auto operator-() const noexcept -> Integer
    {
        return Integer(-mul_prec_type(*this));
    }

    constexpr auto operator~() const noexcept -> Integer
    {
        return Integer(~mul_prec_type(*this));
    }

    constexpr auto operator+(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) + mul_prec_type(other));
    }

    constexpr auto operator-(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) - mul_prec_type(other));
    }

    constexpr auto operator*(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) * mul_prec_type(other));
    }

    constexpr auto operator/(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) / mul_prec_type(other));
    }

    constexpr auto operator%(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) % mul_prec_type(other));
    }

    constexpr auto operator&(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) & mul_prec_type(other));
    }

    constexpr auto operator|(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) | mul_prec_type(other));
    }

    constexpr auto operator^(Integer other) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) ^ mul_prec_type(other));
    }

    constexpr auto operator<<(int n) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) << n);
    }

    constexpr auto operator>>(int n) const noexcept -> Integer
    {
        return Integer(mul_prec_type(*this) >> n);
    }

    constexpr auto operator+=(Integer other) noexcept -> Integer&
    {
        return (*this = *this + other);
    }

    constexpr auto operator-=(Integer other) noexcept -> Integer&
    {
        return (*this = *this - other);
    }

    constexpr auto operator*=(Integer other) noexcept -> Integer&
    {
        return (*this = *this * other);
    }

    constexpr auto operator/=(Integer other) noexcept -> Integer&
    {
        return (*this = *this / other);
    }

    constexpr auto operator%=(Integer other) noexcept -> Integer&
    {
        return (*this = *this % other);
    }

    constexpr auto operator&=(Integer other) noexcept -> Integer&
    {
        return (*this = *this & other);
    }

    constexpr auto operator|=(Integer other) noexcept -> Integer&
    {
        return (*this = *this | other);
    }

    constexpr auto operator^=(Integer other) noexcept -> Integer&
    {
        return (*this = *this ^ other);
    }

    constexpr auto operator<<=(int n) noexcept -> Integer&
    {
        return (*this = *this << n);
    }

    constexpr auto operator>>=(int n) noexcept -> Integer&
    {
        return (*this = *this >> n);
    }

    constexpr bool operator==(Integer other) const noexcept
    {
        return (mul_prec_type(*this) == mul_prec_type(other));
    }

    constexpr bool operator!=(Integer other) const noexcept
    {
        return (mul_prec_type(*this) != mul_prec_type(other));
    }

    constexpr bool operator<(Integer other) const noexcept
    {
        return (mul_prec_type(*this) < mul_prec_type(other));
    }

    constexpr bool operator<=(Integer other) const noexcept
    {
        return (mul_prec_type(*this) <= mul_prec_type(other));
    }

    constexpr bool operator>(Integer other) const noexcept
    {
        return (mul_prec_type(*this) > mul_prec_type(other));
    }

    constexpr bool operator>=(Integer other) const noexcept
    {
        return (mul_prec_type(*this) >= mul_prec_type(other));
    }
};


using BigEndianInteger    = Integer<TestEndianness::big>;
using LittleEndianInteger = Integer<TestEndianness::little>;
using MixedEndianInteger  = Integer<TestEndianness::mixed>;


} // unnamed namespace


namespace archon::core {


template<TestEndianness E> struct IntegerTraits<Integer<E>> {
    using int_type      = Integer<E>;
    using unsigned_type = int_type;

    using mul_prec_type = typename int_type::mul_prec_type;
    using mul_prec_traits_type = core::IntegerTraits<mul_prec_type>;

    static constexpr bool is_specialized   = true;
    static constexpr int  num_value_bits   = mul_prec_traits_type::num_value_bits;
    static constexpr bool is_signed        = mul_prec_traits_type::is_signed;
    static constexpr bool has_divmod       = false;
    static constexpr bool has_find_msb_pos = false;

    static constexpr auto min() noexcept -> int_type
    {
        return mul_prec_traits_type::min();
    }

    static constexpr auto max() noexcept -> int_type
    {
        return mul_prec_traits_type::max();
    }

    using part_type = typename mul_prec_traits_type::part_type;

    static constexpr int num_parts = mul_prec_traits_type::num_parts;

    using parts_type = std::array<part_type, num_parts>;

    static constexpr auto get_parts(int_type val) noexcept -> parts_type
    {
        return mul_prec_traits_type::get_parts(mul_prec_type(val));
    }

    static constexpr auto from_parts(parts_type parts) noexcept -> int_type
    {
        return int_type(mul_prec_traits_type::from_parts(parts));
    }
};


} // namespace archon::core



ARCHON_TEST(Core_Endianness_Basics)
{
    ARCHON_CHECK(core::is_big_endian<BigEndianInteger>());
    ARCHON_CHECK_NOT(core::is_big_endian<LittleEndianInteger>());
    ARCHON_CHECK_NOT(core::is_big_endian<MixedEndianInteger>());

    ARCHON_CHECK_NOT(core::is_little_endian<BigEndianInteger>());
    ARCHON_CHECK(core::is_little_endian<LittleEndianInteger>());
    ARCHON_CHECK_NOT(core::is_little_endian<MixedEndianInteger>());

    ARCHON_CHECK_NOT(core::is_indeterminate_endian<BigEndianInteger>());
    ARCHON_CHECK_NOT(core::is_indeterminate_endian<LittleEndianInteger>());
    ARCHON_CHECK(core::is_indeterminate_endian<MixedEndianInteger>());
}
