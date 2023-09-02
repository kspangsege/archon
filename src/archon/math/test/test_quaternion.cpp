// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <archon/core/char_mapper.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/check.hpp>
#include <archon/math/quaternion.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;
namespace test = math::test;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST(Math_Quaternion_Basics)
{
    math::Quaternion a = { 1, 2, 3, 4 };
    math::Quaternion b = { 2, 6, 4, 8 };
    ARCHON_CHECK_EQUAL(a.w, 1);
    ARCHON_CHECK_EQUAL(a.v[0], 2);
    ARCHON_CHECK_EQUAL(a.v[1], 3);
    ARCHON_CHECK_EQUAL(a.v[2], 4);
    a = b;
    ARCHON_CHECK_EQUAL(a.w, 2);
    ARCHON_CHECK_EQUAL(a.v[0], 6);
    ARCHON_CHECK_EQUAL(a.v[1], 4);
    ARCHON_CHECK_EQUAL(a.v[2], 8);
    a = math::Quaternion(3, { 7, 9, 5 });
    ARCHON_CHECK_EQUAL(a.w, 3);
    ARCHON_CHECK_EQUAL(a.v[0], 7);
    ARCHON_CHECK_EQUAL(a.v[1], 9);
    ARCHON_CHECK_EQUAL(a.v[2], 5);
}


ARCHON_TEST(Math_Quaternion_Comparison)
{
    math::Quaternion quat_1 = { 1, 3, 1, 3 };
    math::Quaternion quat_2 = { 1, 3, 1, 3 };
    math::Quaternion quat_3 = { 1, 2, 1, 2 };
    math::Quaternion quat_4 = { 2, 1, 2, 1 };

    ARCHON_CHECK(quat_1 == quat_2);
    ARCHON_CHECK_NOT(quat_1 == quat_3);
    ARCHON_CHECK_NOT(quat_1 == quat_4);

    ARCHON_CHECK_NOT(quat_1 != quat_2);
    ARCHON_CHECK(quat_1 != quat_3);
    ARCHON_CHECK(quat_1 != quat_4);

    ARCHON_CHECK_NOT(quat_1 < quat_2);
    ARCHON_CHECK_NOT(quat_1 < quat_3);
    ARCHON_CHECK(quat_1 < quat_4);

    ARCHON_CHECK(quat_1 <= quat_2);
    ARCHON_CHECK_NOT(quat_1 <= quat_3);
    ARCHON_CHECK(quat_1 <= quat_4);

    ARCHON_CHECK_NOT(quat_1 > quat_2);
    ARCHON_CHECK(quat_1 > quat_3);
    ARCHON_CHECK_NOT(quat_1 > quat_4);

    ARCHON_CHECK(quat_1 >= quat_2);
    ARCHON_CHECK(quat_1 >= quat_3);
    ARCHON_CHECK_NOT(quat_1 >= quat_4);
}


