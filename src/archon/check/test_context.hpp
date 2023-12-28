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

#ifndef ARCHON_X_CHECK_X_TEST_CONTEXT_HPP
#define ARCHON_X_CHECK_X_TEST_CONTEXT_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <iterator>
#include <array>
#include <string_view>
#include <string>
#include <exception>
#include <locale>
#include <ostream>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/random.hpp>
#include <archon/log/logger.hpp>
#include <archon/check/test_details.hpp>
#include <archon/check/check_arg.hpp>
#include <archon/check/thread_context.hpp>


namespace archon::check {

namespace impl {
class ThreadContextImpl;
} // namespace impl




/// \brief Provide context to executing test case.
///
/// This is the part of the test case execution context that is specific to a particular
/// execution of a particular test case.
///
/// A test context is the interface through which an executing test case communicates with
/// the testing framework. Most prominently, it is the basis for performing, and recording
/// the results of various kinds of checks, such as with \ref ARCHON_CHECK_EQUAL().
///
/// The test context object is available as an in-scope object named `test_context` to any
/// executing test case. A reference to this context object can be passed to other functions
/// in order to make it possible to use check macros such as \ref ARCHON_CHECK_EQUAL() in
/// those other functions, but, to make it work, the name of the reference must again be
/// `test_context`. Here is an example to illustrate this point:
///
/// \code{.cpp}
///
///   void foo(archon::check::TestContext& test_context)
///   {
///       // ...
///       ARCHON_CHECK_EQUAL(a, b);
///   }
///
///   ARCHON_TEST(FOO)
///   {
///       foo(test_context);
///   }
///
/// \endcode
///
/// The test context object will generally be destroyed as soon as the execution of the test
/// case ends, that is, at exit from the main scope of the test case. The test case must
/// therefore take care to ensure that no attempt is made to access the context object
/// beyond this point.
///
/// In addition to enabling checks, a test context also offers a way for test cases to log
/// (\ref logger), to initialize pseudo random number generators (\ref seed_seq()), to
/// create test files (e.g., \ref ARCHON_TEST_FILE()), and to locate additional
/// project-specific resources in the file system (\ref get_data_path()).
///
/// A test context is also used as a means of describing the currently executing test case
/// to a reporter, and to provide the reporter with access to a thread-level, and a
/// root-level logger (\ref check::ThreadContext::report_logger, \ref
/// check::RootContext::report_logger). See also \ref check::Reporter.
///
/// \sa \ref check::FailContext.
///
class TestContext {
public:
    /// \brief Thread-specific execution context.
    ///
    /// This is the part of the execution context that is shared by all test case executions
    /// happening on a particular test thread. The root context is available through the
    /// thread context.
    ///
    const check::ThreadContext& thread_context;

    /// \brief description of executing test case.
    ///
    /// This is a description of the currently executing test case.
    ///
    const check::TestDetails& test_details;

    /// \brief Mapped path to file containing test.
    ///
    /// If a source path mapper is installed (\ref check::TestConfig::source_path_mapper),
    /// this is the result of the mapping of the path specified by
    /// `test_details.location.file_path` (\ref check::TestDetails::location, \ref
    /// check::Location::file_path). Otherwise it is the same as
    /// `test_details.location.file_path`.
    ///
    std::string_view mapped_file_path;

    /// \brief Index of current test in test list.
    ///
    /// This is the index of the executing test case within the list of selected test cases
    /// as presented through \ref check::RootContext::num_tests and \ref
    /// check::RootContext::get_test_details() of the associated root context
    /// (`thread_context.root_context`).
    ///
    const std::size_t test_index;

    /// \brief Repetition number of current execution of this test case.
    ///
    /// This is the ordinal number of the repetition of the executing test case. During the
    /// first execution of the test case, it will be 1, and during the last execution, it
    /// will be equal to the requested number of repetitions, i.e., \ref
    /// check::TestConfig::num_repetitions or `thread_context.root_context.num_repetitions`.
    ///
    const int repetition_no;

    /// \brief For logging from inside test cases.
    ///
    /// Do not use this logger inside custom reporters (\ref check::Reporter). See \ref
    /// check::ThreadContext::report_logger and \ref check::RootContext::report_logger.
    ///
    /// You should use this logger to log from inside your test cases. The log level limit
    /// in effect for this logger is specified via \ref
    /// check::TestList::Config::inner_log_level_limit.
    ///
    log::Logger& logger;

    /// \brief Configured locale.
    ///
    /// A reference to the same locale as is referenced by
    /// `thread_context.root_context.locale`. See \ref check::RootContext::locale, \ref
    /// check::ThreadContext::root_context, and \ref thread_context.
    ///
    const std::locale& locale;

