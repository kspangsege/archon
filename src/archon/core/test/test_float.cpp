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
#include <cmath>
#include <limits>

#include <archon/core/type_list.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/format.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


template<class F, class I> void test_comparisons(check::TestContext& test_context)
{
    if (core::is_signed<I>()) {
        ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(-1), F(-2)), I(-1)));
        ARCHON_CHECK(core::float_less_int(std::nextafter(F(-1), F(-2)), I(-1)));
        ARCHON_CHECK_NOT(core::float_greater_int(std::nextafter(F(-1), F(-2)), I(-1)));
        ARCHON_CHECK(core::float_less_equal_int(std::nextafter(F(-1), F(-2)), I(-1)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(std::nextafter(F(-1), F(-2)), I(-1)));

        ARCHON_CHECK(core::float_equal_int(F(-1), I(-1)));
        ARCHON_CHECK_NOT(core::float_less_int(F(-1), I(-1)));
        ARCHON_CHECK_NOT(core::float_greater_int(F(-1), I(-1)));
        ARCHON_CHECK(core::float_less_equal_int(F(-1), I(-1)));
        ARCHON_CHECK(core::float_greater_equal_int(F(-1), I(-1)));

        ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(-1), F(0)), I(-1)));
        ARCHON_CHECK_NOT(core::float_less_int(std::nextafter(F(-1), F(0)), I(-1)));
        ARCHON_CHECK(core::float_greater_int(std::nextafter(F(-1), F(0)), I(-1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(std::nextafter(F(-1), F(0)), I(-1)));
        ARCHON_CHECK(core::float_greater_equal_int(std::nextafter(F(-1), F(0)), I(-1)));
    }

    ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(0), F(-1)), I(0)));
    ARCHON_CHECK(core::float_less_int(std::nextafter(F(0), F(-1)), I(0)));
    ARCHON_CHECK_NOT(core::float_greater_int(std::nextafter(F(0), F(-1)), I(0)));
    ARCHON_CHECK(core::float_less_equal_int(std::nextafter(F(0), F(-1)), I(0)));
    ARCHON_CHECK_NOT(core::float_greater_equal_int(std::nextafter(F(0), F(-1)), I(0)));

    ARCHON_CHECK(core::float_equal_int(F(0), I(0)));
    ARCHON_CHECK_NOT(core::float_less_int(F(0), I(0)));
    ARCHON_CHECK_NOT(core::float_greater_int(F(0), I(0)));
    ARCHON_CHECK(core::float_less_equal_int(F(0), I(0)));
    ARCHON_CHECK(core::float_greater_equal_int(F(0), I(0)));

    ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(0), F(1)), I(0)));
    ARCHON_CHECK_NOT(core::float_less_int(std::nextafter(F(0), F(1)), I(0)));
    ARCHON_CHECK(core::float_greater_int(std::nextafter(F(0), F(1)), I(0)));
    ARCHON_CHECK_NOT(core::float_less_equal_int(std::nextafter(F(0), F(1)), I(0)));
    ARCHON_CHECK(core::float_greater_equal_int(std::nextafter(F(0), F(1)), I(0)));

    ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(1), F(0)), I(1)));
    ARCHON_CHECK(core::float_less_int(std::nextafter(F(1), F(0)), I(1)));
    ARCHON_CHECK_NOT(core::float_greater_int(std::nextafter(F(1), F(0)), I(1)));
    ARCHON_CHECK(core::float_less_equal_int(std::nextafter(F(1), F(0)), I(1)));
    ARCHON_CHECK_NOT(core::float_greater_equal_int(std::nextafter(F(1), F(0)), I(1)));

    ARCHON_CHECK(core::float_equal_int(F(1), I(1)));
    ARCHON_CHECK_NOT(core::float_less_int(F(1), I(1)));
    ARCHON_CHECK_NOT(core::float_greater_int(F(1), I(1)));
    ARCHON_CHECK(core::float_less_equal_int(F(1), I(1)));
    ARCHON_CHECK(core::float_greater_equal_int(F(1), I(1)));

    ARCHON_CHECK_NOT(core::float_equal_int(std::nextafter(F(1), F(2)), I(1)));
    ARCHON_CHECK_NOT(core::float_less_int(std::nextafter(F(1), F(2)), I(1)));
    ARCHON_CHECK(core::float_greater_int(std::nextafter(F(1), F(2)), I(1)));
    ARCHON_CHECK_NOT(core::float_less_equal_int(std::nextafter(F(1), F(2)), I(1)));
    ARCHON_CHECK(core::float_greater_equal_int(std::nextafter(F(1), F(2)), I(1)));

    using float_lim_type = std::numeric_limits<F>;
    F low  = float_lim_type::lowest();
    F high = float_lim_type::max();

    if (core::is_signed<I>()) {
        ARCHON_CHECK_NOT(core::float_equal_int(low, I(-1)));
        ARCHON_CHECK(core::float_less_int(low, I(-1)));
        ARCHON_CHECK_NOT(core::float_greater_int(low, I(-1)));
        ARCHON_CHECK(core::float_less_equal_int(low, I(-1)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(low, I(-1)));

        ARCHON_CHECK_NOT(core::float_equal_int(high, I(-1)));
        ARCHON_CHECK_NOT(core::float_less_int(high, I(-1)));
        ARCHON_CHECK(core::float_greater_int(high, I(-1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high, I(-1)));
        ARCHON_CHECK(core::float_greater_equal_int(high, I(-1)));
    }

    ARCHON_CHECK_NOT(core::float_equal_int(low, I(0)));
    ARCHON_CHECK(core::float_less_int(low, I(0)));
    ARCHON_CHECK_NOT(core::float_greater_int(low, I(0)));
    ARCHON_CHECK(core::float_less_equal_int(low, I(0)));
    ARCHON_CHECK_NOT(core::float_greater_equal_int(low, I(0)));

    ARCHON_CHECK_NOT(core::float_equal_int(high, I(0)));
    ARCHON_CHECK_NOT(core::float_less_int(high, I(0)));
    ARCHON_CHECK(core::float_greater_int(high, I(0)));
    ARCHON_CHECK_NOT(core::float_less_equal_int(high, I(0)));
    ARCHON_CHECK(core::float_greater_equal_int(high, I(0)));

    ARCHON_CHECK_NOT(core::float_equal_int(low, I(1)));
    ARCHON_CHECK(core::float_less_int(low, I(1)));
    ARCHON_CHECK_NOT(core::float_greater_int(low, I(1)));
    ARCHON_CHECK(core::float_less_equal_int(low, I(1)));
    ARCHON_CHECK_NOT(core::float_greater_equal_int(low, I(1)));

    ARCHON_CHECK_NOT(core::float_equal_int(high, I(1)));
    ARCHON_CHECK_NOT(core::float_less_int(high, I(1)));
    ARCHON_CHECK(core::float_greater_int(high, I(1)));
    ARCHON_CHECK_NOT(core::float_less_equal_int(high, I(1)));
    ARCHON_CHECK(core::float_greater_equal_int(high, I(1)));

    I min = core::int_min<I>();
    I max = core::int_max<I>();

    if (!std::is_same_v<I, bool>) {
        F low_1 = std::nextafter(F(min), low);
        F low_2 = F(min);
        F low_3 = std::nextafter(F(min), F(0));

        F high_1 = std::nextafter(F(max), high);
        F high_2 = F(max);
        F high_3 = std::nextafter(F(max), F(0));

        if (core::is_signed<I>()) {
            ARCHON_CHECK_NOT(core::float_equal_int(low_1, I(-1)));
            ARCHON_CHECK(core::float_less_int(low_1, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_1, I(-1)));
            ARCHON_CHECK(core::float_less_equal_int(low_1, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_1, I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_2, I(-1)));
            ARCHON_CHECK(core::float_less_int(low_2, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_2, I(-1)));
            ARCHON_CHECK(core::float_less_equal_int(low_2, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_2, I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_3, I(-1)));
            ARCHON_CHECK(core::float_less_int(low_3, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_3, I(-1)));
            ARCHON_CHECK(core::float_less_equal_int(low_3, I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_3, I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_1, I(0)));
            ARCHON_CHECK(core::float_less_int(low_1, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_1, I(0)));
            ARCHON_CHECK(core::float_less_equal_int(low_1, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_1, I(0)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_2, I(0)));
            ARCHON_CHECK(core::float_less_int(low_2, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_2, I(0)));
            ARCHON_CHECK(core::float_less_equal_int(low_2, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_2, I(0)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_3, I(0)));
            ARCHON_CHECK(core::float_less_int(low_3, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_3, I(0)));
            ARCHON_CHECK(core::float_less_equal_int(low_3, I(0)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_3, I(0)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_1, I(1)));
            ARCHON_CHECK(core::float_less_int(low_1, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_1, I(1)));
            ARCHON_CHECK(core::float_less_equal_int(low_1, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_1, I(1)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_2, I(1)));
            ARCHON_CHECK(core::float_less_int(low_2, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_2, I(1)));
            ARCHON_CHECK(core::float_less_equal_int(low_2, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_2, I(1)));

            ARCHON_CHECK_NOT(core::float_equal_int(low_3, I(1)));
            ARCHON_CHECK(core::float_less_int(low_3, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_int(low_3, I(1)));
            ARCHON_CHECK(core::float_less_equal_int(low_3, I(1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(low_3, I(1)));

            ARCHON_CHECK(core::float_less_equal_int(std::nextafter(F(min), low), min));
            ARCHON_CHECK(core::float_greater_equal_int(std::nextafter(F(min), F(0)), min));

            ARCHON_CHECK_NOT(core::float_equal_int(high_1, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_int(high_1, I(-1)));
            ARCHON_CHECK(core::float_greater_int(high_1, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_equal_int(high_1, I(-1)));
            ARCHON_CHECK(core::float_greater_equal_int(high_1, I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(high_2, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_int(high_2, I(-1)));
            ARCHON_CHECK(core::float_greater_int(high_2, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_equal_int(high_2, I(-1)));
            ARCHON_CHECK(core::float_greater_equal_int(high_2, I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(high_3, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_int(high_3, I(-1)));
            ARCHON_CHECK(core::float_greater_int(high_3, I(-1)));
            ARCHON_CHECK_NOT(core::float_less_equal_int(high_3, I(-1)));
            ARCHON_CHECK(core::float_greater_equal_int(high_3, I(-1)));
        }

        ARCHON_CHECK_NOT(core::float_equal_int(high_1, I(0)));
        ARCHON_CHECK_NOT(core::float_less_int(high_1, I(0)));
        ARCHON_CHECK(core::float_greater_int(high_1, I(0)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_1, I(0)));
        ARCHON_CHECK(core::float_greater_equal_int(high_1, I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(high_2, I(0)));
        ARCHON_CHECK_NOT(core::float_less_int(high_2, I(0)));
        ARCHON_CHECK(core::float_greater_int(high_2, I(0)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_2, I(0)));
        ARCHON_CHECK(core::float_greater_equal_int(high_2, I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(high_3, I(0)));
        ARCHON_CHECK_NOT(core::float_less_int(high_3, I(0)));
        ARCHON_CHECK(core::float_greater_int(high_3, I(0)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_3, I(0)));
        ARCHON_CHECK(core::float_greater_equal_int(high_3, I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(high_1, I(1)));
        ARCHON_CHECK_NOT(core::float_less_int(high_1, I(1)));
        ARCHON_CHECK(core::float_greater_int(high_1, I(1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_1, I(1)));
        ARCHON_CHECK(core::float_greater_equal_int(high_1, I(1)));

        ARCHON_CHECK_NOT(core::float_equal_int(high_2, I(1)));
        ARCHON_CHECK_NOT(core::float_less_int(high_2, I(1)));
        ARCHON_CHECK(core::float_greater_int(high_2, I(1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_2, I(1)));
        ARCHON_CHECK(core::float_greater_equal_int(high_2, I(1)));

        ARCHON_CHECK_NOT(core::float_equal_int(high_3, I(1)));
        ARCHON_CHECK_NOT(core::float_less_int(high_3, I(1)));
        ARCHON_CHECK(core::float_greater_int(high_3, I(1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(high_3, I(1)));
        ARCHON_CHECK(core::float_greater_equal_int(high_3, I(1)));

        ARCHON_CHECK(core::float_greater_equal_int(std::nextafter(F(max), high), max));
        ARCHON_CHECK(core::float_less_equal_int(std::nextafter(F(max), F(0)), max));
    }

    if (float_lim_type::has_infinity) {
        if (core::is_signed<I>()) {
            ARCHON_CHECK_NOT(core::float_equal_int(-float_lim_type::infinity(), min));
            ARCHON_CHECK(core::float_less_int(-float_lim_type::infinity(), min));
            ARCHON_CHECK_NOT(core::float_greater_int(-float_lim_type::infinity(), min));
            ARCHON_CHECK(core::float_less_equal_int(-float_lim_type::infinity(), min));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(-float_lim_type::infinity(), min));

            ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::infinity(), min));
            ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::infinity(), min));
            ARCHON_CHECK(core::float_greater_int(float_lim_type::infinity(), min));
            ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::infinity(), min));
            ARCHON_CHECK(core::float_greater_equal_int(float_lim_type::infinity(), min));

            ARCHON_CHECK_NOT(core::float_equal_int(-float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK(core::float_less_int(-float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_int(-float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK(core::float_less_equal_int(-float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(-float_lim_type::infinity(), I(-1)));

            ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK(core::float_greater_int(float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::infinity(), I(-1)));
            ARCHON_CHECK(core::float_greater_equal_int(float_lim_type::infinity(), I(-1)));
        }

        ARCHON_CHECK_NOT(core::float_equal_int(-float_lim_type::infinity(), I(0)));
        ARCHON_CHECK(core::float_less_int(-float_lim_type::infinity(), I(0)));
        ARCHON_CHECK_NOT(core::float_greater_int(-float_lim_type::infinity(), I(0)));
        ARCHON_CHECK(core::float_less_equal_int(-float_lim_type::infinity(), I(0)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(-float_lim_type::infinity(), I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::infinity(), I(0)));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::infinity(), I(0)));
        ARCHON_CHECK(core::float_greater_int(float_lim_type::infinity(), I(0)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::infinity(), I(0)));
        ARCHON_CHECK(core::float_greater_equal_int(float_lim_type::infinity(), I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(-float_lim_type::infinity(), I(1)));
        ARCHON_CHECK(core::float_less_int(-float_lim_type::infinity(), I(1)));
        ARCHON_CHECK_NOT(core::float_greater_int(-float_lim_type::infinity(), I(1)));
        ARCHON_CHECK(core::float_less_equal_int(-float_lim_type::infinity(), I(1)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(-float_lim_type::infinity(), I(1)));

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::infinity(), I(1)));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::infinity(), I(1)));
        ARCHON_CHECK(core::float_greater_int(float_lim_type::infinity(), I(1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::infinity(), I(1)));
        ARCHON_CHECK(core::float_greater_equal_int(float_lim_type::infinity(), I(1)));

        ARCHON_CHECK_NOT(core::float_equal_int(-float_lim_type::infinity(), max));
        ARCHON_CHECK(core::float_less_int(-float_lim_type::infinity(), max));
        ARCHON_CHECK_NOT(core::float_greater_int(-float_lim_type::infinity(), max));
        ARCHON_CHECK(core::float_less_equal_int(-float_lim_type::infinity(), max));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(-float_lim_type::infinity(), max));

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::infinity(), max));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::infinity(), max));
        ARCHON_CHECK(core::float_greater_int(float_lim_type::infinity(), max));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::infinity(), max));
        ARCHON_CHECK(core::float_greater_equal_int(float_lim_type::infinity(), max));
    }

    if (float_lim_type::has_quiet_NaN) {
        if (core::is_signed<I>()) {
            ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::quiet_NaN(), min));
            ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::quiet_NaN(), min));
            ARCHON_CHECK_NOT(core::float_greater_int(float_lim_type::quiet_NaN(), min));
            ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::quiet_NaN(), min));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(float_lim_type::quiet_NaN(), min));

            ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::quiet_NaN(), I(-1)));
            ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::quiet_NaN(), I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_int(float_lim_type::quiet_NaN(), I(-1)));
            ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::quiet_NaN(), I(-1)));
            ARCHON_CHECK_NOT(core::float_greater_equal_int(float_lim_type::quiet_NaN(), I(-1)));
        }

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::quiet_NaN(), I(0)));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::quiet_NaN(), I(0)));
        ARCHON_CHECK_NOT(core::float_greater_int(float_lim_type::quiet_NaN(), I(0)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::quiet_NaN(), I(0)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(float_lim_type::quiet_NaN(), I(0)));

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::quiet_NaN(), I(1)));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::quiet_NaN(), I(1)));
        ARCHON_CHECK_NOT(core::float_greater_int(float_lim_type::quiet_NaN(), I(1)));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::quiet_NaN(), I(1)));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(float_lim_type::quiet_NaN(), I(1)));

        ARCHON_CHECK_NOT(core::float_equal_int(float_lim_type::quiet_NaN(), max));
        ARCHON_CHECK_NOT(core::float_less_int(float_lim_type::quiet_NaN(), max));
        ARCHON_CHECK_NOT(core::float_greater_int(float_lim_type::quiet_NaN(), max));
        ARCHON_CHECK_NOT(core::float_less_equal_int(float_lim_type::quiet_NaN(), max));
        ARCHON_CHECK_NOT(core::float_greater_equal_int(float_lim_type::quiet_NaN(), max));
    }
}