ARCHON_TEST(Math_Quaternion_FromAxisAngle)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 1, 0, 0 }, core::deg_to_rad(0)),
                         math::Quaternion(1, 0, 0, 0), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 1, 0 }, core::deg_to_rad(0)),
                         math::Quaternion(1, 0, 0, 0), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 0, 1 }, core::deg_to_rad(0)),
                         math::Quaternion(1, 0, 0, 0), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 1, 0, 0 }, core::deg_to_rad(60)),
                         math::Quaternion(std::sqrt(3), 1, 0, 0) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 1, 0 }, core::deg_to_rad(60)),
                         math::Quaternion(std::sqrt(3), 0, 1, 0) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 0, 1 }, core::deg_to_rad(60)),
                         math::Quaternion(std::sqrt(3), 0, 0, 1) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 1, 0, 0 }, core::deg_to_rad(90)),
                         math::Quaternion(std::sqrt(2), std::sqrt(2), 0, 0) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 1, 0 }, core::deg_to_rad(90)),
                         math::Quaternion(std::sqrt(2), 0, std::sqrt(2), 0) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 0, 1 }, core::deg_to_rad(90)),
                         math::Quaternion(std::sqrt(2), 0, 0, std::sqrt(2)) / 2, test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 1, 0, 0 }, core::deg_to_rad(180)),
                         math::Quaternion(0, 1, 0, 0), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 1, 0 }, core::deg_to_rad(180)),
                         math::Quaternion(0, 0, 1, 0), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 0, 0, 1 }, core::deg_to_rad(180)),
                         math::Quaternion(0, 0, 0, 1), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::Quaternion::from_axis_angle({ 1 / std::sqrt(3), 1 / std::sqrt(3), 1 / std::sqrt(3) },
                                                           core::deg_to_rad(120)),
                         math::Quaternion(1, 1, 1, 1) / 2, test::quaternion_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_ToAxisAngle)
{
    auto test = [](check::TestContext& parent_test_context, const math::Quaternion& quat) {
        ARCHON_TEST_TRAIL(parent_test_context, quat);
        math::Quaternion::vector_type axis;
        math::Quaternion::comp_type angle;
        quat.to_axis_angle(axis, angle);
        auto quat_2 = math::Quaternion::from_axis_angle(axis, angle);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(quat_2, quat, test::quaternion_compare(10 * eps));
    };
    test(test_context, math::Quaternion(1, 0, 0, 0));
    test(test_context, math::Quaternion(std::sqrt(3), 1, 0, 0) / 2);
    test(test_context, math::Quaternion(std::sqrt(3), 0, 1, 0) / 2);
    test(test_context, math::Quaternion(std::sqrt(3), 0, 0, 1) / 2);
    test(test_context, math::Quaternion(std::sqrt(2), std::sqrt(2), 0, 0) / 2);
    test(test_context, math::Quaternion(std::sqrt(2), 0, std::sqrt(2), 0) / 2);
    test(test_context, math::Quaternion(std::sqrt(2), 0, 0, std::sqrt(2)) / 2);
    test(test_context, math::Quaternion(0, 1, 0, 0));
    test(test_context, math::Quaternion(0, 0, 1, 0));
    test(test_context, math::Quaternion(0, 0, 0, 1));
    test(test_context, math::Quaternion(1, 1, 1, 1) / 2);
}


ARCHON_TEST(Math_Quaternion_FromProperEulerAngles)
{
    using comp_type = math::Quaternion::comp_type;
    auto test = [](check::TestContext& parent_test_context, comp_type alpha, comp_type beta, comp_type gamma) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s, %s", alpha, beta, gamma));
        math::Quaternion quat = 1;
        quat = math::Quaternion::from_axis_angle(math::conj({ 0, 0, 1 }, quat), alpha) * quat;
        quat = math::Quaternion::from_axis_angle(math::conj({ 1, 0, 0 }, quat), beta)  * quat;
        quat = math::Quaternion::from_axis_angle(math::conj({ 0, 0, 1 }, quat), gamma) * quat;
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(math::Quaternion::from_proper_euler_angles(alpha, beta, gamma), quat,
                             test::quaternion_compare(10 * eps));
    };
    test(test_context, core::deg_to_rad(0), core::deg_to_rad(0), core::deg_to_rad(0));
    test(test_context, core::deg_to_rad(45), core::deg_to_rad(0), core::deg_to_rad(0));
    test(test_context, core::deg_to_rad(0), core::deg_to_rad(45), core::deg_to_rad(0));
    test(test_context, core::deg_to_rad(0), core::deg_to_rad(0), core::deg_to_rad(45));
    test(test_context, core::deg_to_rad(-45), core::deg_to_rad(0), core::deg_to_rad(0));
    test(test_context, core::deg_to_rad(0), core::deg_to_rad(-45), core::deg_to_rad(0));
    test(test_context, core::deg_to_rad(0), core::deg_to_rad(0), core::deg_to_rad(-45));
    test(test_context, core::deg_to_rad(10), core::deg_to_rad(20), core::deg_to_rad(30));
    test(test_context, core::deg_to_rad(20), core::deg_to_rad(30), core::deg_to_rad(40));
    test(test_context, core::deg_to_rad(30), core::deg_to_rad(40), core::deg_to_rad(50));
    test(test_context, core::deg_to_rad(40), core::deg_to_rad(50), core::deg_to_rad(60));
    test(test_context, core::deg_to_rad(50), core::deg_to_rad(60), core::deg_to_rad(70));
    test(test_context, core::deg_to_rad(60), core::deg_to_rad(70), core::deg_to_rad(80));
    test(test_context, core::deg_to_rad(70), core::deg_to_rad(80), core::deg_to_rad(90));
    test(test_context, core::deg_to_rad(80), core::deg_to_rad(90), core::deg_to_rad(100));
    test(test_context, core::deg_to_rad(90), core::deg_to_rad(100), core::deg_to_rad(110));
}