    /// \brief Entropy for seeding of pseudo random number generators.
    ///
    /// This function offers a seed sequence that can be used for seeding pseudo random
    /// number generators in test cases. The offered seed sequence can be controlled through
    /// \ref check::TestConfig::random_seed and \ref check::TestConfig::rseed_rep_no_override
    /// (see below).
    ///
    /// The offered seed sequence might be used as follows:
    ///
    /// \code{.cpp}
    ///
    ///   ARCHON_TEST(Foo)
    ///   {
    ///       std::mt19937_64 random(test_context.seed_seq());
    ///       // ...
    ///   }
    ///
    /// \endcode
    ///
    /// The offered seed sequence is constructed from an initial seed sequence that is the
    /// concatenation of the sequence specified through \ref check::TestConfig::random_seed
    /// and two additional 32-bit words constructed from the number of the current
    /// repetition (\ref repetition_no) of the executing test case. The first word is
    /// constructed from the 32 least significant bits of the repetition number, and the
    /// second word is constructed from the next 32 bits of the repetition number. The
    /// repetition number, as used for this purpose, can be overridden using \ref
    /// check::TestConfig::rseed_rep_no_override.
    ///
    auto seed_seq() const noexcept -> const core::SeedSeq&;

    /// \brief Get file system path of data file.
    ///
    /// This function constructs the file system path required to reach a data file residing
    /// in the source directory, or residing in a reflection of the source directory
    /// (somewhere with the same directory substructure and containing all the relevant data
    /// files).
    ///
    /// This function first resolves \p subdir_path against the base directory specified by
    /// \ref check::TestConfig::data_file_base_dir, and then resolves \p path against that.
    ///
    /// \param subdir_path The file system path of the caller relative to the root of the
    /// source directory structure. The intention is that this is the directory that
    /// contains the source file that contains the calling code. The path must be specified
    /// in the generic format as understood by `std::filesystem::path`. Use an empty string
    /// to specify the root of the source directory structure.
    ///
    /// \param path The file system path of a data file or directory in the source directory
    /// structure specified relative to \p subdir_path. It must be specified in the generic
    /// format as understood by `std::filesystem::path`. Specify an empty string to get the
    /// path to the directory referred to be \p subdir_path.
    ///
    auto get_data_path(std::string_view subdir_path, std::string_view path) -> std::filesystem::path;

    /// \brief Construct path for test file or test directory.
    ///
    /// This function constructs a file system path for a test file or a test directory
    /// using, as ingredients, the name of this test, its execution recurrence index, and
    /// the specified suffix.
    ///
    /// The directory part of the returned path will depend on configuration parameters \ref
    /// check::TestConfig::test_file_subdir and \ref check::TestConfig::test_file_base_dir.
    ///
    /// This function is used by the standard test file guards, \ref check::TestFileGuard
    /// and \ref check::TestDirGuard. Custom test file guards should use this function to
    /// produce appropriate file system paths.
    ///
    auto make_test_path(std::string_view suffix) const -> std::filesystem::path;

    /// \brief Keep test files.
    ///
    /// This function returns the value of TestConfig::keep_test_files in the configuration
    /// object passed to \ref check::TestList::run().
    ///
    /// This function is used by the standard test file guards, \ref check::TestFileGuard and
    /// \ref check::TestDirGuard. Custom test file guards should call this function to
    /// determine whether the files should be deleted or left in place after end of use.
    ///
    bool keep_test_files() const noexcept;

    /// \brief Basis for checks of general conditions.
    ///
    /// This function is the basis for check macros \ref ARCHON_CHECK() and \ref
    /// ARCHON_CHECK_NOT(). It can also serve as the basis of other check macros whose
    /// failure reports should take on the same form.
    ///
    /// Failures of checks of this kind are reported as `"<macro name>(<cond text>) failed"`
    /// where `<macro name>` and `<cond text>` are the strings passed as \p macro_name and
    /// \p cond_text respectively.
    ///
    /// Applications can define a custom check macro like this, where `foo(cond)` can be
    /// anything that depends on `cond`:
    ///
    /// \code{.cpp}
    ///
    ///   #define CHECK_FOO(cond) test_context.check_general_cond(foo(cond), __FILE__, __LINE__, "CHECK_FOO", #cond)
    ///
    /// \endcode
    ///
    /// This assumes that `test_context` refers to an object of type `TestContext`, which it
    /// does in the scope of a test case.
    ///
    /// \sa \ref ARCHON_CHECK() and \ref ARCHON_CHECK_NOT()
    /// \sa \ref check_special_cond()
    ///
    bool check_general_cond(bool cond, std::string_view file_path, long line_number, std::string_view macro_name,
                            std::string_view cond_text);

