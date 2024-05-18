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

#ifndef ARCHON_X_CHECK_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP
#define ARCHON_X_CHECK_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP


#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <mutex>

#include <archon/core/filesystem.hpp>
#include <archon/check/random_seed.hpp>
#include <archon/check/test_details.hpp>
#include <archon/check/root_context.hpp>
#include <archon/check/test_list.hpp>
#include <archon/check/reporter.hpp>
#include <archon/check/test_config.hpp>


namespace archon::check::impl {


class RootContextImpl
    : public check::RootContext {
public:
    struct Test {
        const check::TestList::Entry* list_entry;
        std::string_view mapped_file_path;
    };

    struct Exec {
        std::size_t test_index;
        int repetition_no;
    };

    // Calls to reporter functions must happen while holding a lock on `mutex`.
    check::Reporter& reporter;

    const core::Span<const Test> tests;
    const core::Span<const Exec> concur_execs, nonconcur_execs;

    std::mutex mutex;
    std::size_t next_concur_exec = 0; // Index into `concur_execs`, protected by `mutex`
    long num_failed_test_executions = 0; //  Protected by `mutex`
    long long num_checks = 0; //  Protected by `mutex`
    long long num_failed_checks = 0; //  Protected by `mutex`
    int num_ended_threads = 0; //  Protected by `mutex`
    int last_thread_to_end = -1;

    RootContextImpl(int num_repetitions, int num_threads, const std::locale&, log::Logger& report_logger,
                    const std::string* log_paths, check::Reporter&, core::Span<const Test>,
                    core::Span<const Exec> concur_execs, core::Span<const Exec> nonconcur_execs, bool abort_on_failure,
                    bool keep_test_files, core::FilesystemPathRef test_file_dir, core::FilesystemPathRef data_file_dir,
                    const check::SourcePathMapper*, core::Span<const core::SeedSeq::result_type> random_seed,
                    int rseed_rep_no_override) noexcept;

    bool abort_on_failure() const noexcept;

    auto map_source_path(std::string_view path) const -> std::string;

    auto get_test_file_dir() const noexcept -> const std::filesystem::path&;

    bool keep_test_files() const noexcept;

    auto get_data_file_dir() const noexcept -> const std::filesystem::path&;

    auto get_random_seed() const noexcept -> core::Span<const core::SeedSeq::result_type>;

    int get_rseed_rep_no_override() const  noexcept;

    // Overriding functions in `RootContext`
    auto get_test_details(std::size_t test_index) const noexcept -> const TestDetails& override final;

private:
    const bool m_abort_on_failure;
    const bool m_keep_test_files;
    const std::filesystem::path& m_test_file_dir;
    const std::filesystem::path& m_data_file_dir;
    const check::SourcePathMapper* const m_source_path_mapper;
    const core::Span<const core::SeedSeq::result_type> m_random_seed;
    const int m_rseed_rep_no_override;
};








// Implementation


inline bool RootContextImpl::abort_on_failure() const noexcept
{
    return m_abort_on_failure;
}


inline auto RootContextImpl::get_test_file_dir() const noexcept -> const std::filesystem::path&
{
    return m_test_file_dir;
}


inline bool RootContextImpl::keep_test_files() const noexcept
{
    return m_keep_test_files;
}


inline auto RootContextImpl::get_data_file_dir() const noexcept -> const std::filesystem::path&
{
    return m_data_file_dir;
}


inline auto RootContextImpl::get_random_seed() const noexcept -> core::Span<const core::SeedSeq::result_type>
{
    return m_random_seed;
}


inline int RootContextImpl::get_rseed_rep_no_override() const  noexcept
{
    return m_rseed_rep_no_override;
}


} // namespace archon::check::impl

#endif // ARCHON_X_CHECK_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP
