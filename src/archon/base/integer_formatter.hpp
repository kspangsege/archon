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

#ifndef ARCHON_X_BASE_X_INTEGER_FORMATTER_HPP
#define ARCHON_X_BASE_X_INTEGER_FORMATTER_HPP

/// \file


#include <cstdint>
#include <type_traits>
#include <algorithm>
#include <string_view>

#include <archon/base/assert.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/char_mapper.hpp>


namespace archon::base {


/// \brief Upper case radix 36 digits.
///
/// This is the upper case variant of the (up to) 36 digits used by \ref
/// BasicIntegerFormatter.
///
inline char format_int_uc_digits[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};


/// \brief Lower case radix 36 digits.
///
/// This is the lower case variant of the (up to) 36 digits used by \ref
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
    /// BasicIntegerFormatter::format(). \ref min_radix() returns 2, and \ref
    /// max_radix() returns 36.
    ///
    static constexpr int min_radix() noexcept;
    static constexpr int max_radix() noexcept;
    /// \}

    /// \brief Maximum possible number of digits.
    ///
    /// The number of digits required to render, using radix 2, any value of the
    /// widest available integer types (`std::intmax_t` and `std::uintmax_t`).
    ///
    /// This constant also acts as a limit on the \p min_num_digits argument
    /// as passed to \ref BasicIntegerFormatter::format().
    ///
    /// Since `std::uint_fast64_t` is a nonoptional type, you know that \ref
    /// max_digits() must be at least 64.
    ///
    static constexpr int max_digits() noexcept;

protected:
    static constexpr int s_min_radix = 2;
    static constexpr int s_max_radix = 36;
    static constexpr int s_max_digits =
        std::max(base::int_max_digits<std::intmax_t>(s_min_radix),
                 base::int_max_digits<std::uintmax_t>(s_min_radix));

    static_assert(sizeof format_int_uc_digits / sizeof *format_int_uc_digits == s_max_radix);
    static_assert(sizeof format_int_lc_digits / sizeof *format_int_lc_digits == s_max_radix);
};




/// \brief Efficient integer formatter.
///
/// An integer formatter designed for efficiency and for the ability to operate
/// without dynamic memory allocation.
///
/// Formatting does not take locale into account other than for the purpose of
/// widening characters.
///
/// As a bonus, it supports all radix values between 2 and 36.
///
template<class C, class T = std::char_traits<C>> class BasicIntegerFormatter :
        public IntegerFormatterBase {
public:
    using string_view_type = std::basic_string_view<C, T>;
    using char_mapper_type = BasicCharMapper<C, T>;

    /// \brief Format an integer.
    ///
    /// Format the specified integer in the specified radix (base), and with the
    /// specified minimum number of digits. Formatting does not take locale into
    /// account other than for the purpose of widening characters.
    ///
    /// \tparam radix The radix (or base) in which the specified value must be
    /// formatted. The value must be between \ref min_radix() and \ref
    /// max_radix(). When the radix is 36, all ten digits (Arabic numerals), and
    /// all 26 letters of the English alphabet are in use.
    ///
    /// \param value The value to be formatted. Any integer type is allowed.
    ///
    /// \param min_num_digits The minimum number of digits to generate. The
    /// number will be silently clamped if it is less than zero, or greater than
    /// \ref max_digits(). If it is zero, and the formatted value is also zero,
    /// no digits will be generated.
    ///
    template<int radix = 10, class I> string_view_type format(I value, int min_num_digits = 1);

    /// \{
    ///
    /// \brief Format an integer.
    ///
    /// These functions are short-hands for calling \ref format() with the
    /// corresponding radix (base).
    ///
    template<class I> string_view_type format_dec(I value, int min_num_digits = 1);
    template<class I> string_view_type format_bin(I value, int min_num_digits = 1);
    template<class I> string_view_type format_oct(I value, int min_num_digits = 1);
    template<class I> string_view_type format_hex(I value, int min_num_digits = 1);
    /// \}

    /// \brief Format an integer.
    ///
    /// This function has the same effect as \ref format(), but allows you to
    /// use a radix that is not known at compile time. This may be at the
    /// expense of reduced efficiency.
    ///
    /// The specified radix will be silently clamped if it is less than
    /// \ref min_radix() or greater than \ref max_radix().
    ///
    template<class I> string_view_type format_a(I value, int radix, int min_num_digits = 1);

    /// \brief Use lower case letters.
    ///
    /// If true is specified, subsequent formatting operations will use lower
    /// case letters when the base is above 10. If false is specified,
    /// subsequent formatting operations will use upper case letters instead
    /// (the default).
    ///
    void use_lower_case(bool value) noexcept;

    /// \brief Construct an integer formatter.
    ///
    /// Construct an integer formatter with association to the specified
    /// character type mapper.
    ///
    explicit BasicIntegerFormatter(const char_mapper_type&) noexcept;

    ~BasicIntegerFormatter() noexcept = default;

private:
    static constexpr int s_buffer_size = 1 + s_max_digits; // Make room for minus sign

    const char_mapper_type& m_mapper;
    const char* m_digits = format_int_uc_digits;
    C m_buffer[s_buffer_size];

    template<int radix, class I> string_view_type do_format(I value, int min_num_digits);
    template<class I> string_view_type do_format_a(I value, int radix, int min_num_digits);
};


