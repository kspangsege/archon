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

#ifndef ARCHON_X_CHECK_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP
#define ARCHON_X_CHECK_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP

/// \file


#include <string_view>
#include <string>
#include <atomic>
#include <mutex>

#include <archon/core/random.hpp>
#include <archon/log/prefix_logger.hpp>
#include <archon/log/limit_logger.hpp>
#include <archon/check/test_context.hpp>
#include <archon/check/noinst/test_level_report_logger.hpp>
#include <archon/check/noinst/root_context_impl.hpp>


namespace archon::check::impl {


class ThreadContextImpl
    : public check::ThreadContext {
public:
    std::atomic<long long> num_checks;

    ThreadContextImpl(impl::RootContextImpl& root_context, int thread_index, log::Logger& logger,
                      log::LogLevel inner_log_level_limit);

    void run();
    void nonconcur_run();

    void run(impl::RootContextImpl::Exec, std::unique_lock<std::mutex>&);
    void finalize(std::unique_lock<std::mutex>&);

    void test_failed(const check::TestContext&, std::string_view message);

    // Specified report logger must be `m_test_level_report_logger`, or a logger that is
    // derived from it.
    void check_failed(const check::TestContext&, check::Location, std::string_view message,
                      log::Logger& report_logger);

    auto get_root_context() noexcept -> impl::RootContextImpl&;

    auto seed_seq() const noexcept -> const core::SeedSeq&;

private:
    impl::RootContextImpl& m_root_context;
    std::mutex m_mutex;
    long long m_num_failed_checks; //  Protected by `m_mutex`
    long m_num_failed_test_executions;
    bool m_errors_seen; //  Protected by `m_mutex`
    std::string_view m_mapped_file_path_cache_key; // Protected by `m_root_context.mutex`
    std::string m_mapped_file_path_cache_value; // Protected by `m_root_context.mutex`
    impl::TestLevelReportLogger m_test_level_report_logger; // Protected by `m_root_context.mutex`
    log::LimitLogger m_inner_logger_1;
    log::PrefixLogger m_inner_logger_2;
    std::array<core::SeedSeq::result_type, 2> m_random_seed_extra;
    const std::array<core::Span<const core::SeedSeq::result_type>, 2> m_random_seed_parts = {
        m_root_context.get_random_seed(),
        m_random_seed_extra
    };
    const core::SeedSeq m_seed_seq;

    void clear_counters() noexcept;
};








// Implementation


inline auto ThreadContextImpl::get_root_context() noexcept -> RootContextImpl&
{
    return m_root_context;
}


inline auto ThreadContextImpl::seed_seq() const noexcept -> const core::SeedSeq&
{
    return m_seed_seq;
}


inline void ThreadContextImpl::clear_counters() noexcept
{
    num_checks = 0;
    m_num_failed_checks = 0;
    m_num_failed_test_executions = 0;
}


} // namespace archon::check::impl

#endif // ARCHON_X_CHECK_X_NOINST_X_THREAD_CONTEXT_IMPL_HPP
