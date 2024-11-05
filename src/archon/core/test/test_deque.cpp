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

#include <archon/core/deque.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_Deque_Empty)
{
    core::Deque<int> deque;
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), 0);
    deque.push_back(1);
    ARCHON_CHECK_NOT(deque.empty());
}


ARCHON_TEST(Core_Deque_PushPopFront)
{
    core::Deque<int> deque;
    deque.push_front(1);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_EQUAL(deque.front(), 1);
    std::size_t capacity = deque.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
    deque.pop_front();
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.push_front(2);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_EQUAL(deque.front(), 2);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.push_front(3);
    deque.push_front(4);
    deque.push_front(5);
    deque.push_front(6);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 5);
    ARCHON_CHECK_EQUAL(deque[0], 6);
    ARCHON_CHECK_EQUAL(deque[1], 5);
    ARCHON_CHECK_EQUAL(deque[2], 4);
    ARCHON_CHECK_EQUAL(deque[3], 3);
    ARCHON_CHECK_EQUAL(deque[4], 2);
    std::size_t capacity_2 = deque.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity_2, capacity);
    deque.pop_front();
    deque.pop_front();
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_EQUAL(deque[0], 4);
    ARCHON_CHECK_EQUAL(deque[1], 3);
    ARCHON_CHECK_EQUAL(deque[2], 2);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity_2);
}


ARCHON_TEST(Core_Deque_PushPopBack)
{
    core::Deque<int> deque;
    deque.push_back(1);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_EQUAL(deque.back(), 1);
    std::size_t capacity = deque.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity, 1);
    deque.pop_back();
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.push_back(2);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_EQUAL(deque.back(), 2);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.push_back(3);
    deque.push_back(4);
    deque.push_back(5);
    deque.push_back(6);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 5);
    ARCHON_CHECK_EQUAL(deque[0], 2);
    ARCHON_CHECK_EQUAL(deque[1], 3);
    ARCHON_CHECK_EQUAL(deque[2], 4);
    ARCHON_CHECK_EQUAL(deque[3], 5);
    ARCHON_CHECK_EQUAL(deque[4], 6);
    std::size_t capacity_2 = deque.capacity();
    ARCHON_CHECK_GREATER_EQUAL(capacity_2, capacity);
    deque.pop_back();
    deque.pop_back();
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_EQUAL(deque[0], 2);
    ARCHON_CHECK_EQUAL(deque[1], 3);
    ARCHON_CHECK_EQUAL(deque[2], 4);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity_2);
}


