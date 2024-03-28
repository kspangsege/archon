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


#include <cstdint>
#include <type_traits>
#include <limits>
#include <vector>
#include <set>

#include <archon/core/type_list.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/super_int.hpp>
#include <archon/check.hpp>
#include <archon/core/test/integer_tests.hpp>


using namespace archon;
namespace test = core::test;


namespace {


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


template<class T> void test_find_most_significant_bit_pos(check::TestContext& test_context)
{
    using type = T;
    ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(0)), -1);
    ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(1)),  0);
    if constexpr (!std::is_same_v<type, bool>) {
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(2)), 1);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(3)), 1);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(4)), 2);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(5)), 2);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(6)), 2);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(7)), 2);
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(8)), 3);
    }
    ARCHON_CHECK_EQUAL(core::int_find_msb_pos(core::int_max<type>()), core::num_value_bits<type>() - 1);
    if constexpr (std::is_signed_v<type>)
        ARCHON_CHECK_EQUAL(core::int_find_msb_pos(type(-1)), core::num_value_bits<type>());
}


struct TestFindMostSignificantBitPos {
    template<class T, std::size_t>
    static void exec(check::TestContext& parent_test_context)
    {
        ARCHON_TEST_TRAIL(parent_test_context, core::get_type_name<T>());
        test_find_most_significant_bit_pos<T>(test_context);
    }
};


} // unnamed namespace


ARCHON_TEST(Core_Integer_FindMostSignificantBitPos)
{
    core::for_each_type_alt<Types, TestFindMostSignificantBitPos>(test_context);
}


ARCHON_TEST(Core_Integer_FindMostSignificantDigitPos)
{
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(0), -1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(1), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(9), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(10), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(11), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(99), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(100), 2);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-1), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-9), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-10), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-11), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-99), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-100), 2);

    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(0, 16), -1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(1, 16), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(15, 16), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(16, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(17, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(255, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(256, 16), 2);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-1, 16), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-15, 16), 0);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-16, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-17, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-255, 16), 1);
    ARCHON_CHECK_EQUAL(core::int_find_msd_pos(-256, 16), 2);
}


ARCHON_TEST(Core_Integer_IsNegative)
{
    std::size_t unsigned_int = 0;
    ARCHON_CHECK(!core::is_negative(unsigned_int));

    unsigned_int = std::size_t(-1);
    ARCHON_CHECK(!core::is_negative(unsigned_int));

    char c = 0;
    ARCHON_CHECK(!core::is_negative(c));

    c = 1;
    ARCHON_CHECK(!core::is_negative(c));

    c = std::numeric_limits<char>::max();
    ARCHON_CHECK(!core::is_negative(c));

    c = char(-1);
    ARCHON_CHECK(core::is_negative(c));

    c = std::numeric_limits<char>::min();
    ARCHON_CHECK(core::is_negative(c));
}


ARCHON_TEST(Core_Integer_CastFromTwosComplA)
{
    using wide_signed_type     = signed long long;
    using wide_unsigned_type   = unsigned long long;
    using narrow_signed_type   = signed char;
    using narrow_unsigned_type = unsigned char;

    test::test_cast_from_twos_compl_a<wide_signed_type, narrow_signed_type>(test_context);
    test::test_cast_from_twos_compl_a<wide_unsigned_type, narrow_signed_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_signed_type, wide_signed_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_unsigned_type, wide_signed_type>(test_context);
}


