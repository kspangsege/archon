// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <cmath>
#include <type_traits>
#include <limits>
#include <random>

#include <archon/core/type_list.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/random.hpp>
#include <archon/core/mul_prec_int.hpp>
#include <archon/check.hpp>
#include <archon/util/unit_frac.hpp>


using namespace archon;
namespace unit_frac = util::unit_frac;


namespace {


using IntTypes = core::TypeList<bool,
                                char,
                                signed char,
                                unsigned char,
                                wchar_t,
                                signed short,
                                unsigned short,
                                signed int,
                                unsigned int,
                                signed long,
                                unsigned long,
                                signed long long,
                                unsigned long long>;


using FltTypes = core::TypeList<float,
                                double,
                                long double>;


template<class F, class I> void test_flt_to_int(check::TestContext& test_context)
{
    using float_lim_type = std::numeric_limits<F>;
    I max = core::int_max<I>();

    ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(F(0)), I(0));
    ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(F(1)), max);

    ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(std::nextafter(F(0), F(-1))), I(0));
    ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(std::nextafter(F(1), F(2))), max);

    if (float_lim_type::has_infinity) {
        ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(-float_lim_type::infinity()), I(0));
        ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(float_lim_type::infinity()), max);
    }

    if (float_lim_type::has_quiet_NaN)
        ARCHON_CHECK_EQUAL(unit_frac::flt_to_int<I>(float_lim_type::quiet_NaN()), I(0));
}


struct TestFltToInt {
    template<class P, std::size_t> static void exec(check::TestContext& parent_test_context)
    {
        using float_type = typename P::first_type;
        using int_type   = typename P::second_type;
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("test<%s, %s>", core::get_type_name<float_type>(),
                                                               core::get_type_name<int_type>()));
        test_flt_to_int<float_type, int_type>(test_context);
    }
};


struct TestIntToFltToInt {
    template<class P, std::size_t> static void exec(check::TestContext& parent_test_context, std::mt19937_64& random)
    {
        ARCHON_TEST_TRAIL(parent_test_context, core::get_type_name<P>());
        using int_type = typename P::first_type;
        using flt_type = typename P::second_type;
        flt_type a = flt_type(core::int_max<int_type>());
        using flt_lim_type = std::numeric_limits<flt_type>;
        flt_type b = flt_type(std::nextafter(a, flt_lim_type::max()) - std::nextafter(a, flt_lim_type::min()));
        using promoted_type = core::promoted_type<int_type>;
        int_type max_diff = core::int_cast_a<int_type>(promoted_type(b));
        constexpr long n = 16384;
        for (long i = 0; i < n; ++i) {
            int_type max = core::int_max<int_type>();
            if (core::chance(random, 1, 2))
                max = core::rand_int<int_type>(random, 1, max);
            int_type val_1 = core::rand_int_max(random, max);
            static_assert((core::int_max<int_type>() & 1) == 1); // Max value for integer type is odd
            flt_type val_2 = flt_type((flt_type(val_1) + 0.5) / ((max & 1) == 1 ? 2 * flt_type(+max / 2 + 1) :
                                                                 flt_type(max + 1)));
            int_type val_3 = unit_frac::flt_to_int<int_type>(val_2, max);
            flt_type val_4 = unit_frac::int_to_flt<flt_type>(val_1, max);
            int_type val_5 = unit_frac::flt_to_int_a<int_type>(val_4, max);
            int_type diff_1 = int_type(val_1 <= val_3 ? val_3 - val_1 : val_1 - val_3);
            int_type diff_2 = int_type(val_1 <= val_5 ? val_5 - val_1 : val_1 - val_5);
            ARCHON_CHECK_LESS_EQUAL(diff_1, max_diff);
            ARCHON_CHECK_LESS_EQUAL(diff_2, max_diff);
        }
    }
};


struct TestChangeBitWidth {
    template<class I, std::size_t> static void exec(check::TestContext& test_context, std::mt19937_64& random)
    {
        constexpr long num_rounds = 64;
        constexpr int num_bits = core::num_value_bits<I>();
        constexpr bool full_coverage = num_rounds / num_bits >= num_bits;
        if constexpr (full_coverage) {
            for (int i = 0; i < num_bits; ++i) {
                int m = i + 1;
                for (int j = 0; j < num_bits; ++j) {
                    int n = j + 1;
                    test<I>(m, n, test_context, random);
                }
            }
        }
        else {
            for (long i = 0; i < num_rounds; ++i) {
                int m = core::rand_int<int>(random, 1, num_bits);
                int n = core::rand_int<int>(random, 1, num_bits);
                test<I>(m, n, test_context, random);
            }
        }
    }

    template<class I> static void test(int m, int n, check::TestContext& parent_test_context, std::mt19937_64& random)
    {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test<%s>(%s, %s)", core::get_type_name<I>(), +m, +n));
        auto subtest = [&](I val) {
            I val_2 = unit_frac::change_bit_width(val, m, n);
            using uint_type = std::make_unsigned_t<core::promoted_type<I>>;
            using mul_prec_type = core::MulPrecInt<uint_type, 2, false>;
            mul_prec_type max = mul_prec_type(core::int_mask<I>(n));
            mul_prec_type val_3 = (mul_prec_type(val) * (max + mul_prec_type(1)) /
                                   mul_prec_type(core::int_mask<I>(m)));
            I val_4 = core::int_cast_a<I>(uint_type(val_3 <= max ? val_3 : max));
            ARCHON_CHECK_EQUAL(val_4, val_2);
        };
        long num_rounds = 2048;
        bool full_coverage = core::int_find_msb_pos(num_rounds) >= m;
        if (full_coverage) {
            long num_rounds_2 = long(core::int_mask<I>(m)) + 1;
            for (long i = 0; i < num_rounds_2; ++i)
                subtest(core::int_cast_a<I>(i));
        }
        else {
            for (long i = 0; i < num_rounds; ++i)
                subtest(core::rand_int_bits<I>(random, m));
        }
    }
};


} // unnamed namespace


ARCHON_TEST(Util_UnitFrac_FltToInt)
{
    core::for_each_type<core::TypeListProduct<FltTypes, IntTypes>, TestFltToInt>(test_context);
}


ARCHON_TEST(Util_UnitFrac_IntToFltToInt)
{
    std::mt19937_64 random(test_context.seed_seq());
    core::for_each_type<core::TypeListProduct<IntTypes, FltTypes>, TestIntToFltToInt>(test_context, random);
}


ARCHON_TEST(Util_UnitFrac_ChangeBitWidth)
{
    std::mt19937_64 random(test_context.seed_seq());
    core::for_each_type<IntTypes, TestChangeBitWidth>(test_context, random);
}
