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

#ifndef ARCHON_X_CORE_X_QUOTE_HPP
#define ARCHON_X_CORE_X_QUOTE_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <string_view>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/integer_formatter.hpp>


namespace archon::core {


/// \{
///
/// \brief Print string in double-quoted form.
///
/// Construct an object that, if written to an output stream, prints the specified string in
/// double-quoted form (`"..."`).
///
/// In double-quoted form, the string is bracketed in double quotes (`"`), and special
/// characters are escaped according to the rules of C/C++ string literals. The characters,
/// that are considered special, are `"` and `\` as well as those that are not printable
/// (`!std::isprint()`).
///
/// Given the strings `foo` and `foo"bar`, for example, the respective outputs will be
/// `"foo"` and `"foo\"bar"`.
///
/// The quoting process happens as the string is written to an output stream, so there is no
/// complete intermediate representation of the quoted form of the string (unless the field
/// width of the target stream is nonzero).
///
/// If a maximum size (\p max_size) is specified (not equal to `std::size_t(-1)`), then the
/// size of the quoted form will be limited to the specified size by replacing a trailing
/// section of characters from the specified string by an ellipsis (`...`). For example,
/// `quoted("foo bar", 9)` will produce `"foo bar"` and `quoted("foo bar", 8)` will produce
/// `"foo..."`.
///
/// More precisely, the *quoted form* is the result of escaping all the special characters
/// in the specified string and adding quotation characters, if that yields a total size
/// that does not exceed a certian adjusted maximum size. In this case, the adjusted maximum
/// size is the larger of the specified maximum size and 5 (size of ellipsis and two
/// quotation characters). Otherwise, the quoted form is the result of escaping special
/// characters in a particular prefix of the string, then adding ellipsis and quotation
/// characters. The particular prefix is the longest prefix such that the total size of the
/// quoted form does not exceed the adjusted maximum size.
///
/// The field width of the target stream will be respected, and the effect will be as if the
/// entire quoted form was written to the stream as a single string object.
///
template<class C> auto quoted(const C* c_str, std::size_t max_size = std::size_t(-1));
template<class C, class T> auto quoted(std::basic_string_view<C, T> string, std::size_t max_size = std::size_t(-1));
/// \}



/// \{
///
/// \brief Print string in single-quoted form.
///
/// These functions are like \ref quoted(), but they use single quotes (`'`) instead of
/// double-quotes (`"`).
///
template<class C> auto quoted_s(const C* c_str, std::size_t max_size = std::size_t(-1));
template<class C, class T> auto quoted_s(std::basic_string_view<C, T> string, std::size_t max_size = std::size_t(-1));
/// \}



/// \{
///
/// \brief Print string in double-quoted form when necessary.
///
/// This function is similar to \ref quoted(), but skips quotation when the specified string
/// is sufficiently simple (see below). For example, `smart_quoted("xy")` will yield `xy`,
/// whereas `smart_quoted("x y")` will yield `"x y"`.
///
/// The unquoted form can also be used when the string is larger than the specified maximum
/// size, provided that the prefix before the ellipsis is sufficiently simple. For example,
/// `smart_quoted("foo bar", 6)` will yield `foo...`.
///
/// If a maximum size (\p max_size) is specified (not equal to `std::size_t(-1)`), and it is
/// greater than, or equal to 5 (size of ellipsis and two quotation characters), the size of
/// the printed string is guaranteed to not exceed the specified maximum size.
///
/// When no maximum size is specified (or when the specified maximum size is large enough),
/// the unquoted form is used when, and only when the specified string is nonempty, contains
/// no space-like characters (`std::ctype_base::space`), and contains no special
/// characters. Here, *special characters* are nonprintable chracters
/// (`std::ctype_base::print`), quotation character (`"`), and backslash (`\`).
///
/// When a maximum size is specified, and both the unquoted and quoted forms (see below)
/// have sizes that do not exceed that maximum size, the unquoted form is used when, and
/// only when the specified string is nonempty, and the unquoted form includes at least as
/// many characters from the specified string as the quoted form.
///
/// When a maximum size is specified, and at least one of the forms is larger than that
/// maximum size, the unquoted form is used when, and only when the specified string is
/// nonempty, and
///
/// - the unquoted form is smaller than the quoted form, or
///
/// - the two forms have the same size and the unquoted form includes at least as many
///   characters from the specified string as the quoted form.
///
/// Here, the *unquoted form* is the specified string itself if the size of that string does
/// not exceed a certain adjusted maximum size, and the string does not contain space-like,
/// or special characters. In this case, the adjusted maximum size is the larger of the
/// specified maximum size and 3 (size of ellipsis). Otherwise, the unquoted form is the
/// longest possible prefix of the specified string followed by an ellipsis, such that the
/// total size, including the ellipsis, does not exceed the adjusted maximum size, and such
/// that the prefix does not contain any space-like or special characters.
///
/// See \ref quoted() for a detailed description of the quoted form.
///
template<class C> auto smart_quoted(const C* c_str, std::size_t max_size = std::size_t(-1));
template<class C, class T> auto smart_quoted(std::basic_string_view<C, T> string,
                                             std::size_t max_size = std::size_t(-1));
/// \}



/// \{
///
/// \brief Print string in single-quoted form when necessary.
///
/// These functions are like \ref smart_quoted(), but they use single quotes (`'`) instead
/// of double-quotes (`"`).
///
template<class C> auto smart_quoted_s(const C* c_str, std::size_t max_size = std::size_t(-1));
template<class C, class T> auto smart_quoted_s(std::basic_string_view<C, T> string,
                                               std::size_t max_size = std::size_t(-1));
/// \}








// Implementation


namespace impl {


template<class C, class T = std::char_traits<C>> struct AsQuoted {
    std::basic_string_view<C, T> string;
    std::size_t max_size;
    bool smart;
    char quote;
};


template<class C, class T>
void do_quote(core::BasicStreamOutputHelper<C, T>& helper, const std::ctype<C>& ctype, const impl::AsQuoted<C, T>& pod)
{
    // Assumption: There are no escape sequences of size less than 1
    using string_view_type = std::basic_string_view<C, T>;
    core::BasicCharMapper mapper(ctype);
    char chars_1[] = { pod.quote, '.', '0', '7', '\\' };
    const std::size_t num_chars = sizeof chars_1 / sizeof *chars_1;
    C chars_2[num_chars];
    mapper.widen(std::string_view(chars_1, num_chars), chars_2); // Throws
    C quote  = chars_2[0]; // `"`
    C dot    = chars_2[1]; // `.`
    C zero   = chars_2[2]; // `0`
    C seven  = chars_2[3]; // `7`
    C bslash = chars_2[4]; // `\`
    C ellipsis[] = { dot, dot, dot }; // `...`
    std::size_t size_of_ellipsis = std::size(ellipsis);
    std::size_t size_of_quotes = 2; // Size of two quotation characters (`""`)
    core::BasicIntegerFormatter<C, T> integer_formatter(mapper);
    core::Vector<C, 24> buffer;

    std::size_t size = pod.string.size();
    const C* begin = pod.string.data();
    const C* end   = begin + size;
    const C* i_1 = begin;

    bool follows_hex = false;
    auto find = [&](string_view_type substr) noexcept {
        bool follows_hex_2 = follows_hex;
        const C* i = substr.data();
        const C* begin = i;
        const C* end   = i + substr.size();
        while (ARCHON_LIKELY(i != end)) {
            C ch = *i;
            bool requires_escaping = (ch == quote || ch == bslash || !ctype.is(ctype.print, ch) ||
                                      (follows_hex_2 && ctype.is(ctype.xdigit, ch)));
            if (ARCHON_LIKELY(!ctype.is(ctype.space, ch) && !requires_escaping)) {
                follows_hex_2 = false;
                ++i;
                continue;
            }
            break;
        }
        return std::size_t(i - begin);
    };

    auto render = [&](const C* i) {
        C ch = *i;
        if (ARCHON_LIKELY(ctype.is(ctype.print, ch))) {
            if (ARCHON_LIKELY(!follows_hex || !ctype.is(ctype.xdigit, ch))) {
                if (ARCHON_LIKELY(ch != quote && ch != bslash))
                    goto put_char;
                goto escape_char;
            }
        }
        {
            char ch_2 = mapper.narrow(ch, '\0'); // Throws
            switch (ch_2) {
                case '\a':
                    ch = mapper.widen('a'); // Throws
                    goto escape_char;
                case '\b':
                    ch = mapper.widen('b'); // Throws
                    goto escape_char;
                case '\t':
                    ch = mapper.widen('t'); // Throws
                    goto escape_char;
                case '\n':
                    ch = mapper.widen('n'); // Throws
                    goto escape_char;
                case '\v':
                    ch = mapper.widen('v'); // Throws
                    goto escape_char;
                case '\f':
                    ch = mapper.widen('f'); // Throws
                    goto escape_char;
                case '\r':
                    ch = mapper.widen('r'); // Throws
                    goto escape_char;
            }
        }
        goto numeric;
      escape_char:
        buffer.push_back(bslash); // Throws
      put_char:
        buffer.push_back(ch); // Throws
      not_hex:
        follows_hex = false;
        return;
      numeric:
        buffer.push_back(bslash); // Throws
        constexpr int char_width = core::int_width<C>();
        using uint_type = core::fast_unsigned_int_type<char_width>;
        static_assert(!std::is_same_v<uint_type, void>); // A hope more than a certainty
        uint_type val = uint_type(ch);
        constexpr int uint_width = core::int_width<uint_type>();
        if constexpr (std::is_signed_v<C> && uint_width > char_width)
            val &= (uint_type(1) << char_width) - 1;
        if (val < 512) {
            // Octal form
            int min_num_digits = 3;
            if (i + 1 == end || i[1] < zero || i[1] > seven)
                min_num_digits = 1;
            string_view_type str = integer_formatter.format_oct(val, min_num_digits); // Throws
            buffer.append(str.begin(), str.end()); // Throws
            goto not_hex;
        }
        // Hexadecimal form
        buffer.push_back(mapper.widen('x')); // Throws
        string_view_type str = integer_formatter.format_hex(val); // Throws
        buffer.append(str.begin(), str.end()); // Throws
        follows_hex = true;
    };

    bool no_max_size = (pod.max_size == std::size_t(-1));
    std::size_t max_size_1 = size_of_ellipsis + size_of_quotes;
    if (ARCHON_LIKELY(pod.max_size >= max_size_1))
        max_size_1 = pod.max_size;
    std::size_t max_size_2 = max_size_1 - size_of_quotes;
    std::size_t max_size_3 = max_size_2 - size_of_ellipsis;
    std::size_t i_2 = 0;
    std::size_t n_1, n_2, n_3, j_2;

    bool try_unquoted = (pod.smart && i_1 != end);
    if (try_unquoted) {
        std::size_t max_size_4 = size_of_ellipsis;
        if (ARCHON_LIKELY(pod.max_size >= max_size_4))
            max_size_4 = pod.max_size;

        if (ARCHON_LIKELY(no_max_size || size <= max_size_4)) {
            j_2 = find(pod.string);
            if (ARCHON_LIKELY(j_2 == size)) {
                helper.write({ begin, size }); // Throws
                return;
            }
            if (ARCHON_LIKELY(no_max_size || j_2 <= max_size_4 - size_of_ellipsis)) {
                i_1 += j_2;
            }
            else {
                i_1 += max_size_4 - size_of_ellipsis;
            }
        }
        else {
            i_1 += find({ begin, max_size_4 - size_of_ellipsis });
        }

        // At this point, `i_1` points to the first character that is not
        // present in the unquoted form.

        render(i_1); // Throws

        const C* j_1 = i_1;
        std::size_t n_1 = std::size_t(j_1 - begin);
        std::size_t n_2 = n_1;
        if (ARCHON_UNLIKELY(!core::try_int_add(n_2, buffer.size())))
            n_2 = std::size_t(-1);
        if (ARCHON_LIKELY(no_max_size || n_2 <= max_size_3)) {
            helper.put(quote); // Throws
            helper.write({ begin, n_1 }); // Throws
            i_2 = n_1;
            goto write_buffer;
        }
      again_1:
        if (ARCHON_LIKELY(n_2 > max_size_2)) {
            // The character at `i_1` is not present in the quoted form, so the unquoted
            // form includes at least as many characters from the specified string as the
            // quoted form.
            //
            // If the two forms include the same number of characters from the specified
            // string, the unquoted form has to be smaller than the quoted form, so the
            // unquoted form is the correct choice.
            //
            // If the unquoted form includes more characters than the quoted form, the
            // unquoted form must include at least one character, but it would not contain
            // any characters if its size was greater than the specified maximum size, so,
            // its size is less than, or equal to the specified maximum size. Note that this
            // argument relies on the fact that the unquoted form must include an ellipsis
            // (because if it did not, we would not have reached this point). If the size of
            // the quoted form is greater than the specified maximum size, the correct
            // choice is the unquoted form because it is smaller. If the size of the quoted
            // form is less than, or equal to the specified maximum size, the correct choice
            // is the unquoted form because it contains at least as many characters from the
            // specified string as the quoted form.
            //
            // Ergo, in all these cases, the correct choice is the unquoted form.

          unquoted:
            helper.write({ begin, n_1 }); // Throws
            helper.write({ ellipsis, size_of_ellipsis }); // Throws
            return;
        }
        ++j_1;
        if (ARCHON_LIKELY(j_1 != end)) {
            render(j_1); // Throws
            n_2 = n_1;
            if (ARCHON_UNLIKELY(!core::try_int_add(n_2, buffer.size())))
                n_2 = std::size_t(-1);
            goto again_1;
        }

        // The character at `i_1` is present in the quoted form, so the quoted form contains
        // more characters from the specified string than the unquoted form.

        {
            bool quoted_form_is_oversized = (n_2 > pod.max_size || size_of_quotes > pod.max_size - n_2);
            bool quoted_form_is_larger = (buffer.size() > size_of_ellipsis ||
                                          size_of_quotes > size_of_ellipsis - buffer.size());
            if (ARCHON_LIKELY(!quoted_form_is_oversized || !quoted_form_is_larger)) {
                helper.put(quote); // Throws
                helper.write({ begin, n_1 }); // Throws
                goto close_2;
            }
        }
        goto unquoted;
    }

    // Quoted form
    helper.put(quote); // Throws
  again_2:
    ARCHON_ASSERT(i_1 <= end);
    n_1 = std::size_t(end - i_1);
    ARCHON_ASSERT(no_max_size || i_2 <= max_size_2);
    n_2 = std::size_t(max_size_2 - i_2);
    if (ARCHON_LIKELY(no_max_size || n_1 <= n_2)) {
        j_2 = find({ i_1, n_1 });
        if (ARCHON_LIKELY(j_2 == n_1)) {
            helper.write({ i_1, n_1 }); // Throws
          close_1:
            helper.put(quote); // Throws
            return;
        }
        ARCHON_ASSERT(i_2 <= max_size_3);
        n_3 = std::size_t(max_size_3 - i_2);
        if (ARCHON_LIKELY(no_max_size || j_2 < n_3)) {
          escape:
            helper.write({ i_1, j_2 }); // Throws
            i_1 += j_2;
            if (ARCHON_LIKELY(j_2 > 0))
                follows_hex = false;
            render(i_1); // Throws
            if (ARCHON_LIKELY(no_max_size || buffer.size() <= n_3 - j_2)) {
                i_2 += j_2;
              write_buffer:
                helper.write({ buffer.data(), buffer.size() }); // Throws
                i_1 += 1;
                i_2 += buffer.size();
                buffer.clear();
                goto again_2;
            }
            n_2 -= j_2;
            goto render_more;
        }
        j_2 = n_3;
        helper.write({ i_1, j_2 }); // Throws
        i_1 += j_2;
        if (ARCHON_LIKELY(j_2 > 0))
            follows_hex = false;
        render(i_1); // Throws
        n_2 -= j_2;
      render_more:
        if (ARCHON_LIKELY(buffer.size() > n_2))
            goto ellipsis;
        ++i_1;
        if (ARCHON_LIKELY(i_1 != end)) {
            render(i_1); // Throws
            goto render_more;
        }
      close_2:
        helper.write({ buffer.data(), buffer.size() }); // Throws
        goto close_1;
    }
    ARCHON_ASSERT(i_2 <= max_size_3);
    n_3 = max_size_3 - i_2;
    j_2 = find({ i_1, n_3 });
    if (ARCHON_LIKELY(j_2 == n_3)) {
        helper.write({ i_1, j_2 }); // Throws
      ellipsis:
        helper.write({ ellipsis, size_of_ellipsis }); // Throws
        goto close_1;
    }
    goto escape;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsQuoted<C, T>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 64> seed_memory;
    return core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<C, T>& helper) {
        std::locale loc = out.getloc(); // Throws
        const std::ctype<C>& ctype = std::use_facet<std::ctype<C>>(loc); // Throws
        do_quote(helper, ctype, pod); // Throws
    }, core::Span(seed_memory)); // Throws
}


} // namespace impl


