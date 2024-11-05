// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <vector>
#include <string_view>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>


using namespace archon;


namespace {


class ProgressTracker final
    : public image::ProgressTracker {
public:
    ProgressTracker(log::Logger& logger) noexcept
        : m_logger(logger)
    {
    }

    void progress(const image::Image&, double fraction) override
    {
        m_logger.info("Load progress: %s", core::as_percent(fraction)); // Throws
    }

private:
    log::Logger& m_logger;
};


class CommentHandler final
    : public image::CommentHandler {
public:
    CommentHandler(log::Logger& logger) noexcept
        : m_logger(logger)
    {
    }

    void handle_comment(std::string_view comment) override
    {
        m_logger.info("Comment: %s", core::quoted(comment)); // Throws
    }

private:
    log::Logger& m_logger;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    std::vector<fs::path> paths;
    bool list_image_file_formats = false;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool abort_on_error = false;
    std::optional<std::string> optional_file_format;
    bool progress = false;
    bool show_comments = false;
    image::LoadConfig load_config;

    cli::Spec spec;
    pat("[<path>...]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(paths)); // Throws
    pat("--list-image-file-formats", cli::no_attributes, spec,
        "List known display implementations.",
        [&] {
            list_image_file_formats = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-e, --abort-on-error", "", cli::no_attributes, spec,
        "If the loading of one image fails, do not proceed to load additional images.",
        cli::raise_flag(abort_on_error)); // Throws

    opt("-s, --file-format", "<ident>", cli::no_attributes, spec,
        "Assume that the specified images use this file format. By default, automatic detection of the file format "
        "will be attempted for each image individually. Use `--list-image-file-formats` to see a list of supported "
        "image file formats.",
        cli::assign(optional_file_format)); // Throws

    opt("-p, --progress", "", cli::no_attributes, spec,
        "Report on loading progress.",
        cli::raise_flag(progress)); // Throws

    opt("-c, --show-comments", "", cli::no_attributes, spec,
        "Show comments in loaded images.",
        cli::raise_flag(show_comments)); // Throws

    opt("-r, --read-buffer-size", "<size>", cli::no_attributes, spec,
        "Set the size of the read buffer used when loading images. The default size is @V.",
        cli::assign(core::as_int(load_config.read_buffer_size))); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    const image::FileFormatRegistry& file_format_registry = image::FileFormatRegistry::get_default_registry();
    if (list_image_file_formats) {
        image::list_file_formats(core::File::get_stdout(), locale, file_format_registry); // Throws
        return EXIT_SUCCESS;
    }
    load_config.registry = &file_format_registry;

    log::FileLogger root_logger(core::File::get_stdout(), locale); // Throws
    log::LimitLogger limit_logger(root_logger, log_level_limit); // Throws
    log::ChannelLogger logger(limit_logger, "root", root_logger); // Throws

    if (ARCHON_UNLIKELY(optional_file_format.has_value()))
        load_config.file_format = optional_file_format.value();

    std::string_view detected_file_format;
    load_config.detected_file_format = &detected_file_format;

    bool errors_occurred = false;
    core::StringFormatter string_formatter(locale); // Throws
    for (const fs::path& path : paths) {
        std::string path_2 = core::path_to_string_native(path, locale); // Throws
        log::PrefixLogger path_logger(logger, string_formatter.format("%s: ", path_2)); // Throws
        load_config.logger = &path_logger;
        log::Logger path_root_logger(path_logger, "root"); // Throws
        ProgressTracker progress_tracker(path_root_logger);
        if (progress)
            load_config.progress_tracker = &progress_tracker;
        CommentHandler comment_handler(path_root_logger);
        if (show_comments)
            load_config.comment_handler = &comment_handler;
        std::unique_ptr<image::WritableImage> image;
        std::error_code ec;
        if (ARCHON_LIKELY(image::try_load(path, image, locale, load_config, ec))) { // Throws
            image::Size size = image->get_size();
            path_root_logger.info("Loaded (%s, %sx%s)", detected_file_format,
                                  core::as_int(size.width), core::as_int(size.height)); // Throws
            continue;
        }
        path_logger.error("Failed to load image: %s", ec.message()); // Throws
        if (abort_on_error)
            return EXIT_FAILURE;
        errors_occurred = true;
    }

    if (errors_occurred) {
        logger.error("Some images failed to load"); // Throws
        return EXIT_FAILURE;
    }
}
