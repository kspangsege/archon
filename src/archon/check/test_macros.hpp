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

#ifndef ARCHON_X_CHECK_X_TEST_MARCOS_HPP
#define ARCHON_X_CHECK_X_TEST_MARCOS_HPP

/// \file


#include <string_view>

#include <archon/core/features.h>
#include <archon/check/test_list.hpp>


/// \brief Define and register a test case.
///
/// This macro is a shorthard for defining a function and registering it as a test case
/// (\ref archon::check::TestList::add()). See also \ref ARCHON_TEST_BATCH() for a way to
/// easily define and register an entire batch of similar test cases.
///
/// For example, a test case named `Foo` can be defined and registered like this:
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
///   void foo(archon::check::TestContext& test_context)
///   {
///       // ...
///       ARCHON_CHECK_EQUAL(a, b);
///       test_context.logger.info("<%s|%s>", a, b);
///   }
///
///   int main()
///   {
///       auto& list = archon::check::TestList::get_default_list();
///       list.add("Foo", __FILE__, __LINE__, &foo);
///       archon::check::run();
///   }
///
/// \endcode
///
/// When using `ARCHON_TEST()`, a variable `test_context` of type `TestContext&` is
/// implicitly available (\ref archon::check::TestContext). Checking-macros such as \ref
/// ARCHON_CHECK_EQUAL() require that a variable of that name and type is
/// available. Provided that you pass along the reference to the `TestContext` object, you
/// can invoke checking-macros from anywhere. However, behaviour is undefined if the
/// `TestContext` object is accessed (via checking-macros or otherwise) after return from
/// the test case.
///
/// When using `ARCHON_TEST()`, a function named `log()` is implicitly available, and can be
/// used as a shorthand for logging at "info" level, as shown above. Its arguments have the
/// same meaning as \ref archon::log::BasicLogger::info(). Additionally, functions
/// `log_fatal()`, `log_error()`, `log_warn()`, `log_info()`, `log_detail()`, `log_debug()`,
/// and `log_trace()` are implicitly available and log at the log levels indicated by their
/// names.
///
/// The log level limit that applies when logging from inside test cases is determined by
/// \ref archon::check::TestConfig::inner_log_level_limit.
///
/// It is an error to register two tests with the same name in the same list. Doing so will
/// cause an exception to be thrown by \ref run().
///
/// All files created by, or on behalf of test cases should be managed by test file
/// guards. Among other things, this ensures that they will be deleted when they should
/// be. See \ref ARCHON_TEST_FILE() and \ref ARCHON_TEST_DIR() for details.
///
/// \sa \ref ARCHON_TEST_IF()
/// \sa \ref ARCHON_NONCONC_TEST()
/// \sa \ref ARCHON_TEST_EX()
/// \sa \ref run()
///
#define ARCHON_TEST(name)                       \
    ARCHON_TEST_IF(name, true)


/// \brief Define, register, and conditionally enable a test case.
///
/// This macro is like \ref ARCHON_TEST() except that it allows you to control whether the
/// test will be enabled or disabled at runtime. The test will be compiled in any case. You
/// can pass any expression that would be a valid condition in an `if` statement. The
/// expression is not evaluated until you call \ref archon::check::TestList::run(). This
/// allows you to base the condition on global variables which can then be adjusted before
/// calling \ref archon::check::TestList::run().
///
/// \sa \ref ARCHON_NONCONC_TEST_IF()
///
#define ARCHON_TEST_IF(name, enabled)                                   \
    ARCHON_TEST_EX(archon::check::TestList::get_default_list(), name, enabled, true)


/// \brief Define and register a nonconcurrent test case.
///
/// This macro is like \ref ARCHON_TEST() except that it declares the test case to be of the
/// "nonconcurrent" type. This means that it will execute at a time where no other test is
/// executing (neither a diffrent test case nor a different execution of the same test
/// case). Use this for tests that cannot safely execute concurrently with other tests, such
/// as tests that rely on unprotected non-constant global state.
///
/// Nonconcurrent tests will always be executed by the thread that calls \ref
/// archon::check::TestList::run().
///
/// \sa \ref ARCHON_NONCONC_TEST_IF()
///
#define ARCHON_NONCONC_TEST(name)         \
    ARCHON_NONCONC_TEST_IF(name, true)


/// \brief Define, register, and conditionally enable a nonconcurrent test case.
///
/// This macro is like \ref ARCHON_TEST() except that it declares the test case to be of the
/// "nonconcurrent" type, and that it allows you to control whether the test case will be
/// enabled or disabled at runtime. See \ref ARCHON_TEST_IF() and \ref ARCHON_NONCONC_TEST()
/// for details.
///
#define ARCHON_NONCONC_TEST_IF(name, enabled)                     \
    ARCHON_TEST_EX(archon::check::TestList::get_default_list(), name, enabled, false)


/// \brief Define and register a test case with full control.
///
/// This macro is like \ref ARCHON_TEST() except that it allows you to specify which list
/// the test case is to be added to (\p list), to control whether the test case will be
/// enabled or disabled at runtime (\p enable), and to control whether the test case is of
/// the "concurrent" or "nonconcurrent" type (\p allow_concur).
///
/// The specified test list must be an object of type \ref archon::check::TestList. The
/// macros, such as \ref ARCHON_TEST(), that do not take a \p list argument, add the test
/// case to the default list, which is \ref archon::check::TestList::get_default_list().
///
/// \sa \ref ARCHON_TEST_IF() and \ref ARCHON_NONCONC_TEST().
///
#define ARCHON_TEST_EX(list, name, enabled, allow_concur)       \
    X_ARCHON_TEST(list, name, enabled, allow_concur)








// Implementation


#define X_ARCHON_TEST(list, name, enabled, allow_concur)                \
    X_ARCHON_TEST_2(list, name, enabled, allow_concur,                  \
                    ARCHON_CONCAT_4(Archon_Check_, __LINE__, _, name),  \
                    ARCHON_CONCAT_4(archon_check_reg_, __LINE__, _, name))


#define X_ARCHON_TEST_2(list, name, enabled, allow_concur, cname, vname) \
    namespace {                                                         \
    class cname                                                         \
        : archon::check::impl::TestBase {                               \
    public:                                                             \
        static bool archon_check_enabled()                              \
        {                                                               \
            return bool(enabled);                                       \
        }                                                               \
        using archon::check::impl::TestBase::TestBase;                  \
        void archon_check_run();                                        \
    };                                                                  \
    archon::check::impl::RegisterTest<cname> vname(list, #name, __FILE__, __LINE__, allow_concur); \
    }                                                                   \
    void cname::archon_check_run()


namespace archon::check::impl {


class TestBase {
public:
    check::TestContext& test_context;

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

    TestBase(check::TestContext& tc) noexcept
        : test_context(tc)
    {
    }
};


template<class T> void run_test(check::TestContext& test_context)
{
    T test(test_context);
    test.archon_check_run(); // Throws
}


template<class T> class RegisterTest {
public:
    RegisterTest(check::TestList& list, std::string_view name, const char* file_path, long line_number,
                 bool allow_concur)
    {
        list.add(name, file_path, line_number, &impl::run_test<T>, &T::archon_check_enabled, allow_concur); // Throws
    }
};


} // namespace archon::check::impl

#endif // ARCHON_X_CHECK_X_TEST_MARCOS_HPP
