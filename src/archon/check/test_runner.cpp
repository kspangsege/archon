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


#include <cstddef>
#include <limits>
#include <algorithm>
#include <string_view>
#include <string>
#include <stdexcept>
#include <chrono>
#include <random>
#include <vector>
#include <map>
#include <stdexcept>
#include <thread>
#include <iostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/time.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/string.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/timestamp_formatter.hpp>
#include <archon/core/format.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/string_template.hpp>
#include <archon/core/random.hpp>
#include <archon/core/timer.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/thread_guard.hpp>
#include <archon/log/timestamp_logger.hpp>
#include <archon/log/prefix_logger.hpp>
#include <archon/check/noinst/root_context_impl.hpp>
#include <archon/check/noinst/thread_context_impl.hpp>
#include <archon/check/test_runner.hpp>


using namespace archon;


namespace {


/// FIXME: Move to `<archon/core/filesystem.hpp>`
bool ensure_subdir(core::FilesystemPathRef subdir, core::FilesystemPathRef base_dir)
{
    namespace fs = std::filesystem;
    // Note: `fs::create_directories()` cannot be used here as that would risk creation of
    // `base_dir`, or of directories outside `base_dir`.
    fs::path subdir_2 = subdir; // Throws
    core::remove_trailing_slash(subdir_2); // Throws
    bool good = (!subdir_2.has_root_name() && !subdir_2.has_root_directory()); // Throws
    if (ARCHON_UNLIKELY(!good))
        return false;
    fs::path path;
    const fs::path& dot     = core::get_fs_dot_path(); // Throws
    const fs::path& dot_dot = core::get_fs_dot_dot_path(); // Throws
    for (const auto& segment : subdir_2) {
        if (ARCHON_LIKELY(segment != dot)) {
            if (ARCHON_LIKELY(segment != dot_dot)) {
                path /= segment; // Throws
                fs::create_directory(base_dir / path); // Throws
                continue;
            }
            if (ARCHON_UNLIKELY(path.empty()))
                return false;
            path = path.parent_path(); // Throws
        }
    }
    return true;
}


void configure_timestamp_logger(log::TimestampLogger::Config& config, const check::TestConfig&)
{
    config.precision = log::TimestampLogger::Precision::milliseconds;
    config.format = "%FT%T: ";
}

void configure_file_logger(log::FileLogger::Config&, const check::TestConfig&)
{
}


} // unnamed namespace


using check::TestRunner;


TestRunner::TestRunner(const std::locale& locale, check::TestConfig config)
    : m_locale(locale)
    , m_config(std::move(config)) // Throws
    , m_logger_owner(make_logger(locale, m_config)) // Throws
    , m_logger(m_logger_owner ? *m_logger_owner : *config.logger) // Throws
{
}


