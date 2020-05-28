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


#include <archon/core/char_mapper.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/format_as.hpp>
#include <archon/check/simple_reporter.hpp>


using namespace archon;
using check::SimpleReporter;


void SimpleReporter::root_begin(const check::RootContext& context)
{
    log::Logger& logger = context.report_logger;
    core::NumOfSpec test_cases_spec = { "test case", "test cases" };
    core::NumOfSpec test_case_executions_spec = { "test case execution", "test case executions" };
    core::NumOfSpec test_threads_spec = { "test thread", "test threads" };
    if (context.num_repetitions == 1) {
        logger.info("Executing %s using %s",
                    core::as_num_of(context.num_tests, test_cases_spec),
                    core::as_num_of(context.num_threads, test_threads_spec)); // Throws
    }
    else {
        logger.info("Executing %s %s times (%s) using %s",
                    core::as_num_of(context.num_tests, test_cases_spec), context.num_repetitions,
                    core::as_num_of(context.num_test_executions, test_case_executions_spec),
                    core::as_num_of(context.num_threads, test_threads_spec)); // Throws
    }
    if (context.log_paths) {
        int n = context.num_threads;
        core::CharMapper char_mapper(context.locale); // Throws
        core::IntegerFormatter integer_formatter(char_mapper);
        int n_width = int(integer_formatter.format_dec(n).size()); // Throws
        for (int i = 0; i < n; ++i) {
            const std::string& path = context.log_paths[i];
            logger.info("Test thread %s is logging to %s", core::with_width(i + 1, n_width), path); // Throws
        }
    }
}


void SimpleReporter::thread_begin(const check::ThreadContext& context)
{
    const check::RootContext& root_context = context.root_context;
    if (root_context.log_paths) {
        log::Logger& logger = context.report_logger;
        logger.info("Beginning of test thread"); // Throws
    }
}


void SimpleReporter::begin(const check::TestContext&, log::Logger& logger)
{
    if (!m_report_progress)
        return;

    logger.info("Started"); // Throws
}


void SimpleReporter::fail(const check::FailContext&, std::string_view message, log::Logger& logger)
{
    logger.error("%s", message); // Throws
}


void SimpleReporter::thread_end(const check::ThreadContext& context)
{
    const check::RootContext& root_context = context.root_context;
    if ((m_report_progress && root_context.num_threads > 1) || root_context.log_paths) {
        log::Logger& logger = context.report_logger;
        logger.info("End of test thread"); // Throws
    }
}


void SimpleReporter::root_end(const check::RootContext& context, const check::Summary& summary)
{
    log::Logger& logger = context.report_logger;
    core::NumOfSpec test_case_executions_spec = { "test case execution", "test case executions" };
    if (context.num_repetitions == 1)
        test_case_executions_spec = { "test case", "test cases" };
    core::NumOfSpec checks_spec = { "check", "checks" };
    if (summary.num_failed_test_executions == 0) {
        if (summary.num_test_executions != 1) {
            logger.info("Success: All %s passed (%s)",
                        core::as_num_of(summary.num_test_executions, test_case_executions_spec),
                        core::as_num_of(summary.num_checks, checks_spec)); // Throws
        }
        else {
            logger.info("Success: The test passed (%s)", core::as_num_of(summary.num_checks, checks_spec)); // Throws
        }
    }
    else {
        logger.info("FAILURE: %s out of %s failed (%s out of %s failed)",
                    summary.num_failed_test_executions,
                    core::as_num_of(summary.num_test_executions, test_case_executions_spec),
                    summary.num_failed_checks,
                    core::as_num_of(summary.num_checks, checks_spec)); // Throws
    }
    logger.info("Test time: %s", core::as_time(summary.elapsed_seconds)); // Throws
    if (summary.num_excluded_tests >= 1) {
        if (summary.num_excluded_tests == 1) {
            logger.info("Note: One test case was excluded!"); // Throws
        }
        else {
            logger.info("Note: %s test cases were excluded!", summary.num_excluded_tests); // Throws
       }
    }
}
