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
#include <type_traits>
#include <limits>
#include <array>
#include <random>

#include <archon/core/type_list.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/random.hpp>
#include <archon/core/mul_prec_int.hpp>
#include <archon/core/super_int.hpp>
#include <archon/check.hpp>
#include <archon/core/test/integer_tests.hpp>


using namespace archon;
namespace test = core::test;


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


using PartTypes = core::TypeList<unsigned char,
                                 unsigned short,
                                 unsigned int,
                                 unsigned long,
                                 unsigned long long>;


template<class P, class I> void test_int_conversion(I val, check::TestContext& test_context)
{
    using part_type = P;
    using int_type  = I;
    constexpr int part_width = core::int_width<part_type>();
    constexpr int int_digits = std::numeric_limits<int_type>::digits;
    if constexpr (!std::is_signed_v<int_type>) {
        constexpr int num_parts = core::int_div_round_up(int_digits, part_width);
        constexpr bool signed_ = false;
        core::MulPrecInt<part_type, num_parts,     signed_> x(val);
        core::MulPrecInt<part_type, num_parts + 1, signed_> y(val);
        ARCHON_CHECK_EQUAL(int_type(x), val);
        ARCHON_CHECK_EQUAL(int_type(y), val);
    }
    {
        constexpr int num_parts = core::int_div_round_up(int_digits + 1, part_width);
        constexpr bool signed_ = true;
        core::MulPrecInt<part_type, num_parts,     signed_> x(val);
        core::MulPrecInt<part_type, num_parts + 1, signed_> y(val);
        ARCHON_CHECK_EQUAL(int_type(x), val);
        ARCHON_CHECK_EQUAL(int_type(y), val);
    }
}


struct TestIntConversion {
    template<class P, std::size_t> static void exec(check::TestContext& test_context,
                                                    std::mt19937_64& random)
    {
        using int_type  = typename P::first_type;
        using part_type = typename P::second_type;
        test_int_conversion<part_type>(core::int_min<int_type>(), test_context);
        test_int_conversion<part_type>(core::int_max<int_type>(), test_context);
        if constexpr (!std::is_same_v<int_type, bool>) {
            constexpr long n = 32768;
            constexpr bool full_coverage = (core::int_find_msb_pos(n) >= core::int_width<int_type>());
            if constexpr (full_coverage) {
                long base = long(core::int_min<int_type>());
                long n_2 = long(core::int_max<int_type>()) - base + 1;
                for (long i = 0; i < n_2; ++i) {
                    int_type val = core::int_cast_a<int_type>(base + i);
                    test_int_conversion<part_type>(val, test_context);
                }
            }
            else {
                for (long i = 0; i < n; ++i) {
                    int_type val = core::rand_int<int_type>(random);
                    test_int_conversion<part_type>(val, test_context);
                }
            }
        }
    }
};


} // unnamed namespace


ARCHON_TEST(Core_MulPrecInt_IntConversion)
{
    std::mt19937_64 random(test_context.seed_seq());
    core::for_each_type_alt<core::TypeListProduct<IntTypes, PartTypes>, TestIntConversion>(test_context, random);
}


namespace {


template<bool is_signed> struct Fixture {
    using uint_type = core::SuperInt::uint_type;
    static_assert(std::is_unsigned_v<uint_type>);

    using part_type = unsigned short;

    static constexpr int num_parts = ((core::SuperInt::digits + (is_signed ? 1 : 0)) / core::int_width<part_type>());

    using mul_prec_type = core::MulPrecInt<part_type, num_parts, is_signed>;

    std::mt19937_64 random;

    Fixture(check::TestContext& test_context)
        : random(test_context.seed_seq())
    {
    }

    auto rand_super(int width = mul_prec_type::width) -> core::SuperInt
    {
        if (width <= core::int_width<uint_type>()) {
            uint_type value = core::rand_int_bits<uint_type>(random, width);
            bool sign_bit = false;
            if constexpr (is_signed) {
                sign_bit = bool((value >> (width - 1)) & 1);
                value |= sign_bit * (uint_type(-1) - core::int_mask<uint_type>(width));
            }
            return { value, sign_bit };
        }
        ARCHON_ASSERT(width - 1 == core::int_width<uint_type>());
        ARCHON_ASSERT(is_signed);
        uint_type value = core::rand_int_bits<uint_type>(random, width - 1);
        bool sign_bit = core::rand_int<bool>(random);
        return { value, sign_bit };
    }

    auto rand_super_a(int width = mul_prec_type::width) -> core::SuperInt
    {
        if (width <= core::int_width<uint_type>()) {
            uint_type value = core::rand_int_bits<uint_type>(random, width);
            bool sign_bit = false;
            return { value, sign_bit };
        }
        return rand_super(width);
    }

    auto rand_nonzero_super(int width = mul_prec_type::width) -> core::SuperInt
    {
        for (int i = 0; i < 10000; ++i) {
            core::SuperInt s = rand_super(width);
            if (s != core::SuperInt(0))
                return s;
        }
        ARCHON_STEADY_ASSERT_UNREACHABLE();
    }

    static auto from_super(core::SuperInt s) -> mul_prec_type
    {
        uint_type value = s.get_value();
        std::array<part_type, num_parts> parts;
        for (int i = 0; i < num_parts; ++i)
            parts[i] = part_type(value >> (i * core::int_width<part_type>()));
        return mul_prec_type(parts);
    }
};


ARCHON_TEST_VARIANTS(signedness_variants,
                     ARCHON_TEST_VALUE(false, Unsigned),
                     ARCHON_TEST_VALUE(true,  Signed));


long num_rounds = 32768;


} // unnamed namespace