struct TestComparisons {
    template<class P, std::size_t> static void exec(check::TestContext& parent_test_context)
    {
        using float_type = typename P::first_type;
        using int_type   = typename P::second_type;
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test<%s, %s>", core::get_type_name<float_type>(),
                                          core::get_type_name<int_type>()));
        test_comparisons<float_type, int_type>(test_context);
    }
};


template<class F, class I> void test_clamped_float_to_int(check::TestContext& test_context)
{
    I min_1 = core::int_min<I>();
    I max_1 = core::int_max<I>();
    auto min_2 = core::promote(min_1);
    auto max_2 = core::promote(max_1);
    ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(F(min_2) * 2 - 1)), min_1);
    ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(F(max_2) * 2 + 1)), max_1);
    F min_3 = std::nextafter(F(min_2), F(0));
    F max_3 = std::nextafter(F(max_2), F(0));
    ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(min_3)), core::float_to_int_a<I>(min_3));
    ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(max_3)), core::float_to_int_a<I>(max_3));

    using float_lim_type = std::numeric_limits<F>;
    if (float_lim_type::has_infinity) {
        ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(-float_lim_type::infinity())), min_1);
        ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(+float_lim_type::infinity())), max_1);
    }
    if (float_lim_type::has_quiet_NaN)
        ARCHON_CHECK_EQUAL((core::clamped_float_to_int<I, F>(float_lim_type::quiet_NaN())), 0);
}


