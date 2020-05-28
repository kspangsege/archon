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


#include <stdexcept>
#include <system_error>

#include <archon/core/impl/config.h>
#include <archon/core/features.h>
#include <archon/core/string.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_mode.hpp>
#include <archon/core/build_environment.hpp>


using namespace archon;
using core::BuildEnvironment;


BuildEnvironment::BuildEnvironment(std::string_view argv0, Params params, const std::locale& locale)
{
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path(); // Throws
    fs::path src_path = core::make_fs_path_generic(params.src_path, locale); // Throws

    // Detect source root
    std::error_code ec;
    fs::path argv0_2 = fs::canonical(core::make_fs_path_auto(argv0, locale), ec); // Throws
    if (!ec && argv0_2.has_parent_path()) {
#if ARCHON_WINDOWS
        std::string bin_path_1 = core::concat(params.bin_path, std::string_view(".exe")); // Throws
#else
        std::string_view bin_path_1 = params.bin_path;
#endif

        fs::path bin_path_2 = core::make_fs_path_generic(bin_path_1, locale); // Throws
#if ARCHON_ASSUME_VISUAL_STUDIO_CMAKE_GENERATOR
        bin_path_2 = (bin_path_2.parent_path() / core::make_fs_path_generic(ARCHON_BUILD_MODE, locale) /
                      bin_path_2.filename()); // Throws
#endif

        if (argv0_2.filename() == bin_path_2.filename()) { // Throws
            fs::path argv0_dir = argv0_2.parent_path(); // Throws
            fs::path bin_dir_path = bin_path_2.parent_path(); // Throws
            fs::path rev_bin_dir_path = fs::path().lexically_relative(bin_dir_path); // Throws
            fs::path build_root = (argv0_dir / rev_bin_dir_path).lexically_normal(); // Throws
            fs::path source_from_build_path = core::make_fs_path_auto(params.source_from_build_path, locale) /
                fs::path(); // Throws
            fs::path source_root = (build_root / source_from_build_path).lexically_normal(); // Throws

            if (exists(source_root / src_path)) { // Throws
                if (source_root.has_relative_path() && build_root.has_relative_path()) { // Throws
                    core::remove_trailing_slash(source_root); // Throws
                    core::remove_trailing_slash(build_root); // Throws
                    m_source_root = std::move(source_root);
                    m_build_root  = std::move(build_root);
                    m_relative_source_root = m_source_root.lexically_relative(cwd); // Throws
                    m_relative_build_root  = m_build_root.lexically_relative(cwd); // Throws
                    core::dot_to_empty(m_relative_source_root); // Throws
                    core::dot_to_empty(m_relative_build_root); // Throws
                    m_source_root_was_detected = true;
                }
            }
        }
    }

    // Detect project root
    if (m_source_root_was_detected) {
        fs::path path = core::make_fs_path_generic(params.src_root, locale); // Throws
        fs::path rev_path = fs::path().lexically_relative(path); // Throws
        fs::path project_root = (m_source_root / rev_path).lexically_normal(); // Throws
        if (project_root / path == m_source_root) { // Throws
            if (project_root.has_relative_path()) { // Throws
                core::remove_trailing_slash(project_root); // Throws
                m_project_root = std::move(project_root);
                m_relative_project_root = m_project_root.lexically_relative(cwd); // Throws
                core::dot_to_empty(m_relative_project_root); // Throws
                m_project_root_was_detected = true;
            }
        }
    }

    // Detect `__FILE__` prefix
    if (!src_path.has_root_path()) {
        fs::path file_path = core::make_fs_path_auto(params.file_path, locale); // Throws
        for (;;) {
            if (src_path.filename() != file_path.filename())
                break;
            if (!src_path.has_parent_path()) {
                file_path.remove_filename(); // Throws
                m_file_path_prefix = file_path; // Throws
                m_file_path_prefix_was_detected = true;
                break;
            }
            if (!file_path.has_parent_path())
                break;
            src_path  = src_path.parent_path(); // Throws
            file_path = file_path.parent_path(); // Throws
        }
    }
}



void BuildEnvironment::remove_file_path_prefix(std::filesystem::path& file_path) const
{
    if (m_file_path_prefix_was_detected) {
        auto i_1   = m_file_path_prefix.begin();
        auto end_1 = m_file_path_prefix.end();
        auto i_2   = file_path.begin();
        auto end_2 = file_path.end();
        if (i_1 != end_1)
            --end_1; // Don't care about final empty part of prefix
        for (;;) {
            if (i_1 == end_1) {
                // End of prefix
                file_path = file_path.lexically_relative(m_file_path_prefix); // Throws
                return;
            }
            if (i_2 == end_2) {
                // End of path before end of prefix
                break;
            }
            if (*i_1 != *i_2) {
                // Mismatch on current part
                break;
            }
            ++i_1;
            ++i_2;
        }
        throw std::runtime_error("File path prefix mismatch");
    }
    throw std::runtime_error("File path prefix was not detected");
}
