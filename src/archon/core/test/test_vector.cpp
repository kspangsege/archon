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


#include <cstddef>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <list>

#include <archon/core/integer.hpp>
#include <archon/core/vector.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_Vector_IncompleteValueType)
{
    struct Foo;
    struct Bar {
        core::Vector<Foo> vec;
    };
    struct Foo {};
    Bar bar;
    static_cast<void>(bar);
}


ARCHON_TEST(Core_Vector_StaticMem)
{
    core::Vector<int, 8> vec;
    ARCHON_CHECK_EQUAL(vec.capacity(), 8);
    vec.push_back(1);
    int* base_1 = vec.data();
    vec.append(7, 2);
    int* base_2 = vec.data();
    ARCHON_CHECK_EQUAL(base_1, base_2);
    ARCHON_CHECK_EQUAL(vec.capacity(), 8);
}


ARCHON_TEST(Core_Vector_DefaultConstruct)
{
    {
        core::Vector<int> vec;
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), 0);
    } {
        core::Vector<int, 3> vec;
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
    }
}


ARCHON_TEST(Core_Vector_MoveConstruct)
{
    {
        core::Vector<int> vec = { 1, 2, 3 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int> vec_2 = std::move(vec);
        ARCHON_CHECK_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 1, 2, 3 }));
    } {
        core::Vector<int, 3> vec = { 4, 5, 6 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int, 3> vec_2 = std::move(vec);
        ARCHON_CHECK_NOT_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 4, 5, 6 }));
    } {
        core::Vector<int, 2> vec = { 7, 8, 9 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int, 2> vec_2 = std::move(vec);
        ARCHON_CHECK_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 7, 8, 9 }));
    }
}


ARCHON_TEST(Core_Vector_ConstructFromInitializerList)
{
    {
        core::Vector<int> vec = { 1, 2, 3 };
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
    } {
        core::Vector<int, 4> vec = { 4, 5, 6 };
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 4, 5, 6 }));
    }
}


ARCHON_TEST(Core_Vector_ConstructFromSize)
{
    {
        core::Vector<int> vec(3);
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0, 0, 0 }));
    } {
        core::Vector<int, 4> vec(3);
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0, 0, 0 }));
    }
}


ARCHON_TEST(Core_Vector_ConstructFromSizeAndValue)
{
    {
        core::Vector<int> vec(3, 7);
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7 }));
    } {
        core::Vector<int, 4> vec(3, 8);
        ARCHON_CHECK_NOT(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 8, 8, 8 }));
    }
}


ARCHON_TEST(Core_Vector_ConstructFromIteratorPair)
{
    // Random access iterators
    {
        std::vector ref = { 1, 2, 3 };
        core::Vector<int> vec(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    } {
        std::vector ref = { 4, 5, 6 };
        core::Vector<int, 4> vec(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    }

    // Non-random access iterators
    {
        std::list ref = { 1, 2, 3 };
        core::Vector<int> vec(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    } {
        std::list ref = { 4, 5, 6 };
        core::Vector<int, 4> vec(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    }
}


ARCHON_TEST(Core_Vector_MoveAssign)
{
    {
        core::Vector<int> vec = { 1, 2, 3 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int> vec_2 = { 4, 5, 6, 7 };
        vec_2 = std::move(vec);
        ARCHON_CHECK_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 1, 2, 3 }));
    } {
        core::Vector<int, 5> vec = { 1, 2, 3 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int, 5> vec_2 = { 4, 5, 6, 7 };
        vec_2 = std::move(vec);
        ARCHON_CHECK_NOT_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 1, 2, 3 }));
    } {
        core::Vector<int, 3> vec = { 1, 2, 3 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int, 3> vec_2 = { 4, 5, 6, 7 };
        vec_2 = std::move(vec);
        ARCHON_CHECK_NOT_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 1, 2, 3 }));
    } {
        core::Vector<int, 3> vec = { 1, 2, 3, 4 };
        auto data = vec.data();
        auto capacity = vec.capacity();
        core::Vector<int, 3> vec_2 = { 5, 6, 7 };
        vec_2 = std::move(vec);
        ARCHON_CHECK_EQUAL(vec_2.data(), data);
        ARCHON_CHECK_EQUAL(vec_2.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec_2, (std::vector { 1, 2, 3, 4 }));
    }
}