ARCHON_TEST_BATCH(Core_MulPrecInt_Pos, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s = fixture.rand_super();
        mul_prec_type v = fixture.from_super(s);
        ARCHON_CHECK_EQUAL(+v, fixture.from_super(+s));
        ARCHON_CHECK_EQUAL(+v, v);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Neg, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s = fixture.rand_super();
        mul_prec_type v = fixture.from_super(s);
        ARCHON_CHECK_EQUAL(-v, fixture.from_super(-s));
        ARCHON_CHECK_EQUAL(-(-v), v);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Add, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 + v_2, fixture.from_super(s_1 + s_2));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Sub, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 - v_2, fixture.from_super(s_1 - s_2));
        ARCHON_CHECK_EQUAL(v_1 - v_2 + v_2, v_1);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Mul, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 * v_2, fixture.from_super(s_1 * s_2));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Div, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        int width = core::rand_int(fixture.random, 1, mul_prec_type::width);
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_nonzero_super(width);
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 / v_2, fixture.from_super(s_1 / s_2));
        ARCHON_CHECK_EQUAL(v_1 % v_2, fixture.from_super(s_1 % s_2));
        ARCHON_CHECK_EQUAL((v_1 / v_2) * v_2 + v_1 % v_2, v_1);
    }

    // FIXME: Add test cases for difficult to trigger conditional branches in implementation                  
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Not, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s = fixture.rand_super_a();
        mul_prec_type v = fixture.from_super(s);
        ARCHON_CHECK_EQUAL(~v, fixture.from_super(~s));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_And, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super_a();
        core::SuperInt s_2 = fixture.rand_super_a();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 & v_2, fixture.from_super(s_1 & s_2));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Or, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super_a();
        core::SuperInt s_2 = fixture.rand_super_a();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 | v_2, fixture.from_super(s_1 | s_2));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Xor, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super_a();
        core::SuperInt s_2 = fixture.rand_super_a();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_EQUAL(v_1 ^ v_2, fixture.from_super(s_1 ^ s_2));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_ShiftLeft, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s = fixture.rand_super_a();
        int n = core::rand_int_mod(fixture.random, mul_prec_type::width);
        mul_prec_type v = fixture.from_super(s);
        ARCHON_CHECK_EQUAL(v << n, fixture.from_super(s << n));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_ShiftRight, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s = fixture.rand_super_a();
        int n = core::rand_int_mod(fixture.random, mul_prec_type::width);
        mul_prec_type v = fixture.from_super(s);
        ARCHON_CHECK_EQUAL(v >> n, fixture.from_super(s >> n));
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Equal, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK(v_1 == v_1);
        ARCHON_CHECK_EQUAL(v_1 == v_2, s_1 == s_2);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_NotEqual, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_NOT(v_1 != v_1);
        ARCHON_CHECK_EQUAL(v_1 != v_2, s_1 != s_2);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Less, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_NOT(v_1 < v_1);
        ARCHON_CHECK_EQUAL(v_1 < v_2, s_1 < s_2);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_LessEqual, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK(v_1 <= v_1);
        ARCHON_CHECK_EQUAL(v_1 <= v_2, s_1 <= s_2);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_Greater, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK_NOT(v_1 > v_1);
        ARCHON_CHECK_EQUAL(v_1 > v_2, s_1 > s_2);
    }
}


