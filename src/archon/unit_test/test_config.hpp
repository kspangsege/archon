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

#ifndef ARCHON_X_UNIT_TEST_X_TEST_CONFIG_HPP
#define ARCHON_X_UNIT_TEST_X_TEST_CONFIG_HPP

/// \file


#include <string_view>
#include <locale>
#include <filesystem>

#include <archon/base/logger.hpp>
#include <archon/unit_test/random_seed.hpp>
#include <archon/unit_test/test_details.hpp>
#include <archon/unit_test/test_list.hpp>
#include <archon/unit_test/reporter.hpp>


namespace archon::unit_test {


/// \brief Base class for unit test filters.
///
/// This is the base class for unit test filters. If a filter is specified
/// through \ref TestConfig::filter, it will be used to determine which unit
/// tests are to be executed. An enabled unit test will be executed if, and only
/// if \ref include() returns `true` for it.
///
/// See \ref WildcardFilter for a concrete filter implementation.
///
class Filter {
public:
    virtual bool include(const TestDetails&) const noexcept = 0;
    virtual ~Filter() noexcept = default;
};



/// \brief Base class for unit test ordering comparators.
///
/// This is the base class for unit test ordering comparators. If an ordering
/// comparator is specified through \ref TestConfig::test_order, it will be used
/// to sort the selected unit tests into their execution order. If shuffling is
/// not enabled (\ref TestConfig::shuffle), and the number of repetitions is 1
/// (\ref TestConfig::num_repetitions), then the execution order is the order in
/// which their execution will begin.
///
/// If \ref less() returns `true` for an ordered pair of unit tests, A and B,
/// then A will come before B in the execution order.
///
/// If two unit tests compare equal (A is not less than B, and B is not less
/// than A), and the two unit tests reside in the same translation unit (`.cpp`
/// file), then those two unit tests will be executed in the order that they
/// occur in that translation unit. If two unit tests compare equal, and they
/// reside in different translation units, then the execution order of those two
/// unit tests is unspecified. Usually, however, the order will agree with the
/// order in which the respective translation units occur in the linker command
/// line.
///
/// See \ref PatternBasedTestOrder for a concrete comparator implementation.
///
class TestOrder {
public:
    virtual bool less(const TestDetails&, const TestDetails&) const noexcept = 0;
    virtual ~TestOrder() noexcept = default;
};



/// \brief Base class for source file path mappers.
///
/// This is the base class for source file path mappers. You can use a source
/// file path mapper to change the paths of source files from their form in the
/// `__FILE__` macro into one that is more desirable for reporting progress and
/// failures during the tesing process.
///
/// If a source file path mapper is specified through \ref
/// TestConfig::source_path_mapper, it will be used to map source file paths as
/// they are presented to unit tests and to reporters (\ref Reporter) through
/// \ref TestContext::mapped_file_path.
///
/// The testing harness constructs a filesystem path object from the value of
/// `__FILE__` and passes it to the mapper by calling \ref map(). If the mapping
/// operation is successful, \ref map() should return `true`. Otherwise it
/// should return `false`.
///
/// See \ref StandardPathMapper for a concrete source file path mapper
/// implementation.
///
class SourcePathMapper {
public:
    virtual bool map(std::filesystem::path&) const = 0;
    virtual ~SourcePathMapper() noexcept = default;
};




/// \brief Unit testing configuration parameters.
///
/// These are the available parameters for controlling the operation of \ref
/// unit_test::run().
///
struct TestConfig {
    /// \brief Number of times to execute each unit test.
    ///
    /// The number of times to repeat the execution of each of the selected and
    /// enabled unit tests.
    ///
    /// Each execution will be identified by its execution recurrence index
    /// (\ref TestContext::recurrence_index). For example, if tests A, B, and C
    /// are selected and enabled, and the number of repetitions is set to 3,
    /// then A0, B0, C0, A1, B1, C1, A2, B2, C2 are the executions that will
    /// take place. If shuffling is disabled, the executions will be initiated
    /// in that order. If shuffling is enabled (see \ref shuffle), they will be
    /// initiated in a random order as if the shown sequence of executions were
    /// shuffled, for example, A0, A2, B1, A1, C2, C0, B0, C1, B2.
    ///
    int num_repetitions = 1;

