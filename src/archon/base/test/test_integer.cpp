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


#include <cstdint>
#include <type_traits>
#include <limits>
#include <vector>
#include <set>

#include <archon/base/type_list.hpp>
#include <archon/base/super_int.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/demangle.hpp>
#include <archon/unit_test.hpp>


using namespace archon;
using namespace archon::base;
using unit_test::TestContext;


ARCHON_TEST(Base_Integer_Add)
{
    int max = std::numeric_limits<int>::max();
    int i = max - 2;
    ARCHON_CHECK_NOTHROW(base::int_add(i, 1));
    ARCHON_CHECK_EQUAL(i, max - 1);
    ARCHON_CHECK_THROW(base::int_add(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max - 1);
}


ARCHON_TEST(Base_Integer_Sub)
{
    int min = std::numeric_limits<int>::min();
    int i = min + 2;
    ARCHON_CHECK_NOTHROW(base::int_sub(i, 1));
    ARCHON_CHECK_EQUAL(i, min + 1);
    ARCHON_CHECK_THROW(base::int_sub(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, min + 1);
}


ARCHON_TEST(Base_Integer_Mul)
{
    int max = std::numeric_limits<int>::max();
    int i = max / 3;
    ARCHON_CHECK_NOTHROW(base::int_mul(i, 2));
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
    ARCHON_CHECK_THROW(base::int_mul(i, 2), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
}


ARCHON_TEST(Base_Integer_ShiftLeft)
{
    int max = std::numeric_limits<int>::max();
    int i = max / 3;
    ARCHON_CHECK_NOTHROW(base::int_shift_left(i, 1));
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
    ARCHON_CHECK_THROW(base::int_shift_left(i, 1), std::overflow_error);
    ARCHON_CHECK_EQUAL(i, max / 3 * 2);
}


ARCHON_TEST(Base_Integer_TryAdd)
{
    // signed and signed
    {
        int lval = 255;
        char rval = 10;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, 255 + 10);

        rval = 1;
        lval = std::numeric_limits<int>::max();
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval)); // does overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max()); // unchanged

        rval = 1;
        lval = std::numeric_limits<int>::max() - 1;
        ARCHON_CHECK(base::try_int_add(lval, rval)); // does not overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max()); // changed

        rval = 0;
        lval = std::numeric_limits<int>::max();
        ARCHON_CHECK(base::try_int_add(lval, rval)); // does not overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max()); // unchanged

        rval = -1;                                       
        lval = std::numeric_limits<int>::min();
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval)); // does overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::min()); // unchanged
    }
    // signed and unsigned
    {
        char lval = std::numeric_limits<char>::max();
        std::size_t rval = 0;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::max());

        lval = std::numeric_limits<char>::max();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::max());

        lval = 0;
        rval = std::numeric_limits<char>::max();
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::max());

        lval = -1;
        rval = std::numeric_limits<char>::max() + 1;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::max());

        lval = -1;
        rval = std::numeric_limits<char>::max() + 2;                                      
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, -1);
    }
    // unsigned and signed
    {
        std::size_t lval = std::numeric_limits<std::size_t>::max();
        char rval = 0;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = std::numeric_limits<std::size_t>::max();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = std::numeric_limits<std::size_t>::max();
        rval = -1;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max() - 1);

        lval = std::numeric_limits<std::size_t>::min();
        rval = 0;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        lval = std::numeric_limits<std::size_t>::min();
        rval = -1;
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        // lval::bits < rval::bits                                                                                                                                                                                                                                                                                       
        unsigned char lval2 = std::numeric_limits<unsigned char>::max();
        int64_t rval2 = 1;
        ARCHON_CHECK_NOT(base::try_int_add(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, std::numeric_limits<unsigned char>::max());

        lval2 = std::numeric_limits<unsigned char>::max() - 1;
        rval2 = 1;
        ARCHON_CHECK(base::try_int_add(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, std::numeric_limits<unsigned char>::max());

        lval2 = 0;
        rval2 = std::numeric_limits<unsigned char>::max() + 1;
        ARCHON_CHECK_NOT(base::try_int_add(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, 0);
    }
    // unsigned and unsigned
    {
        std::size_t lval = std::numeric_limits<std::size_t>::max();
        std::size_t rval = 0;
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = std::numeric_limits<std::size_t>::max();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = 0;
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = 1;
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, 1);

        lval = std::numeric_limits<std::size_t>::max();
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK_NOT(base::try_int_add(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());
    }
}


ARCHON_TEST(Base_Integer_TrySub)
{
    // signed and signed
    {
        int lval = std::numeric_limits<int>::max() - 1;
        char rval = -10;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval)); // does overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max() - 1); // unchanged

        rval = -1;
        lval = std::numeric_limits<int>::max();
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval)); // does overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max()); // unchanged

        rval = 0;
        lval = std::numeric_limits<int>::max();
        ARCHON_CHECK(base::try_int_sub(lval, rval)); // does not overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max()); // unchanged

        rval = 0;
        lval = std::numeric_limits<int>::min();
        ARCHON_CHECK(base::try_int_sub(lval, rval)); // does not overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::min()); // unchanged

        rval = 1;
        lval = std::numeric_limits<int>::min();
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval)); // does overflow
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::min()); // unchanged
    }
    // signed and unsigned
    {
        char lval = std::numeric_limits<char>::min();
        std::size_t rval = 0;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::min());

        lval = std::numeric_limits<char>::min();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::min());

        lval = std::numeric_limits<char>::min() + 1;
        rval = 1;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::min());

        lval = std::numeric_limits<char>::min() + 1;
        rval = 2;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::min() + 1);

        lval = 0;
        rval = -1 * std::numeric_limits<char>::min();
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<char>::min());

        lval = -1;
        rval = -1 * std::numeric_limits<char>::min();
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, -1);
    }
    // unsigned and signed
    {
        std::size_t lval = std::numeric_limits<std::size_t>::min();
        char rval = 0;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        lval = std::numeric_limits<std::size_t>::min();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        lval = std::numeric_limits<std::size_t>::max();
        rval = 1;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max() - 1);

        lval = std::numeric_limits<std::size_t>::max();
        rval = 0;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        lval = std::numeric_limits<std::size_t>::max();
        rval = -1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max());

        // lval::bits < rval::bits                                                                                                                                                                                                                                                                                       
        unsigned char lval2 = 0;
        int64_t rval2 = 1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, 0);

        lval2 = std::numeric_limits<unsigned char>::max();
        rval2 = std::numeric_limits<unsigned char>::max();
        ARCHON_CHECK(base::try_int_sub(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, 0);

        lval2 = std::numeric_limits<unsigned char>::max();
        rval2 = std::numeric_limits<unsigned char>::max() + 1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval2, rval2));
        ARCHON_CHECK_EQUAL(lval2, std::numeric_limits<unsigned char>::max());
    }
    // unsigned and unsigned
    {
        std::size_t lval = std::numeric_limits<std::size_t>::min();
        std::size_t rval = 0;
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        lval = std::numeric_limits<std::size_t>::min();
        rval = 1;
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::min());

        lval = 0;
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, 0);

        lval = std::numeric_limits<std::size_t>::max() - 1;
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK_NOT(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, std::numeric_limits<std::size_t>::max() - 1);

        lval = std::numeric_limits<std::size_t>::max();
        rval = std::numeric_limits<std::size_t>::max();
        ARCHON_CHECK(base::try_int_sub(lval, rval));
        ARCHON_CHECK_EQUAL(lval, 0);
    }
}


