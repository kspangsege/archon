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
#include <optional>
#include <tuple>
#include <string_view>
#include <vector>
#include <locale>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log/logger.hpp>
#include <archon/log/limit_logger.hpp>
#include <archon/cli.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/fallback_implementation.hpp>
#include <archon/font/freetype_implementation.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    std::vector<font::code_point_range> ranges;
    font::size font_size = 16;
    log::LogLevel log_level_limit = log::LogLevel::info;

    cli::Spec spec;
    pat("[<range>...]", cli::no_attributes, spec,
        "If no code point ranges are specified, the single range 0 -> 65534 will be used.",
        std::tie(ranges)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-s, --font-size", "<size>", cli::no_attributes, spec,
        "Set the font size as close to the specified size as possible. The default font size is @V.",
        cli::assign(font_size)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger logger(core::File::get_stdout(), locale);
    std::optional<font::code_point> prev_last;
    for (font::code_point_range range : ranges) {
        if (ARCHON_LIKELY(!prev_last.has_value() ||
                          range.first().to_int() > prev_last.value().to_int())) {
            prev_last = range.last();
            continue;
        }
        logger.error("Overlapping code point ranges");
        return EXIT_FAILURE;
    }

    if (ranges.empty()) {
        font::code_point first, last;
        bool success = (first.try_from_int(0) && last.try_from_int(65534));
        ARCHON_ASSERT(success);
        ranges = {
            { first, last },
        };
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
    build_env_params.bin_path  = "archon/font/tool/archon-regen-fallback-font";
    build_env_params.src_path  = "archon/font/tool/regen_fallback_font.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    namespace fs = std::filesystem;
    fs::path resource_dir = (build_env.get_relative_source_root() /
                             core::make_fs_path_generic("archon/font", locale)); // Throws
    std::string_view file_name = "liberation-mono-regular.ttf";
    namespace fs = std::filesystem;
    fs::path file = resource_dir / core::make_fs_path_generic(file_name, locale); // Throws
    log::LimitLogger limit_logger(logger, log_level_limit); // Throws
    font::loader::config config;
    config.logger = &limit_logger;
    std::unique_ptr<font::loader> loader = font::new_freetype_loader_from_font_file(file, locale, config); // Throws
    std::unique_ptr<font::face> face = loader->load_default_face(); // Throws
    face->set_approx_size(font_size); // Throws

    std::vector<font::code_point_range> orig_ranges;
    font::fallback_font_params orig_params;
    std::unique_ptr<char[]> string_owner;
    font::size orig_size;
    if (ARCHON_LIKELY(font::try_get_fallback_font_params(resource_dir, locale, orig_ranges, orig_params, string_owner,
                                                         orig_size))) { // Throws
        if (ARCHON_UNLIKELY(ranges != orig_ranges)) {
            logger.warn("Using different codepoint ranges (original was %s, now using %s)", core::as_list(orig_ranges),
                        core::as_list(ranges)); // Throws
        }
        if (ARCHON_UNLIKELY(face->get_family_name() != orig_params.family_name)) {
            logger.warn("Using different font face (original family name was %s, now using %s)",
                        core::quoted(orig_params.family_name), core::quoted(face->get_family_name())); // Throws
        }
        if (ARCHON_UNLIKELY(face->get_style_name() != orig_params.style_name)) {
            logger.warn("Using different font face (original style name was %s, now using %s)",
                        core::quoted(orig_params.style_name), core::quoted(face->get_style_name())); // Throws
        }
        if (ARCHON_UNLIKELY(face->is_bold() != orig_params.is_bold)) {
            core::BoolSpec spec { "non-bold", "bold" };
            logger.warn("Using different font face (original face was %s, now using face that is %s)",
                        core::as_bool(orig_params.is_bold, spec), core::as_bool(face->is_bold(), spec)); // Throws
        }
        if (ARCHON_UNLIKELY(face->is_italic() != orig_params.is_italic)) {
            core::BoolSpec spec { "non-italic", "italic" };
            logger.warn("Using different font face (original face was %s, now using face that is %s)",
                        core::as_bool(orig_params.is_italic, spec), core::as_bool(face->is_italic(), spec)); // Throws
        }
        if (ARCHON_UNLIKELY(face->is_monospace() != orig_params.is_monospace)) {
            core::BoolSpec spec { "proportional", "monospace" };
            logger.warn("Using different font face (original face was %s, now using face that is %s)",
                        core::as_bool(orig_params.is_monospace, spec),
                        core::as_bool(face->is_monospace(), spec)); // Throws
        }
        if (ARCHON_UNLIKELY(face->get_size() != orig_size)) {
            logger.warn("Using different font rendering size (original was %s, now using %s)", orig_size,
                        face->get_size()); // Throws
        }
    }
    else {
        logger.warn("Failed to determine original font parameters");
    }

    std::string_view file_name_qual = "-new"; // Don't clobber the original files
    font::regen_fallback_font(*face, ranges, resource_dir, file_name_qual, locale, config); // Throws
}