ARCHON_TEST(Core_Deque_PushPopFrontBack)
{
    core::Deque<int> deque;
    deque.push_front(1);
    deque.push_back(2);
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 2);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    ARCHON_CHECK_EQUAL(deque[1], 2);
    deque.push_front(3);
    deque.push_back(4);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 3);
    ARCHON_CHECK_EQUAL(deque[1], 1);
    ARCHON_CHECK_EQUAL(deque[2], 2);
    ARCHON_CHECK_EQUAL(deque[3], 4);
    std::size_t capacity = deque.capacity();
    deque.pop_front();
    deque.push_back(5);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    ARCHON_CHECK_EQUAL(deque[1], 2);
    ARCHON_CHECK_EQUAL(deque[2], 4);
    ARCHON_CHECK_EQUAL(deque[3], 5);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_front();
    deque.push_back(6);
    deque.pop_front();
    deque.push_back(7);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 4);
    ARCHON_CHECK_EQUAL(deque[1], 5);
    ARCHON_CHECK_EQUAL(deque[2], 6);
    ARCHON_CHECK_EQUAL(deque[3], 7);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_front();
    deque.push_back(8);
    deque.pop_front();
    deque.push_back(9);
    deque.pop_front();
    deque.push_back(10);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 7);
    ARCHON_CHECK_EQUAL(deque[1], 8);
    ARCHON_CHECK_EQUAL(deque[2], 9);
    ARCHON_CHECK_EQUAL(deque[3], 10);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_front();
    deque.push_back(11);
    deque.pop_front();
    deque.push_back(12);
    deque.pop_front();
    deque.push_back(13);
    deque.pop_front();
    deque.push_back(14);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 11);
    ARCHON_CHECK_EQUAL(deque[1], 12);
    ARCHON_CHECK_EQUAL(deque[2], 13);
    ARCHON_CHECK_EQUAL(deque[3], 14);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_back();
    deque.push_front(15);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 15);
    ARCHON_CHECK_EQUAL(deque[1], 11);
    ARCHON_CHECK_EQUAL(deque[2], 12);
    ARCHON_CHECK_EQUAL(deque[3], 13);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_back();
    deque.push_front(16);
    deque.pop_back();
    deque.push_front(17);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 17);
    ARCHON_CHECK_EQUAL(deque[1], 16);
    ARCHON_CHECK_EQUAL(deque[2], 15);
    ARCHON_CHECK_EQUAL(deque[3], 11);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_back();
    deque.push_front(18);
    deque.pop_back();
    deque.push_front(19);
    deque.pop_back();
    deque.push_front(20);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 20);
    ARCHON_CHECK_EQUAL(deque[1], 19);
    ARCHON_CHECK_EQUAL(deque[2], 18);
    ARCHON_CHECK_EQUAL(deque[3], 17);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_back();
    deque.push_front(21);
    deque.pop_back();
    deque.push_front(22);
    deque.pop_back();
    deque.push_front(23);
    deque.pop_back();
    deque.push_front(24);
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 24);
    ARCHON_CHECK_EQUAL(deque[1], 23);
    ARCHON_CHECK_EQUAL(deque[2], 22);
    ARCHON_CHECK_EQUAL(deque[3], 21);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_front();
    deque.pop_back();
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 2);
    ARCHON_CHECK_EQUAL(deque[0], 23);
    ARCHON_CHECK_EQUAL(deque[1], 22);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque.pop_front();
    deque.pop_back();
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
}


ARCHON_TEST(Core_Deque_Erase)
{
    struct Context {
        std::size_t num_constructions = 0;
        std::size_t num_destructions  = 0;
    };

    struct Elem {
        std::size_t value;
        Elem(std::size_t value_2, Context& context) noexcept
            : value(value_2)
            , m_context(&context)
        {
            ++m_context->num_constructions;
        }
        Elem(const Elem& other) noexcept
            : value(other.value)
            , m_context(other.m_context)
        {
            ++m_context->num_constructions;
        }
        ~Elem() noexcept
        {
            ++m_context->num_destructions;
        }
        bool operator==(const Elem& other) const noexcept
        {
            return (value == other.value);
        }
    private:
        Context* m_context;
    };

    std::size_t max_capacity = 5;
    std::vector<Elem> vec;
    vec.reserve(max_capacity);
    for (std::size_t capacity = 0; capacity <= max_capacity; ++capacity) {
        for (std::size_t size = 0; size <= capacity; ++size) {
            for (std::size_t shift = 0; shift <= capacity; ++shift) {
                for (std::size_t begin = 0; begin <= size; ++begin) {
                    for (std::size_t end = begin; end <= size; ++end) {
                        Context context;
                        core::Deque<Elem> deque;
                        for (std::size_t i = 0; i < capacity; ++i)
                            deque.emplace_back(0, context);
                        for (std::size_t i = 0; i < shift; ++i) {
                            deque.pop_front();
                            deque.emplace_back(0, context);
                        }
                        for (std::size_t i = 0; i < std::size_t(capacity - size); ++i)
                            deque.pop_back();
                        ARCHON_CHECK_EQUAL(deque.size(), size);
                        for (std::size_t i = 0; i < size; ++i) {
                            deque[i].value = i;
                            vec.emplace_back(i, context);
                        }

                        auto j = deque.erase(deque.begin() + begin, deque.begin() + end);
                        ARCHON_CHECK(j == deque.begin() + begin);
                        vec.erase(vec.begin() + begin, vec.begin() + end);
                        ARCHON_CHECK_EQUAL_SEQ(deque, vec);

                        deque.emplace_back(std::size_t(-1), context);
                        deque.pop_back();
                        ARCHON_CHECK_EQUAL_SEQ(deque, vec);

                        deque.emplace_front(std::size_t(-1), context);
                        deque.pop_front();
                        ARCHON_CHECK_EQUAL_SEQ(deque, vec);

                        deque.clear();
                        vec.clear();
                        ARCHON_CHECK_EQUAL(context.num_destructions, context.num_constructions);
                    }
                }
            }
        }
    }
}