ARCHON_TEST(Base_Integer_TryMul)
{
    int lval = 256;
    char rval = 2;
    ARCHON_CHECK(base::try_int_mul(lval, rval));
    ARCHON_CHECK_EQUAL(lval, 512);

    lval = std::numeric_limits<int>::max();
    rval = 2;
    ARCHON_CHECK_NOT(base::try_int_mul(lval, rval));
    ARCHON_CHECK_EQUAL(lval, std::numeric_limits<int>::max());

    char lval2 = 2;
    int rval2 = 63;
    ARCHON_CHECK(base::try_int_mul(lval2, rval2));
    ARCHON_CHECK_EQUAL(lval2, 126);

    lval2 = 2;
    rval2 = 64; // numeric_limits<char>::max() is 127                                                                                                         
    ARCHON_CHECK_NOT(base::try_int_mul(lval2, rval2));
    ARCHON_CHECK_EQUAL(lval2, 2);
}


ARCHON_TEST(Base_Integer_TryShiftLeft)
{
    std::size_t unsigned_int;
    int digits;

    unsigned_int = 1;
    ARCHON_CHECK(base::try_int_shift_left(unsigned_int, 0));
    ARCHON_CHECK_EQUAL(unsigned_int, 1);

    unsigned_int = 0;
    ARCHON_CHECK(base::try_int_shift_left(unsigned_int, 1));
    ARCHON_CHECK_EQUAL(unsigned_int, 0);

    unsigned_int = 1;
    ARCHON_CHECK(base::try_int_shift_left(unsigned_int, 1));
    ARCHON_CHECK_EQUAL(unsigned_int, 2);

    unsigned_int = 1;
    digits = std::numeric_limits<std::size_t>::digits;
    ARCHON_CHECK(base::try_int_shift_left(unsigned_int, digits - 1));
    ARCHON_CHECK_EQUAL(unsigned_int, std::size_t(1) << (digits - 1));

    unsigned_int = 2;
    ARCHON_CHECK_NOT(base::try_int_shift_left(unsigned_int, digits - 1));
    ARCHON_CHECK_EQUAL(unsigned_int, 2);

    unsigned_int = std::numeric_limits<std::size_t>::max();
    ARCHON_CHECK_NOT(base::try_int_shift_left(unsigned_int, 1));
    ARCHON_CHECK_EQUAL(unsigned_int, std::numeric_limits<std::size_t>::max());

    int signed_int = 1;
    ARCHON_CHECK(base::try_int_shift_left(signed_int, 0));
    ARCHON_CHECK_EQUAL(signed_int, 1);

    signed_int = 0;
    ARCHON_CHECK(base::try_int_shift_left(signed_int, 1));
    ARCHON_CHECK_EQUAL(signed_int, 0);

    signed_int = 1;
    ARCHON_CHECK(base::try_int_shift_left(signed_int, 1));
    ARCHON_CHECK_EQUAL(signed_int, 2);

    signed_int = 1;
    digits = std::numeric_limits<int>::digits;
    ARCHON_CHECK(base::try_int_shift_left(signed_int, digits - 1));
    ARCHON_CHECK_EQUAL(signed_int, int(1) << (digits - 1));

    signed_int = 2;
    ARCHON_CHECK_NOT(base::try_int_shift_left(signed_int, digits - 1));
    ARCHON_CHECK_EQUAL(signed_int, 2);

    signed_int = std::numeric_limits<int>::max();
    ARCHON_CHECK_NOT(base::try_int_shift_left(signed_int, 1));
    ARCHON_CHECK_EQUAL(signed_int, std::numeric_limits<int>::max());
}


