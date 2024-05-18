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


#include <array>
#include <exception>

#include <archon/core/demangle.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/timer.hpp>
#include <archon/check/noinst/thread_context_impl.hpp>


using namespace archon;
namespace impl = check::impl;
using impl::ThreadContextImpl;


ThreadContextImpl::ThreadContextImpl(impl::RootContextImpl& root_context, int thread_index, log::Logger& logger,
                                     log::LogLevel inner_log_level_limit)
    : ThreadContext(root_context, thread_index, logger)
    , m_root_context(root_context)
    , m_test_level_report_logger(logger)
    , m_inner_logger_1(logger, inner_log_level_limit) // Throws
    , m_inner_logger_2(m_inner_logger_1, "Inner: ") // Throws
    , m_seed_seq(core::SeedSeq::no_copy_a(m_random_seed_parts)) // Throws
{
}


void ThreadContextImpl::run()
{
    clear_counters();

    std::unique_lock lock(m_root_context.mutex); // Throws
    m_root_context.reporter.thread_begin(*this); // Throws

    // First run the tests that can safely run concurrently with other threads and with
    // itself.
    while (m_root_context.next_concur_exec < m_root_context.concur_execs.size()) {
        auto entry = m_root_context.concur_execs[m_root_context.next_concur_exec++];
        run(entry, lock); // Throws
    }

    // When only the last test thread is running, we can run the tests that cannot safely
    // run concurrently with other threads or with itself, but this has to happen on the
    // main thread (the one that calls TestList::run()).
    if (!m_root_context.nonconcur_execs.empty()) {
        int num_remaining_threads = m_root_context.num_threads - m_root_context.num_ended_threads;
        if (num_remaining_threads == 1) {
            // Tell the main thread which thread context to use for executing the
            // nonconcurrent tests (nonconcur_run()).
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


void ThreadContextImpl::run(impl::RootContextImpl::Exec exec, std::unique_lock<std::mutex>& lock)
{
    int rseed_rep_no = exec.repetition_no;
    int rseed_rep_no_override = m_root_context.get_rseed_rep_no_override();
    if (rseed_rep_no_override != 0)
        rseed_rep_no = rseed_rep_no_override;
    int rseed_rep_no_shifted = rseed_rep_no;
    core::int_logic_shift_right(rseed_rep_no_shifted, 32);
    m_random_seed_extra = {
        rseed_rep_no & core::int_mask<core::SeedSeq::result_type>(32),
        rseed_rep_no_shifted & core::int_mask<core::SeedSeq::result_type>(32)
    };
    using Test = impl::RootContextImpl::Test;
    const Test& test = m_root_context.tests[exec.test_index];
    check::TestContext test_context(*this, test.list_entry->details, test.mapped_file_path, exec.test_index,
                                    exec.repetition_no, m_test_level_report_logger, m_inner_logger_2);
    m_test_level_report_logger.test_context = &test_context;
    m_test_level_report_logger.file_path    = test.mapped_file_path;
    m_test_level_report_logger.line_number  = test.list_entry->details.location.line_number;
    m_root_context.reporter.begin(test_context, m_test_level_report_logger); // Throws
    lock.unlock();

    m_errors_seen = false;
    core::Timer timer(core::Timer::Type::monotonic_clock); // Throws
    try {
        (test.list_entry->run_func)(test_context); // Throws
    }
    catch (std::exception& e) {
        std::array<char, 1024> seed_memory;
        core::StringFormatter formatter(seed_memory, m_root_context.locale); // Throws
        std::string_view message = formatter.format("Unhandled exception %s: %s", core::get_type_name(e),
                                                    e.what()); // Throws
        test_context.test_failed(message); // Throws
    }
    catch (...) {
        std::string_view message = "Unhandled exception of unknown type";
        test_context.test_failed(message); // Throws
    }
    double elapsed_time = timer.get_elapsed_time(); // Throws
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


void ThreadContextImpl::test_failed(const check::TestContext& test_context, std::string_view message)
{
    {
        std::lock_guard lock(m_mutex); // Throws
        m_errors_seen = true;
    }
    {
        std::lock_guard lock(m_root_context.mutex); // Throws
        m_test_level_report_logger.file_path   = test_context.mapped_file_path;
        m_test_level_report_logger.line_number = test_context.test_details.location.line_number;
        check::FailContext fail_context(test_context, test_context.test_details.location,
                                        test_context.mapped_file_path);
        m_root_context.reporter.fail(fail_context, message, m_test_level_report_logger); // Throws
        if (ARCHON_UNLIKELY(m_root_context.abort_on_failure()))
            abort();
    }
}


void ThreadContextImpl::check_failed(const check::TestContext& test_context, check::Location location,
                                     std::string_view message, log::Logger& report_logger)
{
    ++num_checks;
    {
        std::lock_guard lock(m_mutex); // Throws
        ++m_num_failed_checks;
        m_errors_seen = true;
    }
    std::string_view mapped_file_path;
    check::Location test_location = test_context.test_details.location;
    if (ARCHON_LIKELY(location.file_path == test_location.file_path)) {
        mapped_file_path = test_context.mapped_file_path;
    }
    else if (ARCHON_LIKELY(location.file_path == m_mapped_file_path_cache_key)) {
        mapped_file_path = m_mapped_file_path_cache_value;
    }
    else {
        m_mapped_file_path_cache_value = m_root_context.map_source_path(location.file_path); // Throws
        m_mapped_file_path_cache_key = location.file_path;
        mapped_file_path = m_mapped_file_path_cache_value;
    }
    {
        std::lock_guard lock(m_root_context.mutex); // Throws
        m_test_level_report_logger.file_path   = mapped_file_path;
        m_test_level_report_logger.line_number = location.line_number;
        check::FailContext fail_context(test_context, location, mapped_file_path);
        m_root_context.reporter.fail(fail_context, message, report_logger); // Throws
        if (ARCHON_UNLIKELY(m_root_context.abort_on_failure()))
            abort();
    }
}