ARCHON_TEST(Core_Vector_AssignFromInitializerList)
{
    {
        core::Vector<int> vec;
        auto data = vec.data();
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_NOT_EQUAL(vec.data(), data);
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5, 6, 7, 8 });
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 5);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 4, 5, 6, 7, 8 }));
        data = vec.data();
        auto capacity = vec.capacity();
        vec.assign({ 9, 10 });
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 9, 10 }));
    } {
        core::Vector<int, 4> vec;
        auto data = vec.data();
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5, 6, 7, 8 });
        ARCHON_CHECK_NOT_EQUAL(vec.data(), data);
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 5);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 4, 5, 6, 7, 8 }));
        data = vec.data();
        auto capacity = vec.capacity();
        vec.assign({ 9, 10 });
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 9, 10 }));
    }
}


ARCHON_TEST(Core_Vector_AssignFromSizeAndValue)
{
    {
        core::Vector<int> vec;
        auto data = vec.data();
        vec.assign(3, 7);
        ARCHON_CHECK_NOT_EQUAL(vec.data(), data);
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7 }));
        vec.assign(5, 8);
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 5);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 8, 8, 8, 8, 8 }));
        data = vec.data();
        auto capacity = vec.capacity();
        vec.assign(2, 9);
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 9, 9 }));
    } {
        core::Vector<int, 4> vec;
        auto data = vec.data();
        vec.assign(3, 7);
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7 }));
        vec.assign(5, 8);
        ARCHON_CHECK_NOT_EQUAL(vec.data(), data);
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 5);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 8, 8, 8, 8, 8 }));
        data = vec.data();
        auto capacity = vec.capacity();
        vec.assign(2, 9);
        ARCHON_CHECK_EQUAL(vec.data(), data);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 9, 9 }));
    }
}


ARCHON_TEST(Core_Vector_AssignFromIteratorPair)
{
    // Random access iterators
    {
        std::vector ref = { 1, 2, 3 };
        core::Vector<int> vec = { 4, 5 };
        vec.assign(ref.begin(), ref.end());
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    } {
        std::vector ref = { 1, 2, 3 };
        core::Vector<int, 4> vec = { 4, 5 };
        vec.assign(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    }

    // Non-random access iterators
    {
        std::list ref = { 1, 2, 3 };
        core::Vector<int> vec = { 4, 5 };
        vec.assign(ref.begin(), ref.end());
        ARCHON_CHECK_GREATER_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    } {
        std::list ref = { 1, 2, 3 };
        core::Vector<int, 4> vec = { 4, 5 };
        vec.assign(ref.begin(), ref.end());
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, ref);
    }
}


ARCHON_TEST(Core_Vector_Subscribe)
{
    core::Vector<int> vec;
    vec.push_back(1);
    ARCHON_CHECK_EQUAL(vec[0], 1);
    vec.push_back(2);
    ARCHON_CHECK_EQUAL(vec[0], 1);
    ARCHON_CHECK_EQUAL(vec[1], 2);
}


ARCHON_TEST(Core_Vector_RangeCheckingSubscribe)
{
    core::Vector<int> vec;
    ARCHON_CHECK_THROW(vec.at(0), std::out_of_range);
    vec.push_back(1);
    ARCHON_CHECK_EQUAL(vec.at(0), 1);
    ARCHON_CHECK_THROW(vec.at(1), std::out_of_range);
    vec.push_back(2);
    ARCHON_CHECK_EQUAL(vec.at(0), 1);
    ARCHON_CHECK_EQUAL(vec.at(1), 2);
    ARCHON_CHECK_THROW(vec.at(2), std::out_of_range);
    ARCHON_CHECK_THROW(vec.at(std::size_t(-1)), std::out_of_range);
}


