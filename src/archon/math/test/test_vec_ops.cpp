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

#include <archon/check.hpp>
#include <archon/math/vec.hpp>


using namespace archon;


ARCHON_TEST(Math_VecOps_AdditionSubtractionNegation)
{
    math::Vec2 a = { 1, 2 };
    math::Vec2F b = { 3, 5 };
    ARCHON_CHECK_EQUAL(a + b, math::Vec2(4, 7));
    ARCHON_CHECK_EQUAL(a - b, math::Vec2(-2, -3));
    ARCHON_CHECK_EQUAL(-a, math::Vec2(-1, -2));
    a += b;
    ARCHON_CHECK_EQUAL(a, math::Vec2(4, 7));
    a -= b;
    ARCHON_CHECK_EQUAL(a, math::Vec2(1, 2));
    static_assert(std::is_same_v<decltype(a + b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a - b)::comp_type, double>);
}


ARCHON_TEST(Math_VecOps_Scaling)
{
    math::Vec2F a = { 1, 2 };
    ARCHON_CHECK_EQUAL(a * 2, math::Vec2(2, 4));
    ARCHON_CHECK_EQUAL(3 * a, math::Vec2(3, 6));
    ARCHON_CHECK_EQUAL(a / 2, math::Vec2(0.5, 1.0));
    a *= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Vec2(2, 4));
    a /= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Vec2(1, 2));
    static_assert(std::is_same_v<decltype(2.0 * a)::comp_type, double>);
    static_assert(std::is_same_v<decltype(2.0F * a)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a * 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * 2.0F)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a / 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a / 2.0F)::comp_type, float>);
}


ARCHON_TEST(Math_VecOps_Length)
{
    math::Vec2 x = { 1, 2 };
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_APPROXIMATELY_EQUAL(len(x), std::sqrt(5), 10 * eps);
}


ARCHON_TEST(Math_VecOps_SumOfComponents)
{
    math::Vec3 x = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL(sum(x), 6);
}


ARCHON_TEST(Math_VecOps_SquareSumOfComponents)
{
    math::Vec3 x = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL(sq_sum(x), 14);
}


ARCHON_TEST(Math_VecOps_DotProduct)
{
    math::Vec2 a = { 1, 2 };
    math::Vec2F b = { 3, 5 };
    ARCHON_CHECK_EQUAL(dot(a, b), 13);
}


ARCHON_TEST(Math_VecOps_Projection)
{
    math::Vec2 a = { 3.5, 0.5 };
    math::Vec2F b = { 4, -3 };
    ARCHON_CHECK_EQUAL(proj(a, b), math::Vec2(2, -1.5));
}


ARCHON_TEST(Math_VecOps_PerpendicularVector)
{
    math::Vec2 a = { 1, 2 };
    ARCHON_CHECK_EQUAL(perp(a), math::Vec2(-2, 1));
}


ARCHON_TEST(Math_VecOps_CrossProduct)
{
    math::Vec3 a = { 1, 2, 3 };
    math::Vec3 b = { 2, 4, 5 };
    ARCHON_CHECK_EQUAL(a * b, math::Vec3(-2, 1, 0));
}
