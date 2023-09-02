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


#include <cmath>
#include <type_traits>
#include <limits>
#include <array>

#include <archon/core/char_mapper.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/check.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;
namespace test = math::test;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST(Math_Vector_ConstructFromComponents)
{
    // This form is only supported for 2, 3, and 4-vectors, and works by way of template
    // specialization for those sizes.
    math::Vector2 v_1 = { 1, 2 };
    math::Vector3 v_2 = { 1, 2, 3 };
    math::Vector4 v_3 = { 1, 2, 3, 4 };
    ARCHON_CHECK_EQUAL(v_1, math::Vector2(1, 2));
    ARCHON_CHECK_EQUAL(v_2, math::Vector3(1, 2, 3));
    ARCHON_CHECK_EQUAL(v_3, math::Vector4(1, 2, 3, 4));
}


ARCHON_TEST(Math_Vector_ConstructFromArray)
{
    math::Vector v_1 = std::array { 1.0 };
    math::Vector v_2 = std::array { 1.0, 2.0 };
    math::Vector v_3 = std::array { 1.0, 2.0, 3.0 };
    math::Vector v_4 = std::array { 1.0, 2.0, 3.0, 4.0 };
    math::Vector v_5 = std::array { 1.0, 2.0, 3.0, 4.0, 5.0 };
    ARCHON_CHECK_EQUAL(v_1, math::Vector(std::array { 1.0 }));
    ARCHON_CHECK_EQUAL(v_2, math::Vector(std::array { 1.0, 2.0 }));
    ARCHON_CHECK_EQUAL(v_3, math::Vector(std::array { 1.0, 2.0, 3.0 }));
    ARCHON_CHECK_EQUAL(v_4, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 }));
    ARCHON_CHECK_EQUAL(v_5, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vector_ConstructFromFundamentalArray)
{
    double a_1[] = { 1 };
    double a_2[] = { 1, 2 };
    double a_3[] = { 1, 2, 3 };
    double a_4[] = { 1, 2, 3, 4 };
    double a_5[] = { 1, 2, 3, 4, 5 };
    ARCHON_CHECK_EQUAL(math::Vector(a_1), math::Vector(std::array { 1.0 }));
    ARCHON_CHECK_EQUAL(math::Vector(a_2), math::Vector(std::array { 1.0, 2.0 }));
    ARCHON_CHECK_EQUAL(math::Vector(a_3), math::Vector(std::array { 1.0, 2.0, 3.0 }));
    ARCHON_CHECK_EQUAL(math::Vector(a_4), math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 }));
    ARCHON_CHECK_EQUAL(math::Vector(a_5), math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vector_AssignFromArray1)
{
    math::Vector<1> v_1;
    double a_1[] = { 1 };
    v_1 = a_1;
    ARCHON_CHECK_EQUAL(v_1, math::Vector(std::array { 1.0 }));
    math::Vector<2> v_2;
    double a_2[] = { 1, 2 };
    v_2 = a_2;
    ARCHON_CHECK_EQUAL(v_2, math::Vector(std::array { 1.0, 2.0 }));
    math::Vector<3> v_3;
    double a_3[] = { 1, 2, 3 };
    v_3 = a_3;
    ARCHON_CHECK_EQUAL(v_3, math::Vector(std::array { 1.0, 2.0, 3.0 }));
    math::Vector<4> v_4;
    double a_4[] = { 1, 2, 3, 4 };
    v_4 = a_4;
    ARCHON_CHECK_EQUAL(v_4, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vector<5> v_5;
    double a_5[] = { 1, 2, 3, 4, 5 };
    v_5 = a_5;
    ARCHON_CHECK_EQUAL(v_5, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vector_AssignFromArray2)
{
    math::Vector<1> v_1;
    v_1 = std::array { 1.0 };
    ARCHON_CHECK_EQUAL(v_1, math::Vector(std::array { 1.0 }));
    math::Vector<2> v_2;
    v_2 = std::array { 1.0, 2.0 };
    ARCHON_CHECK_EQUAL(v_2, math::Vector(std::array { 1.0, 2.0 }));
    math::Vector<3> v_3;
    v_3 = std::array { 1.0, 2.0, 3.0 };
    ARCHON_CHECK_EQUAL(v_3, math::Vector(std::array { 1.0, 2.0, 3.0 }));
    math::Vector<4> v_4;
    v_4 = std::array { 1.0, 2.0, 3.0, 4.0 };
    ARCHON_CHECK_EQUAL(v_4, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vector<5> v_5;
    v_5 = std::array { 1.0, 2.0, 3.0, 4.0, 5.0 };
    ARCHON_CHECK_EQUAL(v_5, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vector_CopyAssign)
{
    math::Vector<1> v_1;
    v_1 = math::Vector(std::array { 1.0 });
    ARCHON_CHECK_EQUAL(v_1, math::Vector(std::array { 1.0 }));
    math::Vector<2> v_2;
    v_2 = math::Vector(std::array { 1.0, 2.0 });
    ARCHON_CHECK_EQUAL(v_2, math::Vector(std::array { 1.0, 2.0 }));
    math::Vector<3> v_3;
    v_3 = math::Vector(std::array { 1.0, 2.0, 3.0 });
    ARCHON_CHECK_EQUAL(v_3, math::Vector(std::array { 1.0, 2.0, 3.0 }));
    math::Vector<4> v_4;
    v_4 = math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 });
    ARCHON_CHECK_EQUAL(v_4, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vector<5> v_5;
    v_5 = math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 });
    ARCHON_CHECK_EQUAL(v_5, math::Vector(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vector_Components)
{
    math::Vector3 vec = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL_SEQ(vec.components(), (std::array { 1, 2, 3 }));
}


ARCHON_TEST(Math_Vector_Comparison)
{
    math::Vector2 vec_1 = { 1, 3 };
    math::Vector2 vec_2 = { 1, 3 };
    math::Vector2 vec_3 = { 1, 2 };
    math::Vector2 vec_4 = { 2, 1 };

    ARCHON_CHECK(vec_1 == vec_2);
    ARCHON_CHECK_NOT(vec_1 == vec_3);
    ARCHON_CHECK_NOT(vec_1 == vec_4);

    ARCHON_CHECK_NOT(vec_1 != vec_2);
    ARCHON_CHECK(vec_1 != vec_3);
    ARCHON_CHECK(vec_1 != vec_4);

    ARCHON_CHECK_NOT(vec_1 < vec_2);
    ARCHON_CHECK_NOT(vec_1 < vec_3);
    ARCHON_CHECK(vec_1 < vec_4);

    ARCHON_CHECK(vec_1 <= vec_2);
    ARCHON_CHECK_NOT(vec_1 <= vec_3);
    ARCHON_CHECK(vec_1 <= vec_4);

    ARCHON_CHECK_NOT(vec_1 > vec_2);
    ARCHON_CHECK(vec_1 > vec_3);
    ARCHON_CHECK_NOT(vec_1 > vec_4);

    ARCHON_CHECK(vec_1 >= vec_2);
    ARCHON_CHECK(vec_1 >= vec_3);
    ARCHON_CHECK_NOT(vec_1 >= vec_4);
}


ARCHON_TEST_BATCH(Math_Vector_Format, char_variants)
{
    using char_type = test_type;
    std::array<char_type, 16> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 16> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(math::Vector2(1.5, 2.5)), widener.widen("[1.5, 2.5]"));
}


ARCHON_TEST(Math_Vector_AdditionSubtractionNegation)
{
    math::Vector2 a = { 1, 2 };
    math::Vector2F b = { 3, 5 };
    ARCHON_CHECK_EQUAL(a + b, math::Vector2(4, 7));
    ARCHON_CHECK_EQUAL(a - b, math::Vector2(-2, -3));
    ARCHON_CHECK_EQUAL(-a, math::Vector2(-1, -2));
    a += b;
    ARCHON_CHECK_EQUAL(a, math::Vector2(4, 7));
    a -= b;
    ARCHON_CHECK_EQUAL(a, math::Vector2(1, 2));
    static_assert(std::is_same_v<decltype(a + b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a - b)::comp_type, double>);
}


ARCHON_TEST(Math_Vector_Scaling)
{
    math::Vector2F a = { 1, 2 };
    ARCHON_CHECK_EQUAL(a * 2, math::Vector2(2, 4));
    ARCHON_CHECK_EQUAL(3 * a, math::Vector2(3, 6));
    ARCHON_CHECK_EQUAL(a / 2, math::Vector2(0.5, 1.0));
    a *= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Vector2(2, 4));
    a /= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Vector2(1, 2));
    static_assert(std::is_same_v<decltype(2.0 * a)::comp_type, double>);
    static_assert(std::is_same_v<decltype(2.0F * a)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a * 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * 2.0F)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a / 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a / 2.0F)::comp_type, float>);
}