ARCHON_TEST(Math_Quaternion_ToRotationMatrix)
{
    auto quat = math::Quaternion::from_axis_angle({ 0, 0, 1 }, core::deg_to_rad(90));
    math::Matrix3 rot = quat.to_rotation_matrix();
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(rot, math::Matrix3({ 0, -1, 0 },
                                            { 1,  0, 0 },
                                            { 0,  0, 1 }),
                         test::matrix_compare(10 * eps));

    rot = (2 * quat).to_rotation_matrix();
    ARCHON_CHECK_COMPARE(rot, math::Matrix3({ 0, -1, 0 },
                                            { 1,  0, 0 },
                                            { 0,  0, 1 }),
                         test::matrix_compare(10 * eps));

    using vector_type = math::Quaternion::vector_type;
    using matrix_type = math::Quaternion::matrix_type;
    auto test_1 = [](check::TestContext& parent_test_context, const math::Quaternion& quat, const matrix_type& mat,
                     const vector_type& vec) {
        ARCHON_TEST_TRAIL(parent_test_context, vec);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(mat * vec, math::conj(vec, quat), test::vector_compare(10 * eps));
    };
    auto test_2 = [&](check::TestContext& parent_test_context, const math::Quaternion& quat) {
        ARCHON_TEST_TRAIL(parent_test_context, quat);
        matrix_type mat = quat.to_rotation_matrix();
        test_1(test_context, quat, mat, { 0, 0, 0 });
        test_1(test_context, quat, mat, { 1, 0, 0 });
        test_1(test_context, quat, mat, { 0, 1, 0 });
        test_1(test_context, quat, mat, { 0, 0, 1 });
        test_1(test_context, quat, mat, { 1, 1, 1 });
    };
    test_2(test_context, math::Quaternion(1, 0, 0, 0));
    test_2(test_context, math::Quaternion(std::sqrt(3), 1, 0, 0) / 2);
    test_2(test_context, math::Quaternion(std::sqrt(3), 0, 1, 0) / 2);
    test_2(test_context, math::Quaternion(std::sqrt(3), 0, 0, 1) / 2);
    test_2(test_context, math::Quaternion(std::sqrt(2), std::sqrt(2), 0, 0) / 2);
    test_2(test_context, math::Quaternion(std::sqrt(2), 0, std::sqrt(2), 0) / 2);
    test_2(test_context, math::Quaternion(std::sqrt(2), 0, 0, std::sqrt(2)) / 2);
    test_2(test_context, math::Quaternion(0, 1, 0, 0));
    test_2(test_context, math::Quaternion(0, 0, 1, 0));
    test_2(test_context, math::Quaternion(0, 0, 0, 1));
    test_2(test_context, math::Quaternion(1, 1, 1, 1) / 2);
}


ARCHON_TEST_BATCH(Math_Quaternion_Format, char_variants)
{
    using char_type = test_type;
    std::array<char_type, 32> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 32> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(math::Quaternion(1.5, 2.5, 3.5, 4.5)), widener.widen("[1.5; 2.5, 3.5, 4.5]"));
}


ARCHON_TEST(Math_Quaternion_AdditionSubtractionNegation)
{
    math::Quaternion a = { 1, 2, 3, 4 };
    math::Quaternion b = { 2, 6, 4, 8 };
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(4 + a, math::Quaternion(5, 2, 3, 4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a + 4, math::Quaternion(5, 2, 3, 4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a + b, math::Quaternion(3, 8, 7, 12), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(4 - a, math::Quaternion(3, -2, -3, -4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a - 4, math::Quaternion(-3, 2, 3, 4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a - b, math::Quaternion(-1, -4, -1, -4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(-a, math::Quaternion(-1, -2, -3, -4), test::quaternion_compare(10 * eps));
    a += 4;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(5, 2, 3, 4), test::quaternion_compare(10 * eps));
    a -= 4;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(1, 2, 3, 4), test::quaternion_compare(10 * eps));
    a += b;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(3, 8, 7, 12), test::quaternion_compare(10 * eps));
    a -= b;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(1, 2, 3, 4), test::quaternion_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_MultiplicationDivision)
{
    math::Quaternion a = { 1, 2, 3, 4 };
    math::Quaternion b = { 2, 6, 4, 8 };
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(2 * a, math::Quaternion(2, 4, 6, 8), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a * 2, math::Quaternion(2, 4, 6, 8), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a * b, math::Quaternion(-54, 18, 18, 6), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(b * a, math::Quaternion(-54, 2, 2, 26), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(30 / a, math::Quaternion(1, -2, -3, -4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(b / 2, math::Quaternion(1, 3, 2, 4), test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a / b, math::Quaternion(29, -5, -3, 5) / 60, test::quaternion_compare(10 * eps));
    a *= 2;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(2, 4, 6, 8), test::quaternion_compare(10 * eps));
    a /= 2;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(1, 2, 3, 4), test::quaternion_compare(10 * eps));
    a *= b;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(-54, 18, 18, 6), test::quaternion_compare(10 * eps));
    a /= b;
    ARCHON_CHECK_COMPARE(a, math::Quaternion(1, 2, 3, 4), test::quaternion_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_ConjugateOfQuaternion)
{
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 0, 0, 0)), math::Quaternion(0, 0, 0, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(1, 0, 0, 0)), math::Quaternion(1, 0, 0, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 1, 0, 0)), math::Quaternion(0, -1, 0, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 0, 1, 0)), math::Quaternion(0, 0, -1, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 0, 0, 1)), math::Quaternion(0, 0, 0, -1));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(2, 0, 0, 0)), math::Quaternion(2, 0, 0, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 2, 0, 0)), math::Quaternion(0, -2, 0, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 0, 2, 0)), math::Quaternion(0, 0, -2, 0));
    ARCHON_CHECK_EQUAL(math::conj(math::Quaternion(0, 0, 0, 2)), math::Quaternion(0, 0, 0, -2));
}


