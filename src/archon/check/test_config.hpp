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

#ifndef ARCHON_X_CHECK_X_TEST_CONFIG_HPP
#define ARCHON_X_CHECK_X_TEST_CONFIG_HPP

/// \file


#include <string_view>
#include <filesystem>

#include <archon/log/logger.hpp>
#include <archon/check/random_seed.hpp>
#include <archon/check/test_details.hpp>
#include <archon/check/test_list.hpp>
#include <archon/check/reporter.hpp>


namespace archon::check {


/// \brief Base class for test case filters.
///
/// This is the base class for test case filters. If a filter is specified through \ref
/// check::TestConfig::filter, it will be used to determine which test cases are to be
/// executed. An enabled test case will be executed if, and only if \ref include() returns
/// `true` for it.
///
/// See \ref check::WildcardFilter for a concrete filter implementation.
///
class Filter {
public:
    virtual bool include(const check::TestDetails&) const noexcept = 0;
    virtual ~Filter() noexcept = default;
};



/// \brief Base class for test case ordering comparators.
///
/// This is the base class for test case ordering comparators. If an ordering comparator is
/// specified through \ref check::TestConfig::test_order, it will be used to sort the
/// selected test cases into their execution order. If shuffling is not enabled (\ref
/// check::TestConfig::shuffle), and the number of repetitions is 1 (\ref
/// check::TestConfig::num_repetitions), then the execution order is the order in which
/// their execution will begin.
///
/// If \ref less() returns `true` for an ordered pair of test cases, A and B, then A will
/// come before B in the execution order.
///
/// If two test cases compare equal (A is not less than B, and B is not less than A), and
/// the two test cases reside in the same translation check (`.cpp` file), then those two
/// test cases will be executed in the order that they occur in that translation check. If
/// two test cases compare equal, and they reside in different translation checks, then the
/// execution order of those two test cases is unspecified. Usually, however, the order will
/// agree with the order in which the respective translation checks occur in the linker
/// command line.
///
/// See \ref check::PatternBasedTestOrder for a concrete comparator implementation.
///
class TestOrder {
public:
    virtual bool less(const check::TestDetails&, const check::TestDetails&) const noexcept = 0;
    virtual ~TestOrder() noexcept = default;
};



/// \brief Base class for source file path mappers.
///
/// This is the base class for source file path mappers. You can use a source file path
/// mapper to change the paths of source files from their form in the `__FILE__` macro into
/// one that is more desirable for reporting progress and failures during the tesing
/// process.
///
/// If a source file path mapper is specified through \ref
/// check::TestConfig::source_path_mapper, it will be used to map source file paths as they
/// are presented to the test cases and to reporters (\ref check::Reporter) through \ref
/// check::TestContext::mapped_file_path.
///
/// The testing harness constructs a filesystem path object from the value of `__FILE__` and
/// passes it to the mapper by calling \ref map(). If the mapping operation is successful,
/// \ref map() should return `true`. Otherwise it should return `false`.
///
/// See \ref check::StandardPathMapper for a concrete source file path mapper implementation.
///
class SourcePathMapper {
public:
    virtual bool map(std::filesystem::path&) const = 0;
    virtual ~SourcePathMapper() noexcept = default;
};




/// \brief Check testing configuration parameters.
///
/// These are the available parameters for controlling the operation of \ref check::run().
///
struct TestConfig {
    /// \brief Number of times to execute each test case.
    ///
    /// The number of times to repeat the execution of each of the selected and enabled test
    /// cases. This must be a non-negative number.
    ///
    /// Each execution will be identified by its execution repetition number (\ref
    /// check::TestContext::repetition_no). For example, if tests A, B, and C are selected
    /// and enabled, and the number of repetitions is set to 3, then A1, B1, C1, A2, B2, C2,
    /// A3, B3, C3 are the executions that will take place. If shuffling is disabled, the
    /// executions will be initiated in that order. If shuffling is enabled (see \ref
    /// shuffle), they will be initiated in a random order as if the shown sequence of
    /// executions were shuffled, for example, A1, A3, B2, A2, C3, C1, B1, C2, B3.
    ///
    int num_repetitions = 1;

    /// \brief Maximum number of testing threads.
    ///
    /// This is the maximum number of threads that will be used to execute test cases. It is
    /// therefore also the maximum number of test cases that will be able to execute
    /// concurrently. This must be a non-negative number.
    ///
    /// If the specified value is zero (the default), the effective number will be the value
    /// returned by `std::thread::hardware_concurrency()`, or 1 if
    /// `std::thread::hardware_concurrency()` returns zero.
    ///
    /// The effective number of testing threads is available through \ref
    /// check::RootContext::num_threads, or
    /// `test_context.thread_context.root_context.num_threads` when starting from \ref
    /// check::TestContext. The effective number of threads can be less than the specified
    /// number if the number of test cases to execute is small.
    ///
    int num_threads = 0;