ARCHON_TEST(Core_Vector_FrontAndBack)
{
    {
        core::Vector<int> vec = { 1, 2, 3 };
        ARCHON_CHECK_EQUAL(vec.front(), 1);
        ARCHON_CHECK_EQUAL(vec.back(), 3);
    } {
        core::Vector<int, 4> vec = { 1, 2, 3 };
        ARCHON_CHECK_EQUAL(vec.front(), 1);
        ARCHON_CHECK_EQUAL(vec.back(), 3);
    }
}


ARCHON_TEST(Core_Vector_DataAndSize)
{
    {
        core::Vector<int> vec;
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector<int> {}));
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5 });
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector { 4, 5 }));
    } {
        core::Vector<int, 4> vec;
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector<int> {}));
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5 });
        ARCHON_CHECK_EQUAL_SEQ(core::Span(vec.data(), vec.size()), (std::vector { 4, 5 }));
    }
}


ARCHON_TEST(Core_Vector_Span)
{
    {
        core::Vector<int> vec;
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector<int> {}));
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5 });
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector { 4, 5 }));
    } {
        core::Vector<int, 4> vec;
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector<int> {}));
        vec.assign({ 1, 2, 3 });
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector { 1, 2, 3 }));
        vec.assign({ 4, 5 });
        ARCHON_CHECK_EQUAL_SEQ(vec.span(), (std::vector { 4, 5 }));
    }
}


ARCHON_TEST(Core_Vector_BeginEnd)
{
    {
        std::vector ref = { 1, 2, 3 };
        core::Vector<int> vec = { 1, 2, 3 };
        const core::Vector<int>& cvec = vec;
        ARCHON_CHECK(std::equal(vec.begin(),   vec.end(),   ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(cvec.begin(),  cvec.end(),  ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(vec.cbegin(),  vec.cend(),  ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(vec.rbegin(),  vec.rend(),  ref.rbegin(), ref.rend()));
        ARCHON_CHECK(std::equal(cvec.rbegin(), cvec.rend(), ref.rbegin(), ref.rend()));
        ARCHON_CHECK(std::equal(vec.crbegin(), vec.crend(), ref.rbegin(), ref.rend()));
    } {
        std::vector ref = { 1, 2, 3 };
        core::Vector<int, 4> vec = { 1, 2, 3 };
        const core::Vector<int, 4>& cvec = vec;
        ARCHON_CHECK(std::equal(vec.begin(),   vec.end(),   ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(cvec.begin(),  cvec.end(),  ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(vec.cbegin(),  vec.cend(),  ref.begin(),  ref.end()));
        ARCHON_CHECK(std::equal(vec.rbegin(),  vec.rend(),  ref.rbegin(), ref.rend()));
        ARCHON_CHECK(std::equal(cvec.rbegin(), cvec.rend(), ref.rbegin(), ref.rend()));
        ARCHON_CHECK(std::equal(vec.crbegin(), vec.crend(), ref.rbegin(), ref.rend()));
    }
}


ARCHON_TEST(Core_Vector_Empty)
{
    {
        core::Vector<int> vec;
        ARCHON_CHECK(vec.empty());
        vec.push_back(1);
        ARCHON_CHECK_NOT(vec.empty());
    } {
        core::Vector<int, 2> vec;
        ARCHON_CHECK(vec.empty());
        vec.push_back(1);
        ARCHON_CHECK_NOT(vec.empty());
    }
}


ARCHON_TEST(Core_Vector_MaxSize)
{
    core::Vector<int> vec;
    ARCHON_CHECK_GREATER(vec.max_size(), 0);
}


ARCHON_TEST(Core_Vector_CapacityAndReserve)
{
    {
        core::Vector<int> vec;
        ARCHON_CHECK_EQUAL(vec.capacity(), 0);
        vec.push_back(1);
        std::size_t capacity = vec.capacity();
        ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
        vec.reserve(capacity);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        std::size_t extra_capacity = std::size_t(capacity - vec.size());
        vec.reserve_extra(extra_capacity);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        vec.append(extra_capacity, 2);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL(vec.size(), capacity);
        vec.reserve_extra(1);
        ARCHON_CHECK_GREATER(vec.capacity(), capacity);
        ARCHON_CHECK_THROW(vec.reserve_extra(core::int_max<std::size_t>()), std::length_error);
    } {
        core::Vector<int, 4> vec;
        std::size_t capacity = vec.capacity();
        ARCHON_CHECK_EQUAL(capacity, 4);
        vec.push_back(1);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        vec.reserve(capacity);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        std::size_t extra_capacity = std::size_t(capacity - vec.size());
        vec.reserve_extra(extra_capacity);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        vec.append(extra_capacity, 2);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        ARCHON_CHECK_EQUAL(vec.size(), capacity);
        vec.reserve_extra(1);
        ARCHON_CHECK_GREATER(vec.capacity(), capacity);
        ARCHON_CHECK_THROW(vec.reserve_extra(core::int_max<std::size_t>()), std::length_error);
    }
}


ARCHON_TEST(Core_Vector_ShrinkToFit)
{
    {
        core::Vector<int> vec;
        vec.shrink_to_fit();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 0);

        vec.push_back(1);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 1);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 1);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));

        vec.push_back(2);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2 }));

        vec.push_back(3);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));

        vec.push_back(4);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4 }));
    } {
        core::Vector<int, 2> vec;
        vec.shrink_to_fit();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);

        vec.push_back(1);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));

        vec.push_back(2);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 2);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2 }));

        vec.push_back(3);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));

        vec.push_back(4);
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4 }));
        vec.shrink_to_fit();
        ARCHON_CHECK_EQUAL(vec.capacity(), 4);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4 }));
    }
}


