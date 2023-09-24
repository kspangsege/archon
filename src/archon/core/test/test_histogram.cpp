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


#include <cstddef>
#include <array>
#include <ios>

#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/histogram.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_Histogram_General, char_variants)
{
    using char_type = test_type;

    double from = 0;
    double to   = 10;
    std::size_t num_bins = 5;
    core::Histogram<double> histogram(from, to, num_bins);

    histogram.add(7);
    histogram.add(9);
    histogram.add(3);
    histogram.add(9);
    histogram.add(1);
    histogram.add(1);
    histogram.add(3);
    histogram.add(9);
    histogram.add(9);
    histogram.add(1);

    std::array<char_type, 256> seed_memory_1;
    core::BasicMemoryOutputStream out(seed_memory_1);
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(test_context.locale);
    int width = 18;
    histogram.print(out, width);

    std::array<char_type, 256> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(out.view(), widener.widen("0 -> 2  : 3 |### |\n"
                                                 "2 -> 4  : 2 |##  |\n"
                                                 "4 -> 6  : 0 |    |\n"
                                                 "6 -> 8  : 1 |#   |\n"
                                                 "8 -> 10 : 4 |####|\n"));
}
