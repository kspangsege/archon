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
#include <locale>
#include <system_error>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    fs::path source_path, destination_path;
    util::Color color = util::colors::white;
    std::optional<image::Pos> optional_pos;
    std::optional<image::Size> optional_size;
    bool blend = false;
    image::float_type opacity = 1;
    log::LogLevel log_level_limit = log::LogLevel::info;

    cli::Spec spec;
    pat("<source path>  <destination path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(source_path, destination_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-c, --color", "<color>", cli::no_attributes, spec,
        "Set the color to fill with. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as "
        "well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default "
        "color is @Q.",
        cli::assign(util::as_css_color(color))); // Throws

    opt("-p, --pos", "<position>", cli::no_attributes, spec,
        "Set position in pixels of upper left corner of the area to be filled relative to the upper left corner of "
        "the image. The position is specified as a pair \"<x>,<y>\". The X and Y coordinates grow towards the right "
        "and downwards respectively. The default position is 0,0.",
        cli::assign(optional_pos)); // Throws

    opt("-s, --size", "<size>", cli::no_attributes, spec,
        "Set size in pixels of area to be filled. The size can be specified either as a pair \"<width>,<height>\", or "
        "as a single number, which is then used as both width and height. The default size is the size of the image.",
        cli::assign(optional_size)); // Throws

    opt("-b, --blend", "", cli::no_attributes, spec,
        "Enable blending.",
        cli::raise_flag(blend)); // Throws

    opt("-o, --opacity", "<value>", cli::no_attributes, spec,
        "Set the opacity of the 'over' image. The default opacity is @V.",
        cli::assign(opacity)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cout(), locale);
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // Load
    std::unique_ptr<image::WritableImage> image;
    {
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        image::LoadConfig load_config;
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(source_path, image, locale, load_config, ec)) { // Throws
            logger.error("Failed to load image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    // Fill
    image::Writer writer(*image); // Throws
    writer.set_foreground_color(color); // Throws
    writer.set_blending_enabled(blend);
    writer.set_opacity(opacity);
    if (optional_pos.has_value() || optional_size.has_value()) {
        image::Pos pos = { 0, 0 };
        if (optional_pos.has_value())
            pos = optional_pos.value();
        image::Size size = image->get_size();
        if (optional_size.has_value())
            size = optional_size.value();
        image::Box area = { pos, size };
        writer.fill(area); // Throws
    }
    else {
        writer.fill(); // throws
    }

    // Save
    {
        log::PrefixLogger save_logger(logger, "Save: "); // Throws
        image::SaveConfig save_config;
        save_config.logger = &save_logger;
        std::error_code ec;
        if (!image::try_save(*image, destination_path, locale, save_config, ec)) { // Throws
            logger.error("Failed to save destination image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    logger.detail("Success"); // Throws
}