ARCHON_TEST(Core_Vector_PushPopBack)
{
    core::Vector<int> vec;
    vec.push_back(1);
    ARCHON_CHECK_NOT(vec.empty());
    ARCHON_CHECK_EQUAL(vec.size(), 1);
    ARCHON_CHECK_EQUAL(vec.back(), 1);
    std::size_t capacity = vec.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
    vec.pop_back();
    ARCHON_CHECK(vec.empty());
    ARCHON_CHECK_EQUAL(vec.size(), 0);
    ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
    vec.push_back(2);
    ARCHON_CHECK_NOT(vec.empty());
    ARCHON_CHECK_EQUAL(vec.size(), 1);
    ARCHON_CHECK_EQUAL(vec.back(), 2);
    ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(6);
    ARCHON_CHECK_NOT(vec.empty());
    ARCHON_CHECK_EQUAL(vec.size(), 5);
    ARCHON_CHECK_EQUAL(vec[0], 2);
    ARCHON_CHECK_EQUAL(vec[1], 3);
    ARCHON_CHECK_EQUAL(vec[2], 4);
    ARCHON_CHECK_EQUAL(vec[3], 5);
    ARCHON_CHECK_EQUAL(vec[4], 6);
    std::size_t capacity_2 = vec.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity_2, capacity);
    vec.pop_back();
    vec.pop_back();
    ARCHON_CHECK_NOT(vec.empty());
    ARCHON_CHECK_EQUAL(vec.size(), 3);
    ARCHON_CHECK_EQUAL(vec[0], 2);
    ARCHON_CHECK_EQUAL(vec[1], 3);
    ARCHON_CHECK_EQUAL(vec[2], 4);
    ARCHON_CHECK_EQUAL(vec.capacity(), capacity_2);
}


ARCHON_TEST(Core_Vector_EmplaceBack)
{
    struct Foo {
        int a, b;
        Foo(int a_2, int b_2) noexcept
            : a(a_2)
            , b(b_2)
        {
        }
    };
    {
        core::Vector<Foo> vec;
        vec.emplace_back(1, 2);
        ARCHON_CHECK_EQUAL(vec.back().a, 1);
        ARCHON_CHECK_EQUAL(vec.back().b, 2);
        vec.emplace_back(3, 4);
        ARCHON_CHECK_EQUAL(vec.front().a, 1);
        ARCHON_CHECK_EQUAL(vec.front().b, 2);
        ARCHON_CHECK_EQUAL(vec.back().a, 3);
        ARCHON_CHECK_EQUAL(vec.back().b, 4);
    } {
        core::Vector<Foo, 2> vec;
        vec.emplace_back(1, 2);
        ARCHON_CHECK_EQUAL(vec.back().a, 1);
        ARCHON_CHECK_EQUAL(vec.back().b, 2);
        vec.emplace_back(3, 4);
        ARCHON_CHECK_EQUAL(vec.front().a, 1);
        ARCHON_CHECK_EQUAL(vec.front().b, 2);
        ARCHON_CHECK_EQUAL(vec.back().a, 3);
        ARCHON_CHECK_EQUAL(vec.back().b, 4);
    }
}


