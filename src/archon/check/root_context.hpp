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

#ifndef ARCHON_X_CHECK_X_ROOT_CONTEXT_HPP
#define ARCHON_X_CHECK_X_ROOT_CONTEXT_HPP

/// \file


#include <cstddef>
#include <string>
#include <locale>

#include <archon/log/logger.hpp>
#include <archon/check/test_details.hpp>


namespace archon::check {


/// \brief Root-level execution context.
///
/// This is the part of the test case execution context that is shared across all test case
/// executions performed on behalf of a particular invocation of \ref check::run().
///
class RootContext {
public:
    /// \brief Number of selected test cases.
    ///
    /// This is the number of test cases selected to be executed as part of a particular
    /// invocation of \ref check::run(). This does not inlcude disabled test cases, and test
    /// cases excluded during filtering.
    ///
    const std::size_t num_tests;

    /// \brief Number of test case executions.
    ///
    /// This is the number of selected test cases (\ref num_tests) times the number of
    /// repetitions (\ref num_repetitions).
    ///
    const std::size_t num_test_executions;

    /// \brief Number of executions of each test case.
    ///
    /// The number of times to execute each of the selected test cases. This is the number
    /// specified in \ref TestConfig::num_repetitions.
    ///
    const int num_repetitions;

    /// \brief Number of testing threads.
    ///
    /// The number of testing threads deployed as a part of an execution of \ref
    /// check::run(). This is at most \ref TestConfig::num_threads.
    ///
    const int num_threads;

    /// \brief Configured locale.
    ///
    /// This is the locale that was specified in \ref TestConfig::locale. Test cases may
    /// choose to use this locale for locale dependent operations. It is up to the designer
    /// of the test cases whether this makes sense.
    ///
    const std::locale& locale;

    /// \brief Top-level logger to be used by customer reporters.
    ///
    /// Do not use this logger inside your test cases. Instead use \ref TestContext::logger
    /// there.
    ///
    /// This is the top-level logger to be used by custom reporters (\ref Reporter). See
    /// also \ref ThreadContext::report_logger.
    ///
    log::Logger& report_logger;

    /// \brief Log file paths when file logging is enabled.
    ///
    /// When file logging is enabled (\ref TestConfig::log_to_files), this is a pointer to
    /// an array of filesystem paths, one for each log file. Since each thread has a log
    /// file, \ref num_threads is also the number of entries in this array. When file
    /// logging is disabled, this is null.
    ///
    const std::string* const log_paths;

    /// \brief Get information about a selected test case.
    ///
    /// This function returns information about one of the test cases selected to be
    /// executed as part of a particular invocation of \ref check::run(). The specified index
    /// refers to the list of selected test cases, and must be less than \ref num_tests.
    ///
    virtual auto get_test_details(std::size_t index) const noexcept -> const check::TestDetails& = 0;

    RootContext(const RootContext&) = delete;
    auto operator=(const RootContext&) -> RootContext& = delete;

protected:
    RootContext(std::size_t num_tests, std::size_t num_test_executions, int num_repetitions, int num_threads,
                const std::locale&, log::Logger& report_logger, const std::string* log_paths) noexcept;
    ~RootContext() noexcept = default;
};








// Implementation


inline RootContext::RootContext(std::size_t nt, std::size_t nte, int nr, int nh, const std::locale& l, log::Logger& rl,
                                const std::string* lp) noexcept
    : num_tests(nt)
    , num_test_executions(nte)
    , num_repetitions(nr)
    , num_threads(nh)
    , locale(l)
    , report_logger(rl)
    , log_paths(lp)
{
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_ROOT_CONTEXT_HPP
