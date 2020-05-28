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

#include <archon/core/memory_output_stream.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_StreamOutput_AltHelperBasics)
{
    std::array<char, 10> memory;
    core::MemoryOutputStream out(memory);
    out.setf(std::ios_base::fixed, std::ios_base::floatfield);
    out.precision(2);
    out.width(8);
    core::StreamOutputAltHelper helper(out);
    helper.out << "(" << 7.0 << ")";
    helper.flush();
    ARCHON_CHECK_EQUAL(out.view(), "  (7.00)");
}
