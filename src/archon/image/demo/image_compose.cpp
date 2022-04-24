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
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    namespace fs = std::filesystem;
    fs::path over_path, under_path, destination_path;
    image::Pos pos = { 0, 0 };
    std::optional<image::Size> optional_size;
    image::Pos shift = { 0, 0 };
    image::Reader::FalloffMode horz_falloff_mode = image::Reader::FalloffMode::background;
    image::Reader::FalloffMode vert_falloff_mode = image::Reader::FalloffMode::background;
    bool blend = false;
    image::float_type opacity = 1;
    log::LogLevel log_level_limit = log::LogLevel::info;

    cli::Spec spec;
    pat("<over path>  <under path>  <destination path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(over_path, under_path, destination_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-p, --pos", "<position>", cli::no_attributes, spec,
        "Set position in pixels of upper left corner of 'over' image relative to upper left corner of 'under' image. "
        "The position is specified as a pair \"<x>,<y>\". The X and Y coordinates grow towards the right and "
        "downwards respectively. The default position is @V.",
        cli::assign(pos)); // Throws

    opt("-s, --size", "<size>", cli::no_attributes, spec,
        "Set size in pixels of block of extracted pixels from 'over' image. The size can be specified either as a "
        "pair \"<width>,<height>\", or as a single number, which is then used as both width and height. The default "
        "size is the size of the 'over' image.",
        cli::assign(optional_size)); // Throws

    opt("-S, --shift", "<position>", cli::no_attributes, spec,
        "Set position in pixels of upper left corner of block of extracted pixels relative to upper left corner of "
        "'over' image. The position is specified as a pair \"<x>,<y>\". The X and Y coordinates grow towards the "
        "right and downwards respectively. The default shift is @V.",
        cli::assign(shift)); // Throws

    opt("-f, --falloff", "<mode>", cli::no_attributes, spec,
        "Set horizontal and vertical falloff modes when reading from the 'over' image. This has the same effect as "
        "setting both modes individually using \"--horz-falloff\" and \"--vert-falloff\".",
        [&](image::Reader::FalloffMode mode) {
            horz_falloff_mode = mode;
            vert_falloff_mode = mode;
        });

    opt("-H, --horz-falloff", "<mode>", cli::no_attributes, spec,
        "Set the horizontal falloff mode to apply when reading from the 'over' image. \"@A\" can be \"background\", "
        "\"edge\", or \"repeat\". The default horizontal mode is @Q.",
        cli::assign(horz_falloff_mode));

    opt("-V, --vert-falloff", "<mode>", cli::no_attributes, spec,
        "Set the vertical falloff mode to apply when reading from the 'over' image. \"@A\" can be \"background\", "
        "\"edge\", or \"repeat\". The default vertical mode is @Q.",
        cli::assign(vert_falloff_mode));

    opt("-b, --blend", "", cli::no_attributes, spec,
        "Enable blending.",
        cli::raise_flag(blend)); // Throws

    opt("-o, --opacity", "<value>", cli::no_attributes, spec,
        "Set the opacity of the 'over' image. The default opacity is @V.",
        cli::assign(opacity)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are \"off\", \"fatal\", \"error\", \"warn\", \"info\", "
        "\"detail\", \"debug\", \"trace\", and \"all\". The default limit is \"@V\".",
        cli::assign(log_level_limit)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cout(), locale);
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // Load
    std::unique_ptr<image::WritableImage> image_1, image_2;
    {
        log::PrefixLogger load_logger(logger, "Load 'over' image: "); // Throws
        image::LoadConfig load_config;
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(over_path, image_1, locale, load_config, ec)) { // Throws
            logger.error("Failed to load 'over' image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }
    {
        log::PrefixLogger load_logger(logger, "Load 'under' image: "); // Throws
        image::LoadConfig load_config;
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(under_path, image_2, locale, load_config, ec)) { // Throws
            logger.error("Failed to load 'under' image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    // Compose
    image::Reader reader(*image_1);
    reader.set_falloff_mode(horz_falloff_mode, vert_falloff_mode);
    image::Writer writer(*image_2); // Throws
    writer.set_blending_enabled(blend);
    writer.set_opacity(opacity);
    image::Size size = (optional_size ? optional_size.value() : image_1->get_size());
    image::Box box = { shift, size };
    writer.put_image_a(pos, reader, box);

    // Save
    {
        log::PrefixLogger save_logger(logger, "Save image: "); // Throws
        image::SaveConfig save_config;
        save_config.logger = &save_logger;
        std::error_code ec;
        if (!image::try_save(*image_2, destination_path, locale, save_config, ec)) { // Throws
            logger.error("Failed to save destination image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    logger.detail("Success"); // Throws
}
