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
#include <archon/core/integer_formatter.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(core::IntegerFormatter,     Nonwide),
                     ARCHON_TEST_TYPE(core::WideIntegerFormatter, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_IntegerFormatter_Basics, variants)
{
    using integer_formatter_type = test_type;
    using char_type        = typename integer_formatter_type::char_type;
    using traits_type      = typename integer_formatter_type::traits_type;
    using char_mapper_type = typename integer_formatter_type::char_mapper_type;
    char_mapper_type char_mapper(test_context.locale);
    integer_formatter_type integer_formatter(char_mapper);
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type, traits_type> widener(test_context.locale, seed_memory);

    ARCHON_CHECK_EQUAL(integer_formatter.format(0), widener.widen("0"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(1), widener.widen("1"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(12), widener.widen("12"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(123), widener.widen("123"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-1), widener.widen("-1"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-12), widener.widen("-12"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-123), widener.widen("-123"));

    ARCHON_CHECK_EQUAL(integer_formatter.format(0, 5), widener.widen("00000"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(1, 5), widener.widen("00001"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(12, 5), widener.widen("00012"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(123, 5), widener.widen("00123"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-1, 5), widener.widen("-00001"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-12, 5), widener.widen("-00012"));
    ARCHON_CHECK_EQUAL(integer_formatter.format(-123, 5), widener.widen("-00123"));

    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(0, 5), widener.widen("00000"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(1, 5), widener.widen("00001"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(2, 5), widener.widen("00010"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(5, 5), widener.widen("00101"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(-1, 5), widener.widen("-00001"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(-2, 5), widener.widen("-00010"));
    ARCHON_CHECK_EQUAL(integer_formatter.template format<2>(-5, 5), widener.widen("-00101"));
}