struct TestClampedFloatToInt {
    template<class P, std::size_t> static void exec(check::TestContext& parent_test_context)
    {
        using float_type = typename P::first_type;
        using int_type   = typename P::second_type;
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test<%s, %s>", core::get_type_name<float_type>(),
                                          core::get_type_name<int_type>()));
        test_clamped_float_to_int<float_type, int_type>(test_context);
    }
};


template<class F, class I> void test_try_float_to_int(check::TestContext& test_context)
{
    I min_1 = core::int_min<I>();
    I max_1 = core::int_max<I>();
    auto min_2 = core::promote(min_1);
    auto max_2 = core::promote(max_1);
    I res = 0;
    ARCHON_CHECK_NOT((core::try_float_to_int<F, I>(F(min_2) * 2 - 1, res)));
    ARCHON_CHECK_NOT((core::try_float_to_int<F, I>(F(max_2) * 2 + 1, res)));
    F min_3 = std::nextafter(F(min_2), F(0));
    F max_3 = std::nextafter(F(max_2), F(0));
    if (ARCHON_LIKELY(ARCHON_CHECK((core::try_float_to_int<F, I>(min_3, res)))))
        ARCHON_CHECK_EQUAL(res, core::float_to_int_a<I>(min_3));
    if (ARCHON_LIKELY(ARCHON_CHECK((core::try_float_to_int<F, I>(max_3, res)))))
        ARCHON_CHECK_EQUAL(res, core::float_to_int_a<I>(max_3));

    using float_lim_type = std::numeric_limits<F>;
    if (float_lim_type::has_infinity) {
        ARCHON_CHECK_NOT((core::try_float_to_int<F, I>(-float_lim_type::infinity(), res)));
        ARCHON_CHECK_NOT((core::try_float_to_int<F, I>(+float_lim_type::infinity(), res)));
    }
    if (float_lim_type::has_quiet_NaN)
        ARCHON_CHECK_NOT((core::try_float_to_int<F, I>(float_lim_type::quiet_NaN(), res)));
}


