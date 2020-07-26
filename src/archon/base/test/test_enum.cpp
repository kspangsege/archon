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


#include <array>

#include <archon/base/char_mapper.hpp>
#include <archon/base/value_formatter.hpp>
#include <archon/base/value_parser.hpp>
#include <archon/base/enum.hpp>
#include <archon/unit_test.hpp>


using namespace archon;
using namespace archon::base;


namespace {

enum class Color { orange, purple, brown };

} // unnamed namespace

namespace archon::base {

template<> struct EnumTraits<Color> {
    static constexpr bool is_specialized = true;
    struct Spec { static EnumAssoc map[]; };
    static constexpr bool ignore_case = false;
};

} // namespace archon::base

inline EnumAssoc archon::base::EnumTraits<Color>::Spec::map[] = {
    { int(Color::orange), "orange" },
    { int(Color::purple), "purple" },
    { int(Color::brown),  "brown"  }
};


ARCHON_TEST(Base_Enum_Basics)
{
    std::array<char,    256> seed_memory_1;
    std::array<wchar_t, 256> seed_memory_2;
    std::array<wchar_t, 256> seed_memory_3;
    ValueFormatter formatter(seed_memory_1, std::locale::classic());
    WideValueFormatter wformatter(seed_memory_2, std::locale::classic());
    WideStringWidener widener(std::locale::classic(), seed_memory_3);
    auto check_write = [&](Color color, std::string_view string) {
        ARCHON_CHECK_EQUAL(formatter.format(color), string);
        std::wstring_view wstring = widener.widen(string);
        ARCHON_CHECK_EQUAL(wformatter.format(color), wstring);
    };

    check_write(Color::orange, "orange");
    check_write(Color::purple, "purple");
    check_write(Color::brown,  "brown");

    ValueParser parser(std::locale::classic());
    WideValueParser wparser(std::locale::classic());
    auto check_read = [&](std::string_view string, Color color) {
        Color color_2 = Color(-1);
        if (ARCHON_CHECK(parser.parse(string, color_2)))
            ARCHON_CHECK_EQUAL(color_2, color);
        std::wstring_view wstring = widener.widen(string);
        Color color_3 = Color(-1);
        if (ARCHON_CHECK(wparser.parse(wstring, color_3)))
            ARCHON_CHECK_EQUAL(color_3, color);
    };

    check_read("brown",  Color::brown);
    check_read("purple", Color::purple);
    check_read("orange", Color::orange);
}
