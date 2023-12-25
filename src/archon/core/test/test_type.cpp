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


#include <type_traits>

#include <archon/core/type.hpp>


using namespace archon;


namespace {

struct Func1 {
    static void foo()
    {
    }
    void bar()
    {
    }
    void baz() const
    {
    }
};

struct Func2 {
    void operator()()
    {
    }
};

struct Func3 {
    void operator()() const
    {
    }
};

} // unnamed namespace


static_assert(std::is_same_v<core::FuncDecay<void()>,    void()>);
static_assert(std::is_same_v<core::FuncDecay<void(*)()>, void()>);
static_assert(std::is_same_v<core::FuncDecay<void(&)()>, void()>);

static_assert(std::is_same_v<core::FuncDecay<decltype(Func1::foo)>,  void()>);
static_assert(std::is_same_v<core::FuncDecay<decltype(&Func1::foo)>, void()>);
static_assert(std::is_same_v<core::FuncDecay<decltype(&Func1::bar)>, void()>);
static_assert(std::is_same_v<core::FuncDecay<decltype(&Func1::baz)>, void()>);

static_assert(std::is_same_v<core::FuncDecay<Func2>, void()>);
static_assert(std::is_same_v<core::FuncDecay<Func3>, void()>);


namespace {

struct NoStreamOutput {};

struct NoWideStreamOutput {};

auto operator<<(std::ostream& out, NoWideStreamOutput) -> std::ostream&
{
    return out;
}

} // unnamed namespace

static_assert(core::has_stream_output_operator<int, char>);
static_assert(core::has_stream_output_operator<int, wchar_t>);

static_assert(core::has_stream_output_operator<NoWideStreamOutput, char>);
static_assert(!core::has_stream_output_operator<NoWideStreamOutput, wchar_t>);

static_assert(!core::has_stream_output_operator<NoStreamOutput, char>);
static_assert(!core::has_stream_output_operator<NoStreamOutput, wchar_t>);
