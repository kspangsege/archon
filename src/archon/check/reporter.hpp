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

#ifndef ARCHON_X_CHECK_X_REPORTER_HPP
#define ARCHON_X_CHECK_X_REPORTER_HPP

/// \file


#include <string_view>

#include <archon/log/logger.hpp>
#include <archon/check/root_context.hpp>
#include <archon/check/thread_context.hpp>
#include <archon/check/test_context.hpp>
#include <archon/check/fail_context.hpp>


namespace archon::check {


/// \brief Summary of testing process.
///
/// This is a summery of a complete testing process. \ref See Reporter::root_end().
///
struct Summary {
    /// \brief Number of disabled test cases.
    ///
    /// The number of test cases that were disabled. A disabled test case is one whose \p
    /// enabled argument to \ref ARCHON_TEST_IF() evaluates to false.
    ///
    long num_disabled_tests;

    /// \brief Number of test cases excluded due to filtering.
    ///
    /// The number of test cases that were excluded due to filtering. See \ref
    /// TestConfig::filter. This does not include disabled test cases (\ref
    /// num_disabled_tests).
    ///
    long num_excluded_tests;

    /// \brief Number of test cases selected to be executed.
    ///
    /// The number of test cases that have been selected to be executed. These are the test
    /// cases that were not excluded during filtering (\ref num_excluded_tests). The number
    /// of selected test cases is also available as \ref RootContext::num_tests.
    ///
    long num_selected_tests;

    /// \brief Number of test case executions.
    ///
    /// The total number of test case executions. This is the number of selected test cases
    /// (\ref num_selected_tests) times the number of repetitions (\ref
    /// TestConfig::num_repetitions). The number of test case executions is also available
    /// as \ref RootContext::num_test_executions.
    ///
    long num_test_executions;

    /// \brief Number of failed test exceutions.
    ///
    /// This is the number of test case executions (out of \ref num_test_executions) that
    /// have failed. A test case can fail due to a failed check, or due to an uncaught
    /// exception.
    ///
    long num_failed_test_executions;

    /// \brief Number of performed checks.
    ///
    /// The number of checks (e.g., \ref ARCHON_CHECK_EQUAL()) that have been performed
    /// during the entire testing process.
    ///
    long long num_checks;

    /// \brief Number of failed checks.
    ///
    /// The number of performed checks (out of \ref num_checks) that have failed.
    ///
    long long num_failed_checks;

    /// \brief Elapsed time for complete testing process.
    ///
    /// The amount of time that it took to execute all the selected test cases the requested
    /// number of times (\ref core::Timer::Type::monotonic_clock).
    ///
    double elapsed_seconds;
};



/// \brief Report on progress of testing process.
///
/// A reporter is an object that can be used to report on the progress of the execution of
/// test cases. To use a particular reporter, refer to it from \ref TestConfig::reporter of
/// the test configuration passed to \ref check::run().
///
/// An instance of this class will not report anything, because the default implementations
/// of all its functions do nothing. To actually report something, you will need to use a
/// subclass, for example, \ref SimpleReporter. See also \ref DuplicatingReporter and \ref
/// XmlReporter.
///
/// While the functions of a reporter may get executed by threads other than the one that
/// calls \ref check::run(), the testing harness ensures that at most one execution of a
/// reporter function can be in progress at any given time on behalf of a particular
/// invocation of \ref check::run(). Therefore, a reporter does not necessarily have to be
/// thread-safe.
///
class Reporter {
public:
    /// \brief Beginning of testing process.
    ///
    /// This function is called at the beginning of the testing process before \ref
    /// thread_begin() is called for each testing thread.
    ///
    virtual void root_begin(const check::RootContext&);

    /// \brief Beginning of testing thread.
    ///
    /// This function is called at the beginning of a testing thread, that is, before \ref
    /// begin() is called for any of the individual test case executions.
    ///
    virtual void thread_begin(const check::ThreadContext&);

    /// \brief Beginning of test case execution.
    ///
    /// This function is called at the beginning of each test case execution. If it wishes,
    /// the reporter implementation can use the specified logger to log something that
    /// should be qualified in a way that is specific to this test case execution.
    ///
    virtual void begin(const check::TestContext&, log::Logger&);

    /// \brief Failed check / test case.
    ///
    /// This function is called whenever a check fails, or the entire test case execution
    /// fails, for example, due to an uncaught exception. If it wishes, the reporter
    /// implementation can use the specified logger to log something that should be
    /// qualified in a way that is specific to the failed check (or test case exceution).
    ///
    virtual void fail(const check::FailContext&, std::string_view message, log::Logger&);

    /// \brief End of test case execution.
    ///
    /// This function is called at the end of each test case exceution. \p elapsed_seconds
    /// is the amount of time that it took to execute the test case (\ref
    /// core::Timer::Type::monotonic_clock). For the purpose of the specified logger, see
    /// \ref begin(). If this execution of the test case failed, this function will be
    /// called after all invocations of \ref fail() have been performed.
    ///
    virtual void end(const check::TestContext&, double elapsed_seconds, log::Logger&);

    /// \brief End of testing thread.
    ///
    /// This function is called at the end of a testing thread after \ref end() has been
    /// called for the last test case execution performed on behalf of this testing thread.
    ///
    virtual void thread_end(const check::ThreadContext&);

    /// \brief End of testing process.
    ///
    /// This function is called at the end of the testing process after \ref thread_end()
    /// has been called for each testing thread. The specified summery describes the entire
    /// testing process.
    ///
    virtual void root_end(const check::RootContext&, const check::Summary&);

    virtual ~Reporter() noexcept = default;
};


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_REPORTER_HPP