    /// \brief Basis for checks of special conditions.
    ///
    /// This function is the basis for \ref ARCHON_CHECK_EQUAL() and a number of other check
    /// macros involving comparisons. It can also serve as the basis of application defined
    /// check macros whose failure reports should take on the same form.
    ///
    /// Failures of checks of this kind are reported as `"<macro name>(<arg texts>) failed
    /// with (<arg values>)"` where `<macro name>` is the string passed as \p macro_name,
    /// `<arg texts>` is a comma-separated list of the values of \ref check::CheckArg::text
    /// of the specified arguments, and `<arg values>` is a comma-separated list of strings
    /// resulting from formatting \ref check::CheckArg::value of the specified
    /// arguments. Check arguments of non-formattable types are formatted as `?`. If `val`
    /// is the value of a check argument and `out` is an object of type `std::ostream`, then
    /// that check argument is formattable if, and only if `out << val` is well-formed.
    ///
    /// Applications can define a custom check macro like this, where `foo(cond)` can be
    /// anything that depends on `cond`:
    ///
    /// \code{.cpp}
    ///
    ///   #define CHECK_FOO(x, y) test_context.check_special_cond(foo(x, y), __FILE__, __LINE__, "CHECK_FOO", ARCHON_CHECK_ARG(x), ARCHON_CHECK_ARG(y))
    ///
    /// \endcode
    ///
    /// This assumes that `test_context` refers to an object of type `TestContext`, which it
    /// does in the scope of a test case. Note also that \ref ARCHON_CHECK_ARG is defined in
    /// \ref archon/check/check_arg.hpp.
    ///
    /// \sa \ref ARCHON_CHECK_EQUAL()
    /// \sa \ref check_general_cond()
    ///
    template<class... T> bool check_special_cond(bool cond, std::string_view file_path, long line_number,
                                                 std::string_view macro_name, check::CheckArg<T>... args);

    /// \brief Custom comparison check.
    ///
    /// See \ref ARCHON_CHECK_COMPARE().
    ///
    template<class A, class B, class C>
    bool check_compare(const A& a, const B& b, C&& comp, const char* file_path, long line_number, const char* a_text,
                       const char* b_text, const char* comp_text);

    /// \{
    ///
    /// \brief Report failure of checks involving exceptions.
    ///
    /// See \ref ARCHON_CHECK_THROW(), \ref ARCHON_CHECK_THROW_EX(), \ref
    /// ARCHON_CHECK_THROW_ANY(), and \ref ARCHON_CHECK_NOTHROW().
    ///
    void check_throw_failed(const char* file_path, long line_number, const char* expr_text,
                            const char* exception_name);
    void check_throw_ex_failed(const char* file_path, long line_number, const char* expr_text,
                               const char* exception_name, const char* exception_cond_text);
    void check_throw_ex_cond_failed(const char* file_path, long line_number, const char* expr_text,
                                    const char* exception_name, const char* exception_cond_text);
    void check_throw_any_failed(const char* file_path, long line_number, const char* expr_text);
    void check_nothrow_failed(const char* file_path, long line_number, const char* expr_text, std::exception*);
    /// \}

    /// \{
    ///
    /// \brief Report success or failure of a check.
    ///
    /// These functions are used to report success or failure of a check. The are invoked as
    /// part of the execution of \ref check_general_cond() and \ref
    /// check_special_cond(). They can also be used in the implementation of custom checks.
    ///
    void check_succeeded() noexcept;
    void check_failed(check::Location, std::string_view message);
    /// \}

    /// \{
    ///
    /// \brief Checks involving sequence comparisons.
    ///
    /// See \ref ARCHON_CHECK_EQUAL_SEQ(), \ref ARCHON_CHECK_COMPARE_SEQ().
    ///
    template<class A, class B>
    bool check_equal_seq(const A& a, const B& b, const char* file_path, long line_number,
                         const char* a_text, const char* b_text);
    template<class A, class B, class C>
    bool check_compare_seq(const A& a, const B& b, C&& comp, const char* file_path, long line_number,
                           const char* a_text, const char* b_text, const char* comp_text);
    /// \}

    /// \{
    ///
    /// \brief Reliable value comparison for check macros.
    ///
    /// These functions exist as a means for check macros to compare values. They are used
    /// by \ref ARCHON_CHECK_EQUAL() and friends.
    ///
    /// When both arguments are of integer type (\ref core::is_integer()), comparison is
    /// done by \ref core::int_equal(), \ref core::int_less(), or \ref
    /// core::int_less_equal(). When one argument is of integer type and the other is of
    /// floating-point type (\ref core::is_float()), comparison is done by \ref
    /// core::float_equal_int(), \ref core::float_less_int(), \ref
    /// core::float_greater_int(), \ref core::float_less_equal_int(), or \ref
    /// core::float_greater_equal_int(). In all other cases, comparison is done using
    /// regular comparison operators.
    ///
    /// FIXME: Find a way to allow for comparison between custom integer and custom floating-point types (currently, if one side is floating-point (custom or standard), the other side has to be a standard integer type).                      
    ///
    template<class A, class B> static bool equal(const A& a, const B& b);
    template<class A, class B> static bool less(const A& a, const B& b);
    template<class A, class B> static bool less_equal(const A& a, const B& b);
    template<class A, class B> static bool greater(const A& a, const B& b);
    template<class A, class B> static bool greater_equal(const A& a, const B& b);
    /// \}

