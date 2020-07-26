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


#include <utility>

#include <archon/unit_test/noinst/root_context_impl.hpp>


using namespace archon;
using namespace archon::unit_test;
using namespace archon::unit_test::detail;


RootContextImpl::RootContextImpl(int num_repetitions, int num_threads, std::locale locale,
                                 base::Logger& report_logger, const std::string* log_paths,
                                 Reporter& r, std::vector<Test> t, std::vector<Exec> ce,
                                 std::vector<Exec> ne, bool abort_on_failure, bool keep_test_files,
                                 base::FilesystemPath test_file_dir,
                                 base::FilesystemPath data_root_dir,
                                 const SourcePathMapper* source_path_mapper,
                                 unit_test::RandomSeed random_seed) noexcept :
    RootContext(t.size(), num_repetitions, num_threads, std::move(locale), report_logger,
                log_paths),
    reporter(r),
    tests(std::move(t)),
    concur_execs(std::move(ce)),
    nonconcur_execs(std::move(ne)),
    m_abort_on_failure(abort_on_failure),
    m_keep_test_files(keep_test_files),
    m_test_file_dir(std::move(test_file_dir)),
    m_data_root_dir(std::move(data_root_dir)),
    m_source_path_mapper(source_path_mapper),
    m_random_seed(std::move(random_seed)),
    m_seed_seq(base::SeedSeq::no_copy(m_random_seed.span()))
{
}


std::string RootContextImpl::map_source_path(std::string_view path) const
{
    if (!m_source_path_mapper)
        return std::string(path); // Throws
    namespace fs = std::filesystem;
    fs::path path_2 = base::make_fs_path_auto(path, locale); // Throws
    m_source_path_mapper->map(path_2); // Throws
    return base::path_to_string_native(path_2, locale); // Throws
}


const TestDetails& RootContextImpl::get_test_details(std::size_t test_index) const noexcept
{
    return tests[test_index].list_entry->details;
}
