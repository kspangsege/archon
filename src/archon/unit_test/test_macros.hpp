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

#ifndef ARCHON_X_UNIT_TEST_X_TEST_MARCOS_HPP
#define ARCHON_X_UNIT_TEST_X_TEST_MARCOS_HPP

/// \file


#include <string_view>

#include <archon/unit_test/test_list.hpp>


/// \brief Define and register a unit test.
///
/// This macro is a shorthard for defining a function and registering it as a
/// unit test (\ref archon::unit_test::TestList::add()).
///
/// For example, a unit test named `Foo` can be defined and registered like
/// this:
///
/// \code{.cpp}
///
///   ARCHON_TEST(Foo)
///   {
///       // ...
///       ARCHON_CHECK_EQUAL(a, b);
///       log("<%s|%s>", a, b);
///   }
///
/// \endcode
///
/// This is equivalent to the following, where `line_number` is assumed to be
/// the line number of `foo()`:
///
/// \code{.cpp}
///
///   void foo(archon::unit_test::TestContext& test_context)
///   {
///       // ...
///       ARCHON_CHECK_EQUAL(a, b);
///       test_context.logger.info("<%s|%s>", a, b);
///   }
///
///   int main()
///   {
///       archon::unit_test::TestList::get_default_list().add("Foo", __FILE__, line_number, &foo);
///       // ...
///   }
///
/// \endcode
///
/// When using `ARCHON_TEST()`, a variable `test_context` of type `TestContext&`
/// is implicitly available (\ref
/// archon::unit_test::TestContext). Checking-macros such as \ref
/// ARCHON_CHECK_EQUAL() require that a variable of that name and type is
/// available. Provided that you pass along the reference to the `TestContext`
/// object, you can invoke checking-macros from anywhere. However, behaviour is
/// undefined if the `TestContext` object is accessed (via checking-macros or
/// otherwise) after return from the unit test.
///
/// When using `ARCHON_TEST()`, a function named `log()` is implicitly
/// available, and can be used as a shorthand for logging at "info" level, as
/// shown above. Its arguments have the same meaning as \ref
/// archon::base::BasicLogger::info(). Additionally, functions `log_fatal()`,
/// `log_error()`, `log_warn()`, `log_info()`, `log_detail()`, `log_debug()`,
/// and `log_trace()` are implicitly available and log at the log levels
/// indicated by their names.
///
/// The log level limit that applies when logging from inside unit tests is
/// determined by \ref archon::unit_test::TestConfig::inner_log_level_limit.
///
/// It is an error to register two tests with the same name in the same list.
///
/// All files created by, or on behalf of unit tests should be managed by a test
/// file guards. Among other things, this ensures that they will be deleted when
/// they should be. See \ref ARCHON_TEST_FILE() and \ref ARCHON_TEST_DIR() for
/// details.
///
/// \sa \ref ARCHON_TEST_IF()
/// \sa \ref ARCHON_NONCONCURRENT_TEST()
/// \sa \ref ARCHON_TEST_EX()
///
#define ARCHON_TEST(name)                       \
    ARCHON_TEST_IF(name, true)


/// \brief Define, register, and conditionally enable a unit test.
///
/// This macro is like \ref ARCHON_TEST() except that it allows you to control
/// whether the test will be enabled or disabled at runtime. The test will be
/// compiled in any case. You can pass any expression that would be a valid
/// condition in an `if` statement. The expression is not evaluated until you
/// call \ref archon::unit_test::TestList::run(). This allows you to base the
/// condition on global variables which can then be adjusted before calling \ref
/// archon::unit_test::TestList::run().
///
/// \sa \ref ARCHON_NONCONCURRENT_TEST_IF()
///
#define ARCHON_TEST_IF(name, enabled)                                   \
    ARCHON_TEST_EX(archon::unit_test::TestList::get_default_list(), name, enabled, true)


/// \brief Define and register a nonconcurrent unit test.
///
/// This macro is like \ref ARCHON_TEST() except that it declares the unit test
/// to be of the "nonconcurrent" type. This means that it will execute at a time
/// where no other test is executing (neither a diffrent unit test nor a
/// different execution of the same unit test). Use this for tests that cannot
/// safely execute concurrently with other tests, such as tests that rely on
/// unprotected non-constant global state.
///
/// Nonconcurrent tests will always be executed by the thread that calls \ref
/// archon::unit_test::TestList::run().
///
/// \sa \ref ARCHON_NONCONCURRENT_TEST_IF()
///
#define ARCHON_NONCONCURRENT_TEST(name)         \
    ARCHON_NONCONCURRENT_TEST_IF(name, true)


