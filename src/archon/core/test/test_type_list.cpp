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


#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

#include <archon/core/type_list.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


using Types1 = core::TypeList<short, int, unsigned>;
static_assert(core::type_count<Types1> == 3);
static_assert(std::is_same_v<core::TypeAt<Types1, 0>, short>);
static_assert(std::is_same_v<core::TypeAt<Types1, 1>, int>);
static_assert(std::is_same_v<core::TypeAt<Types1, 2>, unsigned>);


using Types2 = core::TypeAppend<Types1, long>;
static_assert(core::type_count<Types2> == 4);
static_assert(std::is_same_v<core::TypeAt<Types2, 0>, short>);
static_assert(std::is_same_v<core::TypeAt<Types2, 1>, int>);
static_assert(std::is_same_v<core::TypeAt<Types2, 2>, unsigned>);
static_assert(std::is_same_v<core::TypeAt<Types2, 3>, long>);


struct Predicate1 {
    template<class T> static constexpr bool value = std::is_unsigned_v<T>;
};
struct Predicate2 {
    template<class T> static constexpr bool value = std::is_pointer_v<T>;
};
static_assert(std::is_same_v<core::FindType<Types1, Predicate1>, unsigned>);
static_assert(std::is_same_v<core::FindType<Types1, Predicate2>, void>);


struct Predicate3 {
    template<class T, std::size_t I>
    static bool exec(std::vector<std::pair<const std::type_info*, std::size_t>>& types)
    {
        if (std::is_unsigned_v<T>)
            return true;
        types.emplace_back(&typeid(T), I);
        return false;
    }
};


struct Predicate4 {
    template<class T, std::size_t I>
    static bool exec(std::vector<std::pair<const std::type_info*, std::size_t>>& types)
    {
        types.emplace_back(&typeid(T), I);
        return false;
    }
};


struct Function1 {
    template<class T, std::size_t I>
    static void exec(std::vector<std::pair<const std::type_info*, std::size_t>>& types)
    {
        types.emplace_back(&typeid(T), I);
    }
};


template<class U> struct Function2 {
    template<class T, std::size_t I>
    static bool exec(std::vector<std::pair<const std::type_info*, std::size_t>>& types)
    {
        types.emplace_back(&typeid(T), I);
        return !std::is_same_v<T, U>;
    }
};


} // unnamed namespace


ARCHON_TEST(Core_TypeList_HasTypeA)
{
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_1;
    ARCHON_CHECK((core::has_type_a<Types1, Predicate3>(seen_1)));
    std::vector expected_1 {
        std::pair(&typeid(short), std::size_t(0)),
        std::pair(&typeid(int),   std::size_t(1))
    };
    ARCHON_CHECK(seen_1 == expected_1);
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_2;
    ARCHON_CHECK_NOT((core::has_type_a<Types1, Predicate4>(seen_2)));
    std::vector expected_2 {
        std::pair(&typeid(short),    std::size_t(0)),
        std::pair(&typeid(int),      std::size_t(1)),
        std::pair(&typeid(unsigned), std::size_t(2)),
    };
    ARCHON_CHECK(seen_2 == expected_2);
}


ARCHON_TEST(Core_TypeList_ForEachType)
{
    using types = core::TypeList<short, int, long>;
    std::vector<std::pair<const std::type_info*, std::size_t>> seen;
    core::for_each_type<types>([&](auto tag, std::size_t i) {
        using type = typename decltype(tag)::type;
        seen.emplace_back(&typeid(type), i);
    });
    std::vector expected {
        std::pair(&typeid(short), std::size_t(0)),
        std::pair(&typeid(int),   std::size_t(1)),
        std::pair(&typeid(long),  std::size_t(2)),
    };
    ARCHON_CHECK(seen == expected);
}


ARCHON_TEST(Core_TypeList_ForEachTypeA)
{
    using types = core::TypeList<short, int, long>;
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_1;
    ARCHON_CHECK(core::for_each_type_a<types>([&](auto tag, std::size_t i) {
        using type = typename decltype(tag)::type;
        seen_1.emplace_back(&typeid(type), i);
        return true;
    }));
    std::vector expected_1 {
        std::pair(&typeid(short), std::size_t(0)),
        std::pair(&typeid(int),   std::size_t(1)),
        std::pair(&typeid(long),  std::size_t(2)),
    };
    ARCHON_CHECK(seen_1 == expected_1);
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_2;
    ARCHON_CHECK_NOT(core::for_each_type_a<types>([&](auto tag, std::size_t i) {
        using type = typename decltype(tag)::type;
        seen_2.emplace_back(&typeid(type), i);
        return !std::is_same_v<type, int>;
    }));
    std::vector expected_2 {
        std::pair(&typeid(short), std::size_t(0)),
        std::pair(&typeid(int),   std::size_t(1)),
    };
    ARCHON_CHECK(seen_2 == expected_2);
}


ARCHON_TEST(Core_TypeList_ForEachTypeAlt)
{
    std::vector<std::pair<const std::type_info*, std::size_t>> seen;
    core::for_each_type_alt<Types1, Function1>(seen);
    std::vector expected {
        std::pair(&typeid(short),    std::size_t(0)),
        std::pair(&typeid(int),      std::size_t(1)),
        std::pair(&typeid(unsigned), std::size_t(2)),
    };
    ARCHON_CHECK(seen == expected);
}


ARCHON_TEST(Core_TypeList_ForEachTypeAltA)
{
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_1;
    ARCHON_CHECK((core::for_each_type_alt_a<Types1, Function2<long>>(seen_1)));
    std::vector expected_1 {
        std::pair(&typeid(short),    std::size_t(0)),
        std::pair(&typeid(int),      std::size_t(1)),
        std::pair(&typeid(unsigned), std::size_t(2)),
    };
    ARCHON_CHECK(seen_1 == expected_1);
    std::vector<std::pair<const std::type_info*, std::size_t>> seen_2;
    ARCHON_CHECK_NOT((core::for_each_type_alt_a<Types1, Function2<int>>(seen_2)));
    std::vector expected_2 {
        std::pair(&typeid(short), std::size_t(0)),
        std::pair(&typeid(int),   std::size_t(1)),
    };
    ARCHON_CHECK(seen_2 == expected_2);
}
