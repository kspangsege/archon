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


#include <limits>

#include <archon/check.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/color_space.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>


using namespace archon;


ARCHON_TEST(Util_ColorSpace_HSL)
{
    auto test_1 = [&, &parent_test_context = test_context](util::Color c) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("test_1(%s)", util::as_css_color(c)));
        double rgb[3] = {
            util::unit_frac::int_to_flt<double>(c.red(),   255),
            util::unit_frac::int_to_flt<double>(c.green(), 255),
            util::unit_frac::int_to_flt<double>(c.blue(),  255),
        };
        double hsl[3];
        util::cvt_sRGB_to_HSL(rgb, hsl);
        double rgb_2[3];
        util::cvt_HSL_to_sRGB(hsl, rgb_2);
        using comp_type = util::Color::comp_type;
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[0]), c.red());
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[1]), c.green());
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[2]), c.blue());
    };

    std::size_t n = util::CssColor::get_num_named_colors();
    for (std::size_t i = 0; i <n; ++i) {
        util::CssColor::Hex hex = util::CssColor::get_named_color({ i });
        test_1({ hex.r, hex.g, hex.b });
    }

    auto test_2 = [&, &parent_test_context = test_context](util::Color c, double h, double s, double l) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test_2(%s, %s, %s, %s)", util::as_css_color(c), h, s, l));
        double rgb[3] = {
            util::unit_frac::int_to_flt<double>(c.red(),   255),
            util::unit_frac::int_to_flt<double>(c.green(), 255),
            util::unit_frac::int_to_flt<double>(c.blue(),  255),
        };
        double hsl[3];
        util::cvt_sRGB_to_HSL(rgb, hsl);
        auto comp = [](double x, double y) {
            return std::abs(x - y) < 0.00003;
        };
        ARCHON_CHECK_COMPARE(hsl[0], h, comp);
        ARCHON_CHECK_COMPARE(hsl[1], s, comp);
        ARCHON_CHECK_COMPARE(hsl[2], l, comp);
    };

    test_2(util::colors::chocolate,    0.06944, 0.75000, 0.47059); // 0x00D2691E
    test_2(util::colors::dodgerblue,   0.58222, 1.00000, 0.55882); // 0x001E90FF
    test_2(util::colors::midnightblue, 0.66667, 0.63504, 0.26863); // 0x00191970
    test_2(util::colors::mistyrose,    0.01667, 1.00002, 0.94118); // 0x00FFE4E1
    test_2(util::colors::olivedrab,    0.22118, 0.60452, 0.34706); // 0x006B8E23
    test_2(util::colors::papayawhip,   0.10317, 1.00003, 0.91765); // 0x00FFEFD5
    test_2(util::colors::royalblue,    0.62500, 0.72727, 0.56863); // 0x004169E1
    test_2(util::colors::saddlebrown,  0.06944, 0.75950, 0.30980); // 0x008B4513
    test_2(util::colors::seagreen,     0.40681, 0.50271, 0.36275); // 0x002E8B57
    test_2(util::colors::steelblue,    0.57576, 0.44000, 0.49020); // 0x004682B4
    test_2(util::colors::tan,          0.09524, 0.43750, 0.68627); // 0x00D2B48C
    test_2(util::colors::thistle,      0.83333, 0.24272, 0.79804); // 0x00D8BFD8
}