ARCHON_TEST(Math_Vector_Length)
{
    math::Vector2 x = { 1, 2 };
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::len(x), std::sqrt(5), test::scalar_compare(10 * eps));
}


ARCHON_TEST(Math_Vector_SumOfComponents)
{
    math::Vector3 x = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL(math::sum(x), 6);
}


ARCHON_TEST(Math_Vector_SquareSumOfComponents)
{
    math::Vector3 x = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL(math::sq_sum(x), 14);
}


ARCHON_TEST(Math_Vector_DotProduct)
{
    math::Vector2 a = { 1, 2 };
    math::Vector2F b = { 3, 5 };
    ARCHON_CHECK_EQUAL(math::dot(a, b), 13);
}


ARCHON_TEST(Math_Vector_Projection)
{
    math::Vector2 a = { 3.5, 0.5 };
    math::Vector2F b = { 4, -3 };
    ARCHON_CHECK_EQUAL(math::proj(a, b), math::Vector2(2, -1.5));
}


ARCHON_TEST(Math_Vector_PerpendicularVector)
{
    math::Vector2 a = { 1, 2 };
    ARCHON_CHECK_EQUAL(math::perp(a), math::Vector2(-2, 1));
}


ARCHON_TEST(Math_Vector_CrossProduct)
{
    math::Vector3 a = { 1, 2, 3 };
    math::Vector3 b = { 2, 4, 5 };
    ARCHON_CHECK_EQUAL(a * b, math::Vector3(-2, 1, 0));
}