ARCHON_TEST(Core_Deque_Subscribe)
{
    core::Deque<int> deque;
    deque.push_back(1);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    deque.push_back(2);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    ARCHON_CHECK_EQUAL(deque[1], 2);
}


ARCHON_TEST(Core_Deque_RangeCheckingSubscribe)
{
    core::Deque<int> deque;
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_THROW(deque.at(0), std::out_of_range);
    deque.push_back(1);
    ARCHON_CHECK_EQUAL(deque.at(0), 1);
    ARCHON_CHECK_THROW(deque.at(1), std::out_of_range);
    deque.push_back(2);
    ARCHON_CHECK_EQUAL(deque.at(0), 1);
    ARCHON_CHECK_EQUAL(deque.at(1), 2);
    ARCHON_CHECK_THROW(deque.at(2), std::out_of_range);
    ARCHON_CHECK_THROW(deque.at(std::size_t(-1)), std::out_of_range);
}


ARCHON_TEST(Core_Deque_ConstructFromInitializerList)
{
    core::Deque<int> deque { 1, 2, 3 };
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    ARCHON_CHECK_EQUAL(deque[1], 2);
    ARCHON_CHECK_EQUAL(deque[2], 3);
}


ARCHON_TEST(Core_Deque_AssignFromInitializerList)
{
    core::Deque<int> deque;
    deque = { 1, 2, 3 };
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK_EQUAL(deque[0], 1);
    ARCHON_CHECK_EQUAL(deque[1], 2);
    ARCHON_CHECK_EQUAL(deque[2], 3);
    deque.assign({ 4, 5, 6, 7 });
    ARCHON_CHECK_NOT(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 4);
    ARCHON_CHECK_EQUAL(deque[0], 4);
    ARCHON_CHECK_EQUAL(deque[1], 5);
    ARCHON_CHECK_EQUAL(deque[2], 6);
    ARCHON_CHECK_EQUAL(deque[3], 7);
}


ARCHON_TEST(Core_Deque_Clear)
{
    core::Deque<int> deque { 1, 2, 3 };
    std::size_t capacity = deque.capacity();
    deque.clear();
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
    deque = { 2, 3 };
    deque.clear();
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.size(), 0);
    ARCHON_CHECK_EQUAL(deque.capacity(), capacity);
}


ARCHON_TEST(Core_Deque_Comparison)
{
    core::Deque<int> deque_1 { 1, 2 };
    core::Deque<int> deque_2 { 1, 2, 3 };
    core::Deque<int> deque_3 { 1, 2, 4 };
    core::Deque<int> deque_4 { 1, 2, 4 };

    ARCHON_CHECK_NOT(deque_1 == deque_2);
    ARCHON_CHECK_NOT(deque_2 == deque_3);
    ARCHON_CHECK(deque_3 == deque_4);
    ARCHON_CHECK_NOT(deque_4 == deque_1);

    ARCHON_CHECK(deque_1 != deque_2);
    ARCHON_CHECK(deque_2 != deque_3);
    ARCHON_CHECK_NOT(deque_3 != deque_4);
    ARCHON_CHECK(deque_4 != deque_1);

    ARCHON_CHECK(deque_1 < deque_2);
    ARCHON_CHECK(deque_2 < deque_3);
    ARCHON_CHECK_NOT(deque_3 < deque_4);
    ARCHON_CHECK_NOT(deque_4 < deque_1);

    ARCHON_CHECK(deque_1 <= deque_2);
    ARCHON_CHECK(deque_2 <= deque_3);
    ARCHON_CHECK(deque_3 <= deque_4);
    ARCHON_CHECK_NOT(deque_4 <= deque_1);

    ARCHON_CHECK_NOT(deque_1 > deque_2);
    ARCHON_CHECK_NOT(deque_2 > deque_3);
    ARCHON_CHECK_NOT(deque_3 > deque_4);
    ARCHON_CHECK(deque_4 > deque_1);

    ARCHON_CHECK_NOT(deque_1 >= deque_2);
    ARCHON_CHECK_NOT(deque_2 >= deque_3);
    ARCHON_CHECK(deque_3 >= deque_4);
    ARCHON_CHECK(deque_4 >= deque_1);
}