ARCHON_TEST(Core_Integer_Add)
{
    int max = std::numeric_limits<int>::max();
    int i = max - 2;
    ARCHON_CHECK_NOTHROW(core::int_add(i, 1));
    ARCHON_CHECK_EQUAL(i, max - 1);
    ARCHON_CHECK_THROW(core::int_add(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max - 1);
}


ARCHON_TEST(Core_Integer_Sub)
{
    int min = std::numeric_limits<int>::min();
    int i = min + 2;
    ARCHON_CHECK_NOTHROW(core::int_sub(i, 1));
    ARCHON_CHECK_EQUAL(i, min + 1);
    ARCHON_CHECK_THROW(core::int_sub(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, min + 1);
}


ARCHON_TEST(Core_Integer_Mul)
{
    int max = std::numeric_limits<int>::max();
    int i = max / 3;
    ARCHON_CHECK_NOTHROW(core::int_mul(i, 2));
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
    ARCHON_CHECK_THROW(core::int_mul(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
}


ARCHON_TEST(Core_Integer_Pow)
{
    int val = 10;
    ARCHON_CHECK_NOTHROW(core::int_pow(val, 2));
    ARCHON_CHECK_EQUAL(val, 100);
    int max = core::int_max<int>();
    val = max / 2;
    ARCHON_CHECK_THROW(core::int_pow(val, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(val, max / 2);
}


ARCHON_TEST(Core_Integer_ArithShiftLeft)
{
    int max = std::numeric_limits<int>::max();
    int i = max / 3;
    ARCHON_CHECK_NOTHROW(core::int_arith_shift_left(i, 1));
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
    ARCHON_CHECK_THROW(core::int_arith_shift_left(i, 1), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
}


ARCHON_TEST(Core_Integer_LogicShiftLeft)
{
    using type = unsigned;
    int digits = std::numeric_limits<type>::digits;
    type lval;

    lval = 1;
    core::int_logic_shift_left(lval, 0);
    ARCHON_CHECK_EQUAL(lval, 1);

    lval = 0;
    core::int_logic_shift_left(lval, 1);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = 1;
    core::int_logic_shift_left(lval, 1);
    ARCHON_CHECK_EQUAL(lval, 2);

    lval = 1;
    core::int_logic_shift_left(lval, digits - 1);
    ARCHON_CHECK_EQUAL(lval, type(1) << (digits - 1));

    lval = 2;
    core::int_logic_shift_left(lval, digits - 1);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = 3;
    core::int_logic_shift_left(lval, digits - 1);
    ARCHON_CHECK_EQUAL(lval, type(1) << (digits - 1));

    lval = 1;
    core::int_logic_shift_left(lval, digits);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = 1;
    core::int_logic_shift_left(lval, digits + 1);
    ARCHON_CHECK_EQUAL(lval, 0);
}


ARCHON_TEST(Core_Integer_LogicShiftRight)
{
    using type = signed;
    int digits = std::numeric_limits<type>::digits;
    type lval;

    lval = 1;
    core::int_logic_shift_right(lval, 0);
    ARCHON_CHECK_EQUAL(lval, 1);

    lval = 0;
    core::int_logic_shift_right(lval, 1);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = 1;
    core::int_logic_shift_right(lval, 1);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = 2;
    core::int_logic_shift_right(lval, 1);
    ARCHON_CHECK_EQUAL(lval, 1);

    lval = core::int_mask<type>(digits);
    core::int_logic_shift_right(lval, digits - 2);
    ARCHON_CHECK_EQUAL(lval, 3);

    lval = core::int_mask<type>(digits);
    core::int_logic_shift_right(lval, digits - 1);
    ARCHON_CHECK_EQUAL(lval, 1);

    lval = core::int_mask<type>(digits);
    core::int_logic_shift_right(lval, digits);
    ARCHON_CHECK_EQUAL(lval, 0);

    lval = core::int_mask<type>(digits);
    core::int_logic_shift_right(lval, digits + 1);
    ARCHON_CHECK_EQUAL(lval, 0);
}


ARCHON_TEST(Core_Integer_TryAdd)
{
    using wide_signed_type     = signed long long;
    using wide_unsigned_type   = unsigned long long;
    using narrow_signed_type   = signed char;
    using narrow_unsigned_type = unsigned char;

    test::test_try_int_add<wide_signed_type, narrow_signed_type>(test_context);
    test::test_try_int_add<wide_unsigned_type, narrow_signed_type>(test_context);
    test::test_try_int_add<narrow_signed_type, wide_signed_type>(test_context);
    test::test_try_int_add<narrow_unsigned_type, wide_signed_type>(test_context);
}


ARCHON_TEST(Core_Integer_TrySub)
{
    using wide_signed_type     = signed long long;
    using wide_unsigned_type   = unsigned long long;
    using narrow_signed_type   = signed char;
    using narrow_unsigned_type = unsigned char;

    test::test_try_int_sub<wide_signed_type, narrow_signed_type>(test_context);
    test::test_try_int_sub<wide_unsigned_type, narrow_signed_type>(test_context);
    test::test_try_int_sub<narrow_signed_type, wide_signed_type>(test_context);
    test::test_try_int_sub<narrow_unsigned_type, wide_signed_type>(test_context);
}


ARCHON_TEST(Core_Integer_TryMul)
{
    using wide_signed_type     = signed long long;
    using wide_unsigned_type   = unsigned long long;
    using narrow_signed_type   = signed char;
    using narrow_unsigned_type = unsigned char;

    test::test_try_int_mul<wide_signed_type, narrow_signed_type>(test_context);
    test::test_try_int_mul<wide_unsigned_type, narrow_signed_type>(test_context);
    test::test_try_int_mul<narrow_signed_type, wide_signed_type>(test_context);
    test::test_try_int_mul<narrow_unsigned_type, wide_signed_type>(test_context);
}


ARCHON_TEST(Core_Integer_TryPow)
{
    int val;

    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 0;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);

    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 1);

    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, -1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, -1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, -1);
    val = -1;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, -1);

    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, 10);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, 100);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, 1000);
    val = 10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);

    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, -10);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, 100);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, -1000);
    val = -10;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);

    int max = core::int_max<int>();
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, max);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, max);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, max);
    val = max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);

    int min = core::int_min<int>();
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, min);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, min);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, min);
    val = min;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);

    int sqrt_of_max = core::int_sqrt(core::int_max<int>());
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 0))))
        ARCHON_CHECK_EQUAL(val, 1);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 1))))
        ARCHON_CHECK_EQUAL(val, sqrt_of_max);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -1))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, 2))))
        ARCHON_CHECK_EQUAL(val, sqrt_of_max * sqrt_of_max);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -2))))
        ARCHON_CHECK_EQUAL(val, 0);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK_NOT(core::try_int_pow(val, 3))))
        ARCHON_CHECK_EQUAL(val, sqrt_of_max);
    val = sqrt_of_max;
    if (ARCHON_LIKELY(ARCHON_CHECK(core::try_int_pow(val, -3))))
        ARCHON_CHECK_EQUAL(val, 0);
}