ARCHON_TEST_BATCH(Core_MulPrecInt_GreaterEqual, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using fixture_type = Fixture<is_signed>;
    using mul_prec_type = typename fixture_type::mul_prec_type;

    fixture_type fixture(test_context);

    for (long i = 0; i < num_rounds; ++i) {
        core::SuperInt s_1 = fixture.rand_super();
        core::SuperInt s_2 = fixture.rand_super();
        mul_prec_type v_1 = fixture.from_super(s_1);
        mul_prec_type v_2 = fixture.from_super(s_2);
        ARCHON_CHECK(v_1 >= v_1);
        ARCHON_CHECK_EQUAL(v_1 >= v_2, s_1 >= s_2);
    }
}


ARCHON_TEST(Core_MulPrecInt_CastFromTwosComplA)
{
    // FIXME: Introduce a similar test for a custom integer type that uses the
    // sign/magnitude representation of negative values. This would improve the testing of
    // core::try_int_add().

    using wide_signed_fundamental_type     = signed long long;
    using wide_unsigned_fundamental_type   = unsigned long long;
    using narrow_signed_fundamental_type   = signed char;
    using narrow_unsigned_fundamental_type = unsigned char;

    using wide_signed_mul_prec_int_type     = core::MulPrecInt<unsigned long long, 2, true>;
    using wide_unsigned_mul_prec_int_type   = core::MulPrecInt<unsigned long long, 2, false>;
    using narrow_signed_mul_prec_int_type   = core::MulPrecInt<unsigned char, 1, true>;
    using narrow_unsigned_mul_prec_int_type = core::MulPrecInt<unsigned char, 1, false>;

    // Wide multiple precision vs narrow fundamental
    test::test_cast_from_twos_compl_a<wide_signed_mul_prec_int_type,
                                      narrow_signed_fundamental_type>(test_context);
    test::test_cast_from_twos_compl_a<wide_unsigned_mul_prec_int_type,
                                      narrow_signed_fundamental_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_signed_fundamental_type,
                                      wide_signed_mul_prec_int_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_unsigned_fundamental_type,
                                      wide_signed_mul_prec_int_type>(test_context);

    // Narrow multiple precision vs wide fundamental
    test::test_cast_from_twos_compl_a<narrow_signed_mul_prec_int_type,
                                      wide_signed_fundamental_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_unsigned_mul_prec_int_type,
                                      wide_signed_fundamental_type>(test_context);
    test::test_cast_from_twos_compl_a<wide_signed_fundamental_type,
                                      narrow_signed_mul_prec_int_type>(test_context);
    test::test_cast_from_twos_compl_a<wide_unsigned_fundamental_type,
                                      narrow_signed_mul_prec_int_type>(test_context);

    // Wide multiple precision vs narrow multiple precision
    test::test_cast_from_twos_compl_a<wide_signed_mul_prec_int_type,
                                      narrow_signed_mul_prec_int_type>(test_context);
    test::test_cast_from_twos_compl_a<wide_unsigned_mul_prec_int_type,
                                      narrow_signed_mul_prec_int_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_signed_mul_prec_int_type,
                                      wide_signed_mul_prec_int_type>(test_context);
    test::test_cast_from_twos_compl_a<narrow_unsigned_mul_prec_int_type,
                                      wide_signed_mul_prec_int_type>(test_context);
}


ARCHON_TEST(Core_MulPrecInt_TryIntAdd)
{
    // FIXME: Introduce a similar test for a custom integer type that uses the
    // sign/magnitude representation of negative values. This would improve the testing of
    // core::try_int_add().

    using wide_signed_fundamental_type     = signed long long;
    using wide_unsigned_fundamental_type   = unsigned long long;
    using narrow_signed_fundamental_type   = signed char;
    using narrow_unsigned_fundamental_type = unsigned char;

    using wide_signed_mul_prec_int_type     = core::MulPrecInt<unsigned long long, 2, true>;
    using wide_unsigned_mul_prec_int_type   = core::MulPrecInt<unsigned long long, 2, false>;
    using narrow_signed_mul_prec_int_type   = core::MulPrecInt<unsigned char, 1, true>;
    using narrow_unsigned_mul_prec_int_type = core::MulPrecInt<unsigned char, 1, false>;

    // Wide multiple precision vs narrow fundamental
    test::test_try_int_add<wide_signed_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_add<wide_unsigned_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_add<narrow_signed_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_add<narrow_unsigned_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);

    // Narrow multiple precision vs wide fundamental
    test::test_try_int_add<narrow_signed_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_add<narrow_unsigned_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_add<wide_signed_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_add<wide_unsigned_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);

    // Wide multiple precision vs narrow multiple precision
    test::test_try_int_add<wide_signed_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_add<wide_unsigned_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_add<narrow_signed_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_add<narrow_unsigned_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
}


