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


#include <array>
#include <exception>

#include <archon/base/demangle.hpp>
#include <archon/base/timer.hpp>
#include <archon/base/string_formatter.hpp>
#include <archon/unit_test/noinst/thread_context_impl.hpp>


using namespace archon;
using namespace archon::unit_test;
using namespace archon::unit_test::detail;


ThreadContextImpl::ThreadContextImpl(RootContextImpl& root_context, int thread_index,
                                     base::Logger& logger, base::LogLevel inner_log_level_limit) :
    ThreadContext(root_context, thread_index, logger),
    m_root_context(root_context),
    m_test_level_report_logger(logger),
    m_inner_logger_1(logger, inner_log_level_limit), // Throws
    m_inner_logger_2(m_inner_logger_1, "Inner: ") // Throws
{
}


void ThreadContextImpl::run()
{
    clear_counters();

    std::unique_lock lock(m_root_context.mutex); // Throws
    m_root_context.reporter.thread_begin(*this); // Throws

    // First run the tests that can safely run concurrently with other threads
    // and with itself.
    while (m_root_context.next_concur_exec < m_root_context.concur_execs.size()) {
        auto entry = m_root_context.concur_execs[m_root_context.next_concur_exec++];
        run(entry, lock); // Throws
    }

    // When only the last test thread is running, we can run the tests that
    // cannot safely run concurrently with other threads or with itself, but
    // this has to happen on the main thread (the one that calls
    // TestList::run()).
    if (!m_root_context.nonconcur_execs.empty()) {
        int num_remaining_threads = m_root_context.num_threads - m_root_context.num_ended_threads;
        if (num_remaining_threads == 1) {
            // Tell the main thread which thread context to use for executing
            // the nonconcurrent tests (nonconcur_run()).
            m_root_context.last_thread_to_end = thread_index;
            return;
        }
    }

    ++m_root_context.num_ended_threads;
    finalize(lock); // Throws
}


void ThreadContextImpl::nonconcur_run()
{
    clear_counters();

    std::unique_lock lock(m_root_context.mutex); // Throws

    for (auto entry : m_root_context.nonconcur_execs)
        run(entry, lock); // Throws

    finalize(lock); // Throws
}


void ThreadContextImpl::run(RootContextImpl::Exec exec, std::unique_lock<std::mutex>& lock)
{
    using Test = RootContextImpl::Test;
    const Test& test = m_root_context.tests[exec.test_index];
    TestContext test_context(*this, test.list_entry->details, test.mapped_file_path,
                             exec.test_index, exec.recurrence_index, m_test_level_report_logger,
                             m_inner_logger_2);
    m_test_level_report_logger.test_context = &test_context;
    m_test_level_report_logger.file_path    = test.mapped_file_path;
    m_test_level_report_logger.line_number  = test.list_entry->details.location.line_number;
    m_root_context.reporter.begin(test_context, m_test_level_report_logger); // Throws
    lock.unlock();

    m_errors_seen = false;
    base::Timer timer(base::Timer::Type::monotonic_clock);
    try {
        (*test.list_entry->run_func)(test_context); // Throws
    }
    catch (std::exception& e) {
        std::array<char, 1024> seed_memory;
        base::StringFormatter formatter(seed_memory, m_root_context.locale); // Throws
        std::string_view message =
            formatter.format("Unhandled exception %s: %s", base::get_type_name(e),
                             e.what()); // Throws
        test_context.test_failed(message); // Throws
    }
    catch (...) {
        std::string_view message = "Unhandled exception of unknown type";
        test_context.test_failed(message); // Throws
    }
    double elapsed_time = timer.get_elapsed_time();
    if (m_errors_seen)
        ++m_num_failed_test_executions;

    lock.lock(); // Throws
    m_test_level_report_logger.file_path   = test.mapped_file_path;
    m_test_level_report_logger.line_number = test.list_entry->details.location.line_number;
    m_root_context.reporter.end(test_context, elapsed_time, m_test_level_report_logger); // Throws
}


void ThreadContextImpl::finalize(std::unique_lock<std::mutex>&)
{
    m_root_context.num_failed_test_executions += m_num_failed_test_executions;
    m_root_context.num_checks += num_checks;
    m_root_context.num_failed_checks += m_num_failed_checks;

    m_root_context.reporter.thread_end(*this); // Throws
}


void ThreadContextImpl::test_failed(const TestContext& test_context, std::string_view message)
{
    {
        std::lock_guard lock(m_mutex); // Throws
        m_errors_seen = true;
    }
    {
        std::lock_guard lock(m_root_context.mutex); // Throws
        m_test_level_report_logger.file_path   = test_context.mapped_file_path;
        m_test_level_report_logger.line_number = test_context.test_details.location.line_number;
        FailContext fail_context(test_context, test_context.test_details.location,
                                 test_context.mapped_file_path);
        m_root_context.reporter.fail(fail_context, message, m_test_level_report_logger); // Throws
        if (ARCHON_UNLIKELY(m_root_context.abort_on_failure()))
            abort();
    }
}


void ThreadContextImpl::check_failed(const TestContext& test_context, Location location,
                                     std::string_view message, base::Logger& report_logger)
{
    ++num_checks;
    {
        std::lock_guard lock(m_mutex); // Throws
        ++m_num_failed_checks;
        m_errors_seen = true;
    }
    std::string_view mapped_file_path;
    Location test_location = test_context.test_details.location;
    if (ARCHON_LIKELY(location.file_path == test_location.file_path)) {
        mapped_file_path = test_context.mapped_file_path;
    }
    else if (ARCHON_LIKELY(location.file_path == m_mapped_file_path_cache_key)) {
        mapped_file_path = m_mapped_file_path_cache_value;
    }
    else {
        m_mapped_file_path_cache_value =
            m_root_context.map_source_path(location.file_path); // Throws
        m_mapped_file_path_cache_key = location.file_path;
        mapped_file_path = m_mapped_file_path_cache_value;
    }
    {
        std::lock_guard lock(m_root_context.mutex); // Throws
        m_test_level_report_logger.file_path   = mapped_file_path;
        m_test_level_report_logger.line_number = location.line_number;
        FailContext fail_context(test_context, location, mapped_file_path);
        m_root_context.reporter.fail(fail_context, message, report_logger); // Throws
        if (ARCHON_UNLIKELY(m_root_context.abort_on_failure()))
            abort();
    }
}