ARCHON_TEST(Core_Deque_CopyConstruct)
{
    core::Deque<int> deque_1 { 1, 2, 3 };
    core::Deque<int> deque_2 = deque_1;
    ARCHON_CHECK_EQUAL(deque_2.size(), 3);
    ARCHON_CHECK(deque_2 == deque_1);
}


ARCHON_TEST(Core_Deque_CopyAssign)
{
    core::Deque<int> deque_1 { 1, 2, 3 };
    core::Deque<int> deque_2 { 4, 5, 6 };
    deque_2 = deque_1; // Copy assign
    ARCHON_CHECK_EQUAL(deque_2.size(), 3);
    ARCHON_CHECK(deque_2 == deque_1);
}


ARCHON_TEST(Core_Deque_BeginEnd)
{
    std::vector<int> ref { 1, 2, 3 };
    core::Deque<int> deque { 1, 2, 3 };
    const core::Deque<int>& cdeque = deque;
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  deque.begin(),   deque.end()));
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  cdeque.begin(),  cdeque.end()));
    ARCHON_CHECK(std::equal(ref.begin(),  ref.end(),  deque.cbegin(),  deque.cend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), deque.rbegin(),  deque.rend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), cdeque.rbegin(), cdeque.rend()));
    ARCHON_CHECK(std::equal(ref.rbegin(), ref.rend(), deque.crbegin(), deque.crend()));
}


ARCHON_TEST(Core_Deque_ConstructFromSize)
{
    core::Deque<int> deque(3);
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::all_of(deque.begin(), deque.end(), [](int value) {
        return (value == 0);
    }));
}


ARCHON_TEST(Core_Deque_ConstructFromSizeAndValue)
{
    core::Deque<int> deque(3, 7);
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::all_of(deque.begin(), deque.end(), [](int value) {
        return (value == 7);
    }));
}


ARCHON_TEST(Core_Deque_AssignFromSizeAndValue)
{
    core::Deque<int> deque;
    deque.assign(3, 7);
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::all_of(deque.begin(), deque.end(), [](int value) {
        return (value == 7);
    }));
}


ARCHON_TEST(Core_Deque_ConstructFromNonrandomAccessIterator)
{
    std::list<int> ref { 1, 2, 3 };
    core::Deque<int> deque(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::equal(deque.begin(), deque.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_Deque_ConstructFromRandomAccessIterator)
{
    std::vector<int> ref { 1, 2, 3 };
    core::Deque<int> deque(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::equal(deque.begin(), deque.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_Deque_AssignFromNonrandomAccessIterator)
{
    std::list<int> ref { 1, 2, 3 };
    core::Deque<int> deque { 4, 5, 6 };
    deque.assign(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::equal(deque.begin(), deque.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_Deque_AssignFromRandomAccessIterator)
{
    std::vector<int> ref { 1, 2, 3 };
    core::Deque<int> deque { 4, 5, 6 };
    deque.assign(ref.begin(), ref.end());
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    ARCHON_CHECK(std::equal(deque.begin(), deque.end(), ref.begin(), ref.end()));
}


ARCHON_TEST(Core_Deque_MoveConstruct)
{
    core::Deque<int> deque_1 { 1, 2, 3 };
    core::Deque<int> deque_2(std::move(deque_1));
    ARCHON_CHECK_EQUAL(deque_1.size(), 0);
    ARCHON_CHECK_EQUAL(deque_2.size(), 3);
    ARCHON_CHECK_EQUAL(deque_2[0], 1);
    ARCHON_CHECK_EQUAL(deque_2[1], 2);
    ARCHON_CHECK_EQUAL(deque_2[2], 3);
}


ARCHON_TEST(Core_Deque_MoveAssign)
{
    core::Deque<int> deque_1 { 1, 2, 3 };
    core::Deque<int> deque_2 { 4, 5, 6 };
    deque_2 = std::move(deque_1);
    ARCHON_CHECK_EQUAL(deque_1.size(), 0);
    ARCHON_CHECK_EQUAL(deque_2.size(), 3);
    ARCHON_CHECK_EQUAL(deque_2[0], 1);
    ARCHON_CHECK_EQUAL(deque_2[1], 2);
    ARCHON_CHECK_EQUAL(deque_2[2], 3);
}


ARCHON_TEST(Core_Deque_Append)
{
    core::Deque<int> deque;
    deque.append({ 1, 2, 3 });
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 2, 3 }));
    deque.append({ 4, 5, 6 });
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 2, 3, 4, 5, 6 }));
    deque.clear();
    deque.append(3, 1);
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 1, 1 }));
    deque.append(3, 2);
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 1, 1, 2, 2, 2 }));
    deque.clear();
    std::vector<int> ref { 1, 2, 3 };
    deque.append(ref.begin(), ref.end());
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 2, 3 }));
    deque.append(ref.begin(), ref.end());
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 2, 3, 1, 2, 3 }));
    deque.clear();
    std::list<int> ref_2 { 3, 2, 1 };
    deque.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK(deque == core::Deque<int>({ 3, 2, 1 }));
    deque.append(ref_2.begin(), ref_2.end());
    ARCHON_CHECK(deque == core::Deque<int>({ 3, 2, 1, 3, 2, 1 }));
}


