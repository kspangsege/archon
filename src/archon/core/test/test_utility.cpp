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


#include <archon/core/utility.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_Utility_WithTupleElem)
{
    std::tuple<int, float> tuple;
    const auto& ctuple = tuple;

    core::with_tuple_elem(tuple, 0, [&](auto& elem) {
        elem = 7;
    });
    core::with_tuple_elem(tuple, 1, [&](auto& elem) {
        elem = 8;
    });

    ARCHON_CHECK_EQUAL(std::get<0>(tuple), 7);
    ARCHON_CHECK_EQUAL(std::get<1>(tuple), 8);

    core::with_tuple_elem(ctuple, 0, [&](const auto& elem) {
        ARCHON_CHECK_EQUAL(elem, 7);
    });
    core::with_tuple_elem(ctuple, 1, [&](const auto& elem) {
        ARCHON_CHECK_EQUAL(elem, 8);
    });
}