ARCHON_TEST(Util_ColorSpace_HSV)
{
    auto test_1 = [&, &parent_test_context = test_context](util::Color c) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("test_1(%s)", util::as_css_color(c)));
        double rgb[3] = {
            util::unit_frac::int_to_flt<double>(c.red(),   255),
            util::unit_frac::int_to_flt<double>(c.green(), 255),
            util::unit_frac::int_to_flt<double>(c.blue(),  255),
        };
        double hsv[3];
        util::cvt_sRGB_to_HSV(rgb, hsv);
        double rgb_2[3];
        util::cvt_HSV_to_sRGB(hsv, rgb_2);
        using comp_type = util::Color::comp_type;
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[0]), c.red());
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[1]), c.green());
        ARCHON_CHECK_EQUAL(util::unit_frac::flt_to_int<comp_type>(rgb_2[2]), c.blue());
    };

    std::size_t n = util::CssColor::get_num_named_colors();
    for (std::size_t i = 0; i <n; ++i) {
        util::CssColor::Hex hex = util::CssColor::get_named_color({ i });
        test_1({ hex.r, hex.g, hex.b });
    }

    auto test_2 = [&, &parent_test_context = test_context](util::Color c, double h, double s,
                                                           double v) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted("test_2(%s, %s, %s, %s)", util::as_css_color(c), h, s, v));
        double rgb[3] = {
            util::unit_frac::int_to_flt<double>(c.red(),   255),
            util::unit_frac::int_to_flt<double>(c.green(), 255),
            util::unit_frac::int_to_flt<double>(c.blue(),  255),
        };
        double hsv[3];
        util::cvt_sRGB_to_HSV(rgb, hsv);
        auto comp = [](double x, double y) {
            return std::abs(x - y) < 0.00003;
        };
        ARCHON_CHECK_COMPARE(hsv[0], h, comp);
        ARCHON_CHECK_COMPARE(hsv[1], s, comp);
        ARCHON_CHECK_COMPARE(hsv[2], v, comp);
    };

    test_2(util::colors::chocolate,    0.06944, 0.85714, 0.82353); // 0x00D2691E
    test_2(util::colors::dodgerblue,   0.58222, 0.88235, 1.00000); // 0x001E90FF
    test_2(util::colors::midnightblue, 0.66667, 0.77679, 0.43922); // 0x00191970
    test_2(util::colors::mistyrose,    0.01667, 0.11765, 1.00000); // 0x00FFE4E1
    test_2(util::colors::olivedrab,    0.22118, 0.75352, 0.55686); // 0x006B8E23
    test_2(util::colors::papayawhip,   0.10317, 0.16471, 1.00000); // 0x00FFEFD5
    test_2(util::colors::royalblue,    0.62500, 0.71111, 0.88235); // 0x004169E1
    test_2(util::colors::saddlebrown,  0.06944, 0.86331, 0.54510); // 0x008B4513
    test_2(util::colors::seagreen,     0.40681, 0.66907, 0.54510); // 0x002E8B57
    test_2(util::colors::steelblue,    0.57576, 0.61111, 0.70588); // 0x004682B4
    test_2(util::colors::tan,          0.09524, 0.33333, 0.82353); // 0x00D2B48C
    test_2(util::colors::thistle,      0.83333, 0.11574, 0.84706); // 0x00D8BFD8
}


namespace {


auto color_compare(long double eps)
{
    return [eps](const auto& x, const auto& y)
    {
        using x_type = std::decay_t<decltype(x)>;
        using y_type = std::decay_t<decltype(y)>;
        static_assert(x_type::size == y_type::size);
        for (std::size_t i = 0; i < x.size; ++i) {
            if (ARCHON_LIKELY(std::abs(x[i] - y[i]) < eps))
                continue;
            return false;
        }
        return true;
    };
}


} // unnamed namespace


ARCHON_TEST(Util_ColorSpace_XYZ)
{
    math::Vec3F rgb = { 0.3f, 0.5f, 0.7f };
    math::Vec3F xyz;
    util::cvt_sRGB_to_XYZ(rgb.components().data(), xyz.components().data());
    ARCHON_CHECK_COMPARE(xyz, math::Vec3F(0.1876f, 0.2010f, 0.4527f), color_compare(0.0001));
    util::cvt_XYZ_to_sRGB(xyz.components().data(), rgb.components().data());
    float eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(rgb, math::Vec3F(0.3f, 0.5f, 0.7f), color_compare(100 * eps));
}
