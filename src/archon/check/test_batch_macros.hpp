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

#ifndef ARCHON_X_CHECK_X_TEST_BATCH_MARCOS_HPP
#define ARCHON_X_CHECK_X_TEST_BATCH_MARCOS_HPP

/// \file


#include <memory>
#include <string_view>
#include <string>
#include <tuple>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/string.hpp>
#include <archon/check/test_list.hpp>
#include <archon/check/test_macros.hpp>


/// \brief Define an ordered set of test case variant specifiers.
///
/// This macro is a shorthand for creating a tuple with the specified name, and consisting
/// of the specified components, which are the rest of the arguments. See \ref
/// ARCHON_TEST_BATCH() for an example of how to use it. See \ref ARCHON_TEST_TYPE() and
/// \ref ARCHON_TEST_VALUE() for ways to create type-like and value-like variant specifiers
/// respectively.
///
#define ARCHON_TEST_VARIANTS(name, ...) X_ARCHON_TEST_VARIANTS(name, __VA_ARGS__)


/// \brief Define a type-like test case variant specifier.
///
/// This macro defines a variant specifier where the test type is the specified type, and
/// the test value is \ref core::Empty. See \ref ARCHON_TEST_BATCH() for an example of how
/// to use it.
///
/// \sa \ref ARCHON_TEST_VALUE()
///
#define ARCHON_TEST_TYPE(type, name) X_ARCHON_TEST_TYPE(type, name)


/// \brief Define a value-like test case variant specifier.
///
/// This macro defines a variant specifier where the test type is the type of the specified
/// value, and the test value is the specified value.
///
/// \sa \ref ARCHON_TEST_TYPE()
///
#define ARCHON_TEST_VALUE(value, name) X_ARCHON_TEST_VALUE(value, name)



/// \brief Define and register a batch of test cases.
///
/// This macro is like \ref ARCHON_TEST() except that it allows you to define and register
/// an entire batch of similar test cases, one for each of the specified variants. Here,
/// "similar" means that the tests vary only in a single type and/or a single compile-time
/// constant value.
///
/// A variant specifies a type and a value. The type is available as `test_type` from within
/// the test, and the value as `test_value`. The type of the test value need not be
/// `test_type`. The test value is always a compile-time constant.
///
/// For example, a batch of test cases named `Foo_Short`, `Foo_Int`, and `Foo_Long` can be
/// defined and registered like this:
///
/// Here is how you can use it:
///
/// \code{.cpp}
///
///   ARCHON_TEST_VARIANTS(variants,
///                        ARCHON_TEST_TYPE(short, Short),
///                        ARCHON_TEST_TYPE(int,   Int),
///                        ARCHON_TEST_TYPE(long,  Long))
///
///   ARCHON_TEST_BATCH(Foo, variants)
///   {
///       // ...
///       ARCHON_CHECK_EQUAL(foo<test_type>(x), y);
///   }
///
///   int main()
///   {
///       archon::check::run();
///   }
///
/// \endcode
///
/// This is roughly equivalent to the following:
///
/// \code{.cpp}
///
///   template<class T> void foo(archon::check::TestContext& test_context)
///   {
///       using test_type = T;
///       // ...
///       ARCHON_CHECK_EQUAL(foo<test_type>(x), y);
///   }
///
///   int main()
///   {
///       auto& list = archon::check::TestList::get_default_list();
///       list.add("FooShort", __FILE__, __LINE__, &foo<short>);
///       list.add("FooInt",   __FILE__, __LINE__, &foo<int>);
///       list.add("FooLong",  __FILE__, __LINE__, &foo<long>);
///       archon::check::run();
///   }
///
/// \endcode
///
/// The batch name, that is passed to `ARCHON_TEST_BATCH()`, functions as the common prefix
/// for the names of the individual tests. This batch name does not have to be unique, but
/// the names of the individual tests do, as usual. Indeed, you can define two batches with
/// the same batch name as long as the two sets of variant names are disjoint.
///
/// The \p variants argument must be an object whose type is some instantiation of
/// `std::tuple`, and each of its compoents can be of any type, as long as they define the
/// three members `type`, `value`, and `name` for the test type, test value, and variant
/// name respectively. The test value must be a compile time constant, and the name must be
/// something that can be implicitly converted to `std::string_view`.
///
#define ARCHON_TEST_BATCH(name, variants) ARCHON_TEST_BATCH_IF(name, variants, true)


/// \brief Define, register, and conditionally enable a batch of test cases.
///
/// This macro is like \ref ARCHON_TEST_BATCH() except that it allows you to control whether
/// this batch of test cases will be enabled or disabled at runtime. See \ref
/// ARCHON_TEST_IF() for details on disabling of test cases.
///
/// \sa \ref ARCHON_NONCONC_TEST_BATCH_IF()
///
#define ARCHON_TEST_BATCH_IF(name, variants, enabled)                   \
    ARCHON_TEST_BATCH_EX(archon::check::TestList::get_default_list(), name, variants, enabled, true)


/// \brief Define and register a nonconcurrent batch of test cases.
///
/// This macro is like \ref ARCHON_TEST_BATCH() except that it declares this batch of test
/// cases to be of the "nonconcurrent" type. See \ref ARCHON_NONCONC_TEST() for details on
/// nonconcurrent test cases.
///
/// \sa \ref ARCHON_NONCONC_TEST_BATCH_IF()
///
#define ARCHON_NONCONC_TEST_BATCH(name, variants) ARCHON_NONCONC_TEST_BATCH_IF(name, variants, true)


