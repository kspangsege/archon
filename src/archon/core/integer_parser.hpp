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

#ifndef ARCHON_X_CORE_X_INTEGER_PARSER_HPP
#define ARCHON_X_CORE_X_INTEGER_PARSER_HPP

/// \file


#include <string_view>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {




/// \brief Integer parser base class.
///
/// This is the base class for integer parsers (\ref BasicIntegerParser).
///
class IntegerParserBase {
public:
    /// \brief Sign acceptance mode.
    ///
    /// These are the possbile sign acceptance modes.
    ///
    enum class Sign {
        auto_, ///< \ref accept_minus for signed types and \ref reject for unsigned types.
        accept,
        accept_minus,
        reject,
    };
};




/// \brief Integer parser.
///
/// This class is an integer parser designed for efficiency and for the ability to generally
/// operate without dynamic memory allocation.
///
/// Parsing does not take locale into account other than for the purpose of narrowing
/// characters.
///
/// This integer parser supports all integer types that conform to \ref
/// Concept_Archon_Core_Integer. This includes all fundamental integer types, also `char`
/// and `bool`.
///
/// This integer parser supports all radix values between 2 and 36.
///
/// \sa \ref core::BasicIntegerFormatter
///
template<class C, class T = std::char_traits<C>> class BasicIntegerParser
    : public core::IntegerParserBase {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using char_mapper_type = core::BasicCharMapper<C, T>;

    /// \brief Construct an integer parser.
    ///
    /// Construct an integer parser with association to the specified character mapper.
    ///
    explicit BasicIntegerParser(const char_mapper_type&) noexcept;

    ~BasicIntegerParser() noexcept = default;

    template<int radix = 10, Sign = Sign::auto_, class I> bool parse(string_view_type string, I& var);

    template<Sign = Sign::auto_, class I> bool parse_bin(string_view_type string, I& var);
    template<Sign = Sign::auto_, class I> bool parse_oct(string_view_type string, I& var);
    template<Sign = Sign::auto_, class I> bool parse_dec(string_view_type string, I& var);
    template<Sign = Sign::auto_, class I> bool parse_hex(string_view_type string, I& var);

private:
    const char_mapper_type& m_mapper;

    enum class Sign2 {
        accept,
        accept_minus,
        reject,
    };

    template<class I> static constexpr auto map_sign(Sign) -> Sign2;
};


using IntegerParser     = BasicIntegerParser<char>;
using WideIntegerParser = BasicIntegerParser<wchar_t>;








// Implementation


namespace impl {


constexpr int integer_parser_map_digit(char ch) noexcept
{
    switch (ch) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
            return 10;
        case 'B':
            return 11;
        case 'C':
            return 12;
        case 'D':
            return 13;
        case 'E':
            return 14;
        case 'F':
            return 15;
        case 'G':
            return 16;
        case 'H':
            return 17;
        case 'I':
            return 18;
        case 'J':
            return 19;
        case 'K':
            return 20;
        case 'L':
            return 21;
        case 'M':
            return 22;
        case 'N':
            return 23;
        case 'O':
            return 24;
        case 'P':
            return 25;
        case 'Q':
            return 26;
        case 'R':
            return 27;
        case 'S':
            return 28;
        case 'T':
            return 29;
        case 'U':
            return 30;
        case 'V':
            return 31;
        case 'W':
            return 32;
        case 'X':
            return 33;
        case 'Y':
            return 34;
        case 'Z':
            return 35;
        case 'a':
            return 10;
        case 'b':
            return 11;
        case 'c':
            return 12;
        case 'd':
            return 13;
        case 'e':
            return 14;
        case 'f':
            return 15;
        case 'g':
            return 16;
        case 'h':
            return 17;
        case 'i':
            return 18;
        case 'j':
            return 19;
        case 'k':
            return 20;
        case 'l':
            return 21;
        case 'm':
            return 22;
        case 'n':
            return 23;
        case 'o':
            return 24;
        case 'p':
            return 25;
        case 'q':
            return 26;
        case 'r':
            return 27;
        case 's':
            return 28;
        case 't':
            return 29;
        case 'u':
            return 30;
        case 'v':
            return 31;
        case 'w':
            return 32;
        case 'x':
            return 33;
        case 'y':
            return 34;
        case 'z':
            return 35;
    }
    return -1;
}


