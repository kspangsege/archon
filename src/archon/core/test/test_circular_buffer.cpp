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
#include <algorithm>
#include <iterator>
#include <utility>
#include <stdexcept>
#include <vector>
#include <list>

#include <archon/core/circular_buffer.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_CircularBuffer_Empty)
{
    core::CircularBuffer<int> buffer;
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), 0);
    buffer.push_back(1);
    ARCHON_CHECK_NOT(buffer.empty());
}


ARCHON_TEST(Core_CircularBuffer_PushPopFront)
{
    core::CircularBuffer<int> buffer;
    buffer.push_front(1);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_EQUAL(buffer.front(), 1);
    std::size_t capacity = buffer.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
    buffer.pop_front();
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.push_front(2);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_EQUAL(buffer.front(), 2);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.push_front(3);
    buffer.push_front(4);
    buffer.push_front(5);
    buffer.push_front(6);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 5);
    ARCHON_CHECK_EQUAL(buffer[0], 6);
    ARCHON_CHECK_EQUAL(buffer[1], 5);
    ARCHON_CHECK_EQUAL(buffer[2], 4);
    ARCHON_CHECK_EQUAL(buffer[3], 3);
    ARCHON_CHECK_EQUAL(buffer[4], 2);
    std::size_t capacity_2 = buffer.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity_2, capacity);
    buffer.pop_front();
    buffer.pop_front();
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_EQUAL(buffer[0], 4);
    ARCHON_CHECK_EQUAL(buffer[1], 3);
    ARCHON_CHECK_EQUAL(buffer[2], 2);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity_2);
}


ARCHON_TEST(Core_CircularBuffer_PushPopBack)
{
    core::CircularBuffer<int> buffer;
    buffer.push_back(1);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_EQUAL(buffer.back(), 1);
    std::size_t capacity = buffer.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
    buffer.pop_back();
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.push_back(2);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_EQUAL(buffer.back(), 2);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.push_back(3);
    buffer.push_back(4);
    buffer.push_back(5);
    buffer.push_back(6);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 5);
    ARCHON_CHECK_EQUAL(buffer[0], 2);
    ARCHON_CHECK_EQUAL(buffer[1], 3);
    ARCHON_CHECK_EQUAL(buffer[2], 4);
    ARCHON_CHECK_EQUAL(buffer[3], 5);
    ARCHON_CHECK_EQUAL(buffer[4], 6);
    std::size_t capacity_2 = buffer.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity_2, capacity);
    buffer.pop_back();
    buffer.pop_back();
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_EQUAL(buffer[0], 2);
    ARCHON_CHECK_EQUAL(buffer[1], 3);
    ARCHON_CHECK_EQUAL(buffer[2], 4);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity_2);
}


