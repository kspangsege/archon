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


#include <cstdlib>

#include <archon/base/demangle.hpp>
#include <archon/base/integer_formatter.hpp>
#include <archon/base/string_formatter.hpp>
#include <archon/base/filesystem.hpp>
#include <archon/unit_test/test_context.hpp>
#include <archon/unit_test/noinst/root_context_impl.hpp>
#include <archon/unit_test/noinst/thread_context_impl.hpp>


using namespace archon;
using namespace archon::unit_test;


TestContext::TestContext(detail::ThreadContextImpl& tc, const TestDetails& td,
                         std::string_view mfp, std::size_t ti, int ri, base::Logger& report_logger,
                         base::Logger& inner_logger) noexcept :
    thread_context(tc),
    test_details(td),
    mapped_file_path(mfp),
    test_index(ti),
    recurrence_index(ri),
    logger(inner_logger),
    m_thread_context(tc),
    m_report_logger(report_logger)
{
}


void TestContext::check_succeeded() noexcept
{
    ++m_thread_context.num_checks;
}


unit_test::SeedSeq& TestContext::seed_seq() const noexcept
{
    detail::RootContextImpl& root_context = m_thread_context.get_root_context();
    return root_context.seed_seq();
}


std::filesystem::path TestContext::make_test_path(std::string_view suffix) const
{
    std::array<char, 512> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view path_1 = formatter.format("%s.%s%s", test_details.name,
                                               base::as_dec_int(recurrence_index + 1),
                                               suffix); // Throws
    namespace fs = std::filesystem;
    fs::path path_2 = base::make_fs_path_generic(path_1, locale); // Throws
    const detail::RootContextImpl& root_context = m_thread_context.get_root_context();
    fs::path path_3 = root_context.get_test_file_dir() / path_2; // Throws
    return path_3;
}


bool TestContext::keep_test_files() const noexcept
{
    const detail::RootContextImpl& root_context = m_thread_context.get_root_context();
    return root_context.keep_test_files();
}


auto TestContext::get_data_path(std::string_view subdir_path, std::string_view path) ->
    std::filesystem::path
{
    const std::locale& locale = thread_context.root_context.locale;
    namespace fs = std::filesystem;
    fs::path subdir_path_2 = base::make_fs_path_generic(subdir_path, locale); // Throws
    fs::path path_2 = base::make_fs_path_generic(path, locale); // Throws
    const detail::RootContextImpl& root_context = m_thread_context.get_root_context();
    fs::path path_3 = root_context.get_data_root_dir() / subdir_path_2 / path_2; // Throws
    return path_3;
}


void TestContext::test_failed(std::string_view message)
{
    m_thread_context.test_failed(*this, message); // Throws
}


void TestContext::check_failed(Location location, std::string_view message)
{
    m_thread_context.check_failed(*this, location, message, m_report_logger); // Throws
}


void TestContext::cond_failed(Location location, std::string_view macro_name,
                              std::string_view cond_text)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("%s(%s) failed", macro_name, cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::compare_failed_2(Location location, std::string_view macro_name,
                                   std::string_view a_text, std::string_view b_text,
                                   std::string_view a_val, std::string_view b_val)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("%s(%s, %s) failed with (%s, %s)", macro_name,
                                                a_text, b_text, a_val, b_val); // Throws
    check_failed(location, message); // Throws
}


void TestContext::inexact_compare_failed(Location location, std::string_view macro_name,
                                         std::string_view a_text, std::string_view b_text,
                                         std::string_view eps_text, long double a, long double b,
                                         long double eps)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    int precision = std::numeric_limits<long double>::digits10 + 1;
    std::string_view message = formatter.format("%s(%s, %s, %s) failed with (%s, %s, %s)",
                                                macro_name, a_text, b_text, eps_text,
                                                base::with_precision(a, precision),
                                                base::with_precision(b, precision),
                                                base::with_precision(eps, precision)); // Throws

    check_failed(location, message); // Throws
}


void TestContext::check_throw_failed_2(Location location, std::string_view expr_text,
                                       std::string_view exception_name)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW(%s, %s) failed: Did not throw",
                                                expr_text, exception_name); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_ex_failed_2(Location location, std::string_view expr_text,
                                          std::string_view exception_name,
                                          std::string_view exception_cond_text)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_EX(%s, %s, %s) failed: "
                                                "Did not throw", expr_text, exception_name,
                                                exception_cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_ex_cond_failed_2(Location location, std::string_view expr_text,
                                               std::string_view exception_name,
                                               std::string_view exception_cond_text)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_EX(%s, %s, %s) failed: "
                                                "Did throw, but condition failed", expr_text,
                                                exception_name, exception_cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_any_failed_2(Location location, std::string_view expr_text)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did not throw",
                                                expr_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_nothrow_failed_2(Location location, std::string_view expr_text,
                                         std::exception* exc)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message;
    if (exc) {
        message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did throw %s: %s",
                                   expr_text, base::get_type_name(*exc), exc->what()); // Throws
    }
    else {
        message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did throw "
                                   "exception of unknown type", expr_text); // Throws
    }
    check_failed(location, message); // Throws
}


void TestContext::check_equal_seq_failed(Location location, std::string_view a_text,
                                         std::string_view b_text)
{
    std::array<char, 1024> seed_memory;
    const std::locale& locale = thread_context.root_context.locale;
    base::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_EQUAL_SEQ(%s, %s) failed",
                                                a_text, b_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::format_char(std::ostream& out, char ch)
{
    std::string_view string(&ch, 1);
    out << base::quoted_s(string); // Throws
}


void TestContext::format_string(std::ostream& out, std::string_view string)
{
    out << base::quoted(string, s_max_quoted_string_size); // Throws
}


[[noreturn]] void TestContext::abort()
{
    const RootContext& context = thread_context.root_context;
    if (context.num_threads == 1) {
        context.report_logger.info("Aborting due to failure"); // Throws
    }
    else {
        context.report_logger.info("Aborting due to failure in test thread %s",
                                   thread_context.thread_index + 1); // Throws
    }
    std::abort();
}
