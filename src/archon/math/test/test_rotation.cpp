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
#include <archon/math/rotation.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;
namespace test = math::test;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST(Math_Rotation_Comparison)
{
    auto rot_1 = math::Rotation({ 0, 0, 1 }, 2);
    auto rot_2 = math::Rotation({ 0, 0, 1 }, 2);
    auto rot_3 = math::Rotation({ 0, 0, 1 }, 1);
    auto rot_4 = math::Rotation({ 0, 1, 0 }, 1);

    ARCHON_CHECK(rot_1 == rot_2);
    ARCHON_CHECK_NOT(rot_1 == rot_3);
    ARCHON_CHECK_NOT(rot_1 == rot_4);

    ARCHON_CHECK_NOT(rot_1 != rot_2);
    ARCHON_CHECK(rot_1 != rot_3);
    ARCHON_CHECK(rot_1 != rot_4);

    ARCHON_CHECK_NOT(rot_1 < rot_2);
    ARCHON_CHECK_NOT(rot_1 < rot_3);
    ARCHON_CHECK(rot_1 < rot_4);

    ARCHON_CHECK(rot_1 <= rot_2);
    ARCHON_CHECK_NOT(rot_1 <= rot_3);
    ARCHON_CHECK(rot_1 <= rot_4);

    ARCHON_CHECK_NOT(rot_1 > rot_2);
    ARCHON_CHECK(rot_1 > rot_3);
    ARCHON_CHECK_NOT(rot_1 > rot_4);

    ARCHON_CHECK(rot_1 >= rot_2);
    ARCHON_CHECK(rot_1 >= rot_3);
    ARCHON_CHECK_NOT(rot_1 >= rot_4);
}


ARCHON_TEST(Math_Rotation_FromVersor)
{
    auto test = [](check::TestContext& parent_test_context, const math::Quaternion& ver) {
        ARCHON_TEST_TRAIL(parent_test_context, ver);
        math::Rotation rot;
        ver.to_axis_angle(rot.axis, rot.angle);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(math::Rotation::from_versor(ver), rot, test::rotation_compare(10 * eps));
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


ARCHON_TEST(Math_Rotation_ToVersor)
{
    auto test = [](check::TestContext& parent_test_context, const math::Rotation& rot) {
        ARCHON_TEST_TRAIL(parent_test_context, rot);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(rot.to_versor(), math::Quaternion::from_axis_angle(rot.axis, rot.angle),
                             test::quaternion_compare(10 * eps));
    };
    test(test_context, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(0)));
    test(test_context, math::Rotation({ 0, 1, 0 }, core::deg_to_rad(0)));
    test(test_context, math::Rotation({ 0, 0, 1 }, core::deg_to_rad(0)));
    test(test_context, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(60)));
    test(test_context, math::Rotation({ 0, 1, 0 }, core::deg_to_rad(60)));
    test(test_context, math::Rotation({ 0, 0, 1 }, core::deg_to_rad(60)));
    test(test_context, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(90)));
    test(test_context, math::Rotation({ 0, 1, 0 }, core::deg_to_rad(90)));
    test(test_context, math::Rotation({ 0, 0, 1 }, core::deg_to_rad(90)));
    test(test_context, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(180)));
    test(test_context, math::Rotation({ 0, 1, 0 }, core::deg_to_rad(180)));
    test(test_context, math::Rotation({ 0, 0, 1 }, core::deg_to_rad(180)));
    test(test_context, math::Rotation({ 1 / std::sqrt(3), 1 / std::sqrt(3), 1 / std::sqrt(3) },
                                      core::deg_to_rad(120)));

}


ARCHON_TEST_BATCH(Math_Rotation_Format, char_variants)
{
    using char_type = test_type;
    std::array<char_type, 32> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 32> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(math::Rotation({ 1, 0, 0 }, 1.5)), widener.widen("[1, 0, 0; 1.5]"));
}


ARCHON_TEST(Math_Rotation_AdditionSubtractionNegation)
{
    auto a = math::Rotation({ 1, 0, 0 }, core::deg_to_rad(90));
    auto b = math::Rotation({ 0, 1, 0 }, core::deg_to_rad(90));
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(a + b, math::Rotation({ 1 / std::sqrt(3), 1 / std::sqrt(3), -1 / std::sqrt(3) },
                                               core::deg_to_rad(120)), test::rotation_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a - b, math::Rotation({ 1 / std::sqrt(3), -1 / std::sqrt(3), 1 / std::sqrt(3) },
                                               core::deg_to_rad(120)), test::rotation_compare(10 * eps));
    ARCHON_CHECK_COMPARE(-a, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(-90)), test::rotation_compare(10 * eps));
    a += b;
    ARCHON_CHECK_COMPARE(a, math::Rotation({ 1 / std::sqrt(3), 1 / std::sqrt(3), -1 / std::sqrt(3) },
                                           core::deg_to_rad(120)), test::rotation_compare(10 * eps));
    a -= b;
    ARCHON_CHECK_COMPARE(a, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(90)), test::rotation_compare(10 * eps));
}


ARCHON_TEST(Math_Rotation_MultiplicationDivision)
{
    auto a = math::Rotation({ 1, 0, 0 }, core::deg_to_rad(60));
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(2 * a, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(120)), test::rotation_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a * 2, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(120)), test::rotation_compare(10 * eps));
    ARCHON_CHECK_COMPARE(a / 2, math::Rotation({ 1, 0, 0 }, core::deg_to_rad(30)), test::rotation_compare(10 * eps));
}
