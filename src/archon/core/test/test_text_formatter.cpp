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
#include <ios>

#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_TextFormatter_AdvancedContinuation)
{
    std::array<char, 128> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);

    core::TextFormatter formatter(out);
    formatter.set_adv_continuation(true);
    formatter.set_min_separation(3);
    formatter.set_max_displacement(2);

    formatter.write("a");
    formatter.close_section();
    formatter.set_indent(6);
    formatter.write("b\n");
    formatter.set_indent(0);

    formatter.write("foo");
    formatter.close_section();
    formatter.set_indent(6);
    formatter.write("bar\n");
    formatter.set_indent(0);

    formatter.write("alpha");
    formatter.close_section();
    formatter.set_indent(6);
    formatter.write("beta\n");
    formatter.set_indent(0);

    formatter.write("epsilon");
    formatter.close_section();
    formatter.set_indent(6);
    formatter.write("lambda\n");
    formatter.finalize();

    ARCHON_CHECK_EQUAL(out.view(),
                       "a     b\n"
                       "foo   bar\n"
                       "alpha   beta\n"
                       "epsilon\n"
                       "      lambda\n");
}
