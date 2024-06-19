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

#ifndef ARCHON_X_CORE_X_AS_INT_HPP
#define ARCHON_X_CORE_X_AS_INT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <string_view>
#include <locale>
#include <istream>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_input.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/integer_parser.hpp>
#include <archon/core/value_parser.hpp>


namespace archon::core {


/// \{
///
/// \brief Format or parse integer as specified.
///
/// These functions are shorthands for calling \ref core::as_int() with the corresponding
/// radix (base).
///
/// \sa \ref core::as_flex_int()
/// \sa \ref core::as_int()
///
template<class I> auto as_dec_int(I&& ref, int min_num_digits = 1) noexcept;
template<class I> auto as_bin_int(I&& ref, int min_num_digits = 1) noexcept;
template<class I> auto as_oct_int(I&& ref, int min_num_digits = 1) noexcept;
template<class I> auto as_hex_int(I&& ref, int min_num_digits = 1) noexcept;
/// }


/// \{
///
/// \brief Format or parse unsigned decimal, octal, or hexadecimal integer.
///
/// These functions return objects that, depending on \p ref, can be used for both
/// formatting and parsing of an integer.
///
/// When formatting, the effect, when using `as_flex_int()`, is the same as when using \ref
/// core::as_dec_int(). Similarly, the effect, when using `as_flex_int_h()`, is the same as
/// when using \ref core::as_hex_int(), but with `0x` added as prefix.
///
/// When parsing, both overloads work exactly the same way. If the parsed string has `0x` as
/// a prefix, the effect is the same as for `as_hex_int(ref)` when applied to the part of
/// the parsed string that follows the prefix. Otherwise, if the parsed string has `0` as a
/// prefix, the effect is the same as for `as_oct_int(ref)` when applied to the part of the
/// parsed string that follows the prefix. Otherwise the effect is the same as for
/// `as_dec_int(ref)`.
///
/// \sa \ref core::as_dec_int()
/// \sa \ref core::as_oct_int()
/// \sa \ref core::as_hex_int()
/// \sa \ref core::as_int()
/// \sa \ref core::as_flex_int()
/// \sa \ref core::as_flex_int_h()
///
template<class I> auto as_flex_int(I&& ref) noexcept;
template<class I> auto as_flex_int_h(I&& ref) noexcept;
/// \}


/// \brief Format or parse integer as specified.
///
/// If the referenced integer object (\p ref) is not `const`, the object returned by this
/// function can be used for both formatting and parsing. Formatting happens when the
/// reurned object is passed to the stream output operator (`operator<<` on
/// `std::basic_ostream`). Parsing happens when it is passed to the stream input operator
/// (`operator>>` on `std::basic_istream`) or to \ref core::BasicValueParser::parse().
///
/// If the referenced integer object is `const`, only formatting is possible.
///
/// Formatting will be delegated to an instance of \ref core::BasicIntegerFormatter. Parsing
/// will be delegated to an instance of \ref core::BasicIntegerParser. Parsing will use \ref
/// core::IntegerParserBase::Sign::auto_ as the sign acceptance mode.
///
/// \param min_num_digits The minimum number of digits to generate. See \ref
/// core::IntegerFormatter::format() for the meanings of zero and negative value.
///
/// \sa \ref core::as_dec_int()
/// \sa \ref core::as_oct_int()
/// \sa \ref core::as_hex_int()
/// \sa \ref core::as_int_a()
///
template<int radix = 10, class I> auto as_int(I&& ref, int min_num_digits = 1) noexcept;


/// \brief Format or parse integer as specified.
///
/// This function has the same effect as \ref core::as_int(), but this function allows you
/// to use a radix that is not known at compile time. This may be at the expense of reduced
/// efficiency, though.
///
/// \sa \ref core::as_int()
///
template<class I> auto as_int_a(I&& ref, int radix, int min_num_digits = 1) noexcept;








// Implementation


namespace impl {


template<int radix, class I> struct AsInt {
    I i; // May be a reference
    int radix_2;
    int min_num_digits;
};


template<class C, class T, int radix, class I>
inline auto operator<<(std::basic_ostream<C, T>& out, AsInt<radix, I> pod) -> std::basic_ostream<C, T>&
{
    core::BasicCharMapper<C, T> mapper(out); // Throws
    core::BasicIntegerFormatter<C, T> formatter(mapper);
    if constexpr (radix != 0) {
        out << formatter.template format<radix>(pod.i, pod.min_num_digits); // Throws
    }
    else {
        out << formatter.format_a(pod.i, pod.radix_2, pod.min_num_digits); // Throws
    }
    return out;
}


template<class C, class T, int radix, class I>
auto operator>>(std::basic_istream<C, T>& in, const impl::AsInt<radix, I>& pod) -> std::basic_istream<C, T>&
{
    return core::istream_sentry(in, [&](core::BasicStreamInputHelper<C, T>& helper) {
        std::locale loc = in.getloc();
        const std::ctype<C>& ctype = std::use_facet<std::ctype<C>>(loc);
        using char_mapper_type = core::BasicCharMapper<C, T>;
        char_mapper_type char_mapper(ctype);
        C dash(char_mapper.widen('-')); // Throws
        C underscore(char_mapper.widen('_')); // Throws
        core::ArraySeededBuffer<C, 256> buffer;
        std::size_t size = 0;
        C ch = {};
        for (;;) {
            if (ARCHON_UNLIKELY(!helper.peek(ch)))
                break;
            if (ch == dash) {
                buffer.append_a(ch, size); // Throws
                if (ARCHON_UNLIKELY(!helper.next(ch)))
                    break;
            }
            for (;;) {
                if (ARCHON_UNLIKELY(!ctype.is(std::ctype_base::alnum, ch) && ch != underscore))
                    break;
                buffer.append_a(ch, size); // Throws
                if (ARCHON_UNLIKELY(!helper.next(ch)))
                    break;
            }
            break;
        }
        std::basic_string_view<C, T> string = { buffer.data(), size };
        core::BasicIntegerParser<C, T> parser(char_mapper);
        constexpr auto sign = core::IntegerParserBase::Sign::auto_;
        if constexpr (radix != 0) {
            return parser.template parse<radix, sign>(string, pod.i); // Throws
        }
        else {
            return parser.template parse_a<sign>(string, pod.radix_2, pod.i); // Throws
        }
    });
}


template<class C, class T, int radix, class I>
inline bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsInt<radix, I>& pod)
{
    core::BasicIntegerParser<C, T> parser(src.get_char_mapper());
    constexpr auto sign = core::IntegerParserBase::Sign::auto_;
    if constexpr (radix != 0) {
        return parser.template parse<radix, sign>(src.string(), pod.i); // Throws
    }
    else {
        return parser.template parse_a<sign>(src.string(), pod.radix_2, pod.i); // Throws
    }
}


template<class I> struct AsFlexInt {
    I i; // May be a reference
    bool format_as_hex;
};


template<class C, class T, class I>
inline auto operator<<(std::basic_ostream<C, T>& out, AsFlexInt<I> pod) -> std::basic_ostream<C, T>&
{
    core::BasicCharMapper<C, T> mapper(out); // Throws
    core::BasicIntegerFormatter<C, T> formatter(mapper);
    if (!pod.format_as_hex) {
        out << formatter.format_dec(pod.i); // Throws
    }
    else {
        out << "0x" << formatter.format_hex(pod.i); // Throws
    }
    return out;
}


template<class C, class T, class I>
inline bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsFlexInt<I>& pod)
{
    core::BasicIntegerParser<C, T> parser(src.get_char_mapper());
    using string_view_type = typename core::BasicValueParserSource<C, T>::string_view_type;
    string_view_type string = src.string();
    bool is_dec = (string.size() < 1 || string[0] != src.widen('0')); // Throws
    if (is_dec)
        return parser.parse_dec(src.string(), pod.i); // Throws
    bool is_oct = (string.size() < 2 || (string[1] != src.widen('x') && string[1] != src.widen('X'))); // Throws
    if (is_oct)
        return parser.parse_oct(src.string(), pod.i); // Throws
    return parser.parse_hex(src.string().substr(2), pod.i); // Throws
}


} // namespace impl