    /// \{
    ///
    /// \brief Reliably compare distance between values with given distance.
    ///
    /// These functions compare the distance between the specified values (\p a and \p b)
    /// with the specified distance (\p dist).
    ///
    /// If all arguments (\p a, \p b, and \p dist) have integer type in the sense that \ref
    /// core::is_integer() returns `true` for all three types, the result is
    /// exact. Otherwise, each type (\p A, \p B, and \p D) must either be a standard integer
    /// type (`std::is_integral`) or a standard floating-point type
    /// (`std::is_floating_point`), and the result is computed using an expression on the
    /// form (shown for the case of `dist_less()`) `std::abs(type(a) - type(b)) <
    /// type(dist)` where `type` is `decltype(a + b + dist)`.
    ///
    /// FIXME: Find a way to allow for use of custom floating-point types.         
    ///
    /// FIXME: Try to find a way to ensure exactness even when some, or all arguments have floating-point type.         
    ///
    template<class A, class B, class D> static bool dist_less(const A& a, const B& b, const D& dist);
    template<class A, class B, class D> static bool dist_less_equal(const A& a, const B& b, const D& dist);
    template<class A, class B, class D> static bool dist_greater(const A& a, const B& b, const D& dist);
    template<class A, class B, class D> static bool dist_greater_equal(const A& a, const B& b, const D& dist);
    /// \}

    TestContext(const TestContext&) = delete;
    auto operator=(const TestContext&) -> TestContext& = delete;

protected:
    static auto get_thread_context_impl(check::TestContext&) noexcept -> impl::ThreadContextImpl&;
    static auto get_report_logger(check::TestContext&) noexcept -> log::Logger&;

    TestContext(impl::ThreadContextImpl&, const check::TestDetails&, std::string_view mapped_file_path,
                std::size_t test_index, int repetition_no, log::Logger& report_logger,
                log::Logger& inner_logger) noexcept;

private:
    static constexpr std::size_t s_max_quoted_string_size = 72;

    impl::ThreadContextImpl& m_thread_context;

    // Refers to `m_thread_context.m_test_level_report_logger`, or to a logger that is
    // derived from it.
    log::Logger& m_report_logger;

    void test_failed(std::string_view message);

    void check_general_cond_failed(check::Location, std::string_view macro_name, std::string_view cond_text);

    template<class... T>
    void check_special_cond_failed(check::Location, std::string_view macro_name, check::CheckArg<T>... args);
    void check_special_cond_failed_2(check::Location, std::string_view macro_name,
                                     core::Span<const std::string_view> args,
                                     core::Span<const std::string_view> arg_vals);

    template<class A, class B>
    void compare_failed(check::Location, std::string_view macro_name, std::string_view a_text, std::string_view b_text,
                        std::string_view comp_text, const A& a, const B& b);
    void compare_failed_2(check::Location, std::string_view macro_name, std::string_view a_text,
                          std::string_view b_text, std::string_view comp_text, std::string_view a_val,
                          std::string_view b_val);
    void check_throw_failed_2(check::Location, std::string_view expr_text, std::string_view exception_name);
    void check_throw_ex_failed_2(check::Location, std::string_view expr_text, std::string_view exception_name,
                                 std::string_view exception_cond_text);
    void check_throw_ex_cond_failed_2(check::Location, std::string_view expr_text, std::string_view exception_name,
                                      std::string_view exception_cond_text);
    void check_throw_any_failed_2(check::Location, std::string_view expr_text);
    void check_nothrow_failed_2(check::Location, std::string_view expr_text, std::exception*);
    template<class A>
    void check_equal_seq_failed_1(check::Location, std::string_view a_text, std::string_view b_text, std::size_t index,
                                  const A& a);
    template<class B>
    void check_equal_seq_failed_2(check::Location, std::string_view a_text, std::string_view b_text, std::size_t index,
                                  const B& b);
    template<class A, class B>
    void check_equal_seq_failed_3(check::Location, std::string_view a_text, std::string_view b_text, std::size_t index,
                                  const A& a, const B& b);
    void check_equal_seq_failed_4(check::Location, std::string_view a_text, std::string_view b_text, std::size_t index,
                                  const std::string_view* a_val, const std::string_view* b_val);
    template<class A>
    void check_compare_seq_failed_1(check::Location, std::string_view a_text, std::string_view b_text,
                                    std::string_view comp_text, std::size_t index, const A& a);
    template<class B>
    void check_compare_seq_failed_2(check::Location, std::string_view a_text, std::string_view b_text,
                                    std::string_view comp_text, std::size_t index, const B& b);
    template<class A, class B>
    void check_compare_seq_failed_3(check::Location, std::string_view a_text, std::string_view b_text,
                                    std::string_view comp_text, std::size_t index, const A& a, const B& b);
    void check_compare_seq_failed_4(check::Location, std::string_view a_text, std::string_view b_text,
                                    std::string_view comp_text, std::size_t index, const std::string_view* a_val,
                                    const std::string_view* b_val);

    static void process_check_args(core::SeedMemoryOutputStream&, std::string_view* texts, std::size_t* ends);
    template<class T, class... U>
    static void process_check_args(core::SeedMemoryOutputStream&, std::string_view* texts, std::size_t* ends,
                                   check::CheckArg<T> arg, check::CheckArg<U>... args);

