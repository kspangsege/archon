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


#include <archon/core/integer.hpp>
#include <archon/check.hpp>
#include <archon/util/css_color.hpp>


using namespace archon;


ARCHON_TEST(Util_CssColor_DefaultConstruct)
{
    util::CssColor color;
    util::CssColor::Hex hex;
    if (ARCHON_LIKELY(ARCHON_CHECK(color.get_if(hex)))) {
        ARCHON_CHECK_EQUAL(core::promote(hex.r), 0);
        ARCHON_CHECK_EQUAL(core::promote(hex.g), 0);
        ARCHON_CHECK_EQUAL(core::promote(hex.b), 0);
        ARCHON_CHECK_EQUAL(core::promote(hex.a), 0);
    }
}


ARCHON_TEST(Util_CssColor_Format)
{
    core::Buffer<char> buffer;
    auto format = [&](util::CssColor color) {
        return color.format(buffer);
    };
    using util::CssColor;
    ARCHON_CHECK_EQUAL(format(CssColor::hex(0x44, 0x88, 0xCC, 0xFF)), "#48C");
    ARCHON_CHECK_EQUAL(format(CssColor::hex(0x44, 0x88, 0xCC, 0xEE)), "#48CE");
    ARCHON_CHECK_EQUAL(format(CssColor::hex(0xD2, 0x69, 0x1E, 0xFF)), "#D2691E");
    ARCHON_CHECK_EQUAL(format(CssColor::hex(0xD2, 0x69, 0x1E, 0xFE)), "#D2691EFE");

    ARCHON_CHECK_EQUAL(format(CssColor::name(0)),  "transparent");
    ARCHON_CHECK_EQUAL(format(CssColor::name(16)), "chocolate");

    ARCHON_CHECK_EQUAL(format(CssColor::rgb(210, 105, 30, 1.0)), "rgb(210, 105, 30)");
    ARCHON_CHECK_EQUAL(format(CssColor::rgb(210, 105, 30, 0.5)), "rgba(210, 105, 30, 0.5)");

    ARCHON_CHECK_EQUAL(format(CssColor::rgb_p(82, 41, 12, 1.0)), "rgb(82%, 41%, 12%)");
    ARCHON_CHECK_EQUAL(format(CssColor::rgb_p(82, 41, 12, 0.5)), "rgba(82%, 41%, 12%, 0.5)");

    ARCHON_CHECK_EQUAL(format(CssColor::hsl(25, 75, 47.1f, 1.0)), "hsl(25, 75%, 47.1%)");
    ARCHON_CHECK_EQUAL(format(CssColor::hsl(25, 75, 47.1f, 0.5)), "hsla(25, 75%, 47.1%, 0.5)");
}