ARCHON_TEST(Core_Integer_TryArithShiftLeft)
{
    using type_1 = unsigned;
    int digits_1 = std::numeric_limits<type_1>::digits;
    type_1 lval_1;

    lval_1 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_1, 0));
    ARCHON_CHECK_EQUAL(lval_1, 1);

    lval_1 = 0;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_1, 1));
    ARCHON_CHECK_EQUAL(lval_1, 0);

    lval_1 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_1, 1));
    ARCHON_CHECK_EQUAL(lval_1, 2);

    lval_1 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_1, digits_1 - 1));
    ARCHON_CHECK_EQUAL(lval_1, type_1(1) << (digits_1 - 1));

    lval_1 = 2;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_1, digits_1 - 1));
    ARCHON_CHECK_EQUAL(lval_1, 2);

    lval_1 = 1;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_1, digits_1));
    ARCHON_CHECK_EQUAL(lval_1, 1);

    lval_1 = 1;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_1, digits_1 + 1));
    ARCHON_CHECK_EQUAL(lval_1, 1);

    lval_1 = core::int_max<type_1>();
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_1, 1));
    ARCHON_CHECK_EQUAL(lval_1, core::int_max<type_1>());

    using type_2 = signed;
    int digits_2 = std::numeric_limits<type_2>::digits;
    type_2 lval_2;

    lval_2 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_2, 0));
    ARCHON_CHECK_EQUAL(lval_2, 1);

    lval_2 = 0;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_2, 1));
    ARCHON_CHECK_EQUAL(lval_2, 0);

    lval_2 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_2, 1));
    ARCHON_CHECK_EQUAL(lval_2, 2);

    lval_2 = 1;
    ARCHON_CHECK(core::try_int_arith_shift_left(lval_2, digits_2 - 1));
    ARCHON_CHECK_EQUAL(lval_2, type_2(1) << (digits_2 - 1));

    lval_2 = 2;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_2, digits_2 - 1));
    ARCHON_CHECK_EQUAL(lval_2, 2);

    lval_2 = 1;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_2, digits_2));
    ARCHON_CHECK_EQUAL(lval_2, 1);

    lval_2 = 1;
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_2, digits_2 + 1));
    ARCHON_CHECK_EQUAL(lval_2, 1);

    lval_2 = core::int_max<type_2>();
    ARCHON_CHECK_NOT(core::try_int_arith_shift_left(lval_2, 1));
    ARCHON_CHECK_EQUAL(lval_2, core::int_max<type_2>());
}