ARCHON_TEST(Math_Quaternion_ConjugateQuaternionByQuaternion)
{
    auto test_1 = [](check::TestContext& parent_test_context, const math::Quaternion& a, const math::Quaternion& b) {
        ARCHON_TEST_TRAIL(parent_test_context, b);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(math::conj(a, b), b * a * math::conj(b), test::quaternion_compare(100 * eps));
    };
    auto test_2 = [&](check::TestContext& parent_test_context, const math::Quaternion& a) {
        ARCHON_TEST_TRAIL(parent_test_context, a);
        test_1(test_context, a, { 0, 0, 0, 0 });
        test_1(test_context, a, { 1, 0, 0, 0 });
        test_1(test_context, a, { 0, 1, 0, 0 });
        test_1(test_context, a, { 0, 0, 1, 0 });
        test_1(test_context, a, { 0, 0, 0, 1 });
        test_1(test_context, a, { 1, 1, 1, 1 });
        test_1(test_context, a, { 1, 2, 3, 4 });
        test_1(test_context, a, { 2, 3, 4, 1 });
        test_1(test_context, a, { std::sqrt(3), 1, 0, 0 });
        test_1(test_context, a, { std::sqrt(3), 0, 1, 0 });
        test_1(test_context, a, { std::sqrt(3), 0, 0, 1 });
        test_1(test_context, a, { std::sqrt(2), std::sqrt(2), 0, 0 });
        test_1(test_context, a, { std::sqrt(2), 0, std::sqrt(2), 0 });
        test_1(test_context, a, { std::sqrt(2), 0, 0, std::sqrt(2) });
    };
    test_2(test_context, { 0, 0, 0, 0 });
    test_2(test_context, { 1, 0, 0, 0 });
    test_2(test_context, { 0, 1, 0, 0 });
    test_2(test_context, { 0, 0, 1, 0 });
    test_2(test_context, { 0, 0, 0, 1 });
    test_2(test_context, { 1, 1, 1, 1 });
    test_2(test_context, { 1, 2, 3, 4 });
    test_2(test_context, { 2, 3, 4, 1 });
    test_2(test_context, { std::sqrt(3), 1, 0, 0 });
    test_2(test_context, { std::sqrt(3), 0, 1, 0 });
    test_2(test_context, { std::sqrt(3), 0, 0, 1 });
    test_2(test_context, { std::sqrt(2), std::sqrt(2), 0, 0 });
    test_2(test_context, { std::sqrt(2), 0, std::sqrt(2), 0 });
    test_2(test_context, { std::sqrt(2), 0, 0, std::sqrt(2) });
}


