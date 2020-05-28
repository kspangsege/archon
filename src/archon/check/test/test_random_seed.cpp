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
#include <initializer_list>
#include <string_view>

#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_list.hpp>
#include <archon/check.hpp>
#include <archon/check/random_seed.hpp>


using namespace archon;


ARCHON_TEST(Check_RandomSeed_StreamOutputOutput)
{
    using value_type = check::RandomSeed::value_type;
    core::ValueFormatter formatter(test_context.locale);
    core::ValueParser parser(test_context.locale);

    auto test = [&](check::TestContext& parent_test_context, std::initializer_list<value_type> words,
                    std::string_view string) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s vs %s", core::as_list(words), string));
        check::RandomSeed seed(core::Span(words.begin(), words.end()));
        std::string_view string_2 = formatter.format(seed);
        ARCHON_CHECK_EQUAL(string_2, string);
        check::RandomSeed seed_2;
        if (ARCHON_CHECK(parser.parse(string, seed_2)))
            ARCHON_CHECK_EQUAL_SEQ(seed_2, words);
    };

    test(test_context, {}, "-");
    test(test_context, {  0, 0, 0, 0, 0, 0 }, "000000000000000000000000000000000");
    test(test_context, {  1, 0, 0, 0, 0, 0 }, "000000000010000000000000000000000");
    test(test_context, { 61, 0, 0, 0, 0, 0 }, "0000000000z0000000000000000000000");
    test(test_context, { 62, 0, 0, 0, 0, 0 }, "000000000100000000000000000000000");
    test(test_context, {  0, 1, 0, 0, 0, 0 }, "000004gfFC40000000000000000000000");
    test(test_context, {  1, 1, 0, 0, 0, 0 }, "000004gfFC50000000000000000000000");
    test(test_context, {  0, 0, 1, 1, 0, 0 }, "00000000000000004gfFC500000000000");
    test(test_context, {  0, 0, 0, 0, 1, 1 }, "0000000000000000000000000004gfFC5");
    test(test_context, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         "000000000000000000000000000000000-000000000000000000000000000000000");
    test(test_context, { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
         "000000000000000000000000000000000-000000000010000000000000000000000");
}


ARCHON_TEST(Check_RandomSeed_RandomizedStreamInputOutput)
{
    using value_type = check::RandomSeed::value_type;
    core::ValueFormatter formatter(test_context.locale);
    core::WideValueFormatter wformatter(test_context.locale);
    core::ValueParser parser(test_context.locale);
    core::WideValueParser wparser(test_context.locale);

    auto test = [&](const check::RandomSeed& seed, core::Span<value_type> values) {
        {
            std::string_view string = formatter.format(seed);
            check::RandomSeed seed_2;
            if (ARCHON_CHECK(parser.parse(string, seed_2)))
                ARCHON_CHECK_EQUAL_SEQ(seed_2, values);
        }
        {
            std::wstring_view string = wformatter.format(seed);
            check::RandomSeed seed_2;
            if (ARCHON_CHECK(wparser.parse(string, seed_2)))
                ARCHON_CHECK_EQUAL_SEQ(seed_2, values);
        }
    };

    std::mt19937_64 random(test_context.seed_seq());

    constexpr std::size_t max_num_values = 20;
    std::array<value_type, max_num_values> buffer;

    std::uniform_int_distribution<std::size_t> num_values_distr(0, max_num_values);
    std::uniform_int_distribution<value_type> value_distr(0, core::int_mask<value_type>(32));

    for (int i = 0; i < 128; ++i) {
        std::size_t num_values = num_values_distr(random);
        for (std::size_t j = 0; j < num_values; ++j)
            buffer[j] = value_distr(random);
        std::size_t num_blocks = std::size_t(num_values / 6);
        check::RandomSeed seed(core::Span(buffer.data(), num_values));
        std::size_t num_copied_values = std::size_t(num_blocks * 6);
        ARCHON_CHECK_EQUAL(seed.size(), num_copied_values);
        test(seed, core::Span(buffer.data(), num_copied_values));
    }
}


namespace {


template<class C> void check_output_stream_field_width(check::TestContext& test_context)
{
    check::RandomSeed seed;
    std::array<C, 256> buffer;
    core::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    out.width(3);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    out << seed;
    std::array<C, 8> seed_memory;
    core::BasicStringWidener widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  -"));
}


} // unnamed namespace


ARCHON_TEST(Check_RandomSeed_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}