ARCHON_TEST(Core_CircularBuffer_PushPopFrontBack)
{
    core::CircularBuffer<int> buffer;
    buffer.push_front(1);
    buffer.push_back(2);
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 2);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    ARCHON_CHECK_EQUAL(buffer[1], 2);
    buffer.push_front(3);
    buffer.push_back(4);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 3);
    ARCHON_CHECK_EQUAL(buffer[1], 1);
    ARCHON_CHECK_EQUAL(buffer[2], 2);
    ARCHON_CHECK_EQUAL(buffer[3], 4);
    std::size_t capacity = buffer.capacity();
    buffer.pop_front();
    buffer.push_back(5);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    ARCHON_CHECK_EQUAL(buffer[1], 2);
    ARCHON_CHECK_EQUAL(buffer[2], 4);
    ARCHON_CHECK_EQUAL(buffer[3], 5);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_front();
    buffer.push_back(6);
    buffer.pop_front();
    buffer.push_back(7);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 4);
    ARCHON_CHECK_EQUAL(buffer[1], 5);
    ARCHON_CHECK_EQUAL(buffer[2], 6);
    ARCHON_CHECK_EQUAL(buffer[3], 7);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_front();
    buffer.push_back(8);
    buffer.pop_front();
    buffer.push_back(9);
    buffer.pop_front();
    buffer.push_back(10);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 7);
    ARCHON_CHECK_EQUAL(buffer[1], 8);
    ARCHON_CHECK_EQUAL(buffer[2], 9);
    ARCHON_CHECK_EQUAL(buffer[3], 10);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_front();
    buffer.push_back(11);
    buffer.pop_front();
    buffer.push_back(12);
    buffer.pop_front();
    buffer.push_back(13);
    buffer.pop_front();
    buffer.push_back(14);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 11);
    ARCHON_CHECK_EQUAL(buffer[1], 12);
    ARCHON_CHECK_EQUAL(buffer[2], 13);
    ARCHON_CHECK_EQUAL(buffer[3], 14);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_back();
    buffer.push_front(15);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 15);
    ARCHON_CHECK_EQUAL(buffer[1], 11);
    ARCHON_CHECK_EQUAL(buffer[2], 12);
    ARCHON_CHECK_EQUAL(buffer[3], 13);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_back();
    buffer.push_front(16);
    buffer.pop_back();
    buffer.push_front(17);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 17);
    ARCHON_CHECK_EQUAL(buffer[1], 16);
    ARCHON_CHECK_EQUAL(buffer[2], 15);
    ARCHON_CHECK_EQUAL(buffer[3], 11);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_back();
    buffer.push_front(18);
    buffer.pop_back();
    buffer.push_front(19);
    buffer.pop_back();
    buffer.push_front(20);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 20);
    ARCHON_CHECK_EQUAL(buffer[1], 19);
    ARCHON_CHECK_EQUAL(buffer[2], 18);
    ARCHON_CHECK_EQUAL(buffer[3], 17);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_back();
    buffer.push_front(21);
    buffer.pop_back();
    buffer.push_front(22);
    buffer.pop_back();
    buffer.push_front(23);
    buffer.pop_back();
    buffer.push_front(24);
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 24);
    ARCHON_CHECK_EQUAL(buffer[1], 23);
    ARCHON_CHECK_EQUAL(buffer[2], 22);
    ARCHON_CHECK_EQUAL(buffer[3], 21);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_front();
    buffer.pop_back();
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 2);
    ARCHON_CHECK_EQUAL(buffer[0], 23);
    ARCHON_CHECK_EQUAL(buffer[1], 22);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer.pop_front();
    buffer.pop_back();
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
}


ARCHON_TEST(Core_CircularBuffer_Subscribe)
{
    core::CircularBuffer<int> buffer;
    buffer.push_back(1);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    buffer.push_back(2);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    ARCHON_CHECK_EQUAL(buffer[1], 2);
}


ARCHON_TEST(Core_CircularBuffer_RangeCheckingSubscribe)
{
    core::CircularBuffer<int> buffer;
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_THROW(buffer.at(0), std::out_of_range);
    buffer.push_back(1);
    ARCHON_CHECK_EQUAL(buffer.at(0), 1);
    ARCHON_CHECK_THROW(buffer.at(1), std::out_of_range);
    buffer.push_back(2);
    ARCHON_CHECK_EQUAL(buffer.at(0), 1);
    ARCHON_CHECK_EQUAL(buffer.at(1), 2);
    ARCHON_CHECK_THROW(buffer.at(2), std::out_of_range);
    ARCHON_CHECK_THROW(buffer.at(std::size_t(-1)), std::out_of_range);
}


ARCHON_TEST(Core_CircularBuffer_ConstructFromInitializerList)
{
    core::CircularBuffer<int> buffer { 1, 2, 3 };
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    ARCHON_CHECK_EQUAL(buffer[1], 2);
    ARCHON_CHECK_EQUAL(buffer[2], 3);
}


ARCHON_TEST(Core_CircularBuffer_AssignFromInitializerList)
{
    core::CircularBuffer<int> buffer;
    buffer = { 1, 2, 3 };
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK_EQUAL(buffer[0], 1);
    ARCHON_CHECK_EQUAL(buffer[1], 2);
    ARCHON_CHECK_EQUAL(buffer[2], 3);
    buffer.assign({ 4, 5, 6, 7 });
    ARCHON_CHECK_NOT(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 4);
    ARCHON_CHECK_EQUAL(buffer[0], 4);
    ARCHON_CHECK_EQUAL(buffer[1], 5);
    ARCHON_CHECK_EQUAL(buffer[2], 6);
    ARCHON_CHECK_EQUAL(buffer[3], 7);
}


