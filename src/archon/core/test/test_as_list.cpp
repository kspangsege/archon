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
#include <string_view>

#include <archon/core/features.h>
#include <archon/core/char_mapper.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_with.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_AsList_General, char_variants)
{
    using char_type   = test_type;
    using traits_type = std::char_traits<char_type>;
    using value_formatter_type = core::BasicValueFormatter<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 16> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 16> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    int list[] = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL(formatter.format(core::as_sbr_list(list)),
                       widener.widen("[1, 2, 3]"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_sbr_list(list), 11)),
                       widener.widen("  [1, 2, 3]"));
}


ARCHON_TEST_BATCH(Core_AsList_FormatAndParseWithFunc, char_variants)
{
    using char_type   = test_type;
    using traits_type = std::char_traits<char_type>;
    using string_view_type     = std::basic_string_view<char_type, traits_type>;
    using value_formatter_type = core::BasicValueFormatter<char_type, traits_type>;
    using value_parser_type    = core::BasicValueParser<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 16> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    value_parser_type parser(test_context.locale);
    std::array<char_type, 16> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    int list_1[3] = { 0xE3, 0x27, 0x4A };
    int list_2[3] = {};
    auto func = [](int& v) {
        return core::as_hex_int(v);
    };
    string_view_type str = widener.widen("E3, 27, 4A");
    ARCHON_CHECK_EQUAL(formatter.format(core::as_list(list_1, func)), str);
    if (ARCHON_LIKELY(ARCHON_CHECK(parser.parse(str, core::as_list(list_2, func)))))
        ARCHON_CHECK_EQUAL_SEQ(list_2, list_1);
}


ARCHON_TEST_BATCH(Core_AsListA_General, char_variants)
{
    using char_type   = test_type;
    using traits_type = std::char_traits<char_type>;
    using value_formatter_type = core::BasicValueFormatter<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    std::array<char_type, 16> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 16> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    int list[4] = { 1, 2, 3, 0 };
    ARCHON_CHECK_EQUAL(formatter.format(core::as_list_a(list, 1)),
                       widener.widen("1, 2, 3"));
    ARCHON_CHECK_EQUAL(formatter.format(core::with_width(core::as_list_a(list, 1), 9)),
                       widener.widen("  1, 2, 3"));
}
