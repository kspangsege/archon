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

#ifndef ARCHON_X_UNIT_TEST_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP
#define ARCHON_X_UNIT_TEST_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <string>
#include <vector>
#include <mutex>

#include <archon/base/filesystem.hpp>
#include <archon/unit_test/random_seed.hpp>
#include <archon/unit_test/seed_seq.hpp>
#include <archon/unit_test/test_details.hpp>
#include <archon/unit_test/reporter.hpp>
#include <archon/unit_test/root_context.hpp>
#include <archon/unit_test/test_list.hpp>
#include <archon/unit_test/test_config.hpp>


namespace archon::unit_test::detail {


class RootContextImpl :
        public RootContext {
public:
    struct Test {
        const TestList::Entry* list_entry;
        std::string_view mapped_file_path;
    };

    struct Exec {
        std::size_t test_index;
        int recurrence_index;
    };

    // Calls to reporter functions must happen while holding a lock on `mutex`.
    Reporter& reporter;

    const std::vector<Test> tests;
    const std::vector<Exec> concur_execs, nonconcur_execs;

    std::mutex mutex;
    std::size_t next_concur_exec = 0; // Index into `concur_execs`, protected by `mutex`
    long num_failed_test_executions = 0; //  Protected by `mutex`
    long long num_checks = 0; //  Protected by `mutex`
    long long num_failed_checks = 0; //  Protected by `mutex`
    int num_ended_threads = 0; //  Protected by `mutex`
    int last_thread_to_end = -1;

    RootContextImpl(int num_repetitions, int num_threads, std::locale, base::Logger& report_logger,
                    const std::string* log_paths, Reporter&, std::vector<Test> tests,
                    std::vector<Exec> concur_execs, std::vector<Exec> nonconcur_execs,
                    bool abort_on_failure, bool keep_test_files,
                    base::FilesystemPath test_file_dir, base::FilesystemPath data_root_dir,
                    const SourcePathMapper*, unit_test::RandomSeed) noexcept;

    bool abort_on_failure() const noexcept;

    std::string map_source_path(std::string_view path) const;

    const std::filesystem::path& get_test_file_dir() const noexcept;

    bool keep_test_files() const noexcept;

    const std::filesystem::path& get_data_root_dir() const noexcept;

    unit_test::SeedSeq& seed_seq() noexcept;

    // Overriding functions in `RootContext`
    const TestDetails& get_test_details(std::size_t test_index) const noexcept override final;

private:
    const bool m_abort_on_failure;
    const bool m_keep_test_files;
    const std::filesystem::path m_test_file_dir;
    const std::filesystem::path m_data_root_dir;
    const SourcePathMapper* const m_source_path_mapper;
    const unit_test::RandomSeed m_random_seed;
    unit_test::SeedSeq m_seed_seq;
};








// Implementation


inline bool RootContextImpl::abort_on_failure() const noexcept
{
    return m_abort_on_failure;
}


inline const std::filesystem::path& RootContextImpl::get_test_file_dir() const noexcept
{
    return m_test_file_dir;
}


inline bool RootContextImpl::keep_test_files() const noexcept
{
    return m_keep_test_files;
}


inline const std::filesystem::path& RootContextImpl::get_data_root_dir() const noexcept
{
    return m_data_root_dir;
}


inline unit_test::SeedSeq& RootContextImpl::seed_seq() noexcept
{
    return m_seed_seq;
}


} // namespace archon::unit_test::detail

#endif // ARCHON_X_UNIT_TEST_X_NOINST_X_ROOT_CONTEXT_IMPL_HPP