ARCHON_TEST(Core_Integer_Comparisons)
{
    int lval = 0;
    unsigned char rval = 0;
    ARCHON_CHECK(core::int_equal(lval, rval));
    ARCHON_CHECK(!core::int_not_equal(lval, rval));
    ARCHON_CHECK(!core::int_less(lval, rval));
    ARCHON_CHECK(core::int_less_equal(lval, rval));
    ARCHON_CHECK(!core::int_greater(lval, rval));
    ARCHON_CHECK(core::int_greater_equal(lval, rval));

    lval = std::numeric_limits<int>::max();
    rval = std::numeric_limits<unsigned char>::max();
    ARCHON_CHECK(!core::int_equal(lval, rval));
    ARCHON_CHECK(core::int_not_equal(lval, rval));
    ARCHON_CHECK(!core::int_less(lval, rval));
    ARCHON_CHECK(!core::int_less_equal(lval, rval));
    ARCHON_CHECK(core::int_greater(lval, rval));
    ARCHON_CHECK(core::int_greater_equal(lval, rval));

    ARCHON_CHECK(!core::int_equal(int(-1), std::numeric_limits<unsigned>::max()));
    ARCHON_CHECK(core::int_less(int(-1), std::numeric_limits<unsigned>::max()));
}


ARCHON_TEST(Core_Integer_CastToBool)
{
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(-2), false);
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(-1), true);
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(0), false);
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(1), true);
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(2), false);
    ARCHON_CHECK_EQUAL(core::int_cast_a<bool>(3), true);

    ARCHON_CHECK_NOT(core::can_int_cast<bool>(-2));
    ARCHON_CHECK_NOT(core::can_int_cast<bool>(-1));
    ARCHON_CHECK(core::can_int_cast<bool>(0));
    ARCHON_CHECK(core::can_int_cast<bool>(1));
    ARCHON_CHECK_NOT(core::can_int_cast<bool>(2));
    ARCHON_CHECK_NOT(core::can_int_cast<bool>(3));

    bool result;
    result = true;
    if (ARCHON_CHECK_NOT(core::try_int_cast<bool>(-2, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = false;
    if (ARCHON_CHECK_NOT(core::try_int_cast<bool>(-1, result)))
        ARCHON_CHECK_EQUAL(result, false);
    result = true;
    if (ARCHON_CHECK(core::try_int_cast<bool>(0, result)))
        ARCHON_CHECK_EQUAL(result, false);
    result = false;
    if (ARCHON_CHECK(core::try_int_cast<bool>(1, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = true;
    if (ARCHON_CHECK_NOT(core::try_int_cast<bool>(2, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = false;
    if (ARCHON_CHECK_NOT(core::try_int_cast<bool>(3, result)))
        ARCHON_CHECK_EQUAL(result, false);
}


ARCHON_TEST(Core_Integer_CastToSignedChar)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(min), min);
    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(-2), -2);
    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(-1), -1);
    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(0), 0);
    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(1), 1);
    ARCHON_CHECK_EQUAL(core::int_cast_a<signed char>(max), max);

    ARCHON_CHECK(core::can_int_cast<signed char>(min));
    ARCHON_CHECK(core::can_int_cast<signed char>(-2));
    ARCHON_CHECK(core::can_int_cast<signed char>(-1));
    ARCHON_CHECK(core::can_int_cast<signed char>(0));
    ARCHON_CHECK(core::can_int_cast<signed char>(1));
    ARCHON_CHECK(core::can_int_cast<signed char>(max));

    signed char result;
    result = 0;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(min, result)))
        ARCHON_CHECK_EQUAL(result, min);
    result = 0;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(-2, result)))
        ARCHON_CHECK_EQUAL(result, -2);
    result = 0;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(-1, result)))
        ARCHON_CHECK_EQUAL(result, -1);
    result = 1;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(0, result)))
        ARCHON_CHECK_EQUAL(result, 0);
    result = 0;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(1, result)))
        ARCHON_CHECK_EQUAL(result, 1);
    result = 0;
    if (ARCHON_CHECK(core::try_int_cast<signed char>(max, result)))
        ARCHON_CHECK_EQUAL(result, max);

    if (core::int_width<std::intmax_t>() > core::int_width<signed char>()) {
        std::intmax_t min_2 = min;
        std::intmax_t max_2 = max;

        ARCHON_CHECK(core::can_int_cast<signed char>(min_2));
        ARCHON_CHECK(core::can_int_cast<signed char>(max_2));
        ARCHON_CHECK_NOT(core::can_int_cast<signed char>(min_2 - 1));
        ARCHON_CHECK_NOT(core::can_int_cast<signed char>(max_2 + 1));

        result = 0;
        if (ARCHON_CHECK(core::try_int_cast<signed char>(min_2, result)))
            ARCHON_CHECK_EQUAL(result, min_2);
        result = 0;
        if (ARCHON_CHECK(core::try_int_cast<signed char>(max_2, result)))
            ARCHON_CHECK_EQUAL(result, max_2);
        result = 0;
        if (ARCHON_CHECK_NOT(core::try_int_cast<signed char>(min_2 - 1, result)))
            ARCHON_CHECK_EQUAL(result, 0);
        result = 0;
        if (ARCHON_CHECK_NOT(core::try_int_cast<signed char>(max_2 + 1, result)))
            ARCHON_CHECK_EQUAL(result, 0);
    }
}


