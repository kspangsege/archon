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


#include <array>

#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/format_with.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


template<class C> void check_width(check::TestContext& test_context)
{
    std::array<C, 4> buffer;
    core::BasicMemoryOutputStream out(buffer);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    out << core::with_width('x', 3);
    std::array<C, 4> seed_memory;
    core::BasicStringWidener widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("  x"));
}


} // unnamed namespace


ARCHON_TEST(Core_FormatWith_Width)
{
    check_width<char>(test_context);
    check_width<wchar_t>(test_context);
}
