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


#include <archon/base/char_mapper.hpp>
#include <archon/base/integer_formatter.hpp>
#include <archon/base/format_as.hpp>
#include <archon/base/format_with.hpp>
#include <archon/unit_test/simple_reporter.hpp>


using namespace archon;
using namespace archon::unit_test;


void SimpleReporter::root_begin(const RootContext& context)
{
    base::Logger& logger = context.report_logger;
    if (context.log_paths) {
        int n = context.num_threads;
        base::CharMapper char_mapper(context.locale); // Throws
        base::IntegerFormatter integer_formatter(char_mapper);
        int n_width = int(integer_formatter.format_dec(n).size()); // Throws
        for (int i = 0; i < n; ++i) {
            const std::string& path = context.log_paths[i];
            logger.info("Test thread %s is logging to %s",
                        base::with_width(i + 1, n_width), path); // Throws
        }
    }
    else {
        base::NumOfSpec spec = { "test thread", "test threads" };
        logger.info("Using %s", base::as_num_of(context.num_threads, spec)); // Throws
    }
}


void SimpleReporter::thread_begin(const ThreadContext& context)
{
    const RootContext& root_context = context.root_context;
    if (root_context.log_paths) {
        base::Logger& logger = context.report_logger;
        logger.info("Beginning of test thread"); // Throws
    }
}


void SimpleReporter::begin(const TestContext&, base::Logger& logger)
{
    if (!m_report_progress)
        return;

    logger.info("Started"); // Throws
}


void SimpleReporter::fail(const FailContext&, std::string_view message, base::Logger& logger)
{
    logger.error("%s", message); // Throws
}


void SimpleReporter::thread_end(const ThreadContext& context)
{
    const RootContext& root_context = context.root_context;
    if ((m_report_progress && root_context.num_threads > 1) || root_context.log_paths) {
        base::Logger& logger = context.report_logger;
        logger.info("End of test thread"); // Throws
    }
}


void SimpleReporter::root_end(const RootContext& context, const Summary& summary)
{
    base::Logger& logger = context.report_logger;
    if (summary.num_failed_test_executions == 0) {
        logger.info("Success: All %s tests passed (%s checks)",
                    summary.num_test_executions, summary.num_checks); // Throws
    }
    else {
        logger.info("FAILURE: %s out of %s tests failed (%s out of %s checks failed)",
                    summary.num_failed_test_executions, summary.num_test_executions,
                    summary.num_failed_checks, summary.num_checks); // Throws
    }
    logger.info("Test time: %s", base::as_time(summary.elapsed_seconds)); // Throws
    if (summary.num_excluded_tests >= 1) {
        if (summary.num_excluded_tests == 1) {
            logger.info("Note: One test was excluded!"); // Throws
        }
        else {
            logger.info("Note: %s tests were excluded!", summary.num_excluded_tests); // Throws
       }
    }
}
