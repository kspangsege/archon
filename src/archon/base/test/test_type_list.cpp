// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include <archon/base/type_list.hpp>
#include <archon/unit_test.hpp>


using namespace archon;
using namespace archon::base;


namespace {


using Types1 = TypeList<short, int, unsigned>;
static_assert(base::type_count<Types1> == 3);
static_assert(std::is_same_v<TypeAt<Types1, 0>, short>);
static_assert(std::is_same_v<TypeAt<Types1, 1>, int>);
static_assert(std::is_same_v<TypeAt<Types1, 2>, unsigned>);


using Types2 = TypeAppend<Types1, long>;
static_assert(base::type_count<Types2> == 4);
static_assert(std::is_same_v<TypeAt<Types2, 0>, short>);
static_assert(std::is_same_v<TypeAt<Types2, 1>, int>);
static_assert(std::is_same_v<TypeAt<Types2, 2>, unsigned>);
static_assert(std::is_same_v<TypeAt<Types2, 3>, long>);


template<class T> struct Predicate1 {
    static constexpr bool value = std::is_unsigned_v<T>;
};
template<class T> struct Predicate2 {
    static constexpr bool value = std::is_pointer_v<T>;
};
static_assert(std::is_same_v<FindType<Types1, Predicate1>, unsigned>);
static_assert(std::is_same_v<FindType<Types1, Predicate2>, void>);


template<class T, int i> struct Predicate3 {
    static bool exec(std::vector<std::pair<const std::type_info*, int>>* types)
    {
        if (std::is_unsigned_v<T>)
            return true;
        types->emplace_back(&typeid(T), i);
        return false;
    }
};


template<class T, int i> struct Predicate4 {
    static bool exec(std::vector<std::pair<const std::type_info*, int>>* types)
    {
        types->emplace_back(&typeid(T), i);
        return false;
    }
};


template<class T, int i> struct Function {
    static void exec(std::vector<std::pair<const std::type_info*, int>>* types)
    {
        types->emplace_back(&typeid(T), i);
    }
};


} // unnamed namespace


ARCHON_TEST(Base_TypeList_HasType)
{
    std::vector<std::pair<const std::type_info*, int>> types_1;
    ARCHON_CHECK((base::has_type<Types1, Predicate3>(&types_1)));
    std::vector expected_1 {
        std::pair(&typeid(short), 0),
        std::pair(&typeid(int),   1)
    };
    ARCHON_CHECK(types_1 == expected_1);
    std::vector<std::pair<const std::type_info*, int>> types_2;
    ARCHON_CHECK_NOT((base::has_type<Types1, Predicate4>(&types_2)));
    std::vector expected_2 {
        std::pair(&typeid(short),    0),
        std::pair(&typeid(int),      1),
        std::pair(&typeid(unsigned), 2)
    };
    ARCHON_CHECK(types_2 == expected_2);
}


ARCHON_TEST(Base_TypeList_ForEachType)
{
    std::vector<std::pair<const std::type_info*, int>> types;
    base::for_each_type<Types1, Function>(&types);
    std::vector expected {
        std::pair(&typeid(short),    0),
        std::pair(&typeid(int),      1),
        std::pair(&typeid(unsigned), 2)
    };
    ARCHON_CHECK(types == expected);
}