ARCHON_TEST(Math_Quaternion_ConjugateVectorByQuaternion)
{
    using vector_type = math::Quaternion::vector_type;
    auto test_1 = [](check::TestContext& parent_test_context, const vector_type& vec, const math::Quaternion& quat) {
        ARCHON_TEST_TRAIL(parent_test_context, quat);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(math::conj(vec, quat), math::conj(math::Quaternion(0, vec), quat).v,
                             test::vector_compare(10 * eps));
    };
    auto test_2 = [&](check::TestContext& parent_test_context, const vector_type& vec) {
        ARCHON_TEST_TRAIL(parent_test_context, vec);
        test_1(test_context, vec, { 0, 0, 0, 0 });
        test_1(test_context, vec, { 1, 0, 0, 0 });
        test_1(test_context, vec, { 0, 1, 0, 0 });
        test_1(test_context, vec, { 0, 0, 1, 0 });
        test_1(test_context, vec, { 0, 0, 0, 1 });
        test_1(test_context, vec, { 1, 1, 1, 1 });
        test_1(test_context, vec, { 1, 2, 3, 4 });
        test_1(test_context, vec, { 2, 3, 4, 1 });
        test_1(test_context, vec, { std::sqrt(3), 1, 0, 0 });
        test_1(test_context, vec, { std::sqrt(3), 0, 1, 0 });
        test_1(test_context, vec, { std::sqrt(3), 0, 0, 1 });
        test_1(test_context, vec, { std::sqrt(2), std::sqrt(2), 0, 0 });
        test_1(test_context, vec, { std::sqrt(2), 0, std::sqrt(2), 0 });
        test_1(test_context, vec, { std::sqrt(2), 0, 0, std::sqrt(2) });
    };
    test_2(test_context, { 0, 0, 0 });
    test_2(test_context, { 1, 0, 0 });
    test_2(test_context, { 0, 1, 0 });
    test_2(test_context, { 0, 0, 1 });
    test_2(test_context, { 1, 1, 1 });
    test_2(test_context, { 1, 2, 3 });
    test_2(test_context, { 2, 3, 1 });
    test_2(test_context, { 3, 1, 2 });
}


ARCHON_TEST(Math_Quaternion_Length)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(0, 0, 0, 0)), 0, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(1, 0, 0, 0)), 1, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(2, 0, 0, 0)), 2, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(-1, 0, 0, 0)), 1, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(-2, 0, 0, 0)), 2, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(1, 1, 1, 1)), 2, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(2, 2, 2, 2)), 4, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::len(math::Quaternion(1, 2, 3, 4)), std::sqrt(30), test::scalar_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_Normalize)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(1, 0, 0, 0)), math::Quaternion(1, 0, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 1, 0, 0)), math::Quaternion(0, 1, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 0, 1, 0)), math::Quaternion(0, 0, 1, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 0, 0, 1)), math::Quaternion(0, 0, 0, 1),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(2, 0, 0, 0)), math::Quaternion(1, 0, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 2, 0, 0)), math::Quaternion(0, 1, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 0, 2, 0)), math::Quaternion(0, 0, 1, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(0, 0, 0, 2)), math::Quaternion(0, 0, 0, 1),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::normalize(math::Quaternion(1, 1, 1, 1)), math::Quaternion(1, 1, 1, 1) / 2,
                         test::quaternion_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_SquareSum)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(0, 0, 0, 0)), 0, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(1, 0, 0, 0)), 1, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(2, 0, 0, 0)), 4, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(-1, 0, 0, 0)), 1, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(-2, 0, 0, 0)), 4, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(1, 1, 1, 1)), 4, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(2, 2, 2, 2)), 16, test::scalar_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::sq_sum(math::Quaternion(1, 2, 3, 4)), 30, test::scalar_compare(10 * eps));
}


ARCHON_TEST(Math_Quaternion_Inverse)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(1, 0, 0, 0)), math::Quaternion(1, 0, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(0, 1, 0, 0)), math::Quaternion(0, -1, 0, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(0, 0, 1, 0)), math::Quaternion(0, 0, -1, 0),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(0, 0, 0, 1)), math::Quaternion(0, 0, 0, -1),
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(2 * math::Quaternion(1, 0, 0, 0)), math::Quaternion(1, 0, 0, 0) / 2,
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(2 * math::Quaternion(0, 1, 0, 0)), math::Quaternion(0, -1, 0, 0) / 2,
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(2 * math::Quaternion(0, 0, 1, 0)), math::Quaternion(0, 0, -1, 0) / 2,
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(2 * math::Quaternion(0, 0, 0, 1)), math::Quaternion(0, 0, 0, -1) / 2,
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(1, 1, 1, 1) / 2), math::Quaternion(1, -1, -1, -1) / 2,
                         test::quaternion_compare(10 * eps));
    ARCHON_CHECK_COMPARE(math::inv(math::Quaternion(1, 1, 1, 1)), math::Quaternion(1, -1, -1, -1) / 4,
                         test::quaternion_compare(10 * eps));
}