ARCHON_TEST(Base_Integer_Comparisons)
{
    int lval = 0;
    unsigned char rval = 0;
    ARCHON_CHECK(base::int_equal_to(lval, rval));
    ARCHON_CHECK(!base::int_not_equal_to(lval, rval));
    ARCHON_CHECK(!base::int_less_than(lval, rval));
    ARCHON_CHECK(base::int_less_than_or_equal(lval, rval));
    ARCHON_CHECK(!base::int_greater_than(lval, rval));
    ARCHON_CHECK(base::int_greater_than_or_equal(lval, rval));

    lval = std::numeric_limits<int>::max();
    rval = std::numeric_limits<unsigned char>::max();
    ARCHON_CHECK(!base::int_equal_to(lval, rval));
    ARCHON_CHECK(base::int_not_equal_to(lval, rval));
    ARCHON_CHECK(!base::int_less_than(lval, rval));
    ARCHON_CHECK(!base::int_less_than_or_equal(lval, rval));
    ARCHON_CHECK(base::int_greater_than(lval, rval));
    ARCHON_CHECK(base::int_greater_than_or_equal(lval, rval));
}


ARCHON_TEST(Base_Integer_CastToBool)
{
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(-2), false);
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(-1), true);
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(0), false);
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(1), true);
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(2), false);
    ARCHON_CHECK_EQUAL(base::int_cast<bool>(3), true);

    ARCHON_CHECK_NOT(base::can_int_cast<bool>(-2));
    ARCHON_CHECK_NOT(base::can_int_cast<bool>(-1));
    ARCHON_CHECK(base::can_int_cast<bool>(0));
    ARCHON_CHECK(base::can_int_cast<bool>(1));
    ARCHON_CHECK_NOT(base::can_int_cast<bool>(2));
    ARCHON_CHECK_NOT(base::can_int_cast<bool>(3));

    bool result;
    result = true;
    if (ARCHON_CHECK_NOT(base::try_int_cast<bool>(-2, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = false;
    if (ARCHON_CHECK_NOT(base::try_int_cast<bool>(-1, result)))
        ARCHON_CHECK_EQUAL(result, false);
    result = true;
    if (ARCHON_CHECK(base::try_int_cast<bool>(0, result)))
        ARCHON_CHECK_EQUAL(result, false);
    result = false;
    if (ARCHON_CHECK(base::try_int_cast<bool>(1, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = true;
    if (ARCHON_CHECK_NOT(base::try_int_cast<bool>(2, result)))
        ARCHON_CHECK_EQUAL(result, true);
    result = false;
    if (ARCHON_CHECK_NOT(base::try_int_cast<bool>(3, result)))
        ARCHON_CHECK_EQUAL(result, false);
}


ARCHON_TEST(Base_Integer_CastToSignedChar)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK_EQUAL(int_cast<signed char>(min), min);
    ARCHON_CHECK_EQUAL(int_cast<signed char>(-2), -2);
    ARCHON_CHECK_EQUAL(int_cast<signed char>(-1), -1);
    ARCHON_CHECK_EQUAL(int_cast<signed char>(0), 0);
    ARCHON_CHECK_EQUAL(int_cast<signed char>(1), 1);
    ARCHON_CHECK_EQUAL(int_cast<signed char>(max), max);

    ARCHON_CHECK(base::can_int_cast<signed char>(min));
    ARCHON_CHECK(base::can_int_cast<signed char>(-2));
    ARCHON_CHECK(base::can_int_cast<signed char>(-1));
    ARCHON_CHECK(base::can_int_cast<signed char>(0));
    ARCHON_CHECK(base::can_int_cast<signed char>(1));
    ARCHON_CHECK(base::can_int_cast<signed char>(max));

    signed char result;
    result = 0;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(min, result)))
        ARCHON_CHECK_EQUAL(result, min);
    result = 0;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(-2, result)))
        ARCHON_CHECK_EQUAL(result, -2);
    result = 0;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(-1, result)))
        ARCHON_CHECK_EQUAL(result, -1);
    result = 1;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(0, result)))
        ARCHON_CHECK_EQUAL(result, 0);
    result = 0;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(1, result)))
        ARCHON_CHECK_EQUAL(result, 1);
    result = 0;
    if (ARCHON_CHECK(base::try_int_cast<signed char>(max, result)))
        ARCHON_CHECK_EQUAL(result, max);

    if (base::get_int_width<std::intmax_t>() > base::get_int_width<signed char>()) {
        std::intmax_t min_2 = min;
        std::intmax_t max_2 = max;

        ARCHON_CHECK(base::can_int_cast<signed char>(min_2));
        ARCHON_CHECK(base::can_int_cast<signed char>(max_2));
        ARCHON_CHECK_NOT(base::can_int_cast<signed char>(min_2 - 1));
        ARCHON_CHECK_NOT(base::can_int_cast<signed char>(max_2 + 1));

        result = 0;
        if (ARCHON_CHECK(base::try_int_cast<signed char>(min_2, result)))
            ARCHON_CHECK_EQUAL(result, min_2);
        result = 0;
        if (ARCHON_CHECK(base::try_int_cast<signed char>(max_2, result)))
            ARCHON_CHECK_EQUAL(result, max_2);
        result = 0;
        if (ARCHON_CHECK_NOT(base::try_int_cast<signed char>(min_2 - 1, result)))
            ARCHON_CHECK_EQUAL(result, 0);
        result = 0;
        if (ARCHON_CHECK_NOT(base::try_int_cast<signed char>(max_2 + 1, result)))
            ARCHON_CHECK_EQUAL(result, 0);
    }
}


