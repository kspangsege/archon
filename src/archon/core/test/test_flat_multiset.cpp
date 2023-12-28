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
#include <utility>
#include <vector>

#include <archon/core/pair.hpp>
#include <archon/core/flat_multiset.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_FlatMultiset_Emplace)
{
    core::FlatMultiset<core::Pair<int, int>> set;
    set.emplace(3, 7);
    set.emplace(1, 7);
    set.emplace(3, 4);
    set.emplace(3, 7);
    ARCHON_CHECK_EQUAL_SEQ(set, (std::vector<core::Pair<int, int>> {{ 1, 7 }, { 3, 4 }, { 3, 7 }, { 3, 7 }}));
}


ARCHON_TEST(Core_FlatMultiset_Insert)
{
    core::FlatMultiset<int> set;
    set.insert(3);
    set.insert(1);
    set.insert(4);
    set.insert(3);
    ARCHON_CHECK_EQUAL_SEQ(set, (std::vector<int> { 1, 3, 3, 4 }));
}


ARCHON_TEST(Core_FlatMultiset_Contains)
{
    core::FlatMultiset<int> set = { 1, 2, 2, 4 };

    ARCHON_CHECK_NOT(set.contains(0));
    ARCHON_CHECK(set.contains(1));
    ARCHON_CHECK(set.contains(2));
    ARCHON_CHECK_NOT(set.contains(3));
    ARCHON_CHECK(set.contains(4));
    ARCHON_CHECK_NOT(set.contains(5));
}


ARCHON_TEST(Core_FlatMultiset_Count)
{
    core::FlatMultiset<int> set = { 1, 2, 2, 4 };

    ARCHON_CHECK_EQUAL(set.count(0), 0);
    ARCHON_CHECK_EQUAL(set.count(1), 1);
    ARCHON_CHECK_EQUAL(set.count(2), 2);
    ARCHON_CHECK_EQUAL(set.count(3), 0);
    ARCHON_CHECK_EQUAL(set.count(4), 1);
    ARCHON_CHECK_EQUAL(set.count(5), 0);
}


ARCHON_TEST(Core_FlatMultiset_Find)
{
    core::FlatMultiset<int> set = { 1, 2, 2, 4 };

    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(0)), 4);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(1)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(2)), 1);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(3)), 4);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(4)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.find(5)), 4);
}


ARCHON_TEST(Core_FlatMultiset_LowerUpperBound)
{
    core::FlatMultiset<int> set = { 1, 2, 2, 4 };

    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(0)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(1)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(2)), 1);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(3)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(4)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.lower_bound(5)), 4);

    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(0)), 0);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(1)), 1);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(2)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(3)), 3);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(4)), 4);
    ARCHON_CHECK_EQUAL(std::distance(set.begin(), set.upper_bound(5)), 4);
}


ARCHON_TEST(Core_FlatMultiset_EqualRange)
{
    core::FlatMultiset<int> set = { 1, 2, 2, 4};

    check::TestContext& parent_test_context = test_context;
    for (int i = 0; i < 6; ++i) {
        ARCHON_TEST_TRAIL(parent_test_context, i);
        ARCHON_CHECK(set.equal_range(i) == std::make_pair(set.lower_bound(i), set.upper_bound(i)));
    }
}


ARCHON_TEST(Core_FlatMultiset_IncompleteKeyType)
{
    struct Key;
    struct Foo {
        core::FlatMultiset<Key> set;
    };
    struct Key {};
    Foo foo;
    static_cast<void>(foo);
}