ARCHON_TEST(Core_Deque_IteratorEquality)
{
    core::Deque<int> deque;
    core::Deque<int>& cdeque = deque;
    ARCHON_CHECK(deque.begin()  == deque.end());
    ARCHON_CHECK(deque.cbegin() == deque.cend());
    ARCHON_CHECK(deque.begin()  == deque.cend());
    ARCHON_CHECK(deque.cbegin() == deque.end());
    ARCHON_CHECK(cdeque.begin() == cdeque.end());
    ARCHON_CHECK(deque.begin()  == cdeque.end());
    ARCHON_CHECK(cdeque.begin() == deque.end());
    ARCHON_CHECK_NOT(deque.begin()  != deque.end());
    ARCHON_CHECK_NOT(deque.cbegin() != deque.cend());
    ARCHON_CHECK_NOT(deque.begin()  != deque.cend());
    ARCHON_CHECK_NOT(deque.cbegin() != deque.end());
    ARCHON_CHECK_NOT(cdeque.begin() != cdeque.end());
    ARCHON_CHECK_NOT(deque.begin()  != cdeque.end());
    ARCHON_CHECK_NOT(cdeque.begin() != deque.end());
    deque.push_back(0);
    ARCHON_CHECK_NOT(deque.begin()  == deque.end());
    ARCHON_CHECK_NOT(deque.cbegin() == deque.cend());
    ARCHON_CHECK_NOT(deque.begin()  == deque.cend());
    ARCHON_CHECK_NOT(deque.cbegin() == deque.end());
    ARCHON_CHECK_NOT(cdeque.begin() == cdeque.end());
    ARCHON_CHECK_NOT(deque.begin()  == cdeque.end());
    ARCHON_CHECK_NOT(cdeque.begin() == deque.end());
    ARCHON_CHECK(deque.begin()  != deque.end());
    ARCHON_CHECK(deque.cbegin() != deque.cend());
    ARCHON_CHECK(deque.begin()  != deque.cend());
    ARCHON_CHECK(deque.cbegin() != deque.end());
    ARCHON_CHECK(cdeque.begin() != cdeque.end());
    ARCHON_CHECK(deque.begin()  != cdeque.end());
    ARCHON_CHECK(cdeque.begin() != deque.end());
}


ARCHON_TEST(Core_Deque_IteratorOperations)
{
    core::Deque<int> deque { 1, 2, 3 };
    auto i_1 = deque.begin();
    auto i_2 = 1 + i_1;
    ARCHON_CHECK_EQUAL(*i_2, 2);
}


