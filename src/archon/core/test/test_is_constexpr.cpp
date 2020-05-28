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


#include <archon/core/is_constexpr.hpp>


using namespace archon;


namespace {

constexpr int x_1 = 7;

int x_2 = 7;

constexpr double func_1(double)
{
    return 0;
}

double func_2(double)
{
    return 0;
}

} // unnamed namespace


static_assert(ARCHON_IS_CONSTEXPR(7));
static_assert(ARCHON_IS_CONSTEXPR(x_1));
static_assert(!ARCHON_IS_CONSTEXPR(x_2));
static_assert(ARCHON_IS_CONSTEXPR(func_1(7)));
static_assert(!ARCHON_IS_CONSTEXPR(func_2(7)));
static_assert(ARCHON_IS_CONSTEXPR(func_1(x_1)));
static_assert(!ARCHON_IS_CONSTEXPR(func_2(x_1)));
static_assert(!ARCHON_IS_CONSTEXPR(func_1(x_2)));
static_assert(!ARCHON_IS_CONSTEXPR(func_2(x_2)));