/// \brief Define, register, and conditionally enable a nonconcurrent batch of test cases.
///
/// This macro is like \ref ARCHON_TEST_BATCH() except that it declares this batch of test
/// cases to be of the "nonconcurrent" type, and that it allows you to control whether it
/// will be enabled or disabled at runtime. See \ref ARCHON_TEST_BATCH_IF() and \ref
/// ARCHON_NONCONC_TEST_BATCH() for details.
///
#define ARCHON_NONCONC_TEST_BATCH_IF(name, variants, enabled)           \
    ARCHON_TEST_BATCH_EX(archon::check::TestList::get_default_list(), name, variants, enabled, false)


/// \brief Define and register a batch of test cases with full control.
///
/// This macro is like \ref ARCHON_TEST_BATCH() except that it allows you to specify which
/// list this batch of test cases is to be added to (\p list), to control whether it will be
/// enabled or disabled at runtime (\p enable), and to control whether it is of the
/// "concurrent" or "nonconcurrent" type (\p allow_concur).
///
/// See \ref ARCHON_TEST_EX() for more on the test list argument.
///
/// \sa \ref ARCHON_TEST_BATCH_IF() and \ref ARCHON_NONCONC_TEST_BATCH().
///
#define ARCHON_TEST_BATCH_EX(list, name, variants, enabled, allow_concur) \
    X_ARCHON_TEST_BATCH(list, name, variants, enabled, allow_concur)








// Implementation


#define X_ARCHON_TEST_VARIANTS(name, ...)       \
    auto name = std::make_tuple(__VA_ARGS__)


#define X_ARCHON_TEST_TYPE(type, name)                  \
    archon::check::impl::TestType<type>(#name)


#define X_ARCHON_TEST_VALUE(value, name)                                \
    (archon::check::impl::TestValue<decltype(value), value>(#name))


#define X_ARCHON_TEST_BATCH(list, name, variants, enabled, allow_concur) \
    X_ARCHON_TEST_BATCH_2(list, name, variants, enabled, allow_concur,  \
                          ARCHON_CONCAT_4(Archon_Check_, __LINE__, _, name), \
                          ARCHON_CONCAT_4(archon_check_reg_, __LINE__, _, name))


#define X_ARCHON_TEST_BATCH_2(list, name, variants, enabled, allow_concur, cname, vname) \
    namespace {                                                         \
    template<class V> class cname                                       \
        : public archon::check::impl::TestBase {                        \
    public:                                                             \
        static bool archon_check_enabled()                              \
        {                                                               \
            return bool(enabled);                                       \
        }                                                               \
        using archon::check::impl::TestBase::TestBase;                  \
        using archon::check::impl::TestBase::test_context;              \
        using archon::check::impl::TestBase::log;                       \
        using archon::check::impl::TestBase::log_fatal;                 \
        using archon::check::impl::TestBase::log_error;                 \
        using archon::check::impl::TestBase::log_warn;                  \
        using archon::check::impl::TestBase::log_info;                  \
        using archon::check::impl::TestBase::log_detail;                \
        using archon::check::impl::TestBase::log_debug;                 \
        using archon::check::impl::TestBase::log_trace;                 \
        using test_type = typename V::type;                             \
        static constexpr auto test_value = V::value;                    \
        void archon_check_run();                                        \
    };                                                                  \
    archon::check::impl::RegisterTestBatch<cname> vname(list, #name, variants, __FILE__, __LINE__, allow_concur); \
    }                                                                   \
    template<class T> void cname<T>::archon_check_run()


namespace archon::check::impl {


template<class T> class TestType {
public:
    using type = T;
    static constexpr core::Empty value = {};
    std::string_view name;
    TestType(std::string_view n)
        : name(n)
    {
    }
};


template<class T, T v> class TestValue {
public:
    using type = T;
    static constexpr T value = v;
    std::string_view name;
    TestValue(std::string_view n)
        : name(n)
    {
    }
};


template<template<class> class T> class RegisterTestBatch {
public:
    template<class L> RegisterTestBatch(TestList& list, std::string_view name, const L& variants,
                                        const char* file_path, long line_number, bool allow_concur)
        : m_names(std::make_unique<std::string[]>(std::tuple_size_v<L>)) // Throws
    {
        Helper helper(*this, list, name, file_path, line_number, allow_concur);
        auto func = [&](const auto&... args) {
            helper.register_tests(0, args...); // Throws
        };
        std::apply(func, variants); // Throws
    }

private:
    class Helper;
    std::unique_ptr<std::string[]> m_names;
};


template<template<class> class T>
class RegisterTestBatch<T>::Helper {
public:
    Helper(RegisterTestBatch& reg, TestList& list, std::string_view name, const char* file_path, long line_number,
           bool allow_concur) noexcept
        : m_reg(reg)
        , m_list(list)
        , m_name(name)
        , m_file_path(file_path)
        , m_line_number(line_number)
        , m_allow_concur(allow_concur)
    {
    }

    void register_tests(int) noexcept
    {
    }

    template<class V, class... W> void register_tests(int i, const V& v, const W&... variants)
    {
        m_reg.m_names[i] = core::concat(m_name, std::string_view("_"), std::string_view(v.name)); // Throws
        m_list.add(m_reg.m_names[i], m_file_path, m_line_number, &run_test<T<V>>, &T<V>::archon_check_enabled,
                   m_allow_concur); // Throws
        register_tests(i + 1, variants...); // Throws
    }

private:
    RegisterTestBatch& m_reg;
    TestList& m_list;
    std::string_view m_name;
    const char* m_file_path;
    long m_line_number;
    bool m_allow_concur;
};


} // namespace archon::check::impl

#endif // ARCHON_X_CHECK_X_TEST_BATCH_MARCOS_HPP