constexpr bool integer_parser_map_digit(char ch, int radix, int& digit) noexcept
{
    int value = integer_parser_map_digit(ch);
    if (ARCHON_LIKELY(value != -1 && value < radix)) {
        digit = value;
        return true;
    }
    return false;
}


} // namespace impl


template<class C, class T>
inline BasicIntegerParser<C, T>::BasicIntegerParser(const char_mapper_type& mapper) noexcept
    : m_mapper(mapper)
{
}


template<class C, class T>
template<int radix, core::IntegerParserBase::Sign sign, class I>
bool BasicIntegerParser<C, T>::parse(string_view_type string, I& var)
{
    auto string_2 = string;
    int sign_val = 1;
    switch (map_sign<I>(sign)) {
        case Sign2::accept:
            if (ARCHON_LIKELY(!string_2.empty())) {
                char_type ch = string_2[0];
                char ch_2 = m_mapper.narrow(ch, '\0'); // Throws
                if (ch_2 == '+' || ch_2 == '-') {
                    sign_val = (ch_2 == '+' ? 1 : -1);
                    string_2 = string_2.substr(1);
                }
            }
            break;
        case Sign2::accept_minus:
            if (ARCHON_LIKELY(!string_2.empty())) {
                char_type ch = string_2[0];
                char ch_2 = m_mapper.narrow(ch, '\0'); // Throws
                if (ch_2 == '-') {
                    sign_val = -1;
                    string_2 = string_2.substr(1);
                }
            }
            break;
        case Sign2::reject:
            break;
    }
    if (ARCHON_LIKELY(!string_2.empty())) {
        I val = I(0);
        for (char_type ch : string_2) {
            char ch_2 = m_mapper.narrow(ch, '\0'); // Throws
            int digit = 0;
            if (ARCHON_UNLIKELY(!impl::integer_parser_map_digit(ch_2, radix, digit)))
                return false; // Failure
            if (ARCHON_UNLIKELY(!core::try_int_mul(val, radix)))
                return false; // Failure
            if (ARCHON_UNLIKELY(!core::try_int_add(val, sign_val * digit)))
                return false; // Failure
        }
        var = val;
        return true; // Success
    }
    return false; // Failure
}


template<class C, class T>
template<core::IntegerParserBase::Sign sign, class I>
inline bool BasicIntegerParser<C, T>::parse_bin(string_view_type string, I& var)
{
    return parse<2, sign>(string, var); // Throws
}


template<class C, class T>
template<core::IntegerParserBase::Sign sign, class I>
inline bool BasicIntegerParser<C, T>::parse_oct(string_view_type string, I& var)
{
    return parse<8, sign>(string, var); // Throws
}


template<class C, class T>
template<core::IntegerParserBase::Sign sign, class I>
inline bool BasicIntegerParser<C, T>::parse_dec(string_view_type string, I& var)
{
    return parse<10, sign>(string, var); // Throws
}


template<class C, class T>
template<core::IntegerParserBase::Sign sign, class I>
inline bool BasicIntegerParser<C, T>::parse_hex(string_view_type string, I& var)
{
    return parse<16, sign>(string, var); // Throws
}


template<class C, class T>
template<class I> constexpr auto BasicIntegerParser<C, T>::map_sign(Sign sign) -> Sign2
{
    switch (sign) {
        case Sign::auto_:
            break;
        case Sign::accept:
            return Sign2::accept;
        case Sign::accept_minus:
            return Sign2::accept_minus;
        case Sign::reject:
            return Sign2::reject;
    }
    if (core::is_signed<I>())
        return Sign2::accept_minus;
    return Sign2::reject;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INTEGER_PARSER_HPP
