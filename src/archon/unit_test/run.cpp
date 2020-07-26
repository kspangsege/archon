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


#include <cstddef>
#include <limits>
#include <algorithm>
#include <memory>
#include <utility>
#include <string_view>
#include <string>
#include <stdexcept>
#include <chrono>
#include <random>
#include <vector>
#include <map>
#include <thread>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/string.hpp>
#include <archon/base/time.hpp>
#include <archon/base/timer.hpp>
#include <archon/base/random.hpp>
#include <archon/base/filesystem.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/integer_formatter.hpp>
#include <archon/base/timestamp_formatter.hpp>
#include <archon/base/string_formatter.hpp>
#include <archon/base/format.hpp>
#include <archon/base/timestamp_logger.hpp>
#include <archon/base/prefix_logger.hpp>
#include <archon/base/string_template.hpp>
#include <archon/base/thread_guard.hpp>
#include <archon/unit_test/run.hpp>
#include <archon/unit_test/noinst/root_context_impl.hpp>
#include <archon/unit_test/noinst/thread_context_impl.hpp>


using namespace archon;
using namespace archon::unit_test;


namespace {


/// FIXME: Move to `<archon/base/filesystem.hpp>`
bool ensure_subdir(base::FilesystemPathRef subdir, base::FilesystemPathRef base_dir)
{
    namespace fs = std::filesystem;
    // Note: `fs::create_directories()` cannot be used here as that would risk
    // creation of `base_dir`, or of directories outside `base_dir`.
    fs::path subdir_2 = subdir; // Throws
    base::remove_trailing_slash(subdir_2); // Throws
    bool good = (!subdir_2.has_root_name() && !subdir_2.has_root_directory()); // Throws
    if (ARCHON_UNLIKELY(!good))
        return false;
    fs::path path;
    for (const auto& segment : subdir_2) {
        if (ARCHON_LIKELY(segment != base::fs_dot_path)) {
            if (ARCHON_LIKELY(segment != base::fs_dot_dot_path)) {
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


} // unnamed namespace



bool unit_test::run(TestConfig config)
{
    if (config.num_repetitions < 0)
        throw std::runtime_error("Bad number of repetitions");

    // Filter, check for name clashes, and map file paths
    const TestList& test_list =
        (config.test_list ? *config.test_list : TestList::get_default_list());
    using Test = detail::RootContextImpl::Test;
    std::vector<Test> included_tests;
    std::size_t num_enabled = 0, num_disabled = 0;
    namespace fs = std::filesystem;
    std::map<std::string_view, std::string> mapped_file_paths;
    {
        std::map<std::string_view, const TestList::Entry*> map;
        for (const TestList::Entry& entry : test_list) {
            {
                auto p = map.emplace(entry.details.name, &entry); // Throws
                bool was_inserted = p.second;
                if (ARCHON_UNLIKELY(!was_inserted)) {
                    const TestList::Entry& first = *p.first->second;
                    fs::path path_1 = base::make_fs_path_auto(first.details.location.file_path,
                                                              config.locale); // Throws
                    fs::path path_2 = base::make_fs_path_auto(entry.details.location.file_path,
                                                              config.locale); // Throws
                    if (config.source_path_mapper) {
                        config.source_path_mapper->map(path_1); // Throws
                        config.source_path_mapper->map(path_2); // Throws
                    }
                    long line_1 = first.details.location.line_number;
                    long line_2 = entry.details.location.line_number;
                    std::string message =
                        base::format(config.locale, "Multiple unit tests with name `%s` (`%s:%s` "
                                     "and `%s:%s`)", entry.details.name, path_1, line_1, path_2,
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
            if (config.filter && !config.filter->include(entry.details))
                continue;
            std::string_view mapped_file_path;
            {
                std::string_view path_1 = entry.details.location.file_path;
                auto p = mapped_file_paths.emplace(path_1, std::string()); // Throws
                bool was_inserted = p.second;
                if (was_inserted) {
                    fs::path path_2 = base::make_fs_path_auto(path_1, config.locale); // Throws
                    if (config.source_path_mapper)
                        config.source_path_mapper->map(path_2); // Throws
                    p.first->second = base::path_to_string_native(path_2, config.locale); // Throws
                }
                mapped_file_path = p.first->second;
            }
            included_tests.push_back({ &entry, mapped_file_path }); // Throws
        }
    }
    std::size_t num_selected = included_tests.size();

    // Number of threads
    int num_threads = config.num_threads;
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
    if (config.test_order) {
        const TestOrder& test_order = *config.test_order;
        auto compare = [&](const Test& a, const Test& b) noexcept {
            return test_order.less(a.list_entry->details, b.list_entry->details);
        };
        std::stable_sort(included_tests.begin(), included_tests.end(), compare);
    }

    // Repeat
    using Exec = detail::RootContextImpl::Exec;
    std::vector<Exec> concur_execs, nonconcur_execs;
    std::size_t num_test_executions = 0;
    for (int i = 0; i < config.num_repetitions; ++i) {
        for (std::size_t j = 0; j < num_selected; ++j) {
            Exec exec { j, i };
            const Test& test = included_tests[j];
            // In case only one test thread was asked for, we run all tests as
            // nonconcurrent tests to avoid reordering
            auto& execs = (test.list_entry->allow_concur && num_threads > 1 ? concur_execs :
                           nonconcur_execs);
            execs.push_back(exec); // Throws
            base::int_add(num_test_executions, 1); // Throws
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
    if (config.shuffle) {
        auto seed_seq = base::SeedSeq::no_copy(config.random_seed.span());
        std::mt19937_64 random(seed_seq); // Throws
        std::shuffle(concur_execs.begin(), concur_execs.end(), random);
        std::shuffle(nonconcur_execs.begin(), nonconcur_execs.end(), random);
    }

    // Logging
    auto configure_timestamp_logger = [](base::TimestampLogger::Config& config_2) noexcept {
        config_2.precision = base::TimestampLogger::Precision::milliseconds;
        config_2.format = "%FT%T: ";
    };
    auto configure_file_logger = [](base::FileLogger::Config&) noexcept {
    };
    std::unique_ptr<base::TimestampLogger> timestamp_logger;
    base::Logger* root_logger;
    {
        base::Logger& logger = (config.logger ? *config.logger :
                                base::Logger::get_cout()); // Throws
        bool add_timestamps = (config.log_timestamps && !config.log_to_files);
        if (!add_timestamps) {
            root_logger = &logger;
        }
        else {
            base::TimestampLogger::Config config;
            configure_timestamp_logger(config);
            timestamp_logger = std::make_unique<base::TimestampLogger>(logger, config); // Throws
            root_logger = &*timestamp_logger;
        }
    }
    std::unique_ptr<std::string[]> log_paths;
    std::unique_ptr<std::unique_ptr<base::Logger>[]> loggers =
        std::make_unique<std::unique_ptr<base::Logger>[]>(num_threads); // Throws
    if (num_threads != 1 || config.log_to_files) {
        base::CharMapper char_mapper(config.locale); // Throws
        base::IntegerFormatter integer_formatter(char_mapper);
        int n_width = int(integer_formatter.format_dec(num_threads).size()); // Throws
        if (!config.log_to_files) {
            base::StringFormatter string_formatter(config.locale); // Throws
            for (int i = 0; i != num_threads; ++i) {
                std::string_view formatted_thread_num =
                    integer_formatter.format_dec(i + 1, n_width); // Throws
                std::string_view prefix =
                    string_formatter.format("Thread[%s]: ", formatted_thread_num); // Throws
                loggers[i] = std::make_unique<base::PrefixLogger>(*root_logger, prefix); // Throws
            }
        }
        else {
            base::TimestampFormatter timestamp_formatter(config.locale); // Throws
            auto now = std::chrono::system_clock::now();
            int thread_index;
            int i_max = (num_threads > 0 ? num_threads - 1 : 0);
            int i_width = int(integer_formatter.format_dec(i_max).size()); // Throws
            using Template = base::StringTemplate<>;
            Template::Parameters params;
            params["t"] = [&](std::ostream& out) {
                base::timespec_type now_2 = base::time_point_to_timespec(now);
                base::TimestampFormatter::Params params;
                params.precision = base::TimestampFormatter::Precision::seconds;
                params.format = "%Y%m%d_%H%M%S";
                out << timestamp_formatter.format_local(now_2.tv_sec, params); // Throws
            }; // Throws
            params["T"] = [&](std::ostream& out) {
                base::timespec_type now_2 = base::time_point_to_timespec(now);
                base::TimestampFormatter::Params params;
                params.precision = base::TimestampFormatter::Precision::seconds;
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
            Template::Parser parser(config.locale); // Throws
            Template templ;
            auto error_handler = [&](std::string_view message) -> bool {
                std::string message_2 =
                    base::concat(std::string_view("Bad log path template: "), message); // Throws
                throw std::runtime_error(message_2);
            };
            bool success =
                parser.try_parse(config.log_path_template, params, templ,
                                 std::move(error_handler)); // Throws
            ARCHON_ASSERT(success);
            bool is_thread_specific = (templ.refers_to("i") || templ.refers_to("I") ||
                                       templ.refers_to("n") || templ.refers_to("N"));
            if (!is_thread_specific)
                throw std::runtime_error("Bad log path template: Must be thread specific");
            Template::Expander expander(config.locale); // Throws
            log_paths = std::make_unique<std::string[]>(num_threads); // Throws
            for (int i = 0; i != num_threads; ++i) {
                thread_index = i;
                std::string_view path_1 = expander.expand(templ); // Throws
                namespace fs = std::filesystem;
                fs::path path_2 = base::make_fs_path_generic(path_1, config.locale); // Throws
                if (!ensure_subdir(path_2.parent_path(), config.log_file_base_dir)) // Throws
                    throw std::runtime_error("Bad log path template in test configuration");
                fs::path path_3 = config.log_file_base_dir / path_2; // Throws
                if (!config.log_timestamps) {
                    using logger_type = base::FileLogger;
                    logger_type::Config config_2;
                    configure_file_logger(config_2);
                    loggers[i] = std::make_unique<logger_type>(path_3, config_2); // Throws
                }
                else {
                    using logger_type = base::TimestampFileLogger;
                    logger_type::Config config_2;
                    configure_timestamp_logger(config_2);
                    configure_file_logger(config_2);
                    loggers[i] = std::make_unique<logger_type>(path_3, config_2); // Throws
                }
                log_paths[i] = base::path_to_string_native(path_3, config.locale); // Throws
            }
        }
    }

    // Reporting
    Reporter fallback_reporter;
    Reporter& reporter = (config.reporter ? *config.reporter : fallback_reporter);

    // Directory for test files
    fs::path test_file_subdir =
        base::make_fs_path_generic(config.test_file_subdir, config.locale); // Throws
    if (!ensure_subdir(test_file_subdir, config.test_file_base_dir)) // Throws
        throw std::runtime_error("Bad test file subdirectory specification in test configuration");
    fs::path test_file_dir = config.test_file_base_dir / test_file_subdir; // Throws

    // Execute
    using detail::RootContextImpl;
    RootContextImpl root_context(config.num_repetitions, num_threads, std::move(config.locale),
                                 *root_logger, log_paths.get(), reporter,
                                 std::move(included_tests), std::move(concur_execs),
                                 std::move(nonconcur_execs), config.abort_on_failure,
                                 config.keep_test_files, std::move(test_file_dir),
                                 std::move(config.data_root_dir), config.source_path_mapper,
                                 std::move(config.random_seed));
    reporter.root_begin(root_context); // Throws
    base::Timer timer(base::Timer::Type::monotonic_clock);
    using detail::ThreadContextImpl;
    if (num_threads == 1) {
        base::Logger* logger = loggers[0].get();
        if (!logger)
            logger = root_logger;
        ThreadContextImpl thread_context(root_context, 0, *logger,
                                         config.inner_log_level_limit); // Throws
        thread_context.run(); // Throws
        thread_context.nonconcur_run(); // Throws
    }
    else {
        std::unique_ptr<std::unique_ptr<ThreadContextImpl>[]> thread_contexts =
            std::make_unique<std::unique_ptr<ThreadContextImpl>[]>(num_threads); // Throws
        for (int i = 0; i < num_threads; ++i) {
            thread_contexts[i] =
                std::make_unique<ThreadContextImpl>(root_context, i, *loggers[i],
                                                    config.inner_log_level_limit); // Throws
        }
        // First execute regular (concurrent) tests
        {
            std::unique_ptr<base::ThreadGuard[]> threads =
                std::make_unique<base::ThreadGuard[]>(num_threads); // Throws
            base::CharMapper char_mapper(config.locale); // Throws
            base::IntegerFormatter integer_formatter(char_mapper);
            for (int i = 0; i < num_threads; ++i) {
                auto func = [&, i] {
                    thread_contexts[i]->run(); // Throws
                };
                base::ThreadGuard::Config config;
                config.thread_name = base::concat(std::string_view("test-thread-"),
                                                  integer_formatter.format_dec(i + 1)); // Throws
                config.block_signals = true;
                threads[i] = base::ThreadGuard(func, std::move(config)); // Throws
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
    summary.elapsed_seconds            = timer.get_elapsed_time();
    reporter.root_end(root_context, summary); // Throws

    return (root_context.num_failed_test_executions == 0);
}