ARCHON_TEST(Base_Integer_CanCastToSigned)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK(base::can_int_cast<short>(min));
    ARCHON_CHECK(base::can_int_cast<int>(min));
    ARCHON_CHECK(base::can_int_cast<long>(min));
    ARCHON_CHECK(base::can_int_cast<long long>(min));
    ARCHON_CHECK(base::can_int_cast<std::intmax_t>(min));

    ARCHON_CHECK(base::can_int_cast<short>(-1));
    ARCHON_CHECK(base::can_int_cast<int>(-1));
    ARCHON_CHECK(base::can_int_cast<long>(-1));
    ARCHON_CHECK(base::can_int_cast<long long>(-1));
    ARCHON_CHECK(base::can_int_cast<std::intmax_t>(-1));

    ARCHON_CHECK(base::can_int_cast<short>(0));
    ARCHON_CHECK(base::can_int_cast<int>(0));
    ARCHON_CHECK(base::can_int_cast<long>(0));
    ARCHON_CHECK(base::can_int_cast<long long>(0));
    ARCHON_CHECK(base::can_int_cast<std::intmax_t>(0));

    ARCHON_CHECK(base::can_int_cast<short>(max));
    ARCHON_CHECK(base::can_int_cast<int>(max));
    ARCHON_CHECK(base::can_int_cast<long>(max));
    ARCHON_CHECK(base::can_int_cast<long long>(max));
    ARCHON_CHECK(base::can_int_cast<std::intmax_t>(max));
}