ARCHON_TEST(Core_Vector_Append)
{
    core::Vector<int> vec;
    vec.append({ 1, 2, 3 });
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
    vec.append({ 4, 5, 6 });
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4, 5, 6 }));
    vec.clear();
    vec.append(3, 1);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 1, 1 }));
    vec.append(3, 2);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 1, 1, 2, 2, 2 }));
    vec.clear();
    std::vector<int> ref { 1, 2, 3 };
    vec.append(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3 }));
    vec.append(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 1, 2, 3 }));
    vec.clear();
    std::list<int> ref_2 { 3, 2, 1 };
    vec.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 3, 2, 1 }));
    vec.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 3, 2, 1, 3, 2, 1 }));
}


ARCHON_TEST(Core_Vector_Emplace)
{
    core::Vector<int> vec;
    vec.emplace(vec.begin(), 1);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));
    vec.emplace(vec.begin(), 2);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2, 1 }));
    vec.emplace(vec.begin() + 1, 3);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2, 3, 1 }));
    vec.emplace(vec.end(), 4);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2, 3, 1, 4 }));
}


ARCHON_TEST(Core_Vector_Erase)
{
    core::Vector<int> vec = { 1, 2, 3, 4 };
    vec.erase(vec.begin());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2, 3, 4 }));
    vec.erase(vec.begin() + 1 );
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2, 4 }));
    vec.erase(vec.begin() + 1);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 2 }));
    vec.erase(vec.begin());
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector<int> {}));

    vec.assign({ 1, 2, 3, 4 });
    vec.erase(vec.begin() + 1, vec.begin() + 1);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 2, 3, 4 }));
    vec.erase(vec.begin() + 1, vec.begin() + 3);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1, 4 }));
    vec.erase(vec.begin() + 1, vec.begin() + 2);
    ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 1 }));
}


ARCHON_TEST(Core_Vector_Clear)
{
    {
        core::Vector<int> vec = { 1, 2, 3 };
        std::size_t capacity = vec.capacity();
        vec.clear();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        vec.assign({ 2, 3 });
        vec.clear();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
    } {
        core::Vector<int, 4> vec = { 1, 2, 3 };
        std::size_t capacity = vec.capacity();
        vec.clear();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
        vec.assign({ 2, 3 });
        vec.clear();
        ARCHON_CHECK(vec.empty());
        ARCHON_CHECK_EQUAL(vec.size(), 0);
        ARCHON_CHECK_EQUAL(vec.capacity(), capacity);
    }
}


ARCHON_TEST(Core_Vector_Resize)
{
    {
        core::Vector<int> vec;
        vec.resize(0);
        ARCHON_CHECK(vec.empty());
        vec.resize(0, 7);
        ARCHON_CHECK(vec.empty());
        vec.resize(3);
        std::size_t cap = vec.capacity();
        ARCHON_CHECK_GREATER_EQUAL(cap, 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0, 0, 0 }));
        vec.resize(1);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0 }));
        vec.resize(0, 7);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector<int> {}));
        vec.resize(3, 7);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7 }));
        vec.resize(4, 8);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7, 8 }));
        cap = vec.capacity();
        ARCHON_CHECK_GREATER_EQUAL(cap, 4);
        vec.erase(vec.begin());
        vec.resize(4, 9);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 8, 9 }));
        vec.resize(2, 10);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7 }));
        vec.resize(3);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 0 }));
    } {
        core::Vector<int, 3> vec;
        vec.resize(0);
        ARCHON_CHECK(vec.empty());
        vec.resize(0, 7);
        ARCHON_CHECK(vec.empty());
        vec.resize(3);
        std::size_t cap = vec.capacity();
        ARCHON_CHECK_EQUAL(cap, 3);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0, 0, 0 }));
        vec.resize(1);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 0 }));
        vec.resize(0, 7);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector<int> {}));
        vec.resize(3, 7);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7 }));
        vec.resize(4, 8);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 7, 8 }));
        cap = vec.capacity();
        ARCHON_CHECK_GREATER_EQUAL(cap, 4);
        vec.erase(vec.begin());
        vec.resize(4, 9);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 8, 9 }));
        vec.resize(2, 10);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7 }));
        vec.resize(3);
        ARCHON_CHECK_EQUAL(vec.capacity(), cap);
        ARCHON_CHECK_EQUAL_SEQ(vec, (std::vector { 7, 7, 0 }));
    }
}


