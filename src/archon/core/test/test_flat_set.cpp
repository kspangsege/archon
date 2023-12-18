// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <iterator>
#include <memory>

#include <archon/core/features.h>
#include <archon/core/flat_set.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_FlatSet_LowerUpperBound)
{
    core::FlatSet<int> set = { 1, 2, 3 };

    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(0)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(1)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(2)), 1);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(3)), 2);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(4)), 3);

    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(0)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(1)), 1);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(2)), 2);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(3)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(4)), 3);
}


ARCHON_TEST(Core_FlatSet_EqualRange)
{
    core::FlatSet<int> set = { 1, 2, 3 };

    check::TestContext& parent_test_context = test_context;
    for (int i = 0; i < 5; ++i) {
        ARCHON_TEST_TRAIL(parent_test_context, i);
        ARCHON_CHECK(set.equal_range(i) == std::make_pair(set.lower_bound(i), set.upper_bound(i)));
    }
}


ARCHON_TEST(Core_FlatSet_IncompleteValueType)
{
    struct Foo;
    struct Bar {
        core::FlatSet<Foo> set;
    };
    struct Foo {};
    Bar bar;
    static_cast<void>(bar);
}


ARCHON_TEST(Core_FlatSet_NoncopyableValue)
{
    struct Foo {
        int i;
        Foo(int i_2) noexcept
            : i(i_2)
        {
        }
        Foo(Foo&&) noexcept = default;
        auto operator=(Foo&&) noexcept -> Foo& = default;
        auto operator<=>(const Foo&) const noexcept = default;
    };
    core::FlatSet<Foo> set;
    set.emplace(Foo{17});              
    set.emplace(Foo{12});
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(set.size(), 2))) {
        auto i = set.begin();
        ARCHON_CHECK_EQUAL(i->i, 12);
        ++i;
        ARCHON_CHECK_EQUAL(i->i, 17);
    }
}
