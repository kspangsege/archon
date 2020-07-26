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

#ifndef ARCHON_X_UNIT_TEST_X_TEST_CONTEXT_HPP
#define ARCHON_X_UNIT_TEST_X_TEST_CONTEXT_HPP

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

#include <archon/base/features.h>
#include <archon/base/integer.hpp>
#include <archon/base/seed_memory_output_stream.hpp>
#include <archon/base/string_codec.hpp>
#include <archon/base/value_formatter.hpp>
#include <archon/base/format_with.hpp>
#include <archon/base/quote.hpp>
#include <archon/base/logger.hpp>
#include <archon/unit_test/seed_seq.hpp>
#include <archon/unit_test/test_details.hpp>
#include <archon/unit_test/thread_context.hpp>


namespace archon::unit_test {

namespace detail {
class ThreadContextImpl;
} // namespace detail




/// \brief Provide context to executing unit test.
///
/// This is the part of the unit test execution context that is specific to a
/// particular execution of a particular unit test.
///
/// A test context is the interface through which an executing unit test
/// communicates with the testing harness. Most prominently, it is the basis for
/// performaing, and recording the results of various kinds of checks (e.g.,
/// \ref check_equal()). Normally, a unit test will express these checks in
/// terms of check macros such as \ref ARCHON_CHECK_EQUAL().
///
/// The test context object is available as an in-scope object named
/// `test_context` to any executing unit test. A reference to this context
/// object can be passed to a function in order to make it possible to use check
/// macros such as \ref ARCHON_CHECK_EQUAL() in that function, but, to make it
/// work, the the name of the reference must again be `test_context`. Here is an
/// example to illustrate this point:
///
/// \code{.cpp}
///
///   void foo(archon::unit_test::TestContext& test_context)
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
/// The test context object will generally be destroyed as soon as the execution
/// of the unit test ends, that is, at exit from the main scope of the unit
/// test. The unit test must therefore take case to ensure that no attempt is
/// made to access the context object beyond this point.
///
/// In addition to enabling checks, a test context also offers a way for unit
/// tests to log (\ref logger), to initialize pseudo random number generators
/// (\ref seed_seq()), to create test files (e.g., \ref ARCHON_TEST_FILE()), and
/// to locate additional project-specific filesystem resources (\ref
/// get_data_path()).
///
/// A test context is also used as a means of describing the currently executing
/// unit test to a reporter, and to provide the reporter with access to a
/// thread-level, and a root-level logger (\ref ThreadContext::report_logger,
/// \ref RootContext::report_logger). See also \ref Reporter.
///
/// \sa \ref FailContext.
///
class TestContext {
public:
    /// \brief Thread-specific execution context.
    ///
    /// This is the part of the execution context that is shared by all unit
    /// test executions happening on a particular test thread. The root context
    /// is available through the thread context.
    ///
    const ThreadContext& thread_context;

    /// \brief description of executing unit test.
    ///
    /// This is a description of the currently executing unit test.
    ///
    const TestDetails& test_details;

    /// \brief Mapped path to file containing test.
    ///
    /// If a source path mapper is installed (\ref
    /// TestList::Config::source_path_mapper), this is the result of the mapping
    /// of the path specified by `test_details.location.file_path` (\ref
    /// TestDetails::location, \ref Location::file_path). Otherwise it is the
    /// same as `test_details.location.file_path`.
    ///
    std::string_view mapped_file_path;

    /// \brief Index of current test in test list.
    ///
    /// This is the index of the executing unit test within the list of selected
    /// unit tests as presented through \ref RootContext::num_tests and \ref
    /// RootContext::get_test_details() of the associated root context
    /// (`thread_context.root_context`).
    ///
    const std::size_t test_index;

    /// \brief Recurrence index of current execution of this unit test.
    ///
    /// This is an index into the sequence of repeated executions of this unit
    /// test. `thread_context.root_context.num_recurrences` specifies the number
    /// of requested repetitions.
    ///
    const int recurrence_index;