    /// \brief Randomize test case execution order.
    ///
    /// If set to `true`, the order of execution of test cases will be randomized. See \ref
    /// num_repetitions for further details.
    ///
    bool shuffle = false;

    /// \brief Abort testing process on first failure.
    ///
    /// Abort the testing process as soon as a check fails or an unexpected exception is
    /// thrown in a test case.
    ///
    bool abort_on_failure = false;

    /// \brief Add timestamps to log messages.
    ///
    /// Add timestamps to log messages by inserting a timestamp logger (\ref
    /// log::TimestampLogger) at the outermost level. If file logging is enabled, the
    /// timestamps will be added to messages that are logged to files. Otherwise, the
    /// timestamps will be added to the messages logged via \ref logger, or if \ref logger
    /// is not specified, the fallback logger (\ref log::Logger::get_stdout()).
    ///
    bool log_timestamps = false;

    /// \brief Enable per-thread file logging.
    ///
    /// When `log_to_files` is `false` (the default), all logging is routed through the
    /// specified logger (\ref logger) or sent to `STDOUT` (\ref log::Logger::get_stdout()) if
    /// none is specified.
    ///
    /// When `log_to_files` is set to `true`, most log messages are instead sent to a log
    /// file. Each thread sends messages to a separate log file. See \ref log_path_template
    /// and \ref log_file_base_dir. Log messages that are not specific to a particular
    /// thread will still be routed through the specified logger or sent to `STDOUT`.
    ///
    /// The files will be opened in "append" mode (\ref core::File::Mode::append).
    ///
    bool log_to_files = false;

    /// \brief Keep test files.
    ///
    /// Setting this flag to `true` disables the automatic removal of test files when test
    /// file guards are destroyed (\ref check::TestFileGuard, \ref check::TestDirGuard).
    ///
    /// \sa \ref check::TestContext::keep_test_files()
    ///
    bool keep_test_files = false;

    /// \brief List of test cases to be considered for execution.
    ///
    /// The list of test cases to be considered for execution. See \ref check::TestList. If
    /// no list is specified, the default list will be used. The default list is the one
    /// accessible via \ref check::TestList::get_default_list(), which is also the list to
    /// which tests are added when using \ref ARCHON_TEST().
    ///
    const check::TestList* test_list = nullptr;

    /// \brief Select subset of test cases to execute.
    ///
    /// If a filter is specified, only the test cases that match the filter and are enabled
    /// (parameter \p enabled of \ref ARCHON_TEST_IF()) will be executed. Otherwise all test
    /// cases, that are enabled, will be executed.
    ///
    const check::Filter* filter = nullptr;

    /// \brief Control test case execution order.
    ///
    /// If a test case ordering comparator is specified, it will be used to sort the
    /// selected test cases into their execution order. See \ref check::TestOrder for more
    /// on this.
    ///
    const check::TestOrder* test_order = nullptr;

    /// \brief Log through specified logger.
    ///
    /// If per-thread file logging is not enabled (\ref log_to_files), all logging will go
    /// through the specified logger. This includes logging performed as part of reporting
    /// (\ref check::Reporter), and logging coming from inside test cases (\ref
    /// check::TestContext::logger). If per-thread file logging is enabled, only log
    /// messages that transcend the thread level will be directed through the specified
    /// logger, such as the reporting of the summary at the end of testing.
    ///
    /// If a logger is not specified, messages will be routed to STDOUT.
    ///
    /// If a logger is specified, it must use a locale that is compatible with the locale of
    /// the test runner (\ref check::TestRunner). The important thing is that the character
    /// encodings agree (`std::codecvt` facet).
    ///
    /// The specified logger must be thread-safe.
    ///
    log::Logger* logger = nullptr;

    /// \brief Log level limit lor logging from inside test cases.
    ///
    /// The log level limit to apply to logging coming from inside test cases (\ref
    /// check::TestContext::logger). To disable such logging entirely, set this to \ref
    /// log::LogLevel::off (the default).
    ///
    log::LogLevel inner_log_level_limit = log::LogLevel::off;

    /// \brief Report on progress of testing process.
    ///
    /// If a reporter is specified, it will be given the opportunity to report on the
    /// progress of the testing process.
    ///
    /// If no reporter is specified, nothing will be reported.
    ///
    check::Reporter* reporter = nullptr;

    /// \brief Optional path mapper for source files.
    ///
    /// If a source path mapper is specified, mapped source file paths will be made
    /// available through \ref check::TestContext::mapped_file_path and \ref
    /// check::FailContext::mapped_file_path. The paths that will be passed to the specified
    /// mapper are those passed as `file_path` arguments to \ref check::TestList::add() and
    /// to functions like \ref check::TestContext::check_general_cond() and \ref
    /// check::TestContext::check_special_cond(). Those paths, in turn, must be values of
    /// the `__FILE__` macro for the various source files that contribute test cases, or in
    /// which checks are performed.
    ///
    /// If a source path mapper is not specified, \ref check::TestContext::mapped_file_path
    /// and \ref check::FailContext::mapped_file_path will refer to the the unmapped paths.
    ///
    const check::SourcePathMapper* source_path_mapper = nullptr;

