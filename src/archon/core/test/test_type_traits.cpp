// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <cstdint>
#include <type_traits>

#include <archon/core/type_traits.hpp>
#include <archon/core/integer.hpp>


using namespace archon;


namespace {

using least_signed_0   = core::least_signed_int_type<0>;
using least_unsigned_0 = core::least_unsigned_int_type<0>;
using fast_signed_0    = core::fast_signed_int_type<0>;
using fast_unsigned_0  = core::fast_unsigned_int_type<0>;

using least_signed_8   = core::least_signed_int_type<8>;
using least_unsigned_8 = core::least_unsigned_int_type<8>;
using fast_signed_8    = core::fast_signed_int_type<8>;
using fast_unsigned_8  = core::fast_unsigned_int_type<8>;

constexpr int intmax_width = core::int_width<std::intmax_t>();
constexpr int uintmax_width = core::int_width<std::uintmax_t>();

using least_signed_m   = core::least_signed_int_type<intmax_width>;
using least_unsigned_m = core::least_unsigned_int_type<uintmax_width>;
using fast_signed_m    = core::fast_signed_int_type<intmax_width>;
using fast_unsigned_m  = core::fast_unsigned_int_type<uintmax_width>;

using least_signed_m1   = core::least_signed_int_type<intmax_width + 1>;
using least_unsigned_m1 = core::least_unsigned_int_type<uintmax_width + 1>;
using fast_signed_m1    = core::fast_signed_int_type<intmax_width + 1>;
using fast_unsigned_m1  = core::fast_unsigned_int_type<uintmax_width + 1>;

}


static_assert(std::is_same_v<least_signed_0, std::conditional_t<std::is_signed_v<char>, char, signed char>>);
static_assert(std::is_same_v<least_unsigned_0, std::conditional_t<std::is_unsigned_v<char>, char, unsigned char>>);
static_assert(std::is_same_v<fast_signed_0, int>);
static_assert(std::is_same_v<fast_unsigned_0, unsigned>);

static_assert(std::is_same_v<least_signed_8, std::conditional_t<std::is_signed_v<char>, char, signed char>>);
static_assert(std::is_same_v<least_unsigned_8, std::conditional_t<std::is_unsigned_v<char>, char, unsigned char>>);
static_assert(std::is_same_v<fast_signed_8, int>);
static_assert(std::is_same_v<fast_unsigned_8, unsigned>);

static_assert(!std::is_same_v<least_signed_m, void>);
static_assert(!std::is_same_v<least_unsigned_m, void>);
static_assert(!std::is_same_v<fast_signed_m, void>);
static_assert(!std::is_same_v<fast_unsigned_m, void>);

static_assert(core::is_signed<least_signed_m>());
static_assert(core::is_unsigned<least_unsigned_m>());
static_assert(core::is_signed<fast_signed_m>());
static_assert(core::is_unsigned<fast_unsigned_m>());

static_assert(core::int_width<least_signed_m>() >= intmax_width);
static_assert(core::int_width<least_unsigned_m>() >= uintmax_width);
static_assert(core::int_width<fast_signed_m>() >= intmax_width);
static_assert(core::int_width<fast_unsigned_m>() >= uintmax_width);

static_assert(std::is_same_v<least_signed_m1, void>);
static_assert(std::is_same_v<least_unsigned_m1, void>);
static_assert(std::is_same_v<fast_signed_m1, void>);
static_assert(std::is_same_v<fast_unsigned_m1, void>);