ARCHON_TEST(Core_Vector_Comparison)
{
    core::Vector<int> vec_1 { 1, 2 };
    core::Vector<int> vec_2 { 1, 2, 3 };
    core::Vector<int> vec_3 { 1, 2, 4 };
    core::Vector<int> vec_4 { 1, 2, 4 };

    ARCHON_CHECK_NOT(vec_1 == vec_2);
    ARCHON_CHECK_NOT(vec_2 == vec_3);
    ARCHON_CHECK(vec_3 == vec_4);
    ARCHON_CHECK_NOT(vec_4 == vec_1);

    ARCHON_CHECK(vec_1 != vec_2);
    ARCHON_CHECK(vec_2 != vec_3);
    ARCHON_CHECK_NOT(vec_3 != vec_4);
    ARCHON_CHECK(vec_4 != vec_1);

    ARCHON_CHECK(vec_1 < vec_2);
    ARCHON_CHECK(vec_2 < vec_3);
    ARCHON_CHECK_NOT(vec_3 < vec_4);
    ARCHON_CHECK_NOT(vec_4 < vec_1);

    ARCHON_CHECK(vec_1 <= vec_2);
    ARCHON_CHECK(vec_2 <= vec_3);
    ARCHON_CHECK(vec_3 <= vec_4);
    ARCHON_CHECK_NOT(vec_4 <= vec_1);

    ARCHON_CHECK_NOT(vec_1 > vec_2);
    ARCHON_CHECK_NOT(vec_2 > vec_3);
    ARCHON_CHECK_NOT(vec_3 > vec_4);
    ARCHON_CHECK(vec_4 > vec_1);

    ARCHON_CHECK_NOT(vec_1 >= vec_2);
    ARCHON_CHECK_NOT(vec_2 >= vec_3);
    ARCHON_CHECK(vec_3 >= vec_4);
    ARCHON_CHECK(vec_4 >= vec_1);
}


ARCHON_TEST(Core_Vector_IteratorEquality)
{
    core::Vector<int> vec;
    core::Vector<int>& cvec = vec;
    ARCHON_CHECK(vec.begin()  == vec.end());
    ARCHON_CHECK(vec.cbegin() == vec.cend());
    ARCHON_CHECK(vec.begin()  == vec.cend());
    ARCHON_CHECK(vec.cbegin() == vec.end());
    ARCHON_CHECK(cvec.begin() == cvec.end());
    ARCHON_CHECK(vec.begin()  == cvec.end());
    ARCHON_CHECK(cvec.begin() == vec.end());
    ARCHON_CHECK_NOT(vec.begin()  != vec.end());
    ARCHON_CHECK_NOT(vec.cbegin() != vec.cend());
    ARCHON_CHECK_NOT(vec.begin()  != vec.cend());
    ARCHON_CHECK_NOT(vec.cbegin() != vec.end());
    ARCHON_CHECK_NOT(cvec.begin() != cvec.end());
    ARCHON_CHECK_NOT(vec.begin()  != cvec.end());
    ARCHON_CHECK_NOT(cvec.begin() != vec.end());
    vec.push_back(0);
    ARCHON_CHECK_NOT(vec.begin()  == vec.end());
    ARCHON_CHECK_NOT(vec.cbegin() == vec.cend());
    ARCHON_CHECK_NOT(vec.begin()  == vec.cend());
    ARCHON_CHECK_NOT(vec.cbegin() == vec.end());
    ARCHON_CHECK_NOT(cvec.begin() == cvec.end());
    ARCHON_CHECK_NOT(vec.begin()  == cvec.end());
    ARCHON_CHECK_NOT(cvec.begin() == vec.end());
    ARCHON_CHECK(vec.begin()  != vec.end());
    ARCHON_CHECK(vec.cbegin() != vec.cend());
    ARCHON_CHECK(vec.begin()  != vec.cend());
    ARCHON_CHECK(vec.cbegin() != vec.end());
    ARCHON_CHECK(cvec.begin() != cvec.end());
    ARCHON_CHECK(vec.begin()  != cvec.end());
    ARCHON_CHECK(cvec.begin() != vec.end());
}


