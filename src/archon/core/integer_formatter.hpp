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

#ifndef ARCHON_X_CORE_X_INTEGER_FORMATTER_HPP
#define ARCHON_X_CORE_X_INTEGER_FORMATTER_HPP

/// \file


#include <cstdint>
#include <type_traits>
#include <algorithm>
#include <string_view>

#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \brief Uppercase radix 36 digits.
///
/// This is the uppercase variant of the (up to) 36 digits used by \ref
/// BasicIntegerFormatter.
///
inline char format_int_uc_digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};


/// \brief Lowercase radix 36 digits.
///
/// This is the lowercase variant of the (up to) 36 digits used by \ref
/// BasicIntegerFormatter.
///
inline char format_int_lc_digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z'
};




/// \brief Integer formatter base class.
///
/// This is the base class for integer formatters (\ref BasicIntegerFormatter).
///
class IntegerFormatterBase {
public:
    /// \{
    ///
    /// \brief Range of possible radix values.
    ///
    /// The range of possible values of radix supported by \ref
    /// BasicIntegerFormatter::format(). \ref min_radix() returns 2, and \ref max_radix()
    /// returns 36.
    ///
    static constexpr int min_radix() noexcept;
    static constexpr int max_radix() noexcept;
    /// \}

protected:
    static constexpr int s_min_radix = 2;
    static constexpr int s_max_radix = 36;

    static_assert(sizeof format_int_uc_digits / sizeof *format_int_uc_digits == s_max_radix);
    static_assert(sizeof format_int_lc_digits / sizeof *format_int_lc_digits == s_max_radix);
};




/// \brief Integer formatter.
///
/// This class is an integer formatter designed for efficiency and for the ability to
/// generally operate without dynamic memory allocation.
///
/// Formatting does not take locale into account other than for the purpose of widening
/// characters.
///
/// This integer formatter supports all integer types that conform to \ref
/// Concept_Archon_Core_Integer. This includes all fundamental integer types, also `char`
/// and `bool`.
///
/// This integer formatter supports all radix values between 2 and 36.
///
/// \sa \ref core::BasicIntegerParser
///
template<class C, class T = std::char_traits<C>> class BasicIntegerFormatter
    : public IntegerFormatterBase {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using char_mapper_type = core::BasicCharMapper<C, T>;

    /// \brief Construct an integer formatter.
    ///
    /// Construct an integer formatter with association to the specified character mapper.
    ///
    explicit BasicIntegerFormatter(const char_mapper_type&) noexcept;

    ~BasicIntegerFormatter() noexcept = default;

    /// \brief Format an integer.
    ///
    /// Format the specified integer in the specified radix (base), and with the specified
    /// minimum number of digits. Formatting does not take locale into account other than
    /// for the purpose of widening characters.
    ///
    /// If the specified minimum number of digits (\p min_num_digits) is zero, and the
    /// formatted value is zero, no digits will be generated.
    ///
    /// If the specified minimum number of digits is negative, the actual number of digits
    /// will be set to maximum number of digits for the type of integer being formatted.
    ///
    /// \tparam radix The radix (or base) in which the specified value must be
    /// formatted. The value must be between \ref min_radix() and \ref max_radix(). When the
    /// radix is 36, all ten digits (Arabic numerals), and all 26 letters of the English
    /// alphabet are in use.
    ///
    /// \param value The value to be formatted. The type of this integer must conform to
    /// \ref Concept_Archon_Core_Integer.
    ///
    /// \param min_num_digits The minimum number of digits to generate. See above for
    /// meanings of zero and negative value.
    ///
    template<int radix = 10, class I> string_view_type format(I value, int min_num_digits = 1);

    /// \{
    ///
    /// \brief Format an integer.
    ///
    /// These functions are shorthands for calling \ref format() with the corresponding
    /// radix (base).
    ///
    template<class I> auto format_dec(I value, int min_num_digits = 1) -> string_view_type;
    template<class I> auto format_bin(I value, int min_num_digits = 1) -> string_view_type;
    template<class I> auto format_oct(I value, int min_num_digits = 1) -> string_view_type;
    template<class I> auto format_hex(I value, int min_num_digits = 1) -> string_view_type;
    /// \}

    /// \brief Format an integer.
    ///
    /// This function has the same effect as \ref format(), but allows you to use a radix
    /// that is not known at compile time. This may be at the expense of reduced efficiency.
    ///
    /// The specified radix must be greater than, or equal to \ref min_radix(); and less
    /// than, or equal to \ref max_radix().
    ///
    template<class I> auto format_a(I value, int radix, int min_num_digits = 1) -> string_view_type;

    /// \brief Use lowercase letters.
    ///
    /// If true is specified, subsequent formatting operations will use lowercase letters
    /// when the base is above 10. If false is specified, subsequent formatting operations
    /// will use uppercase letters instead (the default).
    ///
    void use_lowercase(bool value = true) noexcept;

private:
    const char_mapper_type& m_mapper;
    const char* m_digits = format_int_uc_digits;
    std::array<C, 65> m_seed_memory = {};
    core::Buffer<C> m_buffer;

    template<int radix, class I> auto do_format(I value, int min_num_digits) -> string_view_type;
    template<class I> auto do_format_a(I value, int radix, int min_num_digits) -> string_view_type;
};


using IntegerFormatter     = BasicIntegerFormatter<char>;
using WideIntegerFormatter = BasicIntegerFormatter<wchar_t>;








// Implementation


constexpr int IntegerFormatterBase::min_radix() noexcept
{
    return s_min_radix;
}


constexpr int IntegerFormatterBase::max_radix() noexcept
{
    return s_max_radix;
}