bool TestRunner::run() const
{
    if (m_config.num_repetitions < 0)
        throw std::runtime_error("Bad number of repetitions");
    if (m_config.num_threads < 0)
        throw std::runtime_error("Bad number of threads");

    // Map file paths, and check for name clashes, and filter tests
    const TestList& test_list = (m_config.test_list ? *m_config.test_list : TestList::get_default_list());
    using Test = impl::RootContextImpl::Test;
    std::vector<Test> included_tests;
    std::size_t num_enabled = 0, num_disabled = 0;
    namespace fs = std::filesystem;
    std::map<std::string_view, std::string> mapped_file_paths;
    {
        std::map<std::string_view, const TestList::Entry*> map;
        for (const TestList::Entry& entry : test_list) {
            std::string_view mapped_file_path;
            {
                std::string_view path_1 = entry.details.location.file_path;
                auto p = mapped_file_paths.emplace(path_1, std::string()); // Throws
                bool was_inserted = p.second;
                if (was_inserted) {
                    fs::path path_2 = core::make_fs_path_auto(path_1, m_locale); // Throws
                    if (m_config.source_path_mapper)
                        m_config.source_path_mapper->map(path_2); // Throws
                    p.first->second = core::path_to_string_native(path_2, m_locale); // Throws
                }
                mapped_file_path = p.first->second;
            }
            {
                auto p = map.emplace(entry.details.name, &entry); // Throws
                bool was_inserted = p.second;
                if (ARCHON_UNLIKELY(!was_inserted)) {
                    const TestList::Entry& other = *p.first->second;
                    auto i = mapped_file_paths.find(other.details.location.file_path);
                    ARCHON_ASSERT(i != mapped_file_paths.end());
                    std::string_view path_1 = i->second;
                    std::string_view path_2 = mapped_file_path;
                    long line_1 = other.details.location.line_number;
                    long line_2 = entry.details.location.line_number;
                    std::string message = core::format(m_locale, "Multiple test cases with name `%s` (`%s:%s` and "
                                                       "`%s:%s`)", entry.details.name, path_1, line_1, path_2,
                                                       line_2); // Throws
                    throw std::runtime_error(message);
                }
            }
            bool enabled = (!entry.is_enabled_func || (*entry.is_enabled_func)()); // Throws
            if (!enabled) {
                ++num_disabled;
                continue;
            }
            ++num_enabled;
            if (m_config.filter && !m_config.filter->include(entry.details))
                continue;
            included_tests.push_back({ &entry, mapped_file_path }); // Throws
        }
    }
    std::size_t num_selected = included_tests.size();

    // Number of threads
    int num_threads = m_config.num_threads;
    if (num_threads < 1) {
        unsigned n = std::thread::hardware_concurrency();
        if (n > 0) {
            int max = std::numeric_limits<int>::max();
            if (n <= unsigned(max)) {
                num_threads = int(n);
            }
            else {
                num_threads = max;
            }
        }
        else {
            num_threads = 1;
        }
    }

    // Sort
    if (m_config.test_order) {
        const TestOrder& test_order = *m_config.test_order;
        auto compare = [&](const Test& a, const Test& b) noexcept {
            return test_order.less(a.list_entry->details, b.list_entry->details);
        };
        std::stable_sort(included_tests.begin(), included_tests.end(), compare);
    }

    // Repeat
    using Exec = impl::RootContextImpl::Exec;
    std::vector<Exec> concur_execs, nonconcur_execs;
    std::size_t num_test_executions = 0;
    for (int i = 0; i < m_config.num_repetitions; ++i) {
        for (std::size_t j = 0; j < num_selected; ++j) {
            Exec exec { j, i + 1 };
            const Test& test = included_tests[j];
            // In case only one test thread was asked for, we run all tests as nonconcurrent
            // tests to avoid reordering
            auto& execs = (test.list_entry->allow_concur && num_threads > 1 ? concur_execs : nonconcur_execs);
            execs.push_back(exec); // Throws
            core::int_add(num_test_executions, 1); // Throws
        }
    }

    // Don't start more threads than are needed
    {
        std::size_t max_threads = concur_execs.size();
        if (max_threads == 0 && !nonconcur_execs.empty())
            max_threads = 1;
        if (max_threads < unsigned(num_threads))
            num_threads = int(max_threads);
    }

    // Shuffle
    if (m_config.shuffle) {
        auto seed_seq = core::SeedSeq::no_copy(m_config.random_seed.span());
        std::mt19937_64 random(seed_seq); // Throws
        std::shuffle(concur_execs.begin(), concur_execs.end(), random);
        std::shuffle(nonconcur_execs.begin(), nonconcur_execs.end(), random);
    }

    // Logging
    std::unique_ptr<std::string[]> log_paths;
    std::unique_ptr<std::unique_ptr<log::Logger>[]> loggers =
        std::make_unique<std::unique_ptr<log::Logger>[]>(num_threads); // Throws
    if (num_threads != 1 || m_config.log_to_files) {
        core::CharMapper char_mapper(m_locale); // Throws
        core::IntegerFormatter integer_formatter(char_mapper);
        int n_width = int(integer_formatter.format_dec(num_threads).size()); // Throws
        if (!m_config.log_to_files) {
            core::StringFormatter string_formatter(m_locale); // Throws
            for (int i = 0; i != num_threads; ++i) {
                std::string_view formatted_thread_num =
                    integer_formatter.format_dec(i + 1, n_width); // Throws
                std::string_view prefix =
                    string_formatter.format("Thread[%s]: ", formatted_thread_num); // Throws
                loggers[i] = std::make_unique<log::PrefixLogger>(m_logger, prefix); // Throws
            }
        }
        else {
            core::TimestampFormatter timestamp_formatter(m_locale); // Throws
            auto now = std::chrono::system_clock::now();
            int thread_index;
            int i_max = (num_threads > 0 ? num_threads - 1 : 0);
            int i_width = int(integer_formatter.format_dec(i_max).size()); // Throws
            using Template = core::StringTemplate<>;
            Template::Parameters params;
            params["t"] = [&](std::ostream& out) {
                core::timespec_type now_2 = core::time_point_to_timespec(now);
                core::TimestampFormatter::Params params;
                params.precision = core::TimestampFormatter::Precision::seconds;
                params.format = "%Y%m%d_%H%M%S";
                out << timestamp_formatter.format_local(now_2.tv_sec, params); // Throws
            }; // Throws
            params["T"] = [&](std::ostream& out) {
                core::timespec_type now_2 = core::time_point_to_timespec(now);
                core::TimestampFormatter::Params params;
                params.precision = core::TimestampFormatter::Precision::seconds;
                params.format = "%Y%m%d_%H%M%S_";
                out << timestamp_formatter.format_local(now_2.tv_sec, params); // Throws
                long microseconds = now_2.tv_nsec / 1000;
                out << integer_formatter.format_dec(microseconds, 6); // Throws
            }; // Throws
            params["i"] = [&](std::ostream& out) {
                out << integer_formatter.format_dec(thread_index); // Throws
            }; // Throws
            params["I"] = [&](std::ostream& out) {
                out << integer_formatter.format_dec(thread_index, i_width); // Throws
            }; // Throws
            params["n"] = [&](std::ostream& out) {
                out << integer_formatter.format_dec(thread_index + 1); // Throws
            }; // Throws
            params["N"] = [&](std::ostream& out) {
                out << integer_formatter.format_dec(thread_index + 1, n_width); // Throws
            }; // Throws
            Template::Parser parser(m_locale); // Throws
            Template templ;
            auto error_handler = [&](Template::Parser::Error, std::string_view message) -> bool {
                std::string message_2 = core::concat(std::string_view("Bad log path template: "), message); // Throws
                throw std::runtime_error(message_2);
            };
            bool success = parser.try_parse(m_config.log_path_template, params, templ,
                                            std::move(error_handler)); // Throws
            ARCHON_ASSERT(success);
            bool is_thread_specific = (templ.refers_to("i") || templ.refers_to("I") ||
                                       templ.refers_to("n") || templ.refers_to("N"));
            if (!is_thread_specific)
                throw std::runtime_error("Bad log path template: Must be thread specific");
            Template::Expander expander(m_locale); // Throws
            log_paths = std::make_unique<std::string[]>(num_threads); // Throws
            for (int i = 0; i != num_threads; ++i) {
                thread_index = i;
                std::string_view path_1 = expander.expand(templ); // Throws
                namespace fs = std::filesystem;
                fs::path path_2 = core::make_fs_path_generic(path_1, m_locale); // Throws
                if (!ensure_subdir(path_2.parent_path(), m_config.log_file_base_dir)) // Throws
                    throw std::runtime_error("Bad log path template in test configuration");
                fs::path path_3 = m_config.log_file_base_dir / path_2; // Throws
                if (!m_config.log_timestamps) {
                    using logger_type = log::FileLogger;
                    logger_type::Config config;
                    configure_file_logger(config, m_config); // Throws
                    loggers[i] = std::make_unique<logger_type>(path_3, m_locale, config); // Throws
                }
                else {
                    using logger_type = log::TimestampFileLogger;
                    logger_type::Config config;
                    configure_timestamp_logger(config, m_config); // Throws
                    configure_file_logger(config, m_config); // Throws
                    loggers[i] = std::make_unique<logger_type>(path_3, m_locale, config); // Throws
                }
                log_paths[i] = core::path_to_string_native(path_3, m_locale); // Throws
            }
        }
    }

    // Reporting
    Reporter fallback_reporter;
    Reporter& reporter = (m_config.reporter ? *m_config.reporter : fallback_reporter);

    // Directory for test files
    fs::path test_file_subdir = core::make_fs_path_generic(m_config.test_file_subdir, m_locale); // Throws
    if (!ensure_subdir(test_file_subdir, m_config.test_file_base_dir)) // Throws
        throw std::runtime_error("Bad test file subdirectory specification in test configuration");
    fs::path test_file_dir = m_config.test_file_base_dir / test_file_subdir; // Throws

    // Execute
    using impl::RootContextImpl;
    RootContextImpl root_context(m_config.num_repetitions, num_threads, m_locale, m_logger, log_paths.get(), reporter,
                                 included_tests, concur_execs, nonconcur_execs, m_config.abort_on_failure,
                                 m_config.keep_test_files, test_file_dir, m_config.data_file_base_dir,
                                 m_config.source_path_mapper, m_config.random_seed, m_config.rseed_rep_no_override);
    reporter.root_begin(root_context); // Throws
    std::cerr << "----> CLICK 4\n";
    core::Timer timer(core::Timer::Type::monotonic_clock); // Throws
    using impl::ThreadContextImpl;
    if (num_threads == 1) {
        log::Logger& logger = (loggers[0] ? *loggers[0] : m_logger);
        ThreadContextImpl thread_context(root_context, 0, logger, m_config.inner_log_level_limit); // Throws
        thread_context.run(); // Throws
        thread_context.nonconcur_run(); // Throws
    }
    else {
        std::unique_ptr<std::unique_ptr<ThreadContextImpl>[]> thread_contexts =
            std::make_unique<std::unique_ptr<ThreadContextImpl>[]>(num_threads); // Throws
        for (int i = 0; i < num_threads; ++i) {
            thread_contexts[i] = std::make_unique<ThreadContextImpl>(root_context, i, *loggers[i],
                                                                     m_config.inner_log_level_limit); // Throws
        }
        // First execute regular (concurrent) tests
        {
            std::unique_ptr<core::ThreadGuard[]> threads =
                std::make_unique<core::ThreadGuard[]>(num_threads); // Throws
            core::CharMapper char_mapper(m_locale); // Throws
            core::IntegerFormatter integer_formatter(char_mapper);
            for (int i = 0; i < num_threads; ++i) {
                auto func = [&, i] {
                    thread_contexts[i]->run(); // Throws
                };
                core::ThreadGuard::Config config;
                std::string name = core::concat(std::string_view("test-thread-"),
                                                integer_formatter.format_dec(i + 1)); // Throws
                config.thread_name = core::ThreadGuard::ThreadName(std::move(name), m_locale);
                config.block_signals = true;
                threads[i] = core::ThreadGuard(func, std::move(config)); // Throws
            }
            for (int i = 0; i < num_threads; ++i)
                threads[i].join(); // Throws
            for (int i = 0; i < num_threads; ++i)
                threads[i].join_and_rethrow(); // Throws
        }
        // Then execute nonconcurrent tests on main thread
        if (root_context.last_thread_to_end != -1)
            thread_contexts[root_context.last_thread_to_end]->nonconcur_run(); // Throws
    }

    // Summarize
    Summary summary;
    summary.num_disabled_tests         = long(num_disabled);
    summary.num_excluded_tests         = long(num_enabled - num_selected);
    summary.num_selected_tests         = long(num_selected);
    summary.num_test_executions        = long(num_test_executions);
    summary.num_failed_test_executions = root_context.num_failed_test_executions;
    summary.num_checks                 = root_context.num_checks;
    summary.num_failed_checks          = root_context.num_failed_checks;
    summary.elapsed_seconds            = timer.get_elapsed_time(); // Throws
    reporter.root_end(root_context, summary); // Throws

    return (root_context.num_failed_test_executions == 0);
}


auto TestRunner::make_logger(const std::locale& loc, const check::TestConfig& test_config) ->
    std::unique_ptr<log::Logger>
{
    std::cerr << "TestRunner::make_logger() - 1\n";    
    if (!test_config.log_timestamps || test_config.log_to_files) {
        std::cerr << "TestRunner::make_logger() - 2\n";    
        if (!test_config.logger) {
            std::cerr << "TestRunner::make_logger() - 3\n";    
            return std::make_unique<log::FileLogger>(core::File::get_cout(), loc); // Throws                   
        }
        return nullptr;
    }

    std::cerr << "TestRunner::make_logger() - 4\n";    
    if (!test_config.logger) {
        std::cerr << "TestRunner::make_logger() - 5\n";    
        log::TimestampFileLogger::Config config;
        configure_timestamp_logger(config, test_config); // Throws
        return std::make_unique<log::TimestampFileLogger>(core::File::get_cout(), loc, std::move(config)); // Throws
    }
    log::TimestampLogger::Config config;
    configure_timestamp_logger(config, test_config); // Throws
    return std::make_unique<log::TimestampLogger>(*test_config.logger, std::move(config)); // Throws
}
