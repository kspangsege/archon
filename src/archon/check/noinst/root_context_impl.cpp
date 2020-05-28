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


#include <utility>

#include <archon/check/noinst/root_context_impl.hpp>


using namespace archon;
namespace impl = check::impl;
using impl::RootContextImpl;


RootContextImpl::RootContextImpl(int num_repetitions, int num_threads, const std::locale& locale,
                                 log::Logger& report_logger, const std::string* log_paths, check::Reporter& r,
                                 core::Span<const Test> t, core::Span<const Exec> ce, core::Span<const Exec> ne,
                                 bool abort_on_failure, bool keep_test_files, core::FilesystemPathRef test_file_dir,
                                 core::FilesystemPathRef data_file_dir,
                                 const check::SourcePathMapper* source_path_mapper,
                                 core::Span<const core::SeedSeq::result_type> random_seed,
                                 int rseed_rep_no_override) noexcept
    : RootContext(t.size(), ce.size() + ne.size(), num_repetitions, num_threads, locale, report_logger, log_paths)
    , reporter(r)
    , tests(t)
    , concur_execs(ce)
    , nonconcur_execs(ne)
    , m_abort_on_failure(abort_on_failure)
    , m_keep_test_files(keep_test_files)
    , m_test_file_dir(test_file_dir)
    , m_data_file_dir(data_file_dir)
    , m_source_path_mapper(source_path_mapper)
    , m_random_seed(random_seed)
    , m_rseed_rep_no_override(rseed_rep_no_override)
{
}


auto RootContextImpl::map_source_path(std::string_view path) const -> std::string
{
    if (!m_source_path_mapper)
        return std::string(path); // Throws
    namespace fs = std::filesystem;
    fs::path path_2 = core::make_fs_path_auto(path, locale); // Throws
    m_source_path_mapper->map(path_2); // Throws
    return core::path_to_string_native(path_2, locale); // Throws
}


auto RootContextImpl::get_test_details(std::size_t test_index) const noexcept -> const check::TestDetails&
{
    return tests[test_index].list_entry->details;
}