/// \brief Define, register, and conditionally enable a nonconcurrent unit test.
///
/// This macro is like \ref ARCHON_TEST() except that it declares the unit test
/// to be of the "nonconcurrent" type, and that it allows you to control whether
/// the test will be enabled or disabled at runtime. See \ref ARCHON_TEST_IF()
/// and \ref ARCHON_NONCONCURRENT_TEST() for details.
///
#define ARCHON_NONCONCURRENT_TEST_IF(name, enabled)                     \
    ARCHON_TEST_EX(archon::unit_test::TestList::get_default_list(), name, enabled, false)


/// \brief Define and register a unit test.
///
/// This macro is like \ref ARCHON_TEST() except that it allows you to specify
/// which list the test is to be added to (\p list), to control whether the test
/// will be enabled or disabled at runtime (\p enable), and to control whether
/// the test is of the "concurrent" or "nonconcurrent" type (\p allow_concur).
///
/// The specified test list must be an object of type \ref
/// archon::unit_test::TestList. The macros, such as \ref ARCHON_TEST(), that do
/// not take a \p list argument, add the unit test to the default list, which is
/// \ref archon::unit_test::TestList::get_default_list().
///
/// \sa \ref ARCHON_TEST_IF() and \ref ARCHON_NONCONCURRENT_TEST().
///
#define ARCHON_TEST_EX(list, name, enabled, allow_concur)       \
    X_ARCHON_TEST_EX(list, name, enabled, allow_concur)








// Implementation


#define X_ARCHON_TEST_EX(list, name, enabled, allow_concur)             \
    namespace {                                                         \
    class Archon_UnitTest_##name :                                      \
            public archon::unit_test::detail::TestBase {                \
    public:                                                             \
        static bool test_enabled()                                      \
        {                                                               \
            return bool(enabled);                                       \
        }                                                               \
        Archon_UnitTest_##name(archon::unit_test::TestContext& c) noexcept : \
            TestBase(c)                                                 \
        {                                                               \
        }                                                               \
        void test_run();                                                \
    };                                                                  \
    archon::unit_test::detail::RegisterTest<Archon_UnitTest_##name>     \
        archon_unit_test_reg_##name(list, #name, __FILE__, __LINE__, allow_concur); \
    }                                                                   \
    void Archon_UnitTest_##name::test_run()


namespace archon::unit_test::detail {


class TestBase {
protected:
    TestContext& test_context;

    template<class... P> void log(const char* message, const P&... params)
    {
        log_info(message, params...); // Throws
    }

    template<class... P> void log_fatal(const char* message, const P&... params)
    {
        test_context.logger.fatal(message, params...); // Throws
    }

    template<class... P> void log_error(const char* message, const P&... params)
    {
        test_context.logger.error(message, params...); // Throws
    }

    template<class... P> void log_warn(const char* message, const P&... params)
    {
        test_context.logger.warn(message, params...); // Throws
    }

    template<class... P> void log_info(const char* message, const P&... params)
    {
        test_context.logger.info(message, params...); // Throws
    }

    template<class... P> void log_detail(const char* message, const P&... params)
    {
        test_context.logger.detail(message, params...); // Throws
    }

    template<class... P> void log_debug(const char* message, const P&... params)
    {
        test_context.logger.debug(message, params...); // Throws
    }

    template<class... P> void log_trace(const char* message, const P&... params)
    {
        test_context.logger.trace(message, params...); // Throws
    }

    TestBase(TestContext& tc) noexcept :
        test_context(tc)
    {
    }
};


template<class Test> class RegisterTest {
public:
    RegisterTest(TestList& list, std::string_view name, const char* file_path, long line_number,
                 bool allow_concur)
    {
        list.add(name, file_path, line_number, &RegisterTest::run_test,
                 &Test::test_enabled, allow_concur); // Throws
    }

    static void run_test(TestContext& test_context)
    {
        Test test(test_context);
        test.test_run(); // Throws
    }
};


} // namespace archon::unit_test::detail

#endif // ARCHON_X_UNIT_TEST_X_TEST_MARCOS_HPP