    /// \brief Maximum number of testing threads.
    ///
    /// This is the maximum number of threads that will be used to execute unit
    /// tests. It is therefore also the maximum number of unit tests that will
    /// be able to execute concurrently.
    ///
    /// If the specified value is less than, or equal to zero (the default), the
    /// effective number will be the value returned by
    /// `std::thread::hardware_concurrency()`, or 1 if
    /// `std::thread::hardware_concurrency()` returns zero.
    ///
    /// The effective number of testing threads is available through \ref
    /// RootContext::num_threads, or
    /// `test_context.thread_context.root_context.num_threads` when starting
    /// from \ref TestContext. The effective number of threads can be less than
    /// the specified number if the number of unit tests to execute is small.
    ///
    int num_threads = 0;

    /// \brief Randomize unit test execution order.
    ///
    /// If set to `true`, the order of execution of unit tests will be
    /// randomized. See \ref num_repetitions for further details.
    ///
    bool shuffle = false;

    /// \brief Abort testing process on first failure.
    ///
    /// Abort testing process as soon as a check fails or an unexpected
    /// exception is thrown in a test.
    ///
    bool abort_on_failure = false;

    /// \brief Add timestamps to log messages.
    ///
    /// Add timestamps to log messages by inserting a timestamp logger (\ref
    /// base::TimestampLogger) at the outermost level. If file logging is
    /// enabled, the timestamps will be added to messages that are logged to
    /// files. Otherwise, the timestamps will be added to the messages logged
    /// via \ref logger, or if \ref logger is not specified, the fallback logger
    /// (\ref base::Logger::get_cout()).
    ///
    bool log_timestamps = false;

    /// \brief Enable per-thread file logging.
    ///
    /// When `log_to_files` is `false` (the default), all logging is routed
    /// through the specified logger (\ref logger) or sent to `STDOUT` (\ref
    /// base::Logger::get_cout()) if none is specified.
    ///
    /// When `log_to_files` is set to `true`, most log messages are instead sent
    /// to a log file. Each thread sends messages to a separate log file. See
    /// \ref log_path_template and \ref log_file_base_dir. Log messages that
    /// transcend the thread level will still be routed through the specified
    /// logger or sent to `STDOUT`.
    ///
    /// The files will be opened in "append" mode (\ref
    /// base::File::Mode::append).
    ///
    bool log_to_files = false;

    /// \brief Keep test files.
    ///
    /// Setting this flag to `true` disables the automatic removal of test files
    /// when test file guards are destroyed (\ref TestFileGuard, \ref
    /// TestDirGuard).
    ///
    /// \sa \ref TestContext::keep_test_files()
    ///
    bool keep_test_files = false;

    /// \brief List of unit tests to be considered for execution.
    ///
    /// The list of unit tests to be considered for execution. See \ref
    /// TestList. If no list is specified, the default list will be used. The
    /// default list is the one accessible via \ref
    /// TestList::get_default_list(), which is also the list to which tests are
    /// added when using \ref ARCHON_TEST().
    ///
    const TestList* test_list = nullptr;

    /// \brief Select subset of unit tests to execute.
    ///
    /// If a filter is specified, only the unit tests that match the filter and
    /// are enabled (parameter \p enabled of \ref ARCHON_TEST_IF()) will be
    /// executed. Otherwise all unit tests, that are enabled, will be executed.
    ///
    const Filter* filter = nullptr;

    /// \brief Control unit test execution order.
    ///
    /// If a unit test ordering comparator is specified, it will be used to sort
    /// the selected unit tests into their execution order. See \ref TestOrder
    /// for more on this.
    ///
    const TestOrder* test_order = nullptr;

    /// \brief Log through specified logger.
    ///
    /// If per-thread file logging is not enabled (\ref log_to_files), all
    /// logging will go through the specified logger. This includes logging
    /// performed as part of reporting (\ref Reporter), and logging coming from
    /// inside unit tests (\ref TestContext::logger). If per-thread file logging
    /// is enabled, only log messages that transcend the thread level will be
    /// directed through the specified logger, such as the reporting of the
    /// summary at the end of testing.
    ///
    /// If no logger is specified, \ref base::Logger::get_cout() will be used as
    /// a fallback.
    ///
    /// The specified logger must be thread-safe.
    ///
    base::Logger* logger = nullptr;

