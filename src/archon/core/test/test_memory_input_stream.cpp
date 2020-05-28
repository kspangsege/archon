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


#include <archon/core/memory_input_stream.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_MemoryInputStream_Basics)
{
    using namespace std::literals;

    core::MemoryInputStream in;
    in.reset("123 4567"sv);
    in.unsetf(std::ios_base::skipws);

    ARCHON_CHECK_EQUAL(in.eof(), false);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);

    int number;
    char sp;

    in >> number;
    ARCHON_CHECK_EQUAL(number, 123);
    ARCHON_CHECK_EQUAL(in.eof(), false);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 3);

    in >> sp;
    ARCHON_CHECK_EQUAL(sp, ' ');
    ARCHON_CHECK_EQUAL(in.eof(), false);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 4);

    in.seekg(1);
    in >> number;
    ARCHON_CHECK_EQUAL(number, 23);
    ARCHON_CHECK_EQUAL(in.eof(), false);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 3);

    in.seekg(5);
    in >> number;
    ARCHON_CHECK_EQUAL(number, 567);
    ARCHON_CHECK_EQUAL(in.eof(), true);
    ARCHON_CHECK_EQUAL(int(in.tellg()), -1);
}


ARCHON_TEST(Core_MemoryInputStream_Seek)
{
    using namespace std::literals;

    core::MemoryInputStream in;

    // No buffer
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0);
    ARCHON_CHECK(in);
    in.seekg(1); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK(in);
    in.seekg(-1); // Out of range
    ARCHON_CHECK_NOT(in);

    // Absolute
    in.reset("AB"sv);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(1);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 1);
    in.seekg(2);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
    in.seekg(3); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
    ARCHON_CHECK(in);
    in.seekg(-1); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);

    // Relative
    in.reset("AB"sv);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0, std::ios_base::beg);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0, std::ios_base::cur);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(0, std::ios_base::end);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
    in.seekg(+1, std::ios_base::beg);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 1);
    in.seekg(+1, std::ios_base::cur);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
    in.seekg(-1, std::ios_base::end);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 1);
    in.seekg(-1, std::ios_base::cur);
    ARCHON_CHECK(in);
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(-1, std::ios_base::beg); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(+3, std::ios_base::beg); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 0);
    in.seekg(+1, std::ios_base::cur);
    in.seekg(-2, std::ios_base::cur); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 1);
    in.seekg(+2, std::ios_base::cur); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 1);
    in.seekg(+1, std::ios_base::cur);
    in.seekg(-3, std::ios_base::end); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
    in.seekg(+1, std::ios_base::end); // Out of range
    ARCHON_CHECK_NOT(in);
    in.clear();
    ARCHON_CHECK_EQUAL(int(in.tellg()), 2);
}