using IntegerFormatter     = BasicIntegerFormatter<char>;
using WideIntegerFormatter = BasicIntegerFormatter<wchar_t>;




/// \{
///
/// \brief Format an integer as specified.
///
/// These functions are short-hands for calling \ref as_int() with the
/// corresponding radix (base).
///
template<class I> auto as_dec_int(I value, int min_num_digits = 1) noexcept;
template<class I> auto as_bin_int(I value, int min_num_digits = 1) noexcept;
template<class I> auto as_oct_int(I value, int min_num_digits = 1) noexcept;
template<class I> auto as_hex_int(I value, int min_num_digits = 1) noexcept;
/// }


/// \brief Format an integer as specified.
///
/// Construct an object that, if written to an output stream, formats the
/// specified integer in the specified radix (base), and with the specified
/// minimum number of digits.
///
template<int radix, class I> auto as_int(I value, int min_num_digits = 1) noexcept;


/// \brief Format an integer as specified.
///
/// This function has the same effect as \ref as_int(), but allows you to use a
/// radix that is not known at compile time. This may be at the expense of
/// reduced efficiency.
///
template<class I> auto as_int_a(I value, int radix, int min_num_digits = 1) noexcept;








// Implementation


constexpr int IntegerFormatterBase::min_radix() noexcept
{
    return s_min_radix;
}


constexpr int IntegerFormatterBase::max_radix() noexcept
{
    return s_max_radix;
}


constexpr int IntegerFormatterBase::max_digits() noexcept
{
    return s_max_digits;
}


template<class C, class T> template<int radix, class I>
auto BasicIntegerFormatter<C, T>::format(I value, int min_num_digits) -> string_view_type
{
    if constexpr (radix == 10 || radix == 2 || radix == 8 || radix == 16) {
        // Promote value to limit number of template instantiations
        return do_format<radix>(base::promote(value), min_num_digits); // Throws
    }
    else {
        return format_a(value, radix, min_num_digits);
    }
}


template<class C, class T> template<class I>
inline auto BasicIntegerFormatter<C, T>::format_dec(I value, int min_num_digits) -> string_view_type
{
    return format<10>(value, min_num_digits); // Throws
}


template<class C, class T> template<class I>
inline auto BasicIntegerFormatter<C, T>::format_bin(I value, int min_num_digits) -> string_view_type
{
    return format<2>(value, min_num_digits); // Throws
}


template<class C, class T> template<class I>
inline auto BasicIntegerFormatter<C, T>::format_oct(I value, int min_num_digits) -> string_view_type
{
    return format<8>(value, min_num_digits); // Throws
}


template<class C, class T> template<class I>
inline auto BasicIntegerFormatter<C, T>::format_hex(I value, int min_num_digits) -> string_view_type
{
    return format<16>(value, min_num_digits); // Throws
}


template<class C, class T> template<class I>
auto BasicIntegerFormatter<C, T>::format_a(I value, int radix, int min_num_digits) ->
    string_view_type
{
    // Promote value to limit number of template instantiations
    return do_format_a(base::promote(value), radix, min_num_digits); // Throws
}


template<class C, class T>
inline void BasicIntegerFormatter<C, T>::use_lower_case(bool value) noexcept
{
    m_digits = (value ? format_int_lc_digits : format_int_uc_digits);
}


template<class C, class T>
inline BasicIntegerFormatter<C, T>::BasicIntegerFormatter(const char_mapper_type& mapper) noexcept :
    m_mapper(mapper)
{
}