    template<bool greater, bool or_equal, class A, class B, class D>
    static bool dist_compare(const A& a, const B& b, const D& dist);

    template<class T> static void format_value(std::ostream&, const T&);
    template<class C, class T, class A> static void format_value(std::ostream&, const std::basic_string<C, T, A>&);
    template<class C, class T> static void format_value(std::ostream&, const std::basic_string_view<C, T>&);

    static void format_char(std::ostream&, char);
    template<class C> static void format_char(std::ostream&, C);

    static void format_string(std::ostream&, std::string_view);
    template<class C> static void format_string(std::ostream&, std::basic_string_view<C>);

    [[noreturn]] void abort();

    friend class impl::ThreadContextImpl;
};








// Implementation


inline bool TestContext::check_general_cond(bool cond, std::string_view file_path, long line_number,
                                            std::string_view macro_name, std::string_view cond_text)
{
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        check::Location location = { file_path, line_number };
        check_general_cond_failed(location, macro_name, cond_text); // Throws
    }
    return cond;
}


template<class... T>
inline bool TestContext::check_special_cond(bool cond, std::string_view file_path, long line_number,
                                            std::string_view macro_name, check::CheckArg<T>... args)
{
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        check::Location location = { file_path, line_number };
        check_special_cond_failed(location, macro_name, args...); // Throws
    }
    return cond;
}


template<class A, class B, class C>
inline bool TestContext::check_compare(const A& a, const B& b, C&& comp, const char* file_path, long line_number,
                                       const char* a_text, const char* b_text, const char* comp_text)
{
    bool cond = std::forward<C>(comp)(a, b); // Throws
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        check::Location location = { file_path, line_number };
        compare_failed(location, "ARCHON_CHECK_COMPARE", a_text, b_text, comp_text, a, b); // Throws
    }
    return cond;
}


inline void TestContext::check_throw_failed(const char* file_path, long line_number, const char* expr_text,
                                            const char* exception_name)
{
    check::Location location = { file_path, line_number };
    check_throw_failed_2(location, expr_text, exception_name); // Throws
}


inline void TestContext::check_throw_ex_failed(const char* file_path, long line_number, const char* expr_text,
                                               const char* exception_name, const char* exception_cond_text)
{
    check::Location location = { file_path, line_number };
    check_throw_ex_failed_2(location, expr_text, exception_name, exception_cond_text); // Throws
}


inline void TestContext::check_throw_ex_cond_failed(const char* file_path, long line_number, const char* expr_text,
                                                    const char* exception_name, const char* exception_cond_text)
{
    check::Location location = { file_path, line_number };
    check_throw_ex_cond_failed_2(location, expr_text, exception_name, exception_cond_text); // Throws
}


inline void TestContext::check_throw_any_failed(const char* file_path, long line_number, const char* expr_text)
{
    check::Location location = { file_path, line_number };
    check_throw_any_failed_2(location, expr_text); // Throws
}


inline void TestContext::check_nothrow_failed(const char* file_path, long line_number, const char* expr_text,
                                              std::exception* exc)
{
    check::Location location = { file_path, line_number };
    check_nothrow_failed_2(location, expr_text, exc); // Throws
}


template<class A, class B>
bool TestContext::check_equal_seq(const A& a, const B& b, const char* file_path, long line_number,
                                  const char* a_text, const char* b_text)
{
    auto pred = [](const auto& c, const auto& d) {
        return equal(c, d); // Throws
    };
    auto pair = std::mismatch(std::begin(a), std::end(a), std::begin(b), std::end(b), pred); // Throws
    bool equal = (pair.first == std::end(a) && pair.second == std::end(b));
    if (ARCHON_LIKELY(equal)) {
        check_succeeded();
    }
    else {
        check::Location location = { file_path, line_number };
        std::size_t index = std::size_t(pair.first - std::begin(a));
        if (pair.second == std::end(b)) {
            ARCHON_ASSERT(pair.first != std::end(a));
            check_equal_seq_failed_1(location, a_text, b_text, index, *pair.first); // Throws
        }
        else if (pair.first == std::end(a)) {
            ARCHON_ASSERT(pair.second != std::end(b));
            check_equal_seq_failed_2(location, a_text, b_text, index, *pair.second); // Throws
        }
        else {
            ARCHON_ASSERT(pair.second != std::end(b));
            ARCHON_ASSERT(pair.first != std::end(a));
            check_equal_seq_failed_3(location, a_text, b_text, index, *pair.first, *pair.second); // Throws
        }
    }
    return equal;
}


