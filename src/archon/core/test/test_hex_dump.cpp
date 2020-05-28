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
#include <array>
#include <string_view>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/hex_dump.hpp>
#include <archon/core/mul_prec_int.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_HexDump_MaxSize)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    auto check_1 = [&](core::Span<const int> data, core::HexDumpConfig config, std::string_view result) {
        std::string_view formatted = formatter.format(core::as_hex_dump(data, config));
        ARCHON_CHECK_EQUAL(formatted, result);
    };

    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    auto check_2 = [&](core::Span<const int> data, core::HexDumpConfig config, std::wstring_view result) {
        std::wstring_view formatted = wformatter.format(core::as_hex_dump(data, config));
        ARCHON_CHECK_EQUAL(formatted, result);
    };

    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&](core::Span<const int> data, int min_digits, std::size_t max_size, std::string_view result) {
        core::HexDumpConfig config;
        config.min_digits = min_digits;
        config.max_size = max_size;
        check_1(data, config, result);
        std::wstring_view wresult = widener.widen(result);
        check_2(data, config, wresult);
    };

    check(std::array { 0 },       1, 0, "0");
    check(std::array { 0, 0 },    1, 0, "0 0");
    check(std::array { 0, 0, 0 }, 1, 0, "...");

    check(std::array { 0 },       1, 1, "0");
    check(std::array { 0, 0 },    1, 1, "0 0");
    check(std::array { 0, 0, 0 }, 1, 1, "...");

    check(std::array { 0 },       1, 2, "0");
    check(std::array { 0, 0 },    1, 2, "0 0");
    check(std::array { 0, 0, 0 }, 1, 2, "...");

    check(std::array { 0 },       1, 3, "0");
    check(std::array { 0, 0 },    1, 3, "0 0");
    check(std::array { 0, 0, 0 }, 1, 3, "...");

    check(std::array { 0 },       1, 4, "0");
    check(std::array { 0, 0 },    1, 4, "0 0");
    check(std::array { 0, 0, 0 }, 1, 4, "0...");

    check(std::array { 0 },       1, 5, "0");
    check(std::array { 0, 0 },    1, 5, "0 0");
    check(std::array { 0, 0, 0 }, 1, 5, "0 0 0");

    check(std::array { 0 },       2, 0, "00");
    check(std::array { 0, 0 },    2, 0, "...");
    check(std::array { 0, 0, 0 }, 2, 0, "...");

    check(std::array { 0 },       2, 1, "00");
    check(std::array { 0, 0 },    2, 1, "...");
    check(std::array { 0, 0, 0 }, 2, 1, "...");

    check(std::array { 0 },       2, 2, "00");
    check(std::array { 0, 0 },    2, 2, "...");
    check(std::array { 0, 0, 0 }, 2, 2, "...");

    check(std::array { 0 },       2, 3, "00");
    check(std::array { 0, 0 },    2, 3, "...");
    check(std::array { 0, 0, 0 }, 2, 3, "...");

    check(std::array { 0 },       2, 4, "00");
    check(std::array { 0, 0 },    2, 4, "...");
    check(std::array { 0, 0, 0 }, 2, 4, "...");

    check(std::array { 0 },       2, 5, "00");
    check(std::array { 0, 0 },    2, 5, "00 00");
    check(std::array { 0, 0, 0 }, 2, 5, "00...");

    check(std::array { 0 },       2, 6, "00");
    check(std::array { 0, 0 },    2, 6, "00 00");
    check(std::array { 0, 0, 0 }, 2, 6, "00...");

    check(std::array { 0 },       2, 7, "00");
    check(std::array { 0, 0 },    2, 7, "00 00");
    check(std::array { 0, 0, 0 }, 2, 7, "00...");

    check(std::array { 0 },       2, 8, "00");
    check(std::array { 0, 0 },    2, 8, "00 00");
    check(std::array { 0, 0, 0 }, 2, 8, "00 00 00");
}


ARCHON_TEST(Core_HexDump_Separator)
{
    std::array<char, 256> seed_memory_1;
    core::ValueFormatter formatter(seed_memory_1, std::locale::classic());
    auto check_1 = [&](core::Span<const int> data, std::string_view separator, core::HexDumpConfig config,
                       std::string_view result) {
        std::string_view formatted = formatter.format(core::as_hex_dump(data, separator, config));
        ARCHON_CHECK_EQUAL(formatted, result);
    };

    std::array<wchar_t, 256> seed_memory_2;
    core::WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    auto check_2 = [&](core::Span<const int> data, std::wstring_view separator, core::HexDumpConfig config,
                       std::wstring_view result) {
        std::wstring_view formatted = wformatter.format(core::as_hex_dump(data, separator, config));
        ARCHON_CHECK_EQUAL(formatted, result);
    };

    std::array<wchar_t, 256> seed_memory_3;
    core::WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check = [&](core::Span<const int> data, std::string_view separator, std::string_view result) {
        core::HexDumpConfig config;
        config.min_digits = 2;
        check_1(data, separator, config, result);
        std::wstring_view wseparator, wresult;
        core::WideStringWidener::Entry entries[] = {
            { separator, &wseparator },
            { result, &wresult }
        };
        widener.widen(entries);
        check_2(data, wseparator, config, wresult);
    };

    check(std::array { 0, 0 }, "x", "00x00");
}


namespace {


template<class C> void check_output_stream_field_width(check::TestContext& test_context)
{
    const char data[] = { 0 };
    std::array<C, 8> buffer;
    core::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    out.width(4);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    core::HexDumpConfig config;
    config.min_digits = 2;
    out << core::as_hex_dump(core::Span(data), config);
    std::array<C, 8> seed_memory;
    core::BasicStringWidener widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  00"));
}


} // unnamed namespace


ARCHON_TEST(Core_HexDump_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_HexDump_ExoticIntType, char_variants)
{
    using char_type   = test_type;
    using traits_type = std::char_traits<char_type>;
    using value_formatter_type = core::BasicValueFormatter<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 32> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 32> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    using int_type = core::MulPrecInt<unsigned char, 4, true>;
    int_type data[] = {
        core::int_cast_a<int_type>(0x27FEB233),
        core::int_cast_a<int_type>(0x58FA9A0C),
    };
    ARCHON_CHECK_EQUAL(formatter.format(core::as_hex_dump(data)),
                       widener.widen("27FEB233 58FA9A0C"));
}
