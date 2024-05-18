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
#include <random>

#include <archon/core/features.h>
#include <archon/core/type_list.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/integer_parser.hpp>
#include <archon/core/random.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(core::IntegerParser,     Nonwide),
                     ARCHON_TEST_TYPE(core::WideIntegerParser, Wide));


using Types = core::TypeList<bool,
                             char,
                             signed char,
                             unsigned char,
                             wchar_t,
                             short,
                             unsigned short,
                             int,
                             unsigned,
                             long,
                             unsigned long,
                             long long,
                             unsigned long long>;


template<class P, class I> void test_general(check::TestContext& test_context, std::mt19937_64& random)
{
    using integer_parser_type = P;
    using integer_type        = I;

    using char_type        = typename integer_parser_type::char_type;
    using traits_type      = typename integer_parser_type::traits_type;
    using string_view_type = typename integer_parser_type::string_view_type;
    using char_mapper_type = typename integer_parser_type::char_mapper_type;

    using integer_formatter_type = core::BasicIntegerFormatter<char_type, traits_type>;

    char_mapper_type char_mapper(test_context.locale);
    integer_formatter_type integer_formatter(char_mapper);
    integer_parser_type integer_parser(char_mapper);

    auto test = [&](integer_type val) {
        string_view_type str = integer_formatter.format(val);
        integer_type val_2 = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(integer_parser.parse(str, val_2))))
            ARCHON_CHECK_EQUAL(val_2, val);
    };
    constexpr long num_rounds = 32768;
    constexpr bool full_coverage = (core::int_find_msb_pos(num_rounds) >= core::int_width<integer_type>());
    if constexpr (full_coverage) {
            long base = long(core::int_min<integer_type>());
            long n = long(core::int_max<integer_type>()) - base + 1;
            for (long i = 0; i < n; ++i) {
                integer_type val = core::int_cast_a<integer_type>(i);
                test(val);
            }
    }
    else {
        for (long i = 0; i < num_rounds; ++i) {
            integer_type val = core::rand_int<integer_type>(random);
            test(val);
        }
    }
}


template<class P> struct TestGeneral {
    template<class I, std::size_t>
    static void exec(check::TestContext& parent_test_context, std::mt19937_64& random)
    {
        ARCHON_TEST_TRAIL(parent_test_context, core::get_type_name<I>());
        test_general<P, I>(test_context, random);
    }
};


} // unnamed namespace


ARCHON_TEST_BATCH(Core_IntegerParser_General, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    using integer_parser_type = test_type;
    core::for_each_type_alt<Types, TestGeneral<integer_parser_type>>(test_context, random);
}