ARCHON_TEST(Core_MulPrecInt_TryIntSub)
{
    // FIXME: Introduce a similar test for a custom integer type that uses the
    // sign/magnitude representation of negative values. This would improve the testing of
    // core::try_int_add().

    using wide_signed_fundamental_type     = signed long long;
    using wide_unsigned_fundamental_type   = unsigned long long;
    using narrow_signed_fundamental_type   = signed char;
    using narrow_unsigned_fundamental_type = unsigned char;

    using wide_signed_mul_prec_int_type     = core::MulPrecInt<unsigned long long, 2, true>;
    using wide_unsigned_mul_prec_int_type   = core::MulPrecInt<unsigned long long, 2, false>;
    using narrow_signed_mul_prec_int_type   = core::MulPrecInt<unsigned char, 1, true>;
    using narrow_unsigned_mul_prec_int_type = core::MulPrecInt<unsigned char, 1, false>;

    // Wide multiple precision vs narrow fundamental
    test::test_try_int_sub<wide_signed_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_sub<wide_unsigned_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_sub<narrow_signed_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_sub<narrow_unsigned_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);

    // Narrow multiple precision vs wide fundamental
    test::test_try_int_sub<narrow_signed_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_sub<narrow_unsigned_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_sub<wide_signed_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_sub<wide_unsigned_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);

    // Wide multiple precision vs narrow multiple precision
    test::test_try_int_sub<wide_signed_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_sub<wide_unsigned_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_sub<narrow_signed_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_sub<narrow_unsigned_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
}


ARCHON_TEST(Core_MulPrecInt_TryIntMul)
{
    // FIXME: Introduce a similar test for a custom integer type that uses the
    // sign/magnitude representation of negative values. This would improve the testing of
    // core::try_int_add().

    using wide_signed_fundamental_type     = signed long long;
    using wide_unsigned_fundamental_type   = unsigned long long;
    using narrow_signed_fundamental_type   = signed char;
    using narrow_unsigned_fundamental_type = unsigned char;

    using wide_signed_mul_prec_int_type     = core::MulPrecInt<unsigned long long, 2, true>;
    using wide_unsigned_mul_prec_int_type   = core::MulPrecInt<unsigned long long, 2, false>;
    using narrow_signed_mul_prec_int_type   = core::MulPrecInt<unsigned char, 1, true>;
    using narrow_unsigned_mul_prec_int_type = core::MulPrecInt<unsigned char, 1, false>;

    // Wide multiple precision vs narrow fundamental
    test::test_try_int_mul<wide_signed_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_mul<wide_unsigned_mul_prec_int_type,
                           narrow_signed_fundamental_type>(test_context);
    test::test_try_int_mul<narrow_signed_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_mul<narrow_unsigned_fundamental_type,
                           wide_signed_mul_prec_int_type>(test_context);

    // Narrow multiple precision vs wide fundamental
    test::test_try_int_mul<narrow_signed_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_mul<narrow_unsigned_mul_prec_int_type,
                           wide_signed_fundamental_type>(test_context);
    test::test_try_int_mul<wide_signed_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_mul<wide_unsigned_fundamental_type,
                           narrow_signed_mul_prec_int_type>(test_context);

    // Wide multiple precision vs narrow multiple precision
    test::test_try_int_mul<wide_signed_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_mul<wide_unsigned_mul_prec_int_type,
                           narrow_signed_mul_prec_int_type>(test_context);
    test::test_try_int_mul<narrow_signed_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
    test::test_try_int_mul<narrow_unsigned_mul_prec_int_type,
                           wide_signed_mul_prec_int_type>(test_context);
}


ARCHON_TEST_BATCH(Core_MulPrecInt_FormatParse, signedness_variants)
{
    constexpr bool is_signed = test_value;
    using type = core::MulPrecInt<unsigned long long, 4, is_signed>;
    std::mt19937_64 random(test_context.seed_seq());
    core::ValueFormatter formatter(test_context.locale);
    core::ValueParser parser(test_context.locale);
    long num_rounds = 256;
    for (long i = 0; i < num_rounds; ++i) {
        type val = core::rand_int<type>(random);
        std::string_view str = formatter.format(val);
        type var = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(parser.parse(str, var))))
            ARCHON_CHECK_EQUAL(var, val);
    }
}
