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


#include <cstdlib>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/check/test_context.hpp>
#include <archon/check/noinst/root_context_impl.hpp>
#include <archon/check/noinst/thread_context_impl.hpp>


using namespace archon;
namespace impl = check::impl;
using check::TestContext;


auto TestContext::seed_seq() const noexcept -> const core::SeedSeq&
{
    return m_thread_context.seed_seq();
}


auto TestContext::get_data_path(std::string_view subdir_path, std::string_view path) -> std::filesystem::path
{
    namespace fs = std::filesystem;
    fs::path subdir_path_2 = core::make_fs_path_generic(subdir_path, locale); // Throws
    fs::path path_2 = core::make_fs_path_generic(path, locale); // Throws
    const impl::RootContextImpl& root_context = m_thread_context.get_root_context();
    fs::path path_3 = root_context.get_data_file_dir() / subdir_path_2 / path_2; // Throws
    return path_3;
}


auto TestContext::make_test_path(std::string_view suffix) const -> std::filesystem::path
{
    std::array<char, 512> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view path_1 = formatter.format("%s.%s%s", test_details.name, core::as_int(repetition_no),
                                               suffix); // Throws
    namespace fs = std::filesystem;
    fs::path path_2 = core::make_fs_path_generic(path_1, locale); // Throws
    const impl::RootContextImpl& root_context = m_thread_context.get_root_context();
    fs::path path_3 = root_context.get_test_file_dir() / path_2; // Throws
    return path_3;
}


bool TestContext::keep_test_files() const noexcept
{
    const impl::RootContextImpl& root_context = m_thread_context.get_root_context();
    return root_context.keep_test_files();
}


void TestContext::check_succeeded() noexcept
{
    ++m_thread_context.num_checks;
}


void TestContext::check_failed(check::Location location, std::string_view message)
{
    m_thread_context.check_failed(*this, location, message, m_report_logger); // Throws
}


TestContext::TestContext(impl::ThreadContextImpl& tc, const check::TestDetails& td, std::string_view mfp,
                         std::size_t ti, int rn, log::Logger& report_logger, log::Logger& inner_logger) noexcept
    : thread_context(tc)
    , test_details(td)
    , mapped_file_path(mfp)
    , test_index(ti)
    , repetition_no(rn)
    , logger(inner_logger)
    , locale(tc.root_context.locale)
    , m_thread_context(tc)
    , m_report_logger(report_logger)
{
}


void TestContext::test_failed(std::string_view message)
{
    m_thread_context.test_failed(*this, message); // Throws
}


void TestContext::check_general_cond_failed(check::Location location, std::string_view macro_name,
                                            std::string_view cond_text)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("%s(%s) failed", macro_name, cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_special_cond_failed_3(check::Location location, std::string_view macro_name,
                                              core::Span<const std::string_view> args,
                                              core::Span<const std::string_view> arg_vals)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("%s(%s) failed with (%s)", macro_name, core::as_list(args),
                                                core::as_list(arg_vals)); // Throws
    check_failed(location, message); // Throws
}


void TestContext::compare_failed_2(check::Location location, std::string_view macro_name,
                                   std::string_view a_text, std::string_view b_text, std::string_view comp_text,
                                   std::string_view a_val, std::string_view b_val)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("%s(%s, %s, %s) failed with (%s, %s, %s)", macro_name, a_text, b_text,
                                                comp_text, a_val, b_val, comp_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_failed_2(check::Location location, std::string_view expr_text,
                                       std::string_view exception_name)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW(%s, %s) failed: Did not throw", expr_text,
                                                exception_name); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_ex_failed_2(check::Location location, std::string_view expr_text,
                                          std::string_view exception_name, std::string_view exception_cond_text)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_EX(%s, %s, %s) failed: Did not throw", expr_text,
                                                exception_name, exception_cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_ex_cond_failed_2(check::Location location, std::string_view expr_text,
                                               std::string_view exception_name,
                                               std::string_view exception_cond_text)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_EX(%s, %s, %s) failed: Did throw, but condition "
                                                "failed", expr_text, exception_name, exception_cond_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_throw_any_failed_2(check::Location location, std::string_view expr_text)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did not throw",
                                                expr_text); // Throws
    check_failed(location, message); // Throws
}


void TestContext::check_nothrow_failed_2(check::Location location, std::string_view expr_text, std::exception* exc)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message;
    if (exc) {
        message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did throw %s: %s", expr_text,
                                   core::get_type_name(*exc), exc->what()); // Throws
    }
    else {
        message = formatter.format("ARCHON_CHECK_THROW_ANY(%s) failed: Did throw exception of unknown type",
                                   expr_text); // Throws
    }
    check_failed(location, message); // Throws
}


void TestContext::check_equal_seq_failed_4(check::Location location, std::string_view a_text, std::string_view b_text,
                                           std::size_t index, const std::string_view* a_val,
                                           const std::string_view* b_val)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message;
    if (a_val && b_val) {
        message = formatter.format("ARCHON_CHECK_EQUAL_SEQ(%s, %s) failed: Mismatch at index %s: %s vs %s",
                                   a_text, b_text, index, *a_val, *b_val); // Throws
    }
    else if (a_val) {
        message = formatter.format("ARCHON_CHECK_EQUAL_SEQ(%s, %s) failed: Mismatch at index %s: Extra elements in "
                                   "first sequence (starting with %s)", a_text, b_text, index, *a_val); // Throws
    }
    else {
        ARCHON_ASSERT(b_val);
        message = formatter.format("ARCHON_CHECK_EQUAL_SEQ(%s, %s) failed: Mismatch at index %s: Extra elements in "
                                   "second sequence (starting with %s)", a_text, b_text, index, *b_val); // Throws
    }
    check_failed(location, message); // Throws
}


void TestContext::check_compare_seq_failed_4(check::Location location, std::string_view a_text,
                                             std::string_view b_text, std::string_view comp_text, std::size_t index,
                                             const std::string_view* a_val, const std::string_view* b_val)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view message;
    if (a_val && b_val) {
        message = formatter.format("ARCHON_CHECK_COMPARE_SEQ(%s, %s, %s) failed: Mismatch at index %s: %s vs %s",
                                   a_text, b_text, comp_text, index, *a_val, *b_val); // Throws
    }
    else if (a_val) {
        message = formatter.format("ARCHON_CHECK_COMPARE_SEQ(%s, %s, %s) failed: Mismatch at index %s: Extra elements "
                                   "in first sequence (starting with %s)", a_text, b_text, comp_text, index,
                                   *a_val); // Throws
    }
    else {
        ARCHON_ASSERT(b_val);
        message = formatter.format("ARCHON_CHECK_COMPARE_SEQ(%s, %s, %s) failed: Mismatch at index %s: Extra elements "
                                   "in second sequence (starting with %s)", a_text, b_text, comp_text, index,
                                   *b_val); // Throws
    }
    check_failed(location, message); // Throws
}


void TestContext::format_char(std::ostream& out, char ch)
{
    std::string_view string(&ch, 1);
    out << core::quoted_s(string); // Throws
}


void TestContext::format_string(std::ostream& out, std::string_view string)
{
    out << core::quoted(string, s_max_quoted_string_size); // Throws
}


[[noreturn]] void TestContext::abort()
{
    const check::RootContext& context = thread_context.root_context;
    if (context.num_threads == 1) {
        context.report_logger.info("Aborting due to failure"); // Throws
    }
    else {
        context.report_logger.info("Aborting due to failure in test thread %s",
                                   thread_context.thread_index + 1); // Throws
    }
    std::abort();
}
