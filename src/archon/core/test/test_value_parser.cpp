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


#include <type_traits>
#include <array>
#include <string_view>

#include <archon/core/char_mapper.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_ValueParser_Int)
{
    std::array<wchar_t, 256> seed_memory;
    core::WideStringWidener widener(std::locale::classic(), seed_memory);
    core::ValueParser parser(std::locale::classic());
    core::WideValueParser wparser(std::locale::classic());

    auto test = [&](std::string_view string, const auto& value) {
        using T = std::decay_t<decltype(value)>;
        T var = {};
        if (ARCHON_CHECK(parser.parse(string, var)))
            ARCHON_CHECK_EQUAL(var, value);
        std::wstring_view wstring = widener.widen(string);
        if (ARCHON_CHECK(wparser.parse(wstring, var)))
            ARCHON_CHECK_EQUAL(var, value);
    };

    test("123", 123);
    test("-123", -123);
}


ARCHON_TEST(Core_ValueParser_String)
{
    std::array<wchar_t, 256> seed_memory;
    core::WideStringWidener widener(std::locale::classic(), seed_memory);
    core::ValueParser parser(std::locale::classic());
    core::WideValueParser wparser(std::locale::classic());

    auto test = [&](std::string_view string) {
        std::string var = {};
        if (ARCHON_CHECK(parser.parse(string, var)))
            ARCHON_CHECK_EQUAL(var, string);
        std::wstring_view wstring = widener.widen(string);
        std::wstring wvar = {};
        if (ARCHON_CHECK(wparser.parse(wstring, wvar)))
            ARCHON_CHECK_EQUAL(wvar, wstring);
    };

    test("x");
    test("x x");
}


ARCHON_TEST(Core_ValueParser_BadInt)
{
    std::array<wchar_t, 256> seed_memory;
    core::WideStringWidener widener(std::locale::classic(), seed_memory);
    core::ValueParser parser(std::locale::classic());
    core::WideValueParser wparser(std::locale::classic());

    auto test = [&](std::string_view string, const auto& value) {
        using T = std::decay_t<decltype(value)>;
        T var = {};
        ARCHON_CHECK_NOT(parser.parse(string, var));
        std::wstring_view wstring = widener.widen(string);
        ARCHON_CHECK_NOT(wparser.parse(wstring, var));
    };

    test("", int());
    test("x", int());
    test("2x", int());
    test("-123", unsigned());
}
