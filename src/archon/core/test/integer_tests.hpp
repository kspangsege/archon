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

#ifndef ARCHON_X_CORE_X_TEST_X_INTEGER_TESTS_HPP
#define ARCHON_X_CORE_X_TEST_X_INTEGER_TESTS_HPP


#include <utility>
#include <algorithm>

#include <archon/core/demangle.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/check.hpp>


namespace archon::core::test {


template<class F, class T> void test_cast_from_twos_compl_a(check::TestContext&);
template<class L, class R> void test_try_int_add(check::TestContext&);
template<class L, class R> void test_try_int_sub(check::TestContext&);
template<class L, class R> void test_try_int_mul(check::TestContext&);








// Implementation


template<class F, class T> void test_cast_from_twos_compl_a(check::TestContext& parent_test_context)
{
    ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s -> %s", core::get_type_name<F>(),
                                                           core::get_type_name<T>()));
    static_assert(core::is_signed<T>());
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<T>(F(0)), T(0));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<T>(F(1)), T(1));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<T>(F(-1)), T(-1));
    F max_1 = core::int_mask<F>(std::min(core::num_value_bits<F>() - (core::is_signed<F>() ? 0 : 1),
                                         core::num_value_bits<T>()));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<T>(max_1), core::int_cast_a<T>(max_1));
    using common_type = core::common_int_type<int, F, T>;
    common_type alt_min_1;
    if constexpr (core::is_signed<F>()) {
        alt_min_1 = common_type(-1) - core::int_cast_a<common_type>(core::int_min<F>());
    }
    else {
        alt_min_1 = core::int_cast_a<common_type>(core::int_max<F>()) >> 1;
    }
    common_type alt_min_2 = common_type(-1) - core::int_cast_a<common_type>(core::int_min<T>());
    common_type alt_min = std::min(alt_min_1, alt_min_2);
    using promoted_type = core::promoted_type<T>;
    T min = core::int_cast_a<T>(promoted_type(-1) - core::int_cast_a<promoted_type>(alt_min));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<T>(core::int_cast_a<F>(min)), min);
    using unsigned_type = core::unsigned_type<T>;
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<unsigned_type>(F(0)), unsigned_type(0));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<unsigned_type>(F(1)), unsigned_type(1));
    F max_2 = core::int_mask<F>(std::min(core::num_value_bits<F>() - (core::is_signed<F>() ? 0 : 1),
                                         core::num_value_bits<unsigned_type>()));
    ARCHON_CHECK_EQUAL(core::cast_from_twos_compl_a<unsigned_type>(max_2), core::int_cast_a<unsigned_type>(max_2));
}


