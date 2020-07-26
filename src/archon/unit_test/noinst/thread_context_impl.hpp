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

#ifndef ARCHON_X_UNIT_TEST_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP
#define ARCHON_X_UNIT_TEST_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP

/// \file


#include <string_view>
#include <string>
#include <atomic>
#include <mutex>

#include <archon/base/limit_logger.hpp>
#include <archon/base/prefix_logger.hpp>
#include <archon/unit_test/test_context.hpp>
#include <archon/unit_test/noinst/test_level_report_logger.hpp>
#include <archon/unit_test/noinst/root_context_impl.hpp>


namespace archon::unit_test::detail {


class ThreadContextImpl :
        public ThreadContext {
public:
    std::atomic<long long> num_checks;

    ThreadContextImpl(RootContextImpl& root_context, int thread_index, base::Logger& logger,
                      base::LogLevel inner_log_level_limit);

    base::Logger& get_inner_logger() noexcept;

    void run();
    void nonconcur_run();

    void run(RootContextImpl::Exec, std::unique_lock<std::mutex>&);
    void finalize(std::unique_lock<std::mutex>&);

    void test_failed(const TestContext&, std::string_view message);
    void check_failed(const TestContext&, Location, std::string_view message);

    RootContextImpl& get_root_context() noexcept;

private:
    RootContextImpl& m_root_context;
    std::mutex m_mutex;
    long long m_num_failed_checks; //  Protected by `m_mutex`
    long m_num_failed_test_executions;
    bool m_errors_seen; //  Protected by `m_mutex`
    std::string_view m_mapped_file_path_cache_key; // Protected by `m_root_context.mutex`
    std::string m_mapped_file_path_cache_value; // Protected by `m_root_context.mutex`
    TestLevelReportLogger m_test_level_report_logger; // Protected by `m_root_context.mutex`
    base::LimitLogger m_inner_logger_1;
    base::PrefixLogger m_inner_logger_2;

    void clear_counters() noexcept;
};








// Implementation


inline base::Logger& ThreadContextImpl::get_inner_logger() noexcept
{
    return m_inner_logger_2;
}


inline RootContextImpl& ThreadContextImpl::get_root_context() noexcept
{
    return m_root_context;
}


inline void ThreadContextImpl::clear_counters() noexcept
{
    num_checks = 0;
    m_num_failed_checks = 0;
    m_num_failed_test_executions = 0;
}


} // namespace archon::unit_test::detail

#endif // ARCHON_X_UNIT_TEST_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP
