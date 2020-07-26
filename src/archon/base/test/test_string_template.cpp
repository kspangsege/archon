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
#include <archon/base/memory_output_stream.hpp>
#include <archon/base/string_template.hpp>
#include <archon/unit_test.hpp>


using namespace archon;


namespace {


template<class C> void check_output_stream_field_width(unit_test::TestContext& test_context)
{
    const std::locale& locale = test_context.thread_context.root_context.locale;
    int x = 7;
    using Template = base::BasicStringTemplate<std::basic_string_view<C>>;
    typename Template::Parameters params;
    params["x"] = &x;
    Template templ("<@x>", params, locale);
    std::array<C, 256> buffer;
    base::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale);
    out.width(5);
    out.setf(std::ios_base::right, std::ios_base::adjustfield);
    out << expand(templ);
    std::array<C, 256> seed_memory;
    base::BasicStringWidener widener(locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  <7>"));
}


} // unnamed namespace




ARCHON_TEST(Base_StringTemplate_OutputStreamFieldWidth)
{
    check_output_stream_field_width<char>(test_context);
    check_output_stream_field_width<wchar_t>(test_context);
}