ARCHON_TEST(Core_Deque_Resize)
{
    core::Deque<int> deque;
    deque.resize(0);
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.capacity(), 0);
    deque.resize(0, 7);
    ARCHON_CHECK(deque.empty());
    ARCHON_CHECK_EQUAL(deque.capacity(), 0);
    deque.resize(3);
    std::size_t cap = deque.capacity();
    ARCHON_CHECK_GREATER_EQUAL(cap, 3);
    ARCHON_CHECK(deque == core::Deque<int>({ 0, 0, 0 }));
    deque.resize(1);
    ARCHON_CHECK_EQUAL(deque.capacity(), cap);
    ARCHON_CHECK(deque == core::Deque<int>({ 0 }));
    deque.resize(0, 7);
    ARCHON_CHECK_EQUAL(deque.capacity(), cap);
    ARCHON_CHECK(deque == core::Deque<int>());
    deque.resize(3, 7);
    ARCHON_CHECK_EQUAL(deque.capacity(), cap);
    ARCHON_CHECK(deque == core::Deque<int>({ 7, 7, 7 }));
    deque.resize(4, 8);
    ARCHON_CHECK(deque == core::Deque<int>({ 7, 7, 7, 8 }));
    deque.pop_front();
    deque.resize(4, 9);
    ARCHON_CHECK(deque == core::Deque<int>({ 7, 7, 8, 9 }));
    deque.resize(2, 10);
    ARCHON_CHECK(deque == core::Deque<int>({ 7, 7 }));
    deque.resize(3);
    ARCHON_CHECK(deque == core::Deque<int>({ 7, 7, 0 }));
}


ARCHON_TEST(Core_Deque_ShrinkToFit)
{
    core::Deque<int> deque;
    deque.shrink_to_fit();
    ARCHON_CHECK(deque.empty());
    deque.push_back(1);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 1);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 1);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 1);
    deque.push_back(2);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 2);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 2);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 2);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 2);
    deque.push_back(3);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 3);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 3);
    deque.push_back(4);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 4);
    deque.shrink_to_fit();
    ARCHON_CHECK_EQUAL(deque.size(), 4);
    ARCHON_CHECK_GREATER_EQUAL(deque.capacity(), 4);
    ARCHON_CHECK(deque == core::Deque<int>({ 1, 2, 3, 4 }));
}


ARCHON_TEST(Core_Deque_Swap)
{
    core::Deque<int> deque_1 { 1, 2, 3 };
    core::Deque<int> deque_2 { 4, 5 };
    swap(deque_1, deque_2);
    ARCHON_CHECK(deque_1 == core::Deque<int>({ 4, 5 }));
    ARCHON_CHECK(deque_2 == core::Deque<int>({ 1, 2, 3 }));
}


ARCHON_TEST(Core_Deque_ExceptionSafetyInConstructFromIteratorPair)
{
    struct Context {
        bool start_counting_copy_ops = false;
        int num_copy_ops = 0;
        int num_instances = 0;
    };

    class Elem {
    public:
        explicit Elem(Context& context)
            : m_context(context)
        {
            ++m_context.num_instances;
        }
        Elem(const Elem& other)
            : m_context(other.m_context)
        {
            if (m_context.start_counting_copy_ops) {
                if (++m_context.num_copy_ops == 2)
                    throw std::bad_alloc();
            }
            ++m_context.num_instances;
        }
        ~Elem()
        {
            --m_context.num_instances;
        }
    private:
        Context& m_context;
    };

    class Iter {
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = Elem;
        using pointer           = Elem*;
        using reference         = Elem&;
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
        Elem arr[3] = {
            Elem(context),
            Elem(context),
            Elem(context),
        };
        context.start_counting_copy_ops = true;
        ARCHON_CHECK_THROW(core::Deque<Elem>(Iter(arr), Iter(arr + 3)), std::bad_alloc);
    }
    ARCHON_CHECK_EQUAL(context.num_instances, 0);
}


ARCHON_TEST(Foo)
{
/*
    char32_t chars[] = {
        0x0, // 0x10348,
    };
*/
    std::u32string_view str_1;
    std::u32string_view str_2; //  = { chars, std::size(chars) };
    ARCHON_CHECK_NOT_EQUAL(str_1, str_2);
}