template<class I> inline auto as_dec_int(I&& ref, int min_num_digits) noexcept
{
    return core::as_int<10>(std::forward<I>(ref), min_num_digits);
}


template<class I> inline auto as_bin_int(I&& ref, int min_num_digits) noexcept
{
    return core::as_int<2>(std::forward<I>(ref), min_num_digits);
}


template<class I> inline auto as_oct_int(I&& ref, int min_num_digits) noexcept
{
    return core::as_int<8>(std::forward<I>(ref), min_num_digits);
}


template<class I> inline auto as_hex_int(I&& ref, int min_num_digits) noexcept
{
    return core::as_int<16>(std::forward<I>(ref), min_num_digits);
}


template<class I> inline auto as_flex_int(I&& ref) noexcept
{
    static_assert(core::is_unsigned<std::decay_t<I>>());
    bool format_as_hex = false;
    return impl::AsFlexInt<I> { std::forward<I>(ref), format_as_hex };
}


template<class I> inline auto as_flex_int_h(I&& ref) noexcept
{
    static_assert(core::is_unsigned<std::decay_t<I>>());
    bool format_as_hex = true;
    return impl::AsFlexInt<I> { std::forward<I>(ref), format_as_hex };
}


template<int radix, class I> inline auto as_int(I&& ref, int min_num_digits) noexcept
{
    if constexpr (radix == 10 || radix == 2 || radix == 8 || radix == 16) {
        return impl::AsInt<radix, I> { std::forward<I>(ref), 0, min_num_digits };
    }
    else {
        return core::as_int_a(std::forward<I>(ref), radix, min_num_digits);
    }
}


template<class I> inline auto as_int_a(I&& ref, int radix, int min_num_digits) noexcept
{
    return impl::AsInt<0, I> { std::forward<I>(ref), radix, min_num_digits };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_AS_INT_HPP
