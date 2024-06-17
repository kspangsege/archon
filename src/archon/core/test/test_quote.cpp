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


#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <array>
#include <string_view>
#include <random>
#include <set>
#include <locale>
#include <iomanip>

#include <archon/core/features.h>
#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/check.hpp>

// See Cygwin bug filed to mailing list cygwin@cygwin.com and titled: Bug in GCC /
// libstdc++: Space character categorized as non-printable by std::ctype<wchar_t>
//
// See also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115524
//
#if ARCHON_CYGWIN
#  define NO_NONPRINTABLE_SPACE_BUG 0
#else
#  define NO_NONPRINTABLE_SPACE_BUG 1
#endif


using namespace archon;


ARCHON_TEST(Core_Quote_Quoted)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&, &parent_test_context = test_context](const char* c_str, std::size_t max_size,
                                                          std::string_view result) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s, %s", std::quoted(c_str), max_size,
                                                               std::quoted(result)));

        auto check_cs = [&](const char* c_str, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::quoted(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_sv = [&](std::string_view string, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::quoted(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wcs = [&](const wchar_t* c_str, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::quoted(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wsv = [&](std::wstring_view string, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::quoted(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };

        check_cs(c_str, max_size, result);
        check_sv(std::string_view(c_str), max_size, result);
        std::wstring_view wstring, wresult;
        core::WideStringWidener::Entry entries[] = {
            { { c_str, std::strlen(c_str) + 1 }, &wstring },
            { result, &wresult }
        };
        widener.widen(entries);
        const wchar_t* c_wstr = wstring.data();
        check_wcs(c_wstr, max_size, wresult);
        check_wsv(std::wstring_view(c_wstr), max_size, wresult);
    };

    check("x",     2, R"("x")");
    check("xx",    2, R"("xx")");
    check("xxx",   2, R"("xxx")");
    check("xxxx",  2, R"("...")");
    check("xxxxx", 2, R"("...")");

    check("x",     3, R"("x")");
    check("xx",    3, R"("xx")");
    check("xxx",   3, R"("xxx")");
    check("xxxx",  3, R"("...")");
    check("xxxxx", 3, R"("...")");

    check("x",     4, R"("x")");
    check("xx",    4, R"("xx")");
    check("xxx",   4, R"("xxx")");
    check("xxxx",  4, R"("...")");
    check("xxxxx", 4, R"("...")");

    check("x",     5, R"("x")");
    check("xx",    5, R"("xx")");
    check("xxx",   5, R"("xxx")");
    check("xxxx",  5, R"("...")");
    check("xxxxx", 5, R"("...")");

    check("x",     6, R"("x")");
    check("xx",    6, R"("xx")");
    check("xxx",   6, R"("xxx")");
    check("xxxx",  6, R"("xxxx")");
    check("xxxxx", 6, R"("x...")");
}


ARCHON_TEST(Core_Quote_SmartQuoted)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&, &parent_test_context = test_context](const char* c_str, std::size_t max_size,
                                                          std::string_view result) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s, %s", std::quoted(c_str), max_size,
                                                               std::quoted(result)));

        auto check_cs = [&](const char* c_str, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::smart_quoted(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_sv = [&](std::string_view string, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::smart_quoted(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wcs = [&](const wchar_t* c_str, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::smart_quoted(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wsv = [&](std::wstring_view string, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::smart_quoted(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };

        check_cs(c_str, max_size, result);
        check_sv(std::string_view(c_str), max_size, result);
        std::wstring_view wstring, wresult;
        core::WideStringWidener::Entry entries[] = {
            { { c_str, std::strlen(c_str) + 1 }, &wstring },
            { result, &wresult }
        };
        widener.widen(entries);
        const wchar_t* c_wstr = wstring.data();
        check_wcs(c_wstr, max_size, wresult);
        check_wsv(std::wstring_view(c_wstr), max_size, wresult);
    };

    check("",     0, R"("")");
    check("",     1, R"("")");
    check("",     2, R"("")");
    check("",     3, R"("")");
    check("",     4, R"("")");

    check("x",    0, R"(x)");
    check("xx",   0, R"(xx)");
    check("xxx",  0, R"(xxx)");
    check("xxxx", 0, R"(...)");

    check("x",    1, R"(x)");
    check("xx",   1, R"(xx)");
    check("xxx",  1, R"(xxx)");
    check("xxxx", 1, R"(...)");

    check("x",    2, R"(x)");
    check("xx",   2, R"(xx)");
    check("xxx",  2, R"(xxx)");
    check("xxxx", 2, R"(...)");

    check("x",    3, R"(x)");
    check("xx",   3, R"(xx)");
    check("xxx",  3, R"(xxx)");
    check("xxxx", 3, R"(...)");

    check("x",    4, R"(x)");
    check("xx",   4, R"(xx)");
    check("xxx",  4, R"(xxx)");
    check("xxxx", 4, R"(xxxx)");

#if NO_NONPRINTABLE_SPACE_BUG
    check(" ",    0, R"(" ")");
    check("  ",   0, R"(...)");

    check(" ",    1, R"(" ")");
    check("  ",   1, R"(...)");

    check(" ",    2, R"(" ")");
    check("  ",   2, R"(...)");

    check(" ",    3, R"(" ")");
    check("  ",   3, R"(...)");

    check(" ",    4, R"(" ")");
    check("  ",   4, R"("  ")");
#endif

    check("\n",   2, R"(...)");
    check("x\n",  2, R"(...)");
    check("xx\n", 2, R"(...)");

    check("\n",   3, R"(...)");
    check("x\n",  3, R"(...)");
    check("xx\n", 3, R"(...)");

    check("\n",   4, R"("\n")");
    check("x\n",  4, R"(x...)");
    check("xx\n", 4, R"(x...)");

    check("\n",   5, R"("\n")");
    check("x\n",  5, R"("x\n")");
    check("xx\n", 5, R"(xx...)");

    check("\n",   6, R"("\n")");
    check("x\n",  6, R"("x\n")");
    check("xx\n", 6, R"("xx\n")");
}


ARCHON_TEST(Core_Quote_Escape)
{
    std::locale locale = std::locale::classic();
    const std::ctype<wchar_t>& ctype = std::use_facet<std::ctype<wchar_t>>(locale);

    std::array<wchar_t, 256> seed_memory_1;
    core::WideValueFormatter formatter_1(seed_memory_1, locale);
    auto check = [&](core::Span<const wchar_t> string, std::size_t max_size, std::wstring_view result) {
        std::wstring_view string_2(string.data(), string.size());
        std::wstring_view formatted = formatter_1.format(core::quoted(string_2, max_size));
        ARCHON_CHECK_EQUAL(formatted, result);
    };

    // Try to find characters that will be escaped in octal and hexadecimal
    // forms respectively
    wchar_t oct_escape_char = {};
    wchar_t hex_escape_char = {};
    bool found_oct_escape_char = false;
    bool found_hex_escape_char = false;
    constexpr int char_width = core::int_width<wchar_t>();
    using uint_type = core::fast_unsigned_int_type<char_width, std::uintmax_t>;
    uint_type mask = core::int_mask<uint_type>(char_width);
    {
        std::set<wchar_t> specials;
        using namespace std::literals;
        for (char ch : "\a\b\t\n\v\f\r"sv)
            specials.insert(ctype.widen(ch));
        std::mt19937_64 random(test_context.seed_seq());
        uint_type hex_max = mask;
        uint_type oct_max = 511;
        if (hex_max < oct_max)
            oct_max = hex_max;

        auto to_char = [=](uint_type val, wchar_t& ch) noexcept {
            ARCHON_ASSERT(val <= mask);
            if constexpr (std::is_signed_v<wchar_t>) {
                uint_type val_2 = core::twos_compl_sign_extend(val, char_width);
                return core::try_cast_from_twos_compl(val_2, ch);
            }
            ch = wchar_t(val);
            return true;
        };

        {
            std::uniform_int_distribution<uint_type> distr(0, oct_max);
            for (int i = 0; i < 1000; ++i) {
                uint_type val = distr(random);
                wchar_t ch;
                bool good = (to_char(val, ch) && !ctype.is(ctype.print, ch) && specials.count(ch) == 0);
                if (good) {
                    oct_escape_char = ch;
                    found_oct_escape_char = true;
                    break;
                }
            }
        }

        if (hex_max > oct_max) {
            std::uniform_int_distribution<uint_type> distr(oct_max + 1, hex_max);
            for (int i = 0; i < 1000; ++i) {
                uint_type val = distr(random);
                wchar_t ch;
                bool good = (to_char(val, ch) && !ctype.is(ctype.print, ch) && specials.count(ch) == 0);
                if (good) {
                    hex_escape_char = ch;
                    found_hex_escape_char = true;
                    break;
                }
            }
        }
    }

    if (!found_oct_escape_char)
        test_context.logger.warn("Failed to find char for octal escaping");
    if (!found_hex_escape_char)
        test_context.logger.warn("Failed to find char for hexadecimal escaping");

    std::array<wchar_t, 256> seed_memory_2;
    core::WideStringFormatter formatter_2(seed_memory_2, locale);

    auto to_uint = [=](wchar_t ch) noexcept {
        return uint_type(uint_type(ch) & mask);
    };

    if (found_oct_escape_char) {
        wchar_t string[] = {
            ctype.widen('x'),
            oct_escape_char,
            ctype.widen('y')
        };
        std::wstring_view result = formatter_2.format(R"("x\%sy")", core::as_oct_int(to_uint(oct_escape_char)));
        check(core::Span(string), std::size_t(-1), result);
    }

    if (found_hex_escape_char) {
        wchar_t string[] = {
            ctype.widen('x'),
            hex_escape_char,
            ctype.widen('y')
        };
        std::wstring_view result = formatter_2.format(R"("x\x%sy")", core::as_hex_int(to_uint(hex_escape_char)));
        check(core::Span(string), std::size_t(-1), result);
    }

    // FIXME: Add checks for more forms of escaping and more special cases
}


ARCHON_TEST(Core_Quote_SingleQuoted)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&, &parent_test_context = test_context](const char* c_str, std::size_t max_size,
                                                          std::string_view result) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s, %s", std::quoted(c_str), max_size,
                                                               std::quoted(result)));

        auto check_cs = [&](const char* c_str, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::quoted_s(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_sv = [&](std::string_view string, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::quoted_s(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wcs = [&](const wchar_t* c_str, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::quoted_s(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wsv = [&](std::wstring_view string, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::quoted_s(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };

        check_cs(c_str, max_size, result);
        check_sv(std::string_view(c_str), max_size, result);
        std::wstring_view wstring, wresult;
        core::WideStringWidener::Entry entries[] = {
            { { c_str, std::strlen(c_str) + 1 }, &wstring },
            { result, &wresult }
        };
        widener.widen(entries);
        const wchar_t* c_wstr = wstring.data();
        check_wcs(c_wstr, max_size, wresult);
        check_wsv(std::wstring_view(c_wstr), max_size, wresult);
    };

    check("xxxxx",  7, "'xxxxx'");
    check("xxxxxx", 7, "'xx...'");
}


ARCHON_TEST(Core_Quote_SmartSingleQuoted)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&, &parent_test_context = test_context](const char* c_str, std::size_t max_size,
                                                          std::string_view result) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s, %s", std::quoted(c_str), max_size,
                                                               std::quoted(result)));

        auto check_cs = [&](const char* c_str, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::smart_quoted_s(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_sv = [&](std::string_view string, std::size_t max_size, std::string_view result) {
            std::string_view formatted = formatter.format(core::smart_quoted_s(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wcs = [&](const wchar_t* c_str, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::smart_quoted_s(c_str, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };
        auto check_wsv = [&](std::wstring_view string, std::size_t max_size, std::wstring_view result) {
            std::wstring_view formatted = wformatter.format(core::smart_quoted_s(string, max_size));
            ARCHON_CHECK_EQUAL(formatted, result);
        };

        check_cs(c_str, max_size, result);
        check_sv(std::string_view(c_str), max_size, result);
        std::wstring_view wstring, wresult;
        core::WideStringWidener::Entry entries[] = {
            { { c_str, std::strlen(c_str) + 1 }, &wstring },
            { result, &wresult }
        };
        widener.widen(entries);
        const wchar_t* c_wstr = wstring.data();
        check_wcs(c_wstr, max_size, wresult);
        check_wsv(std::wstring_view(c_wstr), max_size, wresult);
    };

    check("xxxxx",  7, "xxxxx");
#if NO_NONPRINTABLE_SPACE_BUG
    check("xx xx",  7, "'xx xx'");
    check("xx xxx", 7, "xx...");
    check("x xxxx", 7, "'x ...'");
#endif
}


namespace {


template<class C> void check_output_stream_field_width(check::TestContext& test_context)
{
    std::array<C, 8> seed_memory;
    core::BasicStringWidener widener(test_context.locale, seed_memory);
    std::array<C, 8> buffer;
    core::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    out.width(5);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    out << core::quoted(widener.widen("x"));
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  \"x\""));
}


} // unnamed namespace


ARCHON_TEST(Core_Quote_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}