ARCHON_TEST(Core_Integer_CanCastToSigned)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK(core::can_int_cast<short>(min));
    ARCHON_CHECK(core::can_int_cast<int>(min));
    ARCHON_CHECK(core::can_int_cast<long>(min));
    ARCHON_CHECK(core::can_int_cast<long long>(min));
    ARCHON_CHECK(core::can_int_cast<std::intmax_t>(min));

    ARCHON_CHECK(core::can_int_cast<short>(-1));
    ARCHON_CHECK(core::can_int_cast<int>(-1));
    ARCHON_CHECK(core::can_int_cast<long>(-1));
    ARCHON_CHECK(core::can_int_cast<long long>(-1));
    ARCHON_CHECK(core::can_int_cast<std::intmax_t>(-1));

    ARCHON_CHECK(core::can_int_cast<short>(0));
    ARCHON_CHECK(core::can_int_cast<int>(0));
    ARCHON_CHECK(core::can_int_cast<long>(0));
    ARCHON_CHECK(core::can_int_cast<long long>(0));
    ARCHON_CHECK(core::can_int_cast<std::intmax_t>(0));

    ARCHON_CHECK(core::can_int_cast<short>(max));
    ARCHON_CHECK(core::can_int_cast<int>(max));
    ARCHON_CHECK(core::can_int_cast<long>(max));
    ARCHON_CHECK(core::can_int_cast<long long>(max));
    ARCHON_CHECK(core::can_int_cast<std::intmax_t>(max));
}


ARCHON_TEST(Core_Integer_CanCastToUnsigned)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK_NOT(core::can_int_cast<bool>(min));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned short>(min));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned int>(min));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned long>(min));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned long long>(min));
    ARCHON_CHECK_NOT(core::can_int_cast<std::uintmax_t>(min));

    ARCHON_CHECK_NOT(core::can_int_cast<bool>(-1));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned short>(-1));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned int>(-1));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned long>(-1));
    ARCHON_CHECK_NOT(core::can_int_cast<unsigned long long>(-1));
    ARCHON_CHECK_NOT(core::can_int_cast<std::uintmax_t>(-1));

    ARCHON_CHECK(core::can_int_cast<bool>(0));
    ARCHON_CHECK(core::can_int_cast<unsigned short>(0));
    ARCHON_CHECK(core::can_int_cast<unsigned int>(0));
    ARCHON_CHECK(core::can_int_cast<unsigned long>(0));
    ARCHON_CHECK(core::can_int_cast<unsigned long long>(0));
    ARCHON_CHECK(core::can_int_cast<std::uintmax_t>(0));

    ARCHON_CHECK_NOT(core::can_int_cast<bool>(max));
    ARCHON_CHECK(core::can_int_cast<unsigned short>(max));
    ARCHON_CHECK(core::can_int_cast<unsigned int>(max));
    ARCHON_CHECK(core::can_int_cast<unsigned long>(max));
    ARCHON_CHECK(core::can_int_cast<unsigned long long>(max));
    ARCHON_CHECK(core::can_int_cast<std::uintmax_t>(max));
}


