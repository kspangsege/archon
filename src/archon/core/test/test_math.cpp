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


#include <limits>

#include <archon/core/math.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_Math_DegreesToRadians)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(0 * 45), 0 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(1 * 45), 1 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(2 * 45), 2 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(3 * 45), 3 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(4 * 45), 4 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(5 * 45), 5 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(6 * 45), 6 * core::pi<double> / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::deg_to_rad(7 * 45), 7 * core::pi<double> / 4, 10 * eps);
}


ARCHON_TEST(Core_Math_RadiansToDegrees)
{
    auto test = [](check::TestContext& parent_test_context, double deg) {
        ARCHON_TEST_TRAIL(parent_test_context, deg);
        auto rad = core::deg_to_rad(deg);
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_APPROXIMATELY_EQUAL(core::rad_to_deg(rad), deg, 10 * eps);
    };
    test(test_context, 0 * 45);
    test(test_context, 1 * 45);
    test(test_context, 2 * 45);
    test(test_context, 3 * 45);
    test(test_context, 4 * 45);
    test(test_context, 5 * 45);
    test(test_context, 6 * 45);
    test(test_context, 7 * 45);
}


ARCHON_TEST(Core_Math_Lerp)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, -1 * 1.0 / 4), -1 + -1 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5,  0 * 1.0 / 4), -1 +  0 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, +1 * 1.0 / 4), -1 + +1 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, +2 * 1.0 / 4), -1 + +2 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, +3 * 1.0 / 4), -1 + +3 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, +4 * 1.0 / 4), -1 + +4 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp(-1, 5, +5 * 1.0 / 4), -1 + +5 * 6.0 / 4, 10 * eps);
}


ARCHON_TEST(Core_Math_LerpA)
{
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + -1), -1 + -1 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 +  0), -1 +  0 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + +1), -1 + +1 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + +2), -1 + +2 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + +3), -1 + +3 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + +4), -1 + +4 * 6.0 / 4, 10 * eps);
    ARCHON_CHECK_APPROXIMATELY_EQUAL(core::lerp_a(-3, -1, 1, 5, -3 + +5), -1 + +5 * 6.0 / 4, 10 * eps);
}