ARCHON_TEST(Core_CircularBuffer_Clear)
{
    core::CircularBuffer<int> buffer { 1, 2, 3 };
    std::size_t capacity = buffer.capacity();
    buffer.clear();
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
    buffer = { 2, 3 };
    buffer.clear();
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.size(), 0);
    ARCHON_CHECK_EQUAL(buffer.capacity(), capacity);
}


ARCHON_TEST(Core_CircularBuffer_Comparison)
{
    core::CircularBuffer<int> buffer_1 { 1, 2 };
    core::CircularBuffer<int> buffer_2 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_3 { 1, 2, 4 };
    core::CircularBuffer<int> buffer_4 { 1, 2, 4 };

    ARCHON_CHECK_NOT(buffer_1 == buffer_2);
    ARCHON_CHECK_NOT(buffer_2 == buffer_3);
    ARCHON_CHECK(buffer_3 == buffer_4);
    ARCHON_CHECK_NOT(buffer_4 == buffer_1);

    ARCHON_CHECK(buffer_1 != buffer_2);
    ARCHON_CHECK(buffer_2 != buffer_3);
    ARCHON_CHECK_NOT(buffer_3 != buffer_4);
    ARCHON_CHECK(buffer_4 != buffer_1);

    ARCHON_CHECK(buffer_1 < buffer_2);
    ARCHON_CHECK(buffer_2 < buffer_3);
    ARCHON_CHECK_NOT(buffer_3 < buffer_4);
    ARCHON_CHECK_NOT(buffer_4 < buffer_1);

    ARCHON_CHECK(buffer_1 <= buffer_2);
    ARCHON_CHECK(buffer_2 <= buffer_3);
    ARCHON_CHECK(buffer_3 <= buffer_4);
    ARCHON_CHECK_NOT(buffer_4 <= buffer_1);

    ARCHON_CHECK_NOT(buffer_1 > buffer_2);
    ARCHON_CHECK_NOT(buffer_2 > buffer_3);
    ARCHON_CHECK_NOT(buffer_3 > buffer_4);
    ARCHON_CHECK(buffer_4 > buffer_1);

    ARCHON_CHECK_NOT(buffer_1 >= buffer_2);
    ARCHON_CHECK_NOT(buffer_2 >= buffer_3);
    ARCHON_CHECK(buffer_3 >= buffer_4);
    ARCHON_CHECK(buffer_4 >= buffer_1);
}


ARCHON_TEST(Core_CircularBuffer_CopyConstruct)
{
    core::CircularBuffer<int> buffer_1 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_2 = buffer_1;
    ARCHON_CHECK_EQUAL(buffer_2.size(), 3);
    ARCHON_CHECK(buffer_2 == buffer_1);
}


ARCHON_TEST(Core_CircularBuffer_CopyAssign)
{
    core::CircularBuffer<int> buffer_1 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_2 { 4, 5, 6 };
    buffer_2 = buffer_1; // Copy assign
    ARCHON_CHECK_EQUAL(buffer_2.size(), 3);
    ARCHON_CHECK(buffer_2 == buffer_1);
}


ARCHON_TEST(Core_CircularBuffer_BeginEnd)
{
    std::vector<int> ref { 1, 2, 3 };
    core::CircularBuffer<int> buffer { 1, 2, 3 };
    const core::CircularBuffer<int>& cbuffer = buffer;
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  buffer.begin(),   buffer.end()));
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  cbuffer.begin(),  cbuffer.end()));
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  buffer.cbegin(),  buffer.cend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), buffer.rbegin(),  buffer.rend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), cbuffer.rbegin(), cbuffer.rend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), buffer.crbegin(), buffer.crend()));
}


ARCHON_TEST(Core_CircularBuffer_ConstructFromSize)
{
    core::CircularBuffer<int> buffer(3);
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::all_of(buffer.begin(), buffer.end(), [](int value) {
        return (value == 0);
    }));
}


ARCHON_TEST(Core_CircularBuffer_ConstructFromSizeAndValue)
{
    core::CircularBuffer<int> buffer(3, 7);
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::all_of(buffer.begin(), buffer.end(), [](int value) {
        return (value == 7);
    }));
}


ARCHON_TEST(Core_CircularBuffer_AssignFromSizeAndValue)
{
    core::CircularBuffer<int> buffer;
    buffer.assign(3, 7);
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::all_of(buffer.begin(), buffer.end(), [](int value) {
        return (value == 7);
    }));
}


