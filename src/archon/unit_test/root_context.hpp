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


/// \file

#ifndef ARCHON__UNIT_TEST__ROOT_CONTEXT_HPP
#define ARCHON__UNIT_TEST__ROOT_CONTEXT_HPP

#include <cstddef>
#include <string>
#include <locale>

#include <archon/base/logger.hpp>
#include <archon/unit_test/test_details.hpp>


namespace archon::unit_test {


/// \brief Root-level execution context.
///
/// This is the part of the unit test execution context that is shared across
/// all unit test executions performed on behalf of a particular execution of
/// \ref unit_test::run().
///
class RootContext {
public:
    /// \brief Number of selected unit tests.
    ///
    /// This is the number of unit tests selected to be executed as part of a
    /// particular invocation of \ref unit_test::run(). This does not inlcude
    /// disabled tests, and tests excluded during filtering.
    ///
    const std::size_t num_tests;

    /// \brief Number of executions of each unit test.
    ///
    /// The number of times to execute each of the selected unit tests. This is
    /// the number specified in \ref TestConfig::num_repetitions.
    ///
    const int num_recurrences;

    /// \brief Number of testing threads.
    ///
    /// The number of testing threads deployed as a part of an execution of \ref
    /// unit_test::run(). This is at most \ref TestConfig::num_threads.
    ///
    const int num_threads;

    /// \brief Configured locale.
    ///
    /// This is the locale that was specified in \ref TestConfig::locale. Unit
    /// tests may choose to use this locale for locale dependent operations. It
    /// is up to the designer of the unit tests whether this makes sense.
    ///
    const std::locale locale;

    /// \brief Top-level logger to be used by customer reporters.
    ///
    /// Do not use this logger inside your unit tests. Instead use \ref
    /// TestContext::logger there.
    ///
    /// This is the top-level logger to be used by custom reporters (\ref
    /// Reporter). See also \ref ThreadContext::report_logger.
    ///
    base::Logger& report_logger;

    /// \brief Log file paths when file logging is enabled.
    ///
    /// When file logging is enabled (\ref TestConfig::log_to_files), this is a
    /// pointer to an array of filesystem paths, one for each log file. Since
    /// each thread has a log file, \ref num_threads is also the number of
    /// entries in this array. When file logging is disabled, this is null.
    ///
    const std::string* const log_paths;

    /// \brief Get information about selected unit test.
    ///
    /// This function returns information about one of the unit tests selected
    /// to be executed as part of a particular invocation of \ref
    /// unit_test::run(). The specified index refers to the list of selected
    /// unit tests, and must be less than \ref num_tests.
    ///
    virtual const TestDetails& get_test_details(std::size_t index) const noexcept = 0;

    RootContext(const RootContext&) = delete;
    RootContext& operator=(const RootContext&) = delete;

protected:
    RootContext(std::size_t num_tests, int num_recurrences, int num_threads, std::locale,
                base::Logger& report_logger, const std::string* log_paths) noexcept;
    ~RootContext() = default;
};








// Implementation


inline RootContext::RootContext(std::size_t nt, int nr, int nh, std::locale l, base::Logger& rl,
                                const std::string* lp) noexcept :
    num_tests(nt),
    num_recurrences(nr),
    num_threads(nh),
    locale(std::move(l)),
    report_logger(rl),
    log_paths(lp)
{
}


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__ROOT_CONTEXT_HPP
