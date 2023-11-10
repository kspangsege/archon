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

#include <archon/core/char_mapper.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/format_as.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(core::ValueFormatter,     ValueFormatter),
                     ARCHON_TEST_TYPE(core::WideValueFormatter, WideValueFormatter));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_FormatAs_Optional, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 16> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 16> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_optional(std::optional<int>(7), "unknown")),
                       widener.widen("7"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_optional(std::optional<int>(), "unknown")),
                       widener.widen("unknown"));
}


ARCHON_TEST_BATCH(Core_FormatAs_Ordinal, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 8> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 8> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_ordinal(7)),
                       widener.widen("7th"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_ordinal(7), 5)),
                       widener.widen("  7th"));
}


ARCHON_TEST_BATCH(Core_FormatAs_NumOf, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);
    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(locale, seed_memory_2);

    core::NumOfSpec cars = { "car", "cars" };
    ARCHON_CHECK_EQUAL(formatter.format(core::as_num_of(3, cars)),
                       widener.widen("3 cars"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_num_of(3, cars), 8)),
                       widener.widen("  3 cars"));
}


ARCHON_TEST_BATCH(Core_FormatAs_Percent, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);
    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0)),     widener.widen("0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.001)), widener.widen("0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.01)),  widener.widen("1%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.1)),   widener.widen("10%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.999)), widener.widen("99%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(1)),     widener.widen("100%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(2)),     widener.widen("200%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(-2)),    widener.widen("-200%"));

    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0, 1)),      widener.widen("0.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.0001, 1)), widener.widen("0.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.001, 1)),  widener.widen("0.1%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.01, 1)),   widener.widen("1.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.1, 1)),    widener.widen("10.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(0.9999, 1)), widener.widen("99.9%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(1, 1)),      widener.widen("100.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(2, 1)),      widener.widen("200.0%"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_percent(-2, 1)),     widener.widen("-200.0%"));
}


ARCHON_TEST_BATCH(Core_FormatAs_TimeA, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_time_a(7)),
                       widener.widen("7s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_time_a(7), 4)),
                       widener.widen("  7s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time_a(427)),
                       widener.widen("7m7s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_time_a(427), 6)),
                       widener.widen("  7m7s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time_a(25627)),
                       widener.widen("7h7m"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_time_a(25627), 6)),
                       widener.widen("  7h7m"));
}


ARCHON_TEST_BATCH(Core_FormatAs_Time, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    using namespace std::chrono_literals;
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time(3ms)), widener.widen("3ms"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time(4s)), widener.widen("4s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time(5min)), widener.widen("5m0s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_time(6h)), widener.widen("6h0m"));
}


ARCHON_TEST_BATCH(Core_FormatAs_ByteSize, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);
    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_byte_size(3)),
                       widener.widen("3 bytes"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_byte_size(3), 9)),
                       widener.widen("  3 bytes"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_byte_size(3075)),
                       widener.widen("3 KiB"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_byte_size(3075), 7)),
                       widener.widen("  3 KiB"));
}


ARCHON_TEST_BATCH(Core_FormatAs_Quant, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);
    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_quant(7, " m/s")),
                       widener.widen("7 m/s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_quant(7, " m/s"), 7)),
                       widener.widen("  7 m/s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_quant(7007, " m/s")),
                       widener.widen("7.01 km/s"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_quant(7007, " m/s"), 11)),
                       widener.widen("  7.01 km/s"));
}


ARCHON_TEST_BATCH(Core_FormatAs_QuantBin, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);
    std::array<char_type, 12> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, locale);
    std::array<char_type, 12> seed_memory_2;
    string_widener_type widener(locale, seed_memory_2);

    ARCHON_CHECK_EQUAL(formatter.format(core::as_quant_bin(7, " B")),
                       widener.widen("7 B"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_quant_bin(7, " B"), 5)),
                       widener.widen("  7 B"));
    ARCHON_CHECK_EQUAL(formatter.format(core::as_quant_bin(7175, " B")),
                       widener.widen("7.01 KiB"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_quant_bin(7175, " B"), 10)),
                       widener.widen("  7.01 KiB"));
}


ARCHON_TEST_BATCH(Core_FormatAs_FormatFunc, variants)
{
    using value_formatter_type = test_type;
    using char_type   = typename value_formatter_type::char_type;
    using traits_type = typename value_formatter_type::traits_type;
    using string_widener_type = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 4> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 4> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    auto func = [](std::basic_ostream<char_type, traits_type>& out) {
        out << "x";
    };
    ARCHON_CHECK_EQUAL(formatter.format(core::as_format_func(func)),
                       widener.widen("x"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_format_func(func), 3)),
                       widener.widen("  x"));
}