    /// \brief For logging from inside unit tests.
    ///
    /// Do not use this logger inside custom reporters (\ref Reporter). See \ref
    /// ThreadContext::report_logger and \ref RootContext::report_logger.
    ///
    /// You should use this logger to log from inside your unit tests. The log
    /// level limit in effect for this logger is specified via \ref
    /// TestList::Config::inner_log_level_limit.
    ///
    base::Logger& logger;

    /// \{
    ///
    /// \brief Checks of arbitrary conditions.
    ///
    /// See \ref ARCHON_CHECK() and \ref ARCHON_CHECK_NOT().
    ///
    bool check(bool cond, const char* file_path, long line_number, const char* cond_text);
    bool check_not(bool cond, const char* file_path, long line_number, const char* cond_text);
    bool check_cond(bool cond, const char* file_path, long line_number, const char* macro_name,
                    const char* cond_text);
    /// \}

    /// \{
    ///
    /// \brief Checks involving comparisons.
    ///
    /// See \ref ARCHON_CHECK_EQUAL(), \ref ARCHON_CHECK_NOT_EQUAL(), \ref
    /// ARCHON_CHECK_LESS(), \ref ARCHON_CHECK_LESS_EQUAL(), \ref
    /// ARCHON_CHECK_GREATER(), and \ref ARCHON_CHECK_GREATER_EQUAL().
    ///
    template<class A, class B>
    bool check_equal(const A& a, const B& b, const char* file_path, long line_number,
                     const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_not_equal(const A& a, const B& b, const char* file_path, long line_number,
                         const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_less(const A& a, const B& b, const char* file_path, long line_number,
                    const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_less_equal(const A& a, const B& b, const char* file_path, long line_number,
                          const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_greater(const A& a, const B& b, const char* file_path, long line_number,
                       const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_greater_equal(const A& a, const B& b, const char* file_path, long line_number,
                             const char* a_text, const char* b_text);
    template<class A, class B>
    bool check_compare(bool cond, const A& a, const B& b, const char* file_path, long line_number,
                       const char* macro_name, const char* a_text, const char* b_text);
    /// \}

    /// \{
    ///
    /// \brief Inexact relational checks.
    ///
    /// See \ref ARCHON_CHECK_APPROXIMATELY_EQUAL(), \ref
    /// ARCHON_CHECK_ESSENTIALLY_EQUAL(), \ref ARCHON_CHECK_DEFINITELY_LESS(),
    /// and ARCHON_CHECK_DEFINITELY_GREATER().
    ///
    bool check_approximately_equal(long double a, long double b, long double eps,
                                   const char* file_path, long line_number,
                                   const char* a_text, const char* b_text, const char* eps_text);
    bool check_essentially_equal(long double a, long double b, long double eps,
                                 const char* file_path, long line_number,
                                 const char* a_text, const char* b_text, const char* eps_text);
    bool check_definitely_less(long double a, long double b, long double eps,
                               const char* file_path, long line_number,
                               const char* a_text, const char* b_text, const char* eps_text);
    bool check_definitely_greater(long double a, long double b, long double eps,
                                  const char* file_path, long line_number,
                                  const char* a_text, const char* b_text, const char* eps_text);
    bool check_inexact_compare(bool cond, long double a, long double b, long double eps,
                               const char* file_path, long line_number, const char* macro_name,
                               const char* a_text, const char* b_text, const char* eps_text);
    /// \}

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
    void check_nothrow_failed(const char* file_path, long line_number, const char* expr_text,
                              std::exception*);
    /// \}

    /// \brief Report success of checks.
    ///
    /// Used in connection with \ref check_throw_failed(), \ref
    /// check_throw_ex_failed(), \ref check_throw_ex_cond_failed(), \ref
    /// check_throw_any_failed(), and \ref check_nothrow_failed() when the check
    /// does not fail.
    ///
    void check_succeeded() noexcept;

    /// \{
    ///
    /// \brief Checks involving sequence comparisons.
    ///
    /// See \ref ARCHON_CHECK_EQUAL_SEQ().
    ///
    template<class A, class B>
    bool check_equal_seq(const A& a, const B& b, const char* file_path, long line_number,
                         const char* a_text, const char* b_text);
    /// \}

    /// \brief Entropy for seeding of pseudo random number genrators.
    ///
    /// This function offers a seed sequence that can be used to seed pseudo
    /// random number generators used in unit tests. The offered seed sequence
    /// can be controlled through \ref TestConfig::random_seed.
    ///
    /// A non-const reference is returned in order to allow compact constructs
    /// like this:
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
    /// In reality, the returned object cannot be mutated (\ref
    /// unit_test::SeedSeq::generate() is `const`), so it is safe to use it for
    /// seeding from multiple threads concurrently.
    ///
    unit_test::SeedSeq& seed_seq() const noexcept;

    /// \brief Construct path for test file or test directory.
    ///
    /// This function constructs a filesystem path for a test file or a test
    /// directory using, as ingredients, the name of this test, its execution
    /// recurrance index, and the specified suffix.
    ///
    /// The directory part of the returned path will depend on configuration
    /// parameters \ref TestConfig::test_file_subdir and \ref
    /// TestConfig::test_file_base_dir.
    ///
    /// This function is used by the standard test file guards, \ref
    /// TestFileGuard and \ref TestDirGuard. Custom test file guards should use
    /// this function to produce appropriate filesystem paths.
    ///
    std::filesystem::path make_test_path(std::string_view suffix) const;

    /// \brief Keep test files.
    ///
    /// This function returns the value of TestConfig::keep_test_files in the
    /// configuration object passed to \ref TestList::run().
    ///
    /// This function is used by the standard test file guards, \ref
    /// TestFileGuard and \ref TestDirGuard. Custom test file guards should call
    /// this function to determine whether the files should be deleted or left
    /// in place after end of use.
    ///
    bool keep_test_files() const noexcept;

    /// \brief Get filesystem path of data file.
    ///
    /// This function constructs the filesystem path required to reach a data
    /// file residing in the source directory, or residing in a reflection of
    /// the source directory (somewhere with the same directory substructure and
    /// containing all the relevant data files).
    ///
    /// This function first resolves \p subdir_path against the root directory
    /// path specified by \ref TestConfig::data_root_dir, and then resolves \p
    /// path against that.
    ///
    /// \param subdir_path The filesystem path of the caller relative to the
    /// root of the source directory structure. The intention is that this is
    /// the directory that contains the source file that conatins the calling
    /// code. It must be specified in the generic format as understood by
    /// `std::filesystem::path`. Use an empty string to specify the root of the
    /// source directory structure.
    ///
    /// \param path The filesystem path of a data file or directory in the
    /// source directory structure specified relative to \p subdir_path. It must
    /// be specified in the generic format as understood by
    /// `std::filesystem::path`. Use an empty string to get the directory (or
    /// file) referred to be \p subdir_path.
    ///
    std::filesystem::path get_data_path(std::string_view subdir_path, std::string_view path);

    TestContext(const TestContext&) = delete;
    TestContext& operator=(const TestContext&) = delete;

private:
    static constexpr std::size_t s_max_quoted_string_size = 72;

    detail::ThreadContextImpl& m_thread_context;

    TestContext(detail::ThreadContextImpl&, const TestDetails&,
                std::string_view mapped_file_path, std::size_t test_index,
                int recurrence_index) noexcept;

    void test_failed(std::string_view message);

    void check_failed(Location, std::string_view message);
    void cond_failed(Location, std::string_view macro_name, std::string_view cond_text);
    template<class A, class B>
    void compare_failed(Location, std::string_view macro_name, std::string_view a_text,
                        std::string_view b_text, const A& a, const B& b);
    void compare_failed_2(Location, std::string_view macro_name, std::string_view a_text,
                          std::string_view b_text, std::string_view a_val, std::string_view b_val);
    void inexact_compare_failed(Location, std::string_view  macro_name, std::string_view a_text,
                                std::string_view b_text, std::string_view eps_text, long double a,
                                long double b, long double eps);
    void check_throw_failed_2(Location, std::string_view expr_text,
                              std::string_view exception_name);
    void check_throw_ex_failed_2(Location, std::string_view expr_text,
                                 std::string_view exception_name,
                                 std::string_view exception_cond_text);
    void check_throw_ex_cond_failed_2(Location, std::string_view expr_text,
                                      std::string_view exception_name,
                                      std::string_view exception_cond_text);
    void check_throw_any_failed_2(Location, std::string_view expr_text);
    void check_nothrow_failed_2(Location, std::string_view expr_text, std::exception*);
    void check_equal_seq_failed(Location, std::string_view a_text, std::string_view b_text);

    template<class A, class B> static bool equal(const A& a, const B& b);
    template<class A, class B> static bool less(const A& a, const B& b);
    static bool approximately_equal(long double a, long double b, long double epsilon) noexcept;
    static bool essentially_equal(long double a, long double b, long double epsilon) noexcept;
    static bool definitely_less(long double a, long double b, long double epsilon) noexcept;

    template<class T> static void format_value(std::ostream&, const T&);

    template<class C, class T, class A>
    static void format_value(std::ostream&, const std::basic_string<C, T, A>&);

    template<class C, class T>
    static void format_value(std::ostream&, const std::basic_string_view<C, T>&);

    static void format_char(std::ostream&, char);
    template<class C> static void format_char(std::ostream&, C);

    static void format_string(std::ostream&, std::string_view);
    template<class C> static void format_string(std::ostream&, std::basic_string_view<C>);

    [[noreturn]] void abort();

    friend class detail::ThreadContextImpl;
};








// Implementation


inline bool TestContext::check(bool cond, const char* file_path, long line_number,
                               const char* cond_text)
{
    return check_cond(cond, file_path, line_number, "ARCHON_CHECK", cond_text); // Throws
}


inline bool TestContext::check_not(bool cond, const char* file_path, long line_number,
                                   const char* cond_text)
{
    return check_cond(!cond, file_path, line_number, "ARCHON_CHECK_NOT", cond_text); // Throws
}


inline bool TestContext::check_cond(bool cond, const char* file_path, long line_number,
                                    const char* macro_name, const char* cond_text)
{
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        Location location = { file_path, line_number };
        cond_failed(location, macro_name, cond_text); // Throws
    }
    return cond;
}


template<class A, class B>
inline bool TestContext::check_equal(const A& a, const B& b, const char* file_path,
                                     long line_number, const char* a_text, const char* b_text)
{
    bool cond = equal(a, b); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_EQUAL",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_not_equal(const A& a, const B& b, const char* file_path,
                                         long line_number, const char* a_text, const char* b_text)
{
    bool cond = !equal(a, b); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_NOT_EQUAL",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_less(const A& a, const B& b, const char* file_path,
                                    long line_number, const char* a_text, const char* b_text)
{
    bool cond = less(a, b); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_LESS",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_less_equal(const A& a, const B& b, const char* file_path,
                                          long line_number, const char* a_text, const char* b_text)
{
    // Note: Reverse operand order
    bool cond = !less(b, a); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_LESS_EQUAL",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_greater(const A& a, const B& b, const char* file_path,
                                       long line_number, const char* a_text, const char* b_text)
{
    // Note: Reverse operand order
    bool cond = less(b, a); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_GREATER",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_greater_equal(const A& a, const B& b, const char* file_path,
                                             long line_number, const char* a_text,
                                             const char* b_text)
{
    bool cond = !less(a, b); // Throws
    return check_compare(cond, a, b, file_path, line_number, "ARCHON_CHECK_GREATER_EQUAL",
                         a_text, b_text); // Throws
}


template<class A, class B>
inline bool TestContext::check_compare(bool cond, const A& a, const B& b, const char* file_path,
                                       long line_number, const char* macro_name,
                                       const char* a_text, const char* b_text)
{
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        Location location = { file_path, line_number };
        compare_failed(location, macro_name, a_text, b_text, a, b); // Throws
    }
    return cond;
}


inline bool TestContext::check_approximately_equal(long double a, long double b, long double eps,
                                                   const char* file_path, long line_number,
                                                   const char* a_text, const char* b_text,
                                                   const char* eps_text)
{
    bool cond = approximately_equal(a, b, eps);
    return check_inexact_compare(cond, a, b, eps, file_path, line_number,
                                 "ARCHON_CHECK_APPROXIMATELY_EQUAL", a_text, b_text,
                                 eps_text); // Throws
}


inline bool TestContext::check_essentially_equal(long double a, long double b, long double eps,
                                                 const char* file_path, long line_number,
                                                 const char* a_text, const char* b_text,
                                                 const char* eps_text)
{
    bool cond = essentially_equal(a, b, eps);
    return check_inexact_compare(cond, a, b, eps, file_path, line_number,
                                 "ARCHON_CHECK_ESSENTIALLY_EQUAL", a_text, b_text,
                                 eps_text); // Throws
}


inline bool TestContext::check_definitely_less(long double a, long double b, long double eps,
                                               const char* file_path, long line_number,
                                               const char* a_text, const char* b_text,
                                               const char* eps_text)
{
    bool cond = definitely_less(a, b, eps);
    return check_inexact_compare(cond, a, b, eps, file_path, line_number,
                                 "ARCHON_CHECK_DEFINITELY_LESS", a_text, b_text,
                                 eps_text); // Throws
}


inline bool TestContext::check_definitely_greater(long double a, long double b, long double eps,
                                                  const char* file_path, long line_number,
                                                  const char* a_text, const char* b_text,
                                                  const char* eps_text)
{
    // Note: Reverse operand order
    bool cond = definitely_less(b, a, eps);
    return check_inexact_compare(cond, a, b, eps, file_path, line_number,
                                 "ARCHON_CHECK_DEFINITELY_GREATER", a_text, b_text,
                                 eps_text); // Throws
}


inline bool TestContext::check_inexact_compare(bool cond, long double a, long double b,
                                               long double eps, const char* file_path,
                                               long line_number, const char* macro_name,
                                               const char* a_text, const char* b_text,
                                               const char* eps_text)
{
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        Location location = { file_path, line_number };
        inexact_compare_failed(location, macro_name, a_text, b_text, eps_text,
                               a, b, eps); // Throws
    }
    return cond;
}


inline void TestContext::check_throw_failed(const char* file_path, long line_number,
                                            const char* expr_text, const char* exception_name)
{
    Location location = { file_path, line_number };
    check_throw_failed_2(location, expr_text, exception_name); // Throws
}


inline void TestContext::check_throw_ex_failed(const char* file_path, long line_number,
                                               const char* expr_text, const char* exception_name,
                                               const char* exception_cond_text)
{
    Location location = { file_path, line_number };
    check_throw_ex_failed_2(location, expr_text, exception_name, exception_cond_text); // Throws
}


inline void TestContext::check_throw_ex_cond_failed(const char* file_path, long line_number,
                                                    const char* expr_text,
                                                    const char* exception_name,
                                                    const char* exception_cond_text)
{
    Location location = { file_path, line_number };
    check_throw_ex_cond_failed_2(location, expr_text, exception_name,
                                 exception_cond_text); // Throws
}


inline void TestContext::check_throw_any_failed(const char* file_path, long line_number,
                                                const char* expr_text)
{
    Location location = { file_path, line_number };
    check_throw_any_failed_2(location, expr_text); // Throws
}


inline void TestContext::check_nothrow_failed(const char* file_path, long line_number,
                                              const char* expr_text, std::exception* exc)
{
    Location location = { file_path, line_number };
    check_nothrow_failed_2(location, expr_text, exc); // Throws
}


template<class A, class B>
bool TestContext::check_equal_seq(const A& a, const B& b, const char* file_path, long line_number,
                                  const char* a_text, const char* b_text)
{
    auto pred = [](const auto& c, const auto& d) {
        return equal(c, d); // Throws
    };
    bool cond = std::equal(std::begin(a), std::end(a), std::begin(b), std::end(b), pred); // Throws
    if (ARCHON_LIKELY(cond)) {
        check_succeeded();
    }
    else {
        Location location = { file_path, line_number };
        check_equal_seq_failed(location, a_text, b_text); // Throws
    }
    return cond;
}


template<class A, class B>
void TestContext::compare_failed(Location location, std::string_view macro_name,
                                 std::string_view a_text, std::string_view b_text,
                                 const A& a, const B& b)
{
    std::array<char, 512> seed_memory;
    base::SeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(thread_context.root_context.locale); // Throws
    format_value(out, a); // Throws
    std::size_t i = out.streambuf().size();
    format_value(out, b); // Throws
    std::string_view a_val = out.view().substr(0, i);
    std::string_view b_val = out.view().substr(i);
    compare_failed_2(location, macro_name, a_text, b_text, a_val, b_val); // Throws
}


template<class A, class B> inline bool TestContext::equal(const A& a, const B& b)
{
    const bool both_are_integral = (std::is_integral_v<A> && std::is_integral_v<B>);
    if constexpr (both_are_integral) {
        return base::int_equal_to(a, b);
    }
    else {
        return (a == b); // Throws
    }
}


template<class A, class B> inline bool TestContext::less(const A& a, const B& b)
{
    const bool both_are_integral = (std::is_integral_v<A> && std::is_integral_v<B>);
    if constexpr (both_are_integral) {
        return base::int_less_than(a, b);
    }
    else {
        return (a < b); // Throws
    }
}


// See Donald. E. Knuth, "The Art of Computer Programming", 3rd edition, volume
// II, section 4.2.2 "Accuracy of Floating Point Arithmetic", definitions
// (21)-(24).
//
inline bool TestContext::approximately_equal(long double a, long double b,
                                             long double epsilon) noexcept
{
    return (std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) * epsilon);
}


inline bool TestContext::essentially_equal(long double a, long double b,
                                           long double epsilon) noexcept
{
    return (std::abs(a - b) <= std::min(std::abs(a), std::abs(b)) * epsilon);
}


inline bool TestContext::definitely_less(long double a, long double b,
                                         long double epsilon) noexcept
{
    return (b - a > std::max(std::abs(a), std::abs(b)) * epsilon);
}


template<class T> inline void TestContext::format_value(std::ostream& out, const T& value)
{
    if constexpr (std::is_floating_point_v<T>) {
        out << base::with_precision(value, std::numeric_limits<T>::digits10 + 1); // Throws
    }
    else if constexpr (std::is_same_v<T, char> || std::is_same_v<T, wchar_t>) {
        format_char(out, value); // Throws
    }
    else if constexpr (std::is_convertible_v<T, std::string_view>) {
        format_string(out, std::string_view(value)); // Throws
    }
    else if constexpr (std::is_convertible_v<T, std::wstring_view>) {
        format_string(out, std::wstring_view(value)); // Throws
    }
    else {
        out << value; // Throws
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
    base::BasicValueFormatter formatter(seed_memory_1, locale); // Throws
    std::basic_string_view<C> string_2 = formatter.format(base::quoted_s(string)); // Throws

    std::array<char, 16> seed_memory_2;
    base::BasicStringEncoder<C> encoder(locale, seed_memory_2); // Throws
    std::string_view string_3 = encoder.encode(string_2); // Throws
    out << string_3; // Throws
}


template<class C>
inline void TestContext::format_string(std::ostream& out, std::basic_string_view<C> string)
{
    // FIXME: Should probably use an encoding stream wrapper instead

    std::locale locale = out.getloc(); // Throws
    constexpr std::size_t seed_memory_size = std::min<std::size_t>(s_max_quoted_string_size, 512);
    std::array<C, seed_memory_size> seed_memory_1;
    base::BasicValueFormatter formatter(seed_memory_1, locale); // Throws
    std::basic_string_view<C> string_2 =
        formatter.format(base::quoted(string, s_max_quoted_string_size)); // Throws

    std::array<char, 512> seed_memory_2;
    base::BasicStringEncoder<C> encoder(locale, seed_memory_2); // Throws
    std::string_view string_3 = encoder.encode(string_2); // Throws
    out << string_3; // Throws
}


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_TEST_CONTEXT_HPP