struct TestTryFloatToInt {
    template<class P, std::size_t> static void exec(check::TestContext& parent_test_context)
    {
        using float_type = typename P::first_type;
        using int_type   = typename P::second_type;
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test<%s, %s>", core::get_type_name<float_type>(),
                                          core::get_type_name<int_type>()));
        test_try_float_to_int<float_type, int_type>(test_context);
    }
};


using FltTypes = core::TypeList<float,
                                double,
                                long double>;

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


} // unnamed namespace


ARCHON_TEST(Core_Float_Comparisons)
{
    core::for_each_type_alt<core::TypeListProduct<FltTypes, IntTypes>,
                            TestComparisons>(test_context);
}


ARCHON_TEST(Core_Float_ClampedFloatToInt)
{
    core::for_each_type_alt<core::TypeListProduct<FltTypes, IntTypes>,
                            TestClampedFloatToInt>(test_context);
}


ARCHON_TEST(Core_Float_TryFloatToInt)
{
    core::for_each_type_alt<core::TypeListProduct<FltTypes, IntTypes>,
                            TestTryFloatToInt>(test_context);
}


ARCHON_TEST(Core_Float_FloatToInt)
{
    ARCHON_CHECK_EQUAL(core::float_to_int_a<bool>(0.0), false);
    ARCHON_CHECK_EQUAL(core::float_to_int_a<bool>(0.5), false);
    ARCHON_CHECK_EQUAL(core::float_to_int_a<bool>(1.0), true);
}
