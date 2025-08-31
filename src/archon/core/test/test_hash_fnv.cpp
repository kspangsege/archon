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

#include <cstddef>
#include <type_traits>
#include <initializer_list>

#include <archon/core/integer.hpp>
#include <archon/core/hash_fnv.hpp>
#include <archon/core/as_list.hpp>
#include <archon/check.hpp>


using namespace archon;


static_assert(core::Hash_FNV_1a_32::bit_width == 32);
static_assert(core::Hash_FNV_1a_64::bit_width == 64);

static_assert(std::is_same_v<core::Hash_FNV_1a_Default,
              std::conditional_t<core::int_width<std::size_t>() <= 32, core::Hash_FNV_1a_32, core::Hash_FNV_1a_64>>);


ARCHON_TEST(Core_HashFNV_32)
{
    using hash_type = core::Hash_FNV_1a_32;
    using value_type = hash_type::value_type;

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int> octets, value_type expected) {
        ARCHON_TEST_TRAIL(parent_test_context, core::as_list(octets));

        hash_type hash;
        for (int octet : octets)
            hash.add_int(octet, 8);
        ARCHON_CHECK_EQUAL(hash.get(), expected);

        hash_type hash_1, hash_2;
        for (int octet : octets) {
            hash_1.add_int(octet);
            hash_2.add_int(octet, core::int_width<decltype(octet)>());
        }
        ARCHON_CHECK_EQUAL(hash_1.get(), hash_2.get());
    };

    test({}, 0x0'811C'9DC5);
    test({ 97 }, 0x0'E40C'292C);
    test({ 98 }, 0x0'E70C'2DE5);
    test({ 99 }, 0x0'E60C'2C52);
    test({ 102, 111, 111, 98, 97, 114 }, 0x0'BF9C'F968);
}


ARCHON_TEST(Core_HashFNV_64)
{
    using hash_type = core::Hash_FNV_1a_64;
    using value_type = hash_type::value_type;

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int> octets, value_type expected) {
        ARCHON_TEST_TRAIL(parent_test_context, core::as_list(octets));

        hash_type hash;
        for (int octet : octets)
            hash.add_int(octet, 8);
        ARCHON_CHECK_EQUAL(hash.get(), expected);

        hash_type hash_1, hash_2;
        for (int octet : octets) {
            hash_1.add_int(octet);
            hash_2.add_int(octet, core::int_width<decltype(octet)>());
        }
        ARCHON_CHECK_EQUAL(hash_1.get(), hash_2.get());
    };

    test({}, 0x0'CBF2'9CE4'8422'2325);
    test({ 97 }, 0x0'AF63'DC4C'8601'EC8C);
    test({ 98 }, 0x0'AF63'DF4C'8601'F1A5);
    test({ 99 }, 0x0'AF63'DE4C'8601'EFF2);
    test({ 102, 111, 111, 98, 97, 114 }, 0x0'8594'4171'F739'67E8);
}