template<class A, class B, class C>
bool TestContext::check_compare_seq(const A& a, const B& b, C&& comp, const char* file_path, long line_number,
                                    const char* a_text, const char* b_text, const char* comp_text)
{
    auto pair = std::mismatch(std::begin(a), std::end(a), std::begin(b), std::end(b), std::forward<C>(comp)); // Throws
    bool success = (pair.first == std::end(a) && pair.second == std::end(b));
    if (ARCHON_LIKELY(success)) {
        check_succeeded();
    }
    else {
        check::Location location = { file_path, line_number };
        std::size_t index = std::size_t(pair.first - std::begin(a));
        if (pair.second == std::end(b)) {
            ARCHON_ASSERT(pair.first != std::end(a));
            check_compare_seq_failed_1(location, a_text, b_text, comp_text, index, *pair.first); // Throws
        }
        else if (pair.first == std::end(a)) {
            ARCHON_ASSERT(pair.second != std::end(b));
            check_compare_seq_failed_2(location, a_text, b_text, comp_text, index, *pair.second); // Throws
        }
        else {
            ARCHON_ASSERT(pair.second != std::end(b));
            ARCHON_ASSERT(pair.first != std::end(a));
            check_compare_seq_failed_3(location, a_text, b_text, comp_text, index, *pair.first,
                                       *pair.second); // Throws
        }
    }
    return success;
}


template<class A, class B> inline bool TestContext::equal(const A& a, const B& b)
{
    constexpr bool int_vs_int = (core::is_integer<A>() && core::is_integer<B>());
    constexpr bool float_vs_int = (core::is_float<A>() && core::is_integer<B>());
    constexpr bool int_vs_float = (core::is_integer<A>() && core::is_float<B>());
    if constexpr (int_vs_int) {
        return core::int_equal(a, b);
    }
    else if constexpr (float_vs_int) {
        return core::float_equal_int(a, b);
    }
    else if constexpr (int_vs_float) {
        // Note: Reverse argument order
        return core::float_equal_int(b, a);
    }
    else {
        return (a == b); // Throws
    }
}


template<class A, class B> inline bool TestContext::less(const A& a, const B& b)
{
    constexpr bool int_vs_int = (core::is_integer<A>() && core::is_integer<B>());
    constexpr bool float_vs_int = (core::is_float<A>() && core::is_integer<B>());
    constexpr bool int_vs_float = (core::is_integer<A>() && core::is_float<B>());
    if constexpr (int_vs_int) {
        return core::int_less(a, b);
    }
    else if constexpr (float_vs_int) {
        return core::float_less_int(a, b);
    }
    else if constexpr (int_vs_float) {
        // Note: Reverse argument order
        return core::float_greater_int(b, a);
    }
    else {
        return (a < b); // Throws
    }
}


template<class A, class B> inline bool TestContext::less_equal(const A& a, const B& b)
{
    constexpr bool int_vs_int = (core::is_integer<A>() && core::is_integer<B>());
    constexpr bool float_vs_int = (core::is_float<A>() && core::is_integer<B>());
    constexpr bool int_vs_float = (core::is_integer<A>() && core::is_float<B>());
    if constexpr (int_vs_int) {
        return core::int_less_equal(a, b);
    }
    else if constexpr (float_vs_int) {
        return core::float_less_equal_int(a, b);
    }
    else if constexpr (int_vs_float) {
        // Note: Reverse argument order
        return core::float_greater_equal_int(b, a);
    }
    else {
        return (a <= b); // Throws
    }
}


template<class A, class B> inline bool TestContext::greater(const A& a, const B& b)
{
    return less(b, a); // Throws
}


template<class A, class B> inline bool TestContext::greater_equal(const A& a, const B& b)
{
    return less_equal(b, a); // Throws
}


template<class A, class B, class D> inline bool TestContext::dist_less(const A& a, const B& b, const D& dist)
{
    constexpr bool greater = false;
    constexpr bool or_equal = false;
    return dist_compare<greater, or_equal>(a, b, dist); // Throws
}


template<class A, class B, class D> inline bool TestContext::dist_less_equal(const A& a, const B& b, const D& dist)
{
    constexpr bool greater = false;
    constexpr bool or_equal = true;
    return dist_compare<greater, or_equal>(a, b, dist); // Throws
}


template<class A, class B, class D> inline bool TestContext::dist_greater(const A& a, const B& b, const D& dist)
{
    constexpr bool greater = true;
    constexpr bool or_equal = false;
    return dist_compare<greater, or_equal>(a, b, dist); // Throws
}


template<class A, class B, class D> inline bool TestContext::dist_greater_equal(const A& a, const B& b, const D& dist)
{
    constexpr bool greater = true;
    constexpr bool or_equal = true;
    return dist_compare<greater, or_equal>(a, b, dist); // Throws
}


inline auto TestContext::get_thread_context_impl(TestContext& test_context) noexcept -> impl::ThreadContextImpl&
{
    return test_context.m_thread_context;
}


inline auto TestContext::get_report_logger(check::TestContext& test_context) noexcept -> log::Logger&
{
    return test_context.m_report_logger;
}


