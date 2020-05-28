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


#include <archon/core/memory_output_stream.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_MemoryOutputStream_Basics)
{
    core::MemoryOutputStream out;
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.get_capacity(), 0);
    ARCHON_CHECK_EQUAL(out.get_size(), 0);
    ARCHON_CHECK_EQUAL(out.view(), "");

    out << "foo";
    ARCHON_CHECK_NOT(out);

    std::array<char, 4> memory;
    out.reset(memory);
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.get_capacity(), 4);
    ARCHON_CHECK_EQUAL(out.get_size(), 0);
    ARCHON_CHECK_EQUAL(out.view(), "");

    out << "foo";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.view(), "foo");

    out.reset(memory);
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.view(), "");

    out.reset(memory, 2);
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.get_capacity(), 4);
    ARCHON_CHECK_EQUAL(out.get_size(), 2);
    ARCHON_CHECK_EQUAL(out.view(), "fo");

    out << "x";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.get_size(), 2);
    ARCHON_CHECK_EQUAL(out.view(), "xo");

    out << "bar";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.get_size(), 4);
    ARCHON_CHECK_EQUAL(out.view(), "xbar");
}


ARCHON_TEST(Core_MemoryOutputStream_Seek)
{
    std::array<char, 6> memory;
    core::MemoryOutputStream out(memory);
    ARCHON_CHECK_EQUAL(out.view(), "");

    out << "foo";
    ARCHON_CHECK_EQUAL(out.view(), "foo");

    out << "bar";
    ARCHON_CHECK_EQUAL(out.view(), "foobar");

    out.seekp(0, std::ios_base::beg);
    out << "x";
    ARCHON_CHECK_EQUAL(out.view(), "xoobar");

    out.seekp(-2, std::ios_base::end);
    out << "y";
    ARCHON_CHECK_EQUAL(out.view(), "xoobyr");

    out.seekp(-3, std::ios_base::cur);
    out << "i";
    ARCHON_CHECK_EQUAL(out.view(), "xoibyr");
}


ARCHON_TEST(Core_MemoryOutputStream_RespectCapacity)
{
    std::array<char, 3> memory;
    core::MemoryOutputStream out(memory);

    out << "a";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.view(), "a");

    out << "b";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.view(), "ab");

    out << "c";
    ARCHON_CHECK(out);
    ARCHON_CHECK_EQUAL(out.view(), "abc");

    out << "d";
    ARCHON_CHECK_NOT(out);
    ARCHON_CHECK_EQUAL(out.view(), "abc");
}