template<class L, class R> void test_try_int_add(check::TestContext& parent_test_context)
{
    ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s vs %s", core::get_type_name<L>(),
                                                           core::get_type_name<R>()));
    static_assert(core::is_signed<R>());
    using lval_type = L;
    using signed_rval_type = R;
    using unsigned_rval_type = core::unsigned_type<R>;
    constexpr lval_type min = core::int_min<lval_type>();
    constexpr lval_type max = core::int_max<lval_type>();
    lval_type val = max;
    ARCHON_CHECK(core::try_int_add(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK(core::try_int_add(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK_NOT(core::try_int_add(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK_NOT(core::try_int_add(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK(core::try_int_add(val, signed_rval_type(-1)));
    ARCHON_CHECK_EQUAL(val, max - lval_type(1));
    ARCHON_CHECK(core::try_int_add(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max);
    val = min;
    ARCHON_CHECK(core::try_int_add(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK(core::try_int_add(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK_NOT(core::try_int_add(val, signed_rval_type(-1)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK(core::try_int_add(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, min + lval_type(1));
    ARCHON_CHECK(core::try_int_add(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, min + lval_type(2));
}


template<class L, class R> void test_try_int_sub(check::TestContext& parent_test_context)
{
    ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s vs %s", core::get_type_name<L>(),
                                                           core::get_type_name<R>()));
    static_assert(core::is_signed<R>());
    using lval_type = L;
    using signed_rval_type = R;
    using unsigned_rval_type = core::unsigned_type<R>;
    constexpr lval_type min = core::int_min<lval_type>();
    constexpr lval_type max = core::int_max<lval_type>();
    lval_type val = min;
    ARCHON_CHECK(core::try_int_sub(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK(core::try_int_sub(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK_NOT(core::try_int_sub(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK_NOT(core::try_int_sub(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, min);
    ARCHON_CHECK(core::try_int_sub(val, signed_rval_type(-1)));
    ARCHON_CHECK_EQUAL(val, min + lval_type(1));
    ARCHON_CHECK(core::try_int_sub(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, min);
    val = max;
    ARCHON_CHECK(core::try_int_sub(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK(core::try_int_sub(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK_NOT(core::try_int_sub(val, signed_rval_type(-1)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK(core::try_int_sub(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max - lval_type(1));
    ARCHON_CHECK(core::try_int_sub(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max - lval_type(2));
}


template<class L, class R> void test_try_int_mul(check::TestContext& parent_test_context)
{
    ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s vs %s", core::get_type_name<L>(),
                                                           core::get_type_name<R>()));
    static_assert(core::is_signed<R>());
    using lval_type = L;
    using signed_rval_type = R;
    using unsigned_rval_type = core::unsigned_type<R>;
    constexpr lval_type max = core::int_max<lval_type>();
    lval_type val;

    val = lval_type(0);
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-2)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));

    val = lval_type(1);
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    val = lval_type(1);
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, lval_type(1));
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, lval_type(2));
    val = lval_type(1);
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    val = lval_type(1);
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, lval_type(1));
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, lval_type(2));
    val = lval_type(1);
    if constexpr (core::is_signed<lval_type>()) {
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
        ARCHON_CHECK_EQUAL(val, lval_type(-1));
        val = lval_type(1);
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, lval_type(-2));
    }
    else {
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-1)));
        ARCHON_CHECK_EQUAL(val, lval_type(1));
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, lval_type(1));
    }

    val = max;
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    val = max;
    ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
    ARCHON_CHECK_EQUAL(val, lval_type(0));
    val = max;
    ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
    ARCHON_CHECK_EQUAL(val, max);
    ARCHON_CHECK_NOT(core::try_int_mul(val, unsigned_rval_type(2)));
    ARCHON_CHECK_EQUAL(val, max);
    if constexpr (core::is_signed<lval_type>()) {
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
        ARCHON_CHECK_EQUAL(val, core::int_cast_a<L>(-max));
        val = max;
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, max);
    }
    else {
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-1)));
        ARCHON_CHECK_EQUAL(val, max);
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, max);
    }

    if constexpr (core::is_signed<lval_type>()) {
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
        ARCHON_CHECK_EQUAL(val, lval_type(0));
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
        ARCHON_CHECK_EQUAL(val, lval_type(-1));
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(2)));
        ARCHON_CHECK_EQUAL(val, lval_type(-2));
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
        ARCHON_CHECK_EQUAL(val, lval_type(0));
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
        ARCHON_CHECK_EQUAL(val, lval_type(-1));
        ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(2)));
        ARCHON_CHECK_EQUAL(val, lval_type(-2));
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
        ARCHON_CHECK_EQUAL(val, lval_type(1));
        val = lval_type(-1);
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, lval_type(2));

        constexpr lval_type min = core::int_min<lval_type>();
        constexpr lval_type neg_max = core::int_cast_a<L>(-max);
        val = min;
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
        ARCHON_CHECK_EQUAL(val, lval_type(0));
        val = min;
        ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
        ARCHON_CHECK_EQUAL(val, min);
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(2)));
        ARCHON_CHECK_EQUAL(val, min);
        ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
        ARCHON_CHECK_EQUAL(val, lval_type(0));
        val = min;
        ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
        ARCHON_CHECK_EQUAL(val, min);
        ARCHON_CHECK_NOT(core::try_int_mul(val, unsigned_rval_type(2)));
        ARCHON_CHECK_EQUAL(val, min);
        if constexpr (min < neg_max) {
            ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-1)));
            ARCHON_CHECK_EQUAL(val, min);
        }
        else {
            ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
            ARCHON_CHECK_EQUAL(val, max);
            val = min;
        }
        ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-2)));
        ARCHON_CHECK_EQUAL(val, min);

        if constexpr (min < neg_max) {
            val = neg_max;
            ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(0)));
            ARCHON_CHECK_EQUAL(val, lval_type(0));
            val = neg_max;
            ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(1)));
            ARCHON_CHECK_EQUAL(val, neg_max);
            ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(2)));
            ARCHON_CHECK_EQUAL(val, neg_max);
            ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(0)));
            ARCHON_CHECK_EQUAL(val, lval_type(0));
            val = neg_max;
            ARCHON_CHECK(core::try_int_mul(val, unsigned_rval_type(1)));
            ARCHON_CHECK_EQUAL(val, neg_max);
            ARCHON_CHECK_NOT(core::try_int_mul(val, unsigned_rval_type(2)));
            ARCHON_CHECK_EQUAL(val, neg_max);
            ARCHON_CHECK(core::try_int_mul(val, signed_rval_type(-1)));
            ARCHON_CHECK_EQUAL(val, max);
            val = neg_max;
            ARCHON_CHECK_NOT(core::try_int_mul(val, signed_rval_type(-2)));
            ARCHON_CHECK_EQUAL(val, neg_max);
        }
    }
}


} // namespace archon::core::test

#endif // ARCHON_X_CORE_X_TEST_X_INTEGER_TESTS_HPP