template<class... T>
void TestContext::check_special_cond_failed(check::Location location, std::string_view macro_name,
                                            check::CheckArg<T>... args)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    constexpr std::size_t num_args = sizeof... (args);
    std::array<std::string_view, num_args> args_2 = {};
    std::array<std::size_t, num_args> ends = {};
    process_check_args(out, args_2.data(), ends.data(), args...); // Throws
    std::array<std::string_view, num_args> arg_vals;
    std::size_t prev_end = 0;
    for (std::size_t i = 0; i < num_args; ++i) {
        std::size_t end = ends[i];
        arg_vals[i] = out.view().substr(prev_end, std::size_t(end - prev_end));
        prev_end = end;
    }
    check_special_cond_failed_2(location, macro_name, args_2, arg_vals); // Throws
}


template<class A, class B>
void TestContext::compare_failed(check::Location location, std::string_view macro_name, std::string_view a_text,
                                 std::string_view b_text, std::string_view comp_text, const A& a, const B& b)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, a); // Throws
    std::size_t i = out.streambuf().size();
    format_value(out, b); // Throws
    std::string_view a_val = out.view().substr(0, i);
    std::string_view b_val = out.view().substr(i);
    compare_failed_2(location, macro_name, a_text, b_text, comp_text, a_val, b_val); // Throws
}


template<class A>
void TestContext::check_equal_seq_failed_1(check::Location location, std::string_view a_text, std::string_view b_text,
                                           std::size_t index, const A& a)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, a); // Throws
    std::string_view a_val = out.view();
    check_equal_seq_failed_4(location, a_text, b_text, index, &a_val, nullptr); // Throws
}


template<class B>
void TestContext::check_equal_seq_failed_2(check::Location location, std::string_view a_text, std::string_view b_text,
                                           std::size_t index, const B& b)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, b); // Throws
    std::string_view b_val = out.view();
    check_equal_seq_failed_4(location, a_text, b_text, index, nullptr, &b_val); // Throws
}


template<class A, class B>
void TestContext::check_equal_seq_failed_3(check::Location location, std::string_view a_text, std::string_view b_text,
                                           std::size_t index, const A& a, const B& b)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, a); // Throws
    std::size_t i = out.streambuf().size();
    format_value(out, b); // Throws
    std::string_view a_val = out.view().substr(0, i);
    std::string_view b_val = out.view().substr(i);
    check_equal_seq_failed_4(location, a_text, b_text, index, &a_val, &b_val); // Throws
}


template<class A>
void TestContext::check_compare_seq_failed_1(check::Location location, std::string_view a_text, std::string_view b_text,
                                             std::string_view comp_text, std::size_t index, const A& a)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, a); // Throws
    std::string_view a_val = out.view();
    check_compare_seq_failed_4(location, a_text, b_text, comp_text, index, &a_val, nullptr); // Throws
}


template<class B>
void TestContext::check_compare_seq_failed_2(check::Location location, std::string_view a_text, std::string_view b_text,
                                             std::string_view comp_text, std::size_t index, const B& b)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, b); // Throws
    std::string_view b_val = out.view();
    check_compare_seq_failed_4(location, a_text, b_text, comp_text, index, nullptr, &b_val); // Throws
}


template<class A, class B>
void TestContext::check_compare_seq_failed_3(check::Location location, std::string_view a_text, std::string_view b_text,
                                             std::string_view comp_text, std::size_t index, const A& a, const B& b)
{
    std::array<char, 512> seed_memory;
    core::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    format_value(out, a); // Throws
    std::size_t i = out.streambuf().size();
    format_value(out, b); // Throws
    std::string_view a_val = out.view().substr(0, i);
    std::string_view b_val = out.view().substr(i);
    check_compare_seq_failed_4(location, a_text, b_text, comp_text, index, &a_val, &b_val); // Throws
}


inline void TestContext::process_check_args(core::SeedMemoryOutputStream&, std::string_view*, std::size_t*)
{
}


template<class T, class... U>
inline void TestContext::process_check_args(core::SeedMemoryOutputStream& out, std::string_view* texts,
                                            std::size_t* ends, check::CheckArg<T> arg, check::CheckArg<U>... args)
{
    if constexpr (check::CheckArg<T>::is_formattable) {
        format_value(out, arg.get_value()); // Throws
    }
    else {
        struct Unformattable {};
        format_value(out, Unformattable()); // Throws
    }
    *texts = arg.get_text();
    *ends = out.streambuf().size();
    process_check_args(out, texts + 1, ends + 1, args...); // Throws
}