template<class C, class T>
inline BasicIntegerFormatter<C, T>::BasicIntegerFormatter(const char_mapper_type& mapper) noexcept
    : m_mapper(mapper)
    , m_buffer(m_seed_memory)
{
}


template<class C, class T>
template<int radix, class I>
auto BasicIntegerFormatter<C, T>::format(I value, int min_num_digits) -> string_view_type
{
    if constexpr (radix == 10 || radix == 2 || radix == 8 || radix == 16) {
        int min_num_digits_2 = min_num_digits;
        if (ARCHON_UNLIKELY(min_num_digits_2 < 0))
            min_num_digits_2 = core::int_max_digits<I>(radix);
        // Promoting value to limit number of template instantiations and to guarantee that
        // the value type can hold all non-negative `int` values.
        return do_format<radix>(core::promote_strongly(value), min_num_digits_2); // Throws
    }
    else {
        return format_a(value, radix, min_num_digits);
    }
}


template<class C, class T>
template<class I>
inline auto BasicIntegerFormatter<C, T>::format_dec(I value, int min_num_digits) -> string_view_type
{
    return format<10>(value, min_num_digits); // Throws
}


template<class C, class T>
template<class I>
inline auto BasicIntegerFormatter<C, T>::format_bin(I value, int min_num_digits) -> string_view_type
{
    return format<2>(value, min_num_digits); // Throws
}


template<class C, class T>
template<class I>
inline auto BasicIntegerFormatter<C, T>::format_oct(I value, int min_num_digits) -> string_view_type
{
    return format<8>(value, min_num_digits); // Throws
}


template<class C, class T>
template<class I>
inline auto BasicIntegerFormatter<C, T>::format_hex(I value, int min_num_digits) -> string_view_type
{
    return format<16>(value, min_num_digits); // Throws
}


template<class C, class T>
template<class I>
auto BasicIntegerFormatter<C, T>::format_a(I value, int radix, int min_num_digits) -> string_view_type
{
    int min_num_digits_2 = min_num_digits;
    if (ARCHON_UNLIKELY(min_num_digits_2 < 0))
        min_num_digits_2 = core::int_max_digits<I>(radix);
    // Promoting value to limit number of template instantiations and to guarantee that the
    // value type can hold all non-negative `int` values.
    return do_format_a(core::promote_strongly(value), radix, min_num_digits_2); // Throws
}


template<class C, class T>
inline void BasicIntegerFormatter<C, T>::use_lowercase(bool value) noexcept
{
    m_digits = (value ? format_int_lc_digits : format_int_uc_digits);
}


template<class C, class T>
template<int radix, class I>
auto BasicIntegerFormatter<C, T>::do_format(I value, int min_num_digits) -> string_view_type
{
    static_assert(radix == 10 || radix == 2 || radix == 8 || radix == 16);
    static_assert(core::is_integer<I>());
    static_assert(std::is_same_v<core::promoted_type<I>, I>);
    static_assert(radix >= s_min_radix);
    static_assert(radix <= s_max_radix);
    std::size_t offset = m_buffer.size();
    I value_2 = value;
    auto prepend = [&](char ch) {
        m_buffer.prepend_a(m_mapper.widen(ch), offset); // Throws
    };
    if (!core::is_negative(value_2)) {
        // Non-negative numbers
        while (value_2 != I(0) || int(m_buffer.size() - offset) < min_num_digits) {
            core::IntDivMod<I> res = core::int_divmod(value_2, I(radix));
            int rem = int(res.rem);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            prepend(m_digits[rem]); // Throws
            value_2 = res.quot;
        }
    }
    else {
        // Negative numbers
        while (value_2 != I(0) || int(m_buffer.size() - offset) < min_num_digits) {
            core::IntDivMod<I> res = core::int_divmod(value_2, I(radix));
            int rem = -int(res.rem);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            prepend(m_digits[rem]); // Throws
            value_2 = res.quot;
        }
        // Add the minus sign
        prepend('-'); // Throws
    }
    return { m_buffer.data() + offset, std::size_t(m_buffer.size() - offset) };
}


template<class C, class T>
template<class I>
auto BasicIntegerFormatter<C, T>::do_format_a(I value, int radix, int min_num_digits) -> string_view_type
{
    static_assert(core::is_integer<I>());
    static_assert(std::is_same_v<core::promoted_type<I>, I>);
    if (ARCHON_UNLIKELY(radix < s_min_radix || radix > s_max_radix))
        throw std::invalid_argument("Unsupported radix");
    std::size_t offset = m_buffer.size();
    I value_2 = value;
    auto prepend = [&](char ch) {
        m_buffer.prepend_a(m_mapper.widen(ch), offset); // Throws
    };
    if (!core::is_negative(value_2)) {
        // Non-negative numbers
        while (value_2 != I(0) || int(m_buffer.size() - offset) < min_num_digits) {
            core::IntDivMod<I> res = core::int_divmod(value_2, I(radix));
            int rem = int(res.rem);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            prepend(m_digits[rem]); // Throws
            value_2 = res.quot;
        }
    }
    else {
        // Negative numbers
        while (value_2 != I(0) || int(m_buffer.size() - offset) < min_num_digits) {
            core::IntDivMod<I> res = core::int_divmod(value_2, I(radix));
            int rem = -int(res.rem);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            prepend(m_digits[rem]); // Throws
            value_2 = res.quot;
        }
        // Add the minus sign
        prepend('-'); // Throws
    }
    return { m_buffer.data() + offset, std::size_t(m_buffer.size() - offset) };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INTEGER_FORMATTER_HPP
