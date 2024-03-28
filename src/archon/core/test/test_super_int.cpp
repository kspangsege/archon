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
#include <array>
#include <set>

#include <archon/core/type_list.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/super_int.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


template<class T> void test_1(check::TestContext& test_context)
{
    using lim = std::numeric_limits<T>;
    std::set<T> values;
    values.insert(lim::min());
    values.insert(lim::max());
    if constexpr (lim::is_signed) {
        values.insert(0);
        values.insert(-1);
    }
    for (T v_1 : values) {
        core::SuperInt v_2(v_1);
        T v_3;
        if (ARCHON_CHECK(v_2.get_as<T>(v_3)))
            ARCHON_CHECK_EQUAL(v_3, v_1);
    }
}


template<class T> void test_2(check::TestContext& test_context)
{
    using lim = std::numeric_limits<T>;
    core::SuperInt v_1;
    T v_2;

    v_1 = core::SuperInt(lim::min());
    ARCHON_CHECK(v_1.subtract_with_overflow_detect(core::SuperInt(1)) || !v_1.get_as<T>(v_2));

    v_1 = core::SuperInt(lim::max());
    ARCHON_CHECK(v_1.add_with_overflow_detect(core::SuperInt(1)) || !v_1.get_as<T>(v_2));
}


using Types = core::TypeList<bool,
                             char,
                             signed char,
                             unsigned char,
                             wchar_t,
                             short,
                             unsigned short,
                             int,
                             unsigned,
                             long,
                             unsigned long,
                             long long,
                             unsigned long long>;


struct Test {
    template<class T, std::size_t> static void exec(check::TestContext& test_context)
    {
        test_1<T>(test_context);
        test_2<T>(test_context);
    }
};


} // unnamed namespace


ARCHON_TEST(Core_SuperInt_Basics)
{
    core::for_each_type_alt<Types, Test>(test_context);
}


namespace {


template<class C> void check_output_stream_field_width(check::TestContext& test_context)
{
    core::SuperInt i(-7);
    std::array<C, 8> buffer;
    core::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    out.width(4);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    out << i;
    std::array<C, 8> seed_memory;
    core::BasicStringWidener widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  -7"));
}


} // unnamed namespace


ARCHON_TEST(Core_SuperInt_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}