// FIXME: Due to bug in Apple clang version 14.0.3 (clang-1403.0.22.14.1) (Xcode 14.3.1)
// (LLVM 15.0.0), the first template parameter cannot be named `greater`. If it was named
// `greater`, compilation would fail because of a spurious clash with the member function of
// that name. See https://feedbackassistant.apple.com/feedback/12369257.
template<bool greater_2, bool or_equal, class A, class B, class D>
inline bool TestContext::dist_compare(const A& a, const B& b, const D& dist)
{
    constexpr bool all_ints = (core::is_integer<A>() && core::is_integer<B>() && core::is_integer<D>());
    if constexpr (all_ints) {
        // Exactness guaranteed
        if constexpr (greater_2) {
            return !dist_compare<false, !or_equal>(a, b, dist);
        }
        else {
            bool neg_a = core::is_negative(a);
            bool neg_b = core::is_negative(b);
            if (ARCHON_LIKELY(neg_a == neg_b)) {
                using type = core::common_int_type<A, B>;
                type a_2 = core::int_cast_a<type>(a);
                type b_2 = core::int_cast_a<type>(b);
                type val = (a_2 <= b_2 ? b_2 - a_2 : a_2 - b_2);
                if constexpr (or_equal) {
                    return core::int_less_equal(val, dist);
                }
                else {
                    return core::int_less(val, dist);
                }
            }
            D val = dist;
            bool no_overflow = (neg_a ?
                                core::try_int_sub(val, b) && core::try_int_add(val, a) :
                                core::try_int_sub(val, a) && core::try_int_add(val, b));
            if (no_overflow) {
                if constexpr (or_equal) {
                    return core::int_less_equal(0, val);
                }
                else {
                    return core::int_less(0, val);
                }
            }
            return false;
        }
    }
    else {
        // Exactness not guaranteed
        static_assert(std::is_integral_v<A> || std::is_floating_point_v<A>);
        static_assert(std::is_integral_v<B> || std::is_floating_point_v<B>);
        static_assert(std::is_integral_v<D> || std::is_floating_point_v<D>);
        using type = decltype(a + b + dist);
        static_assert(std::is_floating_point_v<type>);
        type val = std::abs(type(a) - type(b));
        if constexpr (greater_2) {
            if constexpr (or_equal) {
                return val >= type(dist); // Throws
            }
            else {
                return val > type(dist); // Throws
            }
        }
        else {
            if constexpr (or_equal) {
                return val <= type(dist); // Throws
            }
            else {
                return val < type(dist); // Throws
            }
        }
    }
}


template<class T> inline void TestContext::format_value(std::ostream& out, const T& value)
{
    if constexpr (core::is_integer<T>()) {
        out << core::as_int(value); // Throws
    }
    else if constexpr (std::is_floating_point_v<T>) {
        out << core::with_precision(value, std::numeric_limits<T>::digits10 + 1); // Throws
    }
    else if constexpr (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>) {
        format_char(out, value); // Throws
    }
    else if constexpr (std::is_same_v<T, unsigned char> || std::is_same_v<T, signed char>) {
        format_char(out, char(value)); // Throws
    }
    else if constexpr (std::is_convertible_v<T, std::string_view>) {
        format_string(out, std::string_view(value)); // Throws
    }
    else if constexpr (std::is_convertible_v<T, std::wstring_view>) {
        format_string(out, std::wstring_view(value)); // Throws
    }
#if ARCHON_HAVE_UINTPTR_T
    else if constexpr (std::is_pointer<T>()) {
        int min_num_digits = -1; // Auto-detect max number of digits for type
        out << core::formatted("0x%s", core::as_hex_int(std::uintptr_t(value), min_num_digits)); // Throws
    }
#endif
    else if constexpr (core::has_stream_output_operator<T, char>) {
        out << value; // Throws
    }
    else {
        out << "?"; // Throws
    }
}


template<class C, class T, class A>
inline void TestContext::format_value(std::ostream& out, const std::basic_string<C, T, A>& value)
{
    format_string(out, std::basic_string_view(value.data(), value.size())); // Throws
}


template<class C, class T>
inline void TestContext::format_value(std::ostream& out, const std::basic_string_view<C, T>& value)
{
    format_string(out, std::basic_string_view(value.data(), value.size())); // Throws
}


template<class C> inline void TestContext::format_char(std::ostream& out, C ch)
{
    std::basic_string_view<C> string(&ch, 1);

    // FIXME: Should probably use an encoding stream wrapper instead

    std::locale locale = out.getloc(); // Throws
    std::array<C, 16> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, locale); // Throws
    std::basic_string_view<C> string_2 = formatter.format(core::quoted_s(string)); // Throws

    std::array<char, 16> seed_memory_2;
    core::BasicStringEncoder<C> encoder(locale, seed_memory_2); // Throws
    std::string_view string_3 = encoder.encode_sc(string_2); // Throws
    out << string_3; // Throws
}


template<class C> inline void TestContext::format_string(std::ostream& out, std::basic_string_view<C> string)
{
    // FIXME: Should probably use an encoding stream wrapper instead

    std::locale locale = out.getloc(); // Throws
    constexpr std::size_t seed_memory_size = std::min<std::size_t>(s_max_quoted_string_size, 512);
    std::array<C, seed_memory_size> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, locale); // Throws
    std::basic_string_view<C> string_2 = formatter.format(core::quoted(string, s_max_quoted_string_size)); // Throws

    std::array<char, 512> seed_memory_2;
    core::BasicStringEncoder<C> encoder(locale, seed_memory_2); // Throws
    std::string_view string_3 = encoder.encode_sc(string_2); // Throws
    out << string_3; // Throws
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_TEST_CONTEXT_HPP
