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
#include <utility>
#include <memory>
#include <optional>
#include <tuple>
#include <locale>
#include <system_error>
#include <filesystem>
#include <iostream>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/image/palettes.hpp>
#include <archon/image/file_format_png.hpp>


using namespace archon;


namespace {


enum class Palette {
    bw,    // Black and white
    gray4, // Grayscale with 4 tones
    css16, // 16 basic colors of CSS
};

struct PaletteSpec {
    static constexpr core::EnumAssoc map[] = {
        { int(Palette::bw),    "bw"    },
        { int(Palette::gray4), "gray4" },
        { int(Palette::css16), "css16" },
    };
};
using PaletteEnum = core::Enum<Palette, PaletteSpec>;


class ProgressTracker
    : public image::ProgressTracker {
public:
    bool is_save = false;

    ProgressTracker(log::Logger& logger) noexcept
        : m_logger(logger)
    {
    }

    void progress(const image::Image&, int units_completed, int units_total) override final
    {
        double fraction = double(units_completed) / units_total;
        if (!is_save) {
            m_logger.info("Load progress: %s", core::as_percent(fraction));
        }
        else {
            m_logger.info("Save progress: %s", core::as_percent(fraction));
        }
    }

private:
    log::Logger& m_logger;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    fs::path source_path, destination_path;
    bool list = false;
    std::optional<PaletteEnum> palette;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool progress = false;
    bool interlace = false;
    image::LoadConfig load_config;
    image::SaveConfig save_config;

    cli::Spec spec;
    pat("<source path>  <destination path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(source_path, destination_path)); // Throws
    pat("--list", cli::no_attributes, spec,
        "Lorem ipsum.",
        [&] {
            list = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-p, --palette", "[<name>]", cli::no_attributes, spec,
        "Transform the image into a form that uses an indirect color pixel format. The palette name, if specified, "
        "can be any of \"bw\" (black and white), \"gray4\" (grayscale with 4 tones), and \"css16\" (16 basic colors "
        "of CSS). The default palette is @R.",
        cli::assign(palette, Palette::css16)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-P, --progress", "", cli::no_attributes, spec,
        "Report loading progress.",
        cli::raise_flag(progress)); // Throws

    opt("-i, --interlace", "", cli::no_attributes, spec,
        "Turn on Adam7 interlacing when producing a PNG file.",
        cli::raise_flag(interlace)); // Throws

    opt("-r, --read-buffer-size", "<size>", cli::no_attributes, spec,
        "Set the size of the read buffer used when loading the specified image. The default size "
        "is @V.",
        cli::assign(core::as_int(load_config.read_buffer_size))); // Throws

    opt("-w, --write-buffer-size", "<size>", cli::no_attributes, spec,
        "Set the size of the write buffer used when saving the converted image. The default size is @V.",
        cli::assign(core::as_int(save_config.write_buffer_size))); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    const image::FileFormatRegistry& registry = image::FileFormatRegistry::get_default_registry(); // Throws

    if (list) {
        int n = registry.get_num_file_formats();
        for (int i = 0; i < n; ++i) {
            const image::FileFormat& format = registry.get_file_format(i); // Throws
            std::cout << format.get_ident() << " " << format.get_descr() << "\n"; // Throws
        }
        return EXIT_SUCCESS;
    }

    log::FileLogger root_logger(core::File::get_cout(), locale);
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    ProgressTracker progress_tracker(root_logger);

    std::unique_ptr<image::WritableImage> image;
    std::error_code ec;

    // Load
    {
        log::PrefixLogger load_logger(logger, "Load: "); // Throws

        if (progress) {
            progress_tracker.is_save = false;
            load_config.tracker = &progress_tracker;
        }
        load_config.logger = &load_logger;

        if (!image::try_load(source_path, image, locale, load_config, ec)) { // Throws
            logger.error("Failed to load source image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    // Transform
    if (palette.has_value()) {
        const image::Image* palette_image = &image::get_bw_palette(); // Throws
        switch (palette.value()) {
            case Palette::bw:
                break;
            case Palette::gray4:
                palette_image = &image::get_gray4_palette(); // Throws
                break;
            case Palette::css16:
                palette_image = &image::get_css16_palette(); // Throws
                break;
        }
        image::Size size = image->get_size();
        using format_type = image::IndexedPixelFormat_8<>;
        format_type format(*palette_image);
        using image_type = image::BufferedImage<format_type>;
        auto image_2 = std::make_unique<image_type>(size, std::move(format)); // Throws
        image_2->put_image({ 0, 0 }, *image); // Throws
        image = std::move(image_2);
    }

    // Save
    {
        image::PNGSaveConfig png_save_config;
        png_save_config.use_adam7_interlacing = interlace;

        image::FileFormat::SpecialSaveConfigRegistry special_save_config_registry; // Throws
        special_save_config_registry.register_(png_save_config); // Throws

        log::PrefixLogger save_logger(logger, "Save: "); // Throws

        if (progress) {
            progress_tracker.is_save = true;
            save_config.tracker = &progress_tracker;
        }
        save_config.special = &special_save_config_registry;
        save_config.logger = &save_logger;

        if (!image::try_save(*image, destination_path, locale, save_config, ec)) { // Throws
            logger.error("Failed to save destination image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

    logger.info("Image successfully converted"); // Throws
}