ARCHON_TEST(Core_CircularBuffer_ConstructFromNonrandomAccessIterator)
{
    std::list<int> ref { 1, 2, 3 };
    core::CircularBuffer<int> buffer(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::equal(buffer.begin(), buffer.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_CircularBuffer_ConstructFromRandomAccessIterator)
{
    std::vector<int> ref { 1, 2, 3 };
    core::CircularBuffer<int> buffer(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::equal(buffer.begin(), buffer.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_CircularBuffer_AssignFromNonrandomAccessIterator)
{
    std::list<int> ref { 1, 2, 3 };
    core::CircularBuffer<int> buffer { 4, 5, 6 };
    buffer.assign(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::equal(buffer.begin(), buffer.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_CircularBuffer_AssignFromRandomAccessIterator)
{
    std::vector<int> ref { 1, 2, 3 };
    core::CircularBuffer<int> buffer { 4, 5, 6 };
    buffer.assign(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    ARCHON_CHECK(std::equal(buffer.begin(), buffer.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_CircularBuffer_MoveConstruct)
{
    core::CircularBuffer<int> buffer_1 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_2(std::move(buffer_1));
    ARCHON_CHECK_EQUAL(buffer_1.size(), 0);
    ARCHON_CHECK_EQUAL(buffer_2.size(), 3);
    ARCHON_CHECK_EQUAL(buffer_2[0], 1);
    ARCHON_CHECK_EQUAL(buffer_2[1], 2);
    ARCHON_CHECK_EQUAL(buffer_2[2], 3);
}


ARCHON_TEST(Core_CircularBuffer_MoveAssign)
{
    core::CircularBuffer<int> buffer_1 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_2 { 4, 5, 6 };
    buffer_2 = std::move(buffer_1);
    ARCHON_CHECK_EQUAL(buffer_1.size(), 0);
    ARCHON_CHECK_EQUAL(buffer_2.size(), 3);
    ARCHON_CHECK_EQUAL(buffer_2[0], 1);
    ARCHON_CHECK_EQUAL(buffer_2[1], 2);
    ARCHON_CHECK_EQUAL(buffer_2[2], 3);
}


ARCHON_TEST(Core_CircularBuffer_Append)
{
    core::CircularBuffer<int> buffer;
    buffer.append({ 1, 2, 3 });
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 2, 3 }));
    buffer.append({ 4, 5, 6 });
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 2, 3, 4, 5, 6 }));
    buffer.clear();
    buffer.append(3, 1);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 1, 1 }));
    buffer.append(3, 2);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 1, 1, 2, 2, 2 }));
    buffer.clear();
    std::vector<int> ref { 1, 2, 3 };
    buffer.append(ref.begin(), ref.end());
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 2, 3 }));
    buffer.append(ref.begin(), ref.end());
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 2, 3, 1, 2, 3 }));
    buffer.clear();
    std::list<int> ref_2 { 3, 2, 1 };
    buffer.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 3, 2, 1 }));
    buffer.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 3, 2, 1, 3, 2, 1 }));
}