ARCHON_TEST(Base_Integer_CanCastToUnsigned)
{
    using lim = std::numeric_limits<signed char>;
    int min = lim::min();
    int max = lim::max();

    ARCHON_CHECK_NOT(base::can_int_cast<bool>(min));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned short>(min));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned int>(min));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned long>(min));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned long long>(min));
    ARCHON_CHECK_NOT(base::can_int_cast<std::uintmax_t>(min));

    ARCHON_CHECK_NOT(base::can_int_cast<bool>(-1));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned short>(-1));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned int>(-1));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned long>(-1));
    ARCHON_CHECK_NOT(base::can_int_cast<unsigned long long>(-1));
    ARCHON_CHECK_NOT(base::can_int_cast<std::uintmax_t>(-1));

    ARCHON_CHECK(base::can_int_cast<bool>(0));
    ARCHON_CHECK(base::can_int_cast<unsigned short>(0));
    ARCHON_CHECK(base::can_int_cast<unsigned int>(0));
    ARCHON_CHECK(base::can_int_cast<unsigned long>(0));
    ARCHON_CHECK(base::can_int_cast<unsigned long long>(0));
    ARCHON_CHECK(base::can_int_cast<std::uintmax_t>(0));

    ARCHON_CHECK_NOT(base::can_int_cast<bool>(max));
    ARCHON_CHECK(base::can_int_cast<unsigned short>(max));
    ARCHON_CHECK(base::can_int_cast<unsigned int>(max));
    ARCHON_CHECK(base::can_int_cast<unsigned long>(max));
    ARCHON_CHECK(base::can_int_cast<unsigned long long>(max));
    ARCHON_CHECK(base::can_int_cast<std::uintmax_t>(max));
}


