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
#include <tuple>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <iostream>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/font/size.hpp>
#include <archon/font/loader.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    namespace fs = std::filesystem;
    std::wstring text;
    fs::path path;
    bool list_implementations = false;
    font::Size font_size = 12;
    util::Color color = util::colors::black;
    util::Color background_color = util::colors::white;
    std::optional<image::Size> optional_image_size;
    double padding = 2;
    image::float_type opacity = 1;
    bool vertical = false;
    bool grid_fitting = true;
    bool kerning = true;
    bool reverse = false;
    std::optional<std::string> optional_implementation;
    log::LogLevel log_level_limit = log::LogLevel::warn;

    cli::WideSpec spec;
    pat("<text>  <path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(text, path)); // Throws

    pat("--list-implementations", cli::no_attributes, spec,
        "Lorem ipsum.",
        [&] {
            list_implementations = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    // FIXME: What about font selection                                 

    // FIXME: What about selection of bold and italic variants?                 

    // FIXME: Maybe set padding size in units of "em", or maybe even allow the unit to be specified like CSS: "em", "px" (probably falling back to pixels if no unit is specified)             

    opt("-s, --font-size", "<size>", cli::no_attributes, spec,
        "Set the font size as close to the specified size (\"@A\") as possible. The size is specified in number of "
        "pixels, and does not have to be integer. The size can be specified either as a pair \"<width>,<height>\", or "
        "as a single number, which is then used as both width and height. The default font size is @V.",
        cli::assign(font_size)); // Throws

    opt("-c, --color", "<color>", cli::no_attributes, spec,
        "Set the text color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as well as "
        "the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default color is "
        "@Q.",
        cli::assign(util::as_css_color(color))); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set the background color. See \"--color\" for ways to specify \"@A\". The default background color is @Q. To "
        "get a transparent background, specify \"transparent\".",
        cli::assign(util::as_css_color(background_color))); // Throws

    opt("-S, --image-size", "<size>", cli::no_attributes, spec,
        "Set the image size. \"@A\" can be specified either as a pair \"<width>,<height>\", or as a single number, "
        "which is then used as both width and height. If no image size is specified, the size will be determined from "
        "the contents.",
        cli::assign(optional_image_size)); // Throws

    opt("-p, --padding", "<size>", cli::no_attributes, spec,
        "Set the amount of padding, which is the number of pixels between the generated line box and the image "
        "boundary on all sides. It can be fractional. The default amount is @V. If the image size is specified "
        "(\"--image-size\"), padding is ignored.",
        cli::assign(padding)); // Throws

    opt("-o, --opacity", "<value>", cli::no_attributes, spec,
        "Set the opacity of the rendered text to the specified value. The nominal range is 0 to 1 where 0 means fully "
        "transparent and 1 means fully opaque. The default opacity is @V. The effective opacity of the text is the "
        "value specified here multiplied by the alpha component of the specified text color (\"--color\").",
        cli::assign(opacity)); // Throws

    opt("-v, --vertical", "", cli::no_attributes, spec,
        "Switch to vertical layout (top to bottom).",
        cli::raise_flag(vertical)); // Throws

    opt("-r, --reverse", "", cli::no_attributes, spec,
        "Use right-to-left layout direction instead of left-to-right, or bottom-to-top instead of top-to-bottom if "
        "vertical layout is also selected (\"--vertical\").",
        cli::raise_flag(reverse)); // Throws

    opt("-g, --disable-grid-fitting", "", cli::no_attributes, spec,
        "Disable grid fitting.",
        cli::lower_flag(grid_fitting)); // Throws

    opt("-k, --disable-kerning", "", cli::no_attributes, spec,
        "Disable kerning.",
        cli::lower_flag(kerning)); // Throws

    opt("-i, --implementation", "<name>", cli::no_attributes, spec,
        "Use the font loader implementation identified by \"@A\". If no font loader implementation is specified, the "
        "default implementation will be used. This is the one that is listed first when using "
        "`--list-implementations`.",
        cli::assign(optional_implementation)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    if (list_implementations) {
        log::FileLogger stdout_logger(core::File::get_cout(), locale); // Throws
        int n = font::Loader::get_num_implementations();
        for (int i = 0; i < n; ++i) {
            const font::Loader::Implementation& impl = font::Loader::get_implementation(i); // Throws
            stdout_logger.info("%s", impl.ident()); // Throws
        }
        return EXIT_SUCCESS;
    }

    // FIXME: Logger used internally by command line processor should probably be a STDERR logger, not a STDOUT logger                                     
    log::FileLogger root_logger(core::File::get_cerr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    const font::Loader::Implementation* impl;
    if (optional_implementation.has_value()) {
        std::string_view ident = optional_implementation.value();
        impl = font::Loader::lookup_implementation(ident);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("No such font loader implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
    }
    else {
        impl = &font::Loader::get_default_implementation();
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
    build_env_params.bin_path  = "archon/font/demo/archon-render-text";
    build_env_params.src_path  = "archon/font/demo/render_text.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    namespace fs = std::filesystem;
    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/font", locale)); // Throws

    font::Loader::Config loader_config;
    loader_config.logger = &logger;
    std::unique_ptr<font::Loader> loader = impl->new_loader(resource_path, locale, std::move(loader_config)); // Throws
    std::unique_ptr<font::Face> face = loader->load_default_face(); // Throws
    face->set_approx_size(font_size); // Throws

    using float_type  = font::Face::float_type;
    using vector_type = font::Face::vector_type;
    vector_type length_direction  = (!vertical ? vector_type(1, 0) : vector_type(0,  1));
    vector_type breadth_direction = (!vertical ? vector_type(0, 1) : vector_type(1,  0));
    vector_type layout_direction  = (!vertical ? vector_type(1, 0) : vector_type(0, -1));
    if (reverse)
        layout_direction = -layout_direction;

    struct Glyph {
        std::size_t index;
        float_type pos;
    };

    float_type line_length;
    std::vector<Glyph> glyphs;
    {
        float_type pos = 0;
        std::size_t prev_glyph_index = 0;
        for (wchar_t ch : text) {
            std::size_t glyph_index = face->find_glyph(ch); // Throws
            if (kerning) {
                std::size_t glyph_index_1 = prev_glyph_index;
                std::size_t glyph_index_2 = glyph_index;
                if (reverse)
                    std::swap(glyph_index_1, glyph_index_2);
                pos += face->get_kerning(glyph_index_1, glyph_index_2, vertical, grid_fitting); // Throws
            }
            glyphs.push_back({ glyph_index, pos }); // Throws
            face->load_glyph(glyph_index, grid_fitting); // Throws
            pos += face->get_glyph_advance(vertical);
            prev_glyph_index = glyph_index;
        }
        line_length = pos;
    }

    float_type baseline_spacing = face->get_baseline_spacing(vertical, grid_fitting);
    float_type baseline_offset = face->get_baseline_offset(vertical, grid_fitting);

    vector_type line_box_size = (line_length * length_direction + baseline_spacing * breadth_direction);

    image::Size image_size = {
        int(std::ceil(line_box_size[0] + 2 * padding)),
        int(std::ceil(line_box_size[1] + 2 * padding)),
    };
    if (optional_image_size.has_value())
        image_size = optional_image_size.value();
    image::Pos target_pos = { 0, image_size.height };
    face->set_target_pos(target_pos);

    vector_type line_box_pos = {
        (float_type(image_size.width)  - line_box_size[0]) / 2,
        (float_type(image_size.height) - line_box_size[1]) / 2,
    };
    if (grid_fitting) {
        line_box_pos = vector_type {
            std::round(line_box_pos[0]),
            std::round(line_box_pos[1]),
        };
    }

    vector_type cursor_start_pos = (line_box_pos + int(vertical != reverse) * line_length * length_direction +
                                    baseline_offset * breadth_direction);

    std::unique_ptr<image::WritableImage> image;
    if (background_color.is_opaque()) {
        image = std::make_unique<image::BufferedImage_RGB_8>(image_size); // Throws
    }
    else {
        image = std::make_unique<image::BufferedImage_RGBA_8>(image_size); // Throws
    }
    image::Writer writer(*image); // Throws

    writer.set_background_color(background_color); // Throws
    writer.fill(image::Writer::ColorSlot::background); // Throws

/*
    if (true) {
        image::Pos pos_1 = {
            target_pos.x + int(std::round(line_box_pos[0])),
            target_pos.y - int(std::round(line_box_pos[1] + line_box_size[1])),
        };
        image::Pos pos_2 = {
            target_pos.x + int(std::round(line_box_pos[0] + line_box_size[0])),
            target_pos.y - int(std::round(line_box_pos[1])),
        };
        writer.set_foreground_color(util::colors::yellow); // Throws
        writer.fill(image::Box(pos_1, pos_2 - pos_1)); // Throws
    }
*/

    bool overlapping_glyphs = true; // false;                                                                                          
    if (overlapping_glyphs) {
        writer.enable_blending(); // Throws
        writer.set_background_color(util::colors::transparent); // Throws
    }

    writer.set_foreground_color(color, opacity); // Throws
    std::size_t n = text.size();
    for (std::size_t i = 0; i < n; ++i) {
        Glyph glyph = glyphs[i];
        face->load_glyph(glyph.index, grid_fitting); // Throws
        vector_type bearing = face->get_glyph_bearing(vertical);
        if (!vertical) {
            if (reverse)
                bearing[0] += face->get_glyph_advance(false);
        }
        else {
            if (!reverse)
                bearing[1] += face->get_glyph_advance(true);
        }
        vector_type cursor_pos = cursor_start_pos + glyph.pos * layout_direction;
        face->translate_glyph(cursor_pos - bearing);
        face->render_glyph_mask(writer); // Throws
    }

    image::save(*image, path, locale); // Throws
}