ARCHON_TEST(Core_CircularBuffer_IteratorEquality)
{
    core::CircularBuffer<int> buffer;
    core::CircularBuffer<int>& cbuffer = buffer;
    ARCHON_CHECK(buffer.begin()  == buffer.end());
    ARCHON_CHECK(buffer.cbegin() == buffer.cend());
    ARCHON_CHECK(buffer.begin()  == buffer.cend());
    ARCHON_CHECK(buffer.cbegin() == buffer.end());
    ARCHON_CHECK(cbuffer.begin() == cbuffer.end());
    ARCHON_CHECK(buffer.begin()  == cbuffer.end());
    ARCHON_CHECK(cbuffer.begin() == buffer.end());
    ARCHON_CHECK_NOT(buffer.begin()  != buffer.end());
    ARCHON_CHECK_NOT(buffer.cbegin() != buffer.cend());
    ARCHON_CHECK_NOT(buffer.begin()  != buffer.cend());
    ARCHON_CHECK_NOT(buffer.cbegin() != buffer.end());
    ARCHON_CHECK_NOT(cbuffer.begin() != cbuffer.end());
    ARCHON_CHECK_NOT(buffer.begin()  != cbuffer.end());
    ARCHON_CHECK_NOT(cbuffer.begin() != buffer.end());
    buffer.push_back(0);
    ARCHON_CHECK_NOT(buffer.begin()  == buffer.end());
    ARCHON_CHECK_NOT(buffer.cbegin() == buffer.cend());
    ARCHON_CHECK_NOT(buffer.begin()  == buffer.cend());
    ARCHON_CHECK_NOT(buffer.cbegin() == buffer.end());
    ARCHON_CHECK_NOT(cbuffer.begin() == cbuffer.end());
    ARCHON_CHECK_NOT(buffer.begin()  == cbuffer.end());
    ARCHON_CHECK_NOT(cbuffer.begin() == buffer.end());
    ARCHON_CHECK(buffer.begin()  != buffer.end());
    ARCHON_CHECK(buffer.cbegin() != buffer.cend());
    ARCHON_CHECK(buffer.begin()  != buffer.cend());
    ARCHON_CHECK(buffer.cbegin() != buffer.end());
    ARCHON_CHECK(cbuffer.begin() != cbuffer.end());
    ARCHON_CHECK(buffer.begin()  != cbuffer.end());
    ARCHON_CHECK(cbuffer.begin() != buffer.end());
}


ARCHON_TEST(Core_CircularBuffer_IteratorOperations)
{
    core::CircularBuffer<int> buffer { 1, 2, 3 };
    auto i_1 = buffer.begin();
    auto i_2 = 1 + i_1;
    ARCHON_CHECK_EQUAL(*i_2, 2);
}


ARCHON_TEST(Core_CircularBuffer_Resize)
{
    core::CircularBuffer<int> buffer;
    buffer.resize(0);
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.capacity(), 0);
    buffer.resize(0, 7);
    ARCHON_CHECK(buffer.empty());
    ARCHON_CHECK_EQUAL(buffer.capacity(), 0);
    buffer.resize(3);
    std::size_t cap = buffer.capacity();
    ARCHON_CHECK_GREATER_EQUAL(cap, 3);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 0, 0, 0 }));
    buffer.resize(1);
    ARCHON_CHECK_EQUAL(buffer.capacity(), cap);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 0 }));
    buffer.resize(0, 7);
    ARCHON_CHECK_EQUAL(buffer.capacity(), cap);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>());
    buffer.resize(3, 7);
    ARCHON_CHECK_EQUAL(buffer.capacity(), cap);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 7, 7, 7 }));
    buffer.resize(4, 8);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 7, 7, 7, 8 }));
    buffer.pop_front();
    buffer.resize(4, 9);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 7, 7, 8, 9 }));
    buffer.resize(2, 10);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 7, 7 }));
    buffer.resize(3);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 7, 7, 0 }));
}


ARCHON_TEST(Core_CircularBuffer_ShrinkToFit)
{
    core::CircularBuffer<int> buffer;
    buffer.shrink_to_fit();
    ARCHON_CHECK(buffer.empty());
    buffer.push_back(1);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 1);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 1);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 1);
    buffer.push_back(2);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 2);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 2);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 2);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 2);
    buffer.push_back(3);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 3);
    buffer.push_back(4);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 4);
    buffer.shrink_to_fit();
    ARCHON_CHECK_EQUAL(buffer.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(buffer.capacity(), 4);
    ARCHON_CHECK(buffer == core::CircularBuffer<int>({ 1, 2, 3, 4 }));
}


ARCHON_TEST(Core_CircularBuffer_Swap)
{
    core::CircularBuffer<int> buffer_1 { 1, 2, 3 };
    core::CircularBuffer<int> buffer_2 { 4, 5 };
    swap(buffer_1, buffer_2);
    ARCHON_CHECK(buffer_1 == core::CircularBuffer<int>({ 4, 5 }));
    ARCHON_CHECK(buffer_2 == core::CircularBuffer<int>({ 1, 2, 3 }));
}


ARCHON_TEST(Core_CircularBuffer_ExceptionSafetyInConstructFromIteratorPair)
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
        ARCHON_CHECK_THROW(core::CircularBuffer<X>(Iter(arr), Iter(arr + 3)), std::bad_alloc);
    }
    ARCHON_CHECK_EQUAL(context.num_instances, 0);
}