ARCHON_TEST(Base_Integer_IsNegative)
{
    std::size_t unsigned_int = 0;
    ARCHON_CHECK(!base::is_negative(unsigned_int));

    unsigned_int = std::size_t(-1);
    ARCHON_CHECK(!base::is_negative(unsigned_int));

    char c = 0;
    ARCHON_CHECK(!base::is_negative(c));

    c = 1;
    ARCHON_CHECK(!base::is_negative(c));

    c = std::numeric_limits<char>::max();
    ARCHON_CHECK(!base::is_negative(c));

    c = char(-1);
    ARCHON_CHECK(base::is_negative(c));

    c = std::numeric_limits<char>::min();
    ARCHON_CHECK(base::is_negative(c));
}


namespace {

using MaskTypes = base::TypeList<char,
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

template<class T, int> struct MaskTest {
    static void exec(TestContext* test_context_ptr)
    {
        TestContext& test_context = *test_context_ptr;
        using lim = std::numeric_limits<T>;
        ARCHON_CHECK_EQUAL(base::int_mask<T>(0), 0b000);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(1), 0b001);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(2), 0b011);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(3), 0b111);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(lim::digits - 1), lim::max() / 2);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(lim::digits + 0), lim::max() / 1);
        ARCHON_CHECK_EQUAL(base::int_mask<T>(lim::digits + 1), lim::max() / 1);
    }
};


} // unnamed namespace


ARCHON_TEST(Base_Integer_Mask)
{
    base::for_each_type<MaskTypes, MaskTest>(&test_context);
}



namespace {


template<class T_1, class T_2>
void test_two_types(TestContext& test_context, const std::set<SuperInt>& values)
{
    test_context.logger.detail("test_two_arg_ops: %s vs %s", base::get_type_name<T_1>(),
                               base::get_type_name<T_2>());

    std::vector<T_1> values_1;
    std::vector<T_2> values_2;
    {
        for (SuperInt value : values) {
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
            bool representable_in_u =
                (std::make_unsigned_t<S>(s < 0 ? -1 - s : s) < (U(1) << (lim_u::digits - 1)));
            if (representable_in_u)
                ARCHON_CHECK_EQUAL(s, base::cast_from_twos_compl<S>(U(s)));
        }

        for (T_2 value_2 : values_2) {
            test_context.logger.trace("----> %s vs %s", base::promote(value_1),
                                      base::promote(value_2));
            // Comparisons
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                SuperInt s_1(v_1), s_2(v_2);
                bool eq_1 = s_1 == s_2;
                bool eq_2 = base::int_equal_to(v_1, v_2);
                ARCHON_CHECK_EQUAL(eq_1, eq_2);
                bool ne_1 = s_1 != s_2;
                bool ne_2 = base::int_not_equal_to(v_1, v_2);
                ARCHON_CHECK_EQUAL(ne_1, ne_2);
                bool lt_1 = s_1 < s_2;
                bool lt_2 = base::int_less_than(v_1, v_2);
                ARCHON_CHECK_EQUAL(lt_1, lt_2);
                bool gt_1 = s_1 > s_2;
                bool gt_2 = base::int_greater_than(v_1, v_2);
                ARCHON_CHECK_EQUAL(gt_1, gt_2);
                bool le_1 = s_1 <= s_2;
                bool le_2 = base::int_less_than_or_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(le_1, le_2);
                bool ge_1 = s_1 >= s_2;
                bool ge_2 = base::int_greater_than_or_equal(v_1, v_2);
                ARCHON_CHECK_EQUAL(ge_1, ge_2);
            }
            // Addition
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                SuperInt s_1(v_1), s_2(v_2);
                bool add_overflow_1 = (s_1.add_with_overflow_detect(s_2) ||
                                       s_1.cast_has_overflow<T_1>());
                bool add_overflow_2 = !base::try_int_add(v_1, v_2);
                ARCHON_CHECK_EQUAL(add_overflow_1, add_overflow_2);
                if (!add_overflow_1 && !add_overflow_2)
                    ARCHON_CHECK_EQUAL(s_1, SuperInt(v_1));
            }
            // Subtraction
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                SuperInt s_1(v_1), s_2(v_2);
                bool sub_overflow_1 = (s_1.subtract_with_overflow_detect(s_2) ||
                                       s_1.cast_has_overflow<T_1>());
                bool sub_overflow_2 = !base::try_int_sub(v_1, v_2);
                ARCHON_CHECK_EQUAL(sub_overflow_1, sub_overflow_2);
                if (!sub_overflow_1 && !sub_overflow_2)
                    ARCHON_CHECK_EQUAL(s_1, SuperInt(v_1));
            }
            // Multiplication
            {
                T_1 v_1 = value_1;
                T_2 v_2 = value_2;
                if (!base::is_negative(v_1) && base::promote(v_2) > 0) {
                    SuperInt s_1(v_1), s_2(v_2);
                    bool mul_overflow_1 = (s_1.multiply_with_overflow_detect(s_2) ||
                                           s_1.cast_has_overflow<T_1>());
                    bool mul_overflow_2 = !base::try_int_mul(v_1, v_2);
                    ARCHON_CHECK_EQUAL(mul_overflow_1, mul_overflow_2);
                    if (!mul_overflow_1 && !mul_overflow_2)
                        ARCHON_CHECK_EQUAL(s_1, SuperInt(v_1));
                }
            }
        }
    }
}