    /// \brief Root directory for data files.
    ///
    /// The base directory for data files. This is the directory against which \ref
    /// check::TestContext::get_data_path() resolves the specified relative paths.
    ///
    /// This base directory must be the root of the source file directory structure, or the
    /// root of a reflection of the source file directory structure in which all relevant
    /// data files are present. See \ref core::BuildEnvironment::get_relative_source_root().
    ///
    /// If the specified base directory path is not absolute, it will be understood as
    /// relative to the current working directory.
    ///
    /// It makes no difference whether the specified path has a final directory separator
    /// (`/`) as long as the path would be nonempty without one.
    ///
    std::filesystem::path data_file_base_dir;

    /// \brief Path template for per-thread log files.
    ///
    /// The specified string will be used as a template for constructing log file paths (see
    /// table of available parameters below). It will be separately expanded for each test
    /// thread. The path must be on relative form, and will be resolved against \ref
    /// log_file_base_dir. Any directories explicitely mentioned in the specified path will
    /// be created, if they do not already exist. The path must be specified in the generic
    /// format as understood by `std::filesystem::path`.
    ///
    ///   | Parameter | Meaning
    ///   |-----------|-------------------------------------------------------------------------
    ///   | `@t`      | Timestamp with seconds precision (`<date>_<time>`)
    ///   | `@T`      | Timestamp with microseconds precision (`<date>_<time>_<micro seconds>`)
    ///   | `@i`      | Thread index (0 -> N-1) where N is number of threads
    ///   | `@I`      | Thread index with leading zeroes included
    ///   | `@n`      | Thread number (1 -> N) where N is number of threads
    ///   | `@N`      | Thread number with leading zeroes included
    ///
    /// Template expansion is performed as if by \ref core::BasicStringTemplate.
    ///
    std::string_view log_path_template =  "tmp/log/@T/thread_@N.log";

    /// \brief Base path for per-thread log files.
    ///
    /// If the paths expanded from \ref log_path_template are relative, they will be
    /// resolved against the base directory path specified here.
    ///
    /// If the path specified here is relative, it will be understood as being relative to
    /// the current working directory (`std::std::filesystem::current_path()`). This also
    /// means that if this path is empty (the default), the effective base directory is the
    /// current working diretory.
    ///
    /// It makes no difference whether the specified path has a final directory separator
    /// (`/`) as long as the path would be nonempty without one.
    ///
    /// An appropriate path to use here might be the project root directory as returned by
    /// \ref core::BuildEnvironment::get_relative_project_root().
    ///
    /// It is an error if the specified directory does not already exist.
    ///
    std::filesystem::path log_file_base_dir;

    /// \brief Subdirectory for test files.
    ///
    /// This is the subdirectory in which test files and directories will be placed, that
    /// is, the test files and directories created by test cases (\ref
    /// check::TestContext::make_test_path(), \ref check::TestFileGuard, \ref
    /// check::TestDirGuard). The path must be on relative form (no root name and no root
    /// directory part), and will be resolved against \ref test_file_base_dir. Specifying an
    /// empty path causes files and directories to be placed directly in the base directory.
    ///
    /// The path must be specified in the generic format as understood by
    /// `std::filesystem::path`.
    ///
    /// It makes no difference whether the specified path has a final directory separator
    /// (`/`) as long as the path would be nonempty without one.
    ///
    /// All directories explicitely mentioned in the specified path will be created, if they
    /// do not already exist.
    ///
    std::string_view test_file_subdir = "tmp/test";

    /// \brief Base directory for test files.
    ///
    /// The base directory against which \ref test_file_subdir is resolved. It can be
    /// relative or absolute. If it is relative, it will be interpreted as being relative to
    /// the current working directory.
    ///
    /// It makes no difference whether the specified path has a final directory separator
    /// (`/`) as long as the path would be nonempty without one.
    ///
    /// An appropriate path to use here might be the project root directory as returned by
    /// \ref core::BuildEnvironment::get_relative_project_root().
    ///
    /// It is an error if the specified directory does not already exist.
    ///
    std::filesystem::path test_file_base_dir;

    /// \brief Random seed offered to test cases.
    ///
    /// The specified random seed will be used as input to the seed sequence offered through
    /// \ref check::TestContext::seed_seq(). To get a nondeterministic seed, use \ref
    /// check::RandomSeed::random().
    ///
    check::RandomSeed random_seed;

    /// \brief Override repetition number for random seed sequence.
    ///
    /// When zero (the default), random seed sequences offered through \ref
    /// check::TestContext::seed_seq() are based in part by \ref random_seed and in part by
    /// the ordinal number of the current repetition of the executing test case. When
    /// nonzero, the specified value is used in place of the true repetition number.
    ///
    int rseed_rep_no_override = 0;
};


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_TEST_CONFIG_HPP