template<class C, class T> template<int radix, class I>
auto BasicIntegerFormatter<C, T>::do_format(I value, int min_num_digits) -> string_view_type
{
    static_assert(radix == 10 || radix == 2 || radix == 8 || radix == 16);
    static_assert(std::is_integral_v<I>);
    static_assert(std::is_same_v<decltype(base::promote(value)), I>);
    static_assert(radix >= s_min_radix);
    static_assert(radix <= s_max_radix);
    ARCHON_ASSERT(1 + base::int_max_digits<I>(radix) <= s_buffer_size);
    int min_num_digits_2 = std::min(min_num_digits, s_max_digits);
    C* end = m_buffer + s_buffer_size;
    int n = 0;
    I value_2 = value;
    if (is_negative(value_2)) {
        // Negative numbers
        while (value_2 != 0 || n < min_num_digits_2) {
            I quot = value_2 / radix;
            int rem = int(quot * radix - value_2);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            *(end - ++n) = m_mapper.widen(m_digits[rem]); // Throws
            value_2 = quot;
        }
        // Add the minus sign
        *(end - ++n) = m_mapper.widen('-'); // Throws
    }
    else {
        // Nonnegative numbers
        while (value_2 != 0 || n < min_num_digits_2) {
            I quot = value_2 / radix;
            int rem = int(value_2 - quot * radix);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            *(end - ++n) = m_mapper.widen(m_digits[rem]); // Throws
            value_2 = quot;
        }
    }
    return { end - n, std::size_t(n) };
}


template<class C, class T> template<class I>
auto BasicIntegerFormatter<C, T>::do_format_a(I value, int radix, int min_num_digits) ->
    string_view_type
{
    static_assert(std::is_integral_v<I>);
    static_assert(std::is_same_v<decltype(base::promote(value)), I>);
    int radix_2 = std::clamp(radix, s_min_radix, s_max_radix);
    ARCHON_ASSERT(1 + base::int_max_digits<I>(radix_2) <= s_buffer_size);
    int min_num_digits_2 = std::min(min_num_digits, s_max_digits);
    C* end = m_buffer + s_buffer_size;
    int n = 0;
    I value_2 = base::promote(value);
    if (is_negative(value_2)) {
        // Negative numbers
        while (value_2 != 0 || n < min_num_digits_2) {
            I quot = value_2 / radix_2;
            int rem = int(quot * radix_2 - value_2);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            *(end - ++n) = m_mapper.widen(m_digits[rem]); // Throws
            value_2 = quot;
        }
        // Add the minus sign
        *(end - ++n) = m_mapper.widen('-'); // Throws
    }
    else {
        // Nonnegative numbers
        while (value_2 != 0 || n < min_num_digits_2) {
            I quot = value_2 / radix_2;
            int rem = int(value_2 - quot * radix_2);
            ARCHON_ASSERT(rem >= 0 && rem < s_max_radix);
            *(end - ++n) = m_mapper.widen(m_digits[rem]); // Throws
            value_2 = quot;
        }
    }
    return { end - n, std::size_t(n) };
}


namespace detail {


template<int radix, class I> struct AsInt {
    I value;
    int min_num_digits;
};


template<class C, class T, int radix, class I>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out, AsInt<radix, I> pod)
{
    BasicCharMapper<C, T> mapper(out); // Throws
    BasicIntegerFormatter<C, T> formatter(mapper);
    out << formatter.template format<radix>(pod.value, pod.min_num_digits); // Throws
    return out;
}


template<class I> struct AsIntA {
    I value;
    int radix;
    int min_num_digits;
};


template<class C, class T, class I>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out, AsIntA<I> pod)
{
    BasicCharMapper<C, T> mapper(out); // Throws
    BasicIntegerFormatter<C, T> formatter(mapper);
    out << formatter.format_a(pod.value, pod.radix, pod.min_num_digits); // Throws
    return out;
}


} // namespace detail


template<class I> inline auto as_dec_int(I value, int min_num_digits) noexcept
{
    return as_int<10>(value, min_num_digits);
}


template<class I> inline auto as_bin_int(I value, int min_num_digits) noexcept
{
    return as_int<2>(value, min_num_digits);
}


template<class I> inline auto as_oct_int(I value, int min_num_digits) noexcept
{
    return as_int<8>(value, min_num_digits);
}


template<class I> inline auto as_hex_int(I value, int min_num_digits) noexcept
{
    return as_int<16>(value, min_num_digits);
}


template<int radix, class I> inline auto as_int(I value, int min_num_digits) noexcept
{
    if constexpr (radix == 10 || radix == 2 || radix == 8 || radix == 16) {
        // Promote value to limit number of template instantiations
        using J = decltype(base::promote(value));
        return detail::AsInt<radix, J> { base::promote(value), min_num_digits };
    }
    else {
        return as_int_a(value, radix, min_num_digits);
    }
}


template<class I> inline auto as_int_a(I value, int radix, int min_num_digits) noexcept
{
    // Promote value to limit number of template instantiations
    using J = decltype(base::promote(value));
    return detail::AsIntA<J> { base::promote(value), radix, min_num_digits };
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_INTEGER_FORMATTER_HPP
