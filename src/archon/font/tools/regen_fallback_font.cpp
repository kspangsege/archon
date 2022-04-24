// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <memory>
#include <utility>
#include <tuple>
#include <optional>
#include <vector>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log/logger.hpp>
#include <archon/log/limit_logger.hpp>
#include <archon/cli.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/loader_fallback.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    std::vector<font::CodePointRange> ranges;
    log::LogLevel log_level_limit = log::LogLevel::info;

    cli::Spec spec;
    pat("[<range>...]", cli::no_attributes, spec,
        "If no code point ranges are specified, an attempt will be made to load them from the existing fallback font. "
        "If this fails, the single range 0 -> 127 will be used.",
        std::tie(ranges)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are \"off\", \"fatal\", \"error\", \"warn\", \"info\", "
        "\"detail\", \"debug\", \"trace\", and \"all\". The default limit is \"@V\".",
        cli::assign(log_level_limit)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger logger(core::File::get_cout(), locale);
    std::optional<font::CodePoint> prev_last;
    for (font::CodePointRange range : ranges) {
        if (ARCHON_LIKELY(!prev_last.has_value() ||
                          range.first().to_int() > prev_last.value().to_int())) {
            prev_last = range.last();
            continue;
        }
        logger.error("Overlapping code point ranges");
        return EXIT_FAILURE;
    }

    // `src_root` is the relative path to the root of the source tree from the root of the
    // project.
    //
    // `src_path` is the relative path to this source file from the root of source tree.
    //
    // `bin_path` is the relative path to the executable from the root of the source root as
    // it is reflected into the build directory.
    //
    core::BuildEnvironment::Params build_env_params;
    build_env_params.file_path = __FILE__;
    build_env_params.bin_path  = "archon/font/tools/archon-regen-fallback-font";
    build_env_params.src_path  = "archon/font/tools/regen_fallback_font.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    namespace fs = std::filesystem;
    fs::path resource_dir = (build_env.get_relative_source_root() /
                             core::make_fs_path_generic("archon/font", locale)); // Throws
    log::LimitLogger limit_logger(logger, log_level_limit); // Throws
    font::Loader::Config config;
    config.logger = &limit_logger;
    std::unique_ptr<font::Loader> font_loader =
        font::Loader::new_default_loader(resource_dir, locale, config); // Throws
    std::unique_ptr<font::Face> font_face = font_loader->load_default_face(); // Throws

    bool try_keep_orig_font_size = true;
    font::regen_fallback_font(*font_face, try_keep_orig_font_size, ranges, resource_dir, locale, config); // Throws
}