ARCHON_TEST(Core_Integer_PeriodicMod)
{
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-7, +3), +2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-6, +3), +0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-5, +3), +1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-4, +3), +2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-3, +3), +0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-2, +3), +1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-1, +3), +2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+0, +3), +0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+1, +3), +1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+2, +3), +2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+3, +3), +0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+4, +3), +1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+5, +3), +2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+6, +3), +0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+7, +3), +1);

    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-7, -3), -1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-6, -3), -0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-5, -3), -2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-4, -3), -1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-3, -3), -0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-2, -3), -2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(-1, -3), -1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+0, -3), -0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+1, -3), -2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+2, -3), -1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+3, -3), -0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+4, -3), -2);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+5, -3), -1);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+6, -3), -0);
    ARCHON_CHECK_EQUAL(core::int_periodic_mod(+7, -3), -2);
}


namespace {

using MaskTypes = core::TypeList<char,
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
                                 unsigned long long,
                                 std::intmax_t,
                                 std::uintmax_t>;

struct MaskTest {
    template<class T, std::size_t> static void exec(check::TestContext& test_context)
    {
        using lim = std::numeric_limits<T>;
        ARCHON_CHECK_EQUAL(core::int_mask<T>(0), 0b000);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(1), 0b001);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(2), 0b011);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(3), 0b111);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(lim::digits - 1), lim::max() / 2);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(lim::digits + 0), lim::max() / 1);
        ARCHON_CHECK_EQUAL(core::int_mask<T>(lim::digits + 1), lim::max() / 1);
    }
};


} // unnamed namespace


ARCHON_TEST(Core_Integer_Mask)
{
    core::for_each_type_alt<MaskTypes, MaskTest>(test_context);
}



namespace {


template<class T_1, class T_2>
void test_two_types(check::TestContext& parent_test_context, const std::set<core::SuperInt>& values)
{
    ARCHON_TEST_TRAIL(parent_test_context,
                      core::formatted("%s vs %s", core::get_type_name<T_1>(), core::get_type_name<T_2>()));

    std::vector<T_1> values_1;
    std::vector<T_2> values_2;
    {
        for (core::SuperInt value : values) {
            T_1 v_1;
            if (value.get_as<T_1>(v_1))
                values_1.push_back(v_1);
            T_2 v_2;
            if (value.get_as<T_2>(v_2))
                values_2.push_back(v_2);
        }
    }

    for (T_1 value_1 : values_1) {
        if constexpr (std::is_signed_v<T_1> && std::is_unsigned_v<T_2>) {
            using S = T_1;
            using U = T_2;
            S s = value_1;
            using lim_u = std::numeric_limits<U>;
            bool representable_in_u = (std::make_unsigned_t<S>(s < 0 ? -1 - s : s) < (U(1) << (lim_u::digits - 1)));
            if (representable_in_u)
                ARCHON_CHECK_EQUAL(s, core::cast_from_twos_compl_a<S>(U(s)));
        }

        for (T_2 value_2 : values_2) {
            test_context.logger.trace("%s vs %s", core::as_int(core::promote(value_1)),
                                      core::as_int(core::promote(value_2)));
            // Comparisons
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                core::SuperInt s_1(v_1), s_2(v_2);
                bool eq_1 = s_1 == s_2;
                bool eq_2 = core::int_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(eq_1, eq_2);
                bool ne_1 = s_1 != s_2;
                bool ne_2 = core::int_not_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(ne_1, ne_2);
                bool lt_1 = s_1 < s_2;
                bool lt_2 = core::int_less(v_1, v_2);
                ARCHON_CHECK_EQUAL(lt_1, lt_2);
                bool gt_1 = s_1 > s_2;
                bool gt_2 = core::int_greater(v_1, v_2);
                ARCHON_CHECK_EQUAL(gt_1, gt_2);
                bool le_1 = s_1 <= s_2;
                bool le_2 = core::int_less_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(le_1, le_2);
                bool ge_1 = s_1 >= s_2;
                bool ge_2 = core::int_greater_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(ge_1, ge_2);
            }
            // Addition
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                core::SuperInt s_1(v_1), s_2(v_2);
                bool add_overflow_1 = (s_1.add_with_overflow_detect(s_2) || s_1.cast_has_overflow<T_1>());
                bool add_overflow_2 = !core::try_int_add(v_1, v_2);
                ARCHON_CHECK_EQUAL(add_overflow_1, add_overflow_2);
                if (!add_overflow_1 && !add_overflow_2)
                    ARCHON_CHECK_EQUAL(s_1, core::SuperInt(v_1));
            }
            // Subtraction
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                core::SuperInt s_1(v_1), s_2(v_2);
                bool sub_overflow_1 = (s_1.subtract_with_overflow_detect(s_2) || s_1.cast_has_overflow<T_1>());
                bool sub_overflow_2 = !core::try_int_sub(v_1, v_2);
                ARCHON_CHECK_EQUAL(sub_overflow_1, sub_overflow_2);
                if (!sub_overflow_1 && !sub_overflow_2)
                    ARCHON_CHECK_EQUAL(s_1, core::SuperInt(v_1));
            }
            // Multiplication
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                core::SuperInt s_1(v_1), s_2(v_2);
                bool mul_overflow_1 = (s_1.multiply_with_overflow_detect(s_2) || s_1.cast_has_overflow<T_1>());
                bool mul_overflow_2 = !core::try_int_mul(v_1, v_2);
                ARCHON_CHECK_EQUAL(mul_overflow_1, mul_overflow_2);
                if (!mul_overflow_1 && !mul_overflow_2)
                    ARCHON_CHECK_EQUAL(s_1, core::SuperInt(v_1));
            }
            // Periodic modulo
            if (ARCHON_LIKELY(value_2 != 0)) {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                T_2 v_3 = core::int_periodic_mod(v_1, v_2);
                core::SuperInt s_1(v_1), s_2(v_2);
                core::SuperInt s_3 = (s_2 != core::SuperInt(-1) ? s_1 % s_2 : core::SuperInt(0));
                if (s_2 >= core::SuperInt(0)) {
                    if (s_3 < core::SuperInt(0))
                        s_3 += s_2;
                }
                else {
                    if (s_3 > core::SuperInt(0))
                        s_3 += s_2;
                }
                ARCHON_CHECK_EQUAL(core::SuperInt(v_3), s_3);
            }
        }
    }
}