template<class C> inline auto quoted(const C* c_str, std::size_t max_size)
{
    std::basic_string_view<C> string = c_str; // Throws
    return core::quoted(string, max_size); // Throws
}


template<class C, class T> inline auto quoted(std::basic_string_view<C, T> string, std::size_t max_size)
{
    bool smart = false;
    return impl::AsQuoted<C, T> { string, max_size, smart, '"' };
}


template<class C> inline auto quoted_s(const C* c_str, std::size_t max_size)
{
    std::basic_string_view<C> string = c_str; // Throws
    return core::quoted_s(string, max_size); // Throws
}


template<class C, class T> inline auto quoted_s(std::basic_string_view<C, T> string, std::size_t max_size)
{
    bool smart = false;
    return impl::AsQuoted<C, T> { string, max_size, smart, '\'' };
}


template<class C> inline auto smart_quoted(const C* c_str, std::size_t max_size)
{
    std::basic_string_view<C> string = c_str; // Throws
    return core::smart_quoted(string, max_size); // Throws
}


template<class C, class T> inline auto smart_quoted(std::basic_string_view<C, T> string, std::size_t max_size)
{
    bool smart = true;
    return impl::AsQuoted<C, T> { string, max_size, smart, '"' };
}


template<class C> inline auto smart_quoted_s(const C* c_str, std::size_t max_size)
{
    std::basic_string_view<C> string = c_str; // Throws
    return core::smart_quoted_s(string, max_size); // Throws
}


template<class C, class T> inline auto smart_quoted_s(std::basic_string_view<C, T> string, std::size_t max_size)
{
    bool smart = true;
    return impl::AsQuoted<C, T> { string, max_size, smart, '\'' };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_QUOTE_HPP