using Types = base::TypeList<bool,
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


template<class T, int> struct AddMinMax {
    static void exec(std::set<SuperInt>* values)
    {
        using lim = std::numeric_limits<T>;
        values->insert(SuperInt(lim::min()));
        values->insert(SuperInt(lim::max()));
    }
};


template<class T_1, int> struct TestTwoTypes1 {
    template<class T_2, int> struct TestTwoTypes2 {
        static void exec(TestContext* test_context, const std::set<SuperInt>* values)
        {
            test_two_types<T_1, T_2>(*test_context, *values);
        }
    };
    static void exec(TestContext* test_context, const std::set<SuperInt>* values)
    {
        base::for_each_type<Types, TestTwoTypes2>(test_context, values);
    }
};


} // unnamed namespace


ARCHON_TEST(Base_Integer_General)
{
    // Generate a set of interesting values in three steps
    std::set<SuperInt> values;

    // Add 0 to the set (worst case 1)
    values.insert(SuperInt(0));

    // Add min and max for all integer types to set (worst case 27)
    base::for_each_type<Types, AddMinMax>(&values);

    // Add x-1 and x+1 to the set for all x in set (worst case 81)
    {
        SuperInt min_val(std::numeric_limits<std::intmax_t>::min());
        SuperInt max_val(std::numeric_limits<std::uintmax_t>::max());
        std::set<SuperInt> values_2 = values;
        for (SuperInt value : values_2) {
            if (value > min_val)
                values.insert(value - SuperInt(1));
            if (value < max_val)
                values.insert(value + SuperInt(1));
        }
    }

    // Add x+y and x-y to the set for all x and y in set (worst case
    // 13203)
    {
        SuperInt min_val(std::numeric_limits<std::intmax_t>::min());
        SuperInt max_val(std::numeric_limits<std::uintmax_t>::max());
        std::set<SuperInt> values_2 = values;
        for (SuperInt value_1 : values_2) {
            for (SuperInt value_2 : values_2) {
                SuperInt v_1 = value_1;
                if (!v_1.add_with_overflow_detect(value_2)) {
                    if (v_1 >= min_val && v_1 <= max_val)
                        values.insert(v_1);
                }
                SuperInt v_2 = value_1;
                if (!v_2.subtract_with_overflow_detect(value_2)) {
                    if (v_2 >= min_val && v_2 <= max_val)
                        values.insert(v_2);
                }
            }
        }
    }

    for (SuperInt value : values)
        test_context.logger.detail("Value: %s", value);

    base::for_each_type<Types, TestTwoTypes1>(&test_context, &values);
}