struct AddMinMax {
    template<class T, std::size_t> static void exec(std::set<core::SuperInt>& values)
    {
        using lim = std::numeric_limits<T>;
        values.insert(core::SuperInt(lim::min()));
        values.insert(core::SuperInt(lim::max()));
    }
};


struct AddHalfMinMax {
    template<class T, std::size_t> static void exec(std::set<core::SuperInt>& values)
    {
        using lim = std::numeric_limits<T>;
        values.insert(core::SuperInt(core::promote(lim::min()) / 2));
        values.insert(core::SuperInt(core::promote(lim::max()) / 2));
    }
};


template<class T_1> struct TestTwoTypes2 {
    template<class T_2, std::size_t>
    static void exec(check::TestContext& test_context, const std::set<core::SuperInt>& values)
    {
        test_two_types<T_1, T_2>(test_context, values);
    }
};

struct TestTwoTypes1 {
    template<class T_1, std::size_t>
    static void exec(check::TestContext& test_context, const std::set<core::SuperInt>& values)
    {
        core::for_each_type_alt<Types, TestTwoTypes2<T_1>>(test_context, values);
    }
};


} // unnamed namespace


ARCHON_TEST(Core_Integer_General)
{
    // Generate a set of interesting values in three steps
    std::set<core::SuperInt> values;

    // Add 0, 1, 2, and 3 to the set (worst case 4)
    for (int i = 0; i < 4; ++i)
        values.insert(core::SuperInt(i));

    // Add min and max for all integer types to set (worst case 30)
    core::for_each_type_alt<Types, AddMinMax>(values);

    // Add half of min and half of max for all integer types to set (worst case 56)
    core::for_each_type_alt<Types, AddHalfMinMax>(values);

    // Add x-1 and x+1 to the set for all x in set (worst case 168)
    {
        core::SuperInt min_val(std::numeric_limits<std::intmax_t>::min());
        core::SuperInt max_val(std::numeric_limits<std::uintmax_t>::max());
        std::set<core::SuperInt> values_2 = values;
        for (core::SuperInt value : values_2) {
            if (value > min_val)
                values.insert(value - core::SuperInt(1));
            if (value < max_val)
                values.insert(value + core::SuperInt(1));
        }
    }

    // Add x+y and x-y to the set for all x and y in set (worst case 56616)
    {
        core::SuperInt min_val(std::numeric_limits<std::intmax_t>::min());
        core::SuperInt max_val(std::numeric_limits<std::uintmax_t>::max());
        std::set<core::SuperInt> values_2 = values;
        for (core::SuperInt value_1 : values_2) {
            for (core::SuperInt value_2 : values_2) {
                core::SuperInt v_1 = value_1;
                if (!v_1.add_with_overflow_detect(value_2)) {
                    if (v_1 >= min_val && v_1 <= max_val)
                        values.insert(v_1);
                }
                core::SuperInt v_2 = value_1;
                if (!v_2.subtract_with_overflow_detect(value_2)) {
                    if (v_2 >= min_val && v_2 <= max_val)
                        values.insert(v_2);
                }
            }
        }
    }

    for (core::SuperInt value : values)
        test_context.logger.detail("Value: %s", value);

    core::for_each_type_alt<Types, TestTwoTypes1>(test_context, values);
}


