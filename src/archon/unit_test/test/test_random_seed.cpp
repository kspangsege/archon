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


#include <cstddef>
#include <initializer_list>
#include <string_view>

#include <archon/base/char_mapper.hpp>
#include <archon/base/memory_output_stream.hpp>
#include <archon/base/value_formatter.hpp>
#include <archon/base/value_parser.hpp>
#include <archon/unit_test/random_seed.hpp>
#include <archon/unit_test.hpp>


using namespace archon;


ARCHON_TEST(UnitTest_RandomSeed_StreamOutput)
{
    using value_type = unit_test::RandomSeed::value_type;
    const std::locale& locale = test_context.get_locale();
    base::ValueFormatter formatter(locale);

    auto test = [&](std::initializer_list<value_type> values, std::string_view string) {
        unit_test::RandomSeed seed(base::Span(values.begin(), values.end()));
        std::string_view string_2 = formatter.format(seed);
        ARCHON_CHECK_EQUAL(string_2, string);
    };

    test({}, "-");
    test({0, 0, 0, 0, 0, 0}, "-000000000000000000000000000000000-");
    test({ 1, 0, 0, 0, 0, 0}, "-000000000010000000000000000000000-");
    test({61, 0, 0, 0, 0, 0}, "-0000000000z0000000000000000000000-");
    test({62, 0, 0, 0, 0, 0}, "-000000000100000000000000000000000-");
    test({ 0, 1, 0, 0, 0, 0}, "-000004gfFC40000000000000000000000-");
    test({ 1, 1, 0, 0, 0, 0}, "-000004gfFC50000000000000000000000-");
    test({ 0, 0, 1, 1, 0, 0}, "-00000000000000004gfFC500000000000-");
    test({ 0, 0, 0, 0, 1, 1}, "-0000000000000000000000000004gfFC5-");
    test({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
         "-000000000000000000000000000000000-000000000000000000000000000000000-");
    test({0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
         "-000000000000000000000000000000000-000000000010000000000000000000000-");
}


ARCHON_TEST(UnitTest_RandomSeed_StreamInput)
{
    using value_type = unit_test::RandomSeed::value_type;
    const std::locale& locale = test_context.get_locale();
    base::ValueParser parser(locale);

    auto test = [&](std::string_view string, std::initializer_list<value_type> values) {
        unit_test::RandomSeed seed;
        if (ARCHON_CHECK(parser.parse(string, seed)))
            ARCHON_CHECK_EQUAL_SEQ(seed, values);
    };

    test("-", {});
}


ARCHON_TEST(UnitTest_RandomSeed_RandomizedStreamInputOutput)
{
    using value_type = unit_test::RandomSeed::value_type;
    const std::locale& locale = test_context.get_locale();
    base::ValueFormatter formatter(locale);
    base::WideValueFormatter wformatter(locale);
    base::ValueParser parser(locale);
    base::WideValueParser wparser(locale);

    auto test = [&](const unit_test::RandomSeed& seed, base::Span<value_type> values) {
        {
            std::string_view string = formatter.format(seed);
            unit_test::RandomSeed seed_2;
            if (ARCHON_CHECK(parser.parse(string, seed_2)))
                ARCHON_CHECK_EQUAL_SEQ(seed_2, values);
        }
        {
            std::wstring_view string = wformatter.format(seed);
            unit_test::RandomSeed seed_2;
            if (ARCHON_CHECK(wparser.parse(string, seed_2)))
                ARCHON_CHECK_EQUAL_SEQ(seed_2, values);
        }
    };

    std::mt19937_64 random(test_context.seed_seq());

    constexpr std::size_t max_num_values = 20;
    std::array<value_type, max_num_values> buffer;

    std::uniform_int_distribution<std::size_t> num_values_distr(0, max_num_values);
    std::uniform_int_distribution<value_type> value_distr(0, base::int_mask<value_type>(32));

    for (int i = 0; i < 128; ++i) {
        std::size_t num_values = num_values_distr(random);
        for (std::size_t j = 0; j < num_values; ++j)
            buffer[j] = value_distr(random);
        std::size_t num_blocks = std::size_t(num_values / 6);
        unit_test::RandomSeed seed(base::Span(buffer.data(), num_values));
        std::size_t num_copied_values = std::size_t(num_blocks * 6);
        ARCHON_CHECK_EQUAL(seed.size(), num_copied_values);
        test(seed, base::Span(buffer.data(), num_copied_values));
    }
}


namespace {


template<class C> void check_output_stream_field_width(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.get_locale();
    unit_test::RandomSeed seed;
    std::array<C, 256> buffer;
    base::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale);
    out.width(3);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    out << seed;
    std::array<C, 8> seed_memory;
    base::BasicStringWidener widener(locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  -"));
}


} // unnamed namespace


ARCHON_TEST(UnitTest_RandomSeed_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}