    /// \brief Log level limit lor logging from inside unit tests.
    ///
    /// The log level limit to apply to logging coming from inside unit tests
    /// (\ref TestContext::logger). To disable such logging entirely, set this
    /// to \ref base::LogLevel::off (the default).
    ///
    base::LogLevel inner_log_level_limit = base::LogLevel::off;

    /// \brief Report on progress of testing process.
    ///
    /// If a reporter is specified, it will be given the opportunity to report
    /// on the progress of the testing process. If no reporter is specified,
    /// nothing will be reported.
    ///
    Reporter* reporter = nullptr;

    /// \brief Optional path mapper for source files.
    ///
    /// If a source path mapper is specified, mapped source file paths will be
    /// made available through \ref TestContext::mapped_file_path and \ref
    /// FailContext::mapped_file_path. The paths that will be passed to the
    /// specified mapper are those passed as `file_path` arguments to \ref
    /// TestList::add() and to functions like \ref TestContext::check(). Those
    /// paths, in turn, must be values of the `__FILE__` macro for the various
    /// source files that contribute unit tests, or in which checks are
    /// performed.
    ///
    /// If a source path mapper is not specified, \ref
    /// TestContext::mapped_file_path and \ref FailContext::mapped_file_path
    /// will refer to the the unmapped paths.
    ///
    const SourcePathMapper* source_path_mapper = nullptr;

    /// \brief Root directory for data files.
    ///
    /// The root of the source file directory structure, or the root of a
    /// reflection of that directory structure in which all relevant data files
    /// are present. This is the directory against against which \ref
    /// TestContext::get_data_path() resolves the specified paths.
    ///
    /// A relative path will be understood as relative to the current working
    /// directory.
    ///
    std::filesystem::path data_root_dir;

    /// \brief Path template for per-thread log files.
    ///
    /// The specified string will be used as a template for constructing log
    /// file paths (see table of available parameters below). It will be
    /// separately expanded for each test thread. The path must be on relative
    /// form, and will be resolved against \ref log_file_base_dir. Any
    /// directories explicitely mentioned in the specified path will be created,
    /// if they do not already exist. The path must be specified in the generic
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
    /// Template expansion is performed as if by \ref BasicStringTemplate.
    ///
    std::string_view log_path_template =  "tmp/log/@T/thread_@N.log";

    /// \brief Base path for per-thread log files.
    ///
    /// If the paths expanded from \ref log_path_template are relative, they
    /// will be resolved against the base directory path specified here.
    ///
    /// If left empty (the default), the effective base directory is the current
    /// working diretory (`std::std::filesystem::current_path()`).
    ///
    std::filesystem::path log_file_base_dir;

    /// \brief Subdirectory for test files.
    ///
    /// This is the subdirectory, relative to \ref test_file_base_dir, in which
    /// the files and directories, that are created by unit tests (\ref
    /// TestContext::make_test_path(), \ref TestFileGuard, \ref TestDirGuard),
    /// will be placed. If it is set to the empty string, those test files and
    /// directories will be placed in the directory specified by \ref
    /// test_file_base_dir.
    ///
    /// The path must be specified in the generic format as understood by
    /// `std::filesystem::path`, and it must be on relative form (no root name
    /// and no root directory part). A final slash is optional.
    ///
    /// All directories explicitely mentioned in the specified path will be
    /// created, if they do not already exist.
    ///
    std::string_view test_file_subdir = "tmp/test";

    /// \brief Base directory for test files.
    ///
    /// The base directory against which \ref test_file_subdir is resolved. It
    /// can be relative or absolute. If it is relative, it will be interpreted
    /// as being relative to the current working directory. It makes no
    /// difference whether it is specified with, or without a final directory
    /// separator (`/`).
    ///
    /// It is an error if the specified directory does not already exist.
    ///
    std::filesystem::path test_file_base_dir;

    /// \brief Locale to be used in test framework and optionally in unit tests.
    ///
    /// The specified locale will be use internally in the testing framework,
    /// and will be exposed to unit tests via \ref TestContext::get_locale().
    ///
    std::locale locale;

    /// \brief Random seed offered to unit tests.
    ///
    /// The specified random seed will be used as input to the seed sequence
    /// offered through \ref TestContext::seed_seq(). To get a nondeterministic
    /// seed, use \ref unit_test::RandomSeed::random().
    ///
    unit_test::RandomSeed random_seed;
};


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_TEST_CONFIG_HPP