ARCHON_TEST(Core_Integer_DivRoundUp)
{
    ARCHON_CHECK_EQUAL(core::int_div_round_up(0, 1), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(1, 1), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(2, 1), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_up(0, 2), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(1, 2), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(2, 2), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(3, 2), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(4, 2), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_up(0, 3), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(1, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(2, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(3, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(4, 3), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(5, 3), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(6, 3), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_up(0, 4), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(1, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(2, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(3, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(4, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(5, 4), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(6, 4), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(7, 4), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(8, 4), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_up(0, 5), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(1, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(2, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(3, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(4, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(5, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(6, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(7, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(8, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(9, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_up(10, 5), 2);
}


ARCHON_TEST(Core_Integer_DivRoundHalfDown)
{
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(0, 1), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(1, 1), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(2, 1), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(0, 2), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(1, 2), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(2, 2), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(3, 2), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(4, 2), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(0, 3), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(1, 3), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(2, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(3, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(4, 3), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(5, 3), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(6, 3), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(0, 4), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(1, 4), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(2, 4), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(3, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(4, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(5, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(6, 4), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(7, 4), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(8, 4), 2);

    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(0, 5), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(1, 5), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(2, 5), 0);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(3, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(4, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(5, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(6, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(7, 5), 1);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(8, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(9, 5), 2);
    ARCHON_CHECK_EQUAL(core::int_div_round_half_down(10, 5), 2);
}


namespace {


template<class T> void test_square_root(check::TestContext& test_context, std::mt19937_64& random)
{
    using type = T;
    auto test = [&](type val) {
        type res = core::int_sqrt(val);
        // Check that `res` squared is less than, or equal to `val`, and that `res + 1`
        // squared would either overflow, or be larger than `val`.
        auto val_2 = core::promote(val);
        auto res_2 = core::promote(res);
        ARCHON_CHECK_LESS_EQUAL(res_2 * res_2, val_2);
        auto val_3 = res_2 + 1;
        ARCHON_CHECK(!core::try_int_mul(val_3, res_2 + 1) || val_3 > val_2);
    };
    constexpr long num_rounds = 32768;
    constexpr bool full_coverage = (core::int_find_msb_pos(num_rounds) >=
                                    core::num_value_bits<type>());
    if constexpr (full_coverage) {
            long n = long(core::int_max<type>()) + 1;
            for (long i = 0; i < n; ++i) {
                type val = core::int_cast_a<type>(i);
                test(val);
            }
    }
    else {
        for (long i = 0; i < num_rounds; ++i) {
            type val = core::rand_int_max<type>(random, core::int_max<type>());
            test(val);
        }
    }
}


struct TestSquareRoot {
    template<class T, std::size_t>
    static void exec(check::TestContext& parent_test_context, std::mt19937_64& random)
    {
        ARCHON_TEST_TRAIL(parent_test_context, core::get_type_name<T>());
        test_square_root<T>(test_context, random);
    }
};


} // unnamed namespace


ARCHON_TEST(Core_Integer_SquareRoot)
{
    std::mt19937_64 random(test_context.seed_seq());
    core::for_each_type_alt<Types, TestSquareRoot>(test_context, random);
}