ARCHON_TEST(Core_Vector_IteratorOperations)
{
    core::Vector<int> vec = { 1, 2, 3 };
    auto i_1 = vec.begin();
    auto i_2 = 1 + i_1;
    ARCHON_CHECK_EQUAL(*i_2, 2);
}


ARCHON_TEST(Core_Vector_MoveOnly)
{
    // Verify that one can use a "move only" value type with the vector so long as the move
    // operations do not throw

    class X {
    public:
        X() = default;
        X(X&&) noexcept = default;
        auto operator=(X&&) noexcept -> X& = default;
    };

    core::Vector<X> vec;
    vec.push_back(X());
    vec.emplace(vec.begin(), X());
}


ARCHON_TEST(Core_Vector_ThrowingMove)
{
    // Verify that one can use a value type that may throw when moved so long as the value
    // type is copyable and positioned insertion and removal (emplace(), rease()) is not
    // used

    class X {
    public:
        X() = default;
        X(X&&)
        {
        }
        X(const X&)
        {
        }
        auto operator=(X&&) -> X&
        {
            return *this;
        }
        auto operator=(const X&) -> X&
        {
            return *this;
        }
    };

    core::Vector<X> vec;
    vec.push_back(X());
}


ARCHON_TEST(Core_Vector_ExceptionSafetyInConstructFromIteratorPair)
{
    struct Context {
        bool start_counting_copy_ops = false;
        int num_copy_ops = 0;
        int num_instances = 0;
    };

    class X {
    public:
        explicit X(Context& context)
            : m_context(context)
        {
            ++m_context.num_instances;
        }
        X(const X& x)
            : m_context(x.m_context)
        {
            if (m_context.start_counting_copy_ops) {
                if (++m_context.num_copy_ops == 2)
                    throw std::bad_alloc();
            }
            ++m_context.num_instances;
        }
        ~X()
        {
            --m_context.num_instances;
        }
    private:
        Context& m_context;
    };

    class Iter {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = X;
        using pointer           = X*;
        using reference         = X&;
        using iterator_category = std::input_iterator_tag;
        Iter(pointer ptr)
            : m_ptr(ptr)
        {
        }
        reference operator*() const
        {
            return *m_ptr;
        }
        pointer operator->() const
        {
            return m_ptr;
        }
        Iter& operator++()
        {
            ++m_ptr;
            return *this;
        }
        Iter operator++(int)
        {
            Iter i = *this;
            operator++();
            return i;
        }
        bool operator==(Iter i) const
        {
            return (m_ptr == i.m_ptr);
        }
        bool operator!=(Iter i) const
        {
            return (m_ptr != i.m_ptr);
        }
    private:
        pointer m_ptr = nullptr;
    };

    Context context;
    {
        X arr[3] = {
            X(context),
            X(context),
            X(context),
        };
        context.start_counting_copy_ops = true;
        ARCHON_CHECK_THROW(core::Vector<X>(Iter(arr), Iter(arr + 3)), std::bad_alloc);
    }
    ARCHON_CHECK_EQUAL(context.num_instances, 0);
}
