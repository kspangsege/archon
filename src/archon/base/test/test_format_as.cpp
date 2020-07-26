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


#include <array>

#include <archon/base/char_mapper.hpp>
#include <archon/base/value_formatter.hpp>
#include <archon/base/format_as.hpp>
#include <archon/base/format_with.hpp>
#include <archon/unit_test.hpp>


using namespace archon;


namespace {


template<class C> void check_list(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 16> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    int list[] = { 1, 2, 3 };
    std::array<C, 16> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_list(list)),
                       widener.widen("[1, 2, 3]"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_list(list), 11)),
                       widener.widen("  [1, 2, 3]"));
}


template<class C> void check_ordinal(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 8> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 8> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_ordinal(7)),
                       widener.widen("7th"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_ordinal(7), 5)),
                       widener.widen("  7th"));
}


template<class C> void check_num_of(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 12> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 12> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    base::NumOfSpec cars = { "car", "cars" };
    ARCHON_CHECK_EQUAL(formatter.format(base::as_num_of(3, cars)),
                       widener.widen("3 cars"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_num_of(3, cars), 8)),
                       widener.widen("  3 cars"));
}


template<class C> void check_time(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 8> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 8> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_time(7)),
                       widener.widen("7s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_time(7), 4)),
                       widener.widen("  7s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::as_time(427)),
                       widener.widen("7m7s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_time(427), 6)),
                       widener.widen("  7m7s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::as_time(25627)),
                       widener.widen("7h7m"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_time(25627), 6)),
                       widener.widen("  7h7m"));
}


template<class C> void check_byte_size(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 12> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 12> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_byte_size(3)),
                       widener.widen("3 bytes"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_byte_size(3), 9)),
                       widener.widen("  3 bytes"));
    ARCHON_CHECK_EQUAL(formatter.format(base::as_byte_size(3075)),
                       widener.widen("3 KiB"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_byte_size(3075), 7)),
                       widener.widen("  3 KiB"));
}


template<class C> void check_quant(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 12> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 12> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_quant(7, " m/s")),
                       widener.widen("7 m/s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_quant(7, " m/s"), 7)),
                       widener.widen("  7 m/s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::as_quant(7007, " m/s")),
                       widener.widen("7.01 km/s"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_quant(7007, " m/s"), 11)),
                       widener.widen("  7.01 km/s"));
}


template<class C> void check_quant_bin(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 12> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 12> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(base::as_quant_bin(7, " B")),
                       widener.widen("7 B"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_quant_bin(7, " B"), 5)),
                       widener.widen("  7 B"));
    ARCHON_CHECK_EQUAL(formatter.format(base::as_quant_bin(7175, " B")),
                       widener.widen("7.01 KiB"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_quant_bin(7175, " B"), 10)),
                       widener.widen("  7.01 KiB"));
}


template<class C> void check_format_func(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    std::array<C, 4> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale);
    std::array<C, 4> seed_memory_2;
    base::BasicStringWidener widener(locale, seed_memory_2);
    auto func = [](std::basic_ostream<C>& out) {
        out << "x";
    };
    ARCHON_CHECK_EQUAL(formatter.format(base::as_format_func(func)),
                       widener.widen("x"));
    ARCHON_CHECK_EQUAL(formatter.format(base::with_width(base::as_format_func(func), 3)),
                       widener.widen("  x"));
}


} // unnamed namespace


ARCHON_TEST(Base_FormatAs_List)
{
    check_list<char>(test_context);
    check_list<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_Ordinal)
{
    check_ordinal<char>(test_context);
    check_ordinal<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_NumOf)
{
    check_num_of<char>(test_context);
    check_num_of<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_Time)
{
    check_time<char>(test_context);
    check_time<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_ByteSize)
{
    check_byte_size<char>(test_context);
    check_byte_size<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_Quant)
{
    check_quant<char>(test_context);
    check_quant<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_QuantBin)
{
    check_quant_bin<char>(test_context);
    check_quant_bin<wchar_t>(test_context);
}


ARCHON_TEST(Base_FormatAs_FormatFunc)
{
    check_format_func<char>(test_context);
    check_format_func<wchar_t>(test_context);
}
