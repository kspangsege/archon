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


#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <memory>
#include <optional>
#include <tuple>
#include <string_view>
#include <string>
#include <vector>
#include <locale>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/matrix.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/font/size.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/implementation.hpp>
#include <archon/font/freetype_implementation.hpp>
#include <archon/font/list_implementations.hpp>
#include <archon/font/list_font_faces.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    std::wstring text;
    fs::path path;
    bool list_implementations = false;
    bool list_font_faces = false;
    font::size font_size = 16;
    util::Color color = util::colors::black;
    util::Color background_color = util::colors::white;
    bool enable_colored_glyphs = false;
    std::optional<image::Size> optional_image_size;
    font::size padding = 0.5;
    font::size position = 0.5;
    bool scale_to_fit = false;
    bool nonuniform_scaling = false;
    image::float_type opacity = 1;
    bool vertical = false;
    bool reverse = false;
    bool major_reverse = false;
    bool grid_fitting = true;
    bool kerning = true;
    font::face::float_type alignment = 0;
    double rotate = 0;
    font::face::float_type line_distance = 1;
    bool half_leading = false;
    font::face::float_type tracking = 0;
    font::size resolution = 72;
    std::optional<std::string> optional_implementation;
    std::optional<fs::path> optional_font_file;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool show_line_boxes = false;
    bool show_bearing_points = false;
    font::freetype_subconfig freetype_subconfig;

    cli::WideSpec spec;
    pat("<text>  <path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(text, path)); // Throws

    pat("--list-implementations", cli::no_attributes, spec,
        "Lorem ipsum.",
        [&] {
            list_implementations = true;
        }); // Throws

    pat("--list-font-faces", cli::no_attributes, spec,
        "Lorem ipsum.",
        [&] {
            list_font_faces = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    // FIXME: What about font selection                                 

    // FIXME: What about selection of bold and italic variants?                 

    opt("-s, --font-size", "<size>", cli::no_attributes, spec,
        "Set font size in pixels. Can have one or two components (e.g. 14x16). Components can be fractional. Default "
        "is @V.",
        cli::assign(font_size)); // Throws

    opt("-c, --color", "<color>", cli::no_attributes, spec,
        "Set text color using CSS syntax with alpha-hex-form extension. Default is @Q.",
        cli::assign(util::as_css_color(color))); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set background color using CSS syntax with alpha-hex-form extension. Default is @Q.",
        cli::assign(util::as_css_color(background_color))); // Throws

    opt("-C, --enable-colored-glyphs", "", cli::no_attributes, spec,
        "Enable colored glyphs. Forces all uncolored glyphs to be rendered as black.",
        cli::raise_flag(enable_colored_glyphs)); // Throws

    opt("-S, --image-size", "<size>", cli::no_attributes, spec,
        "Set image size. Can have one or two components (e.g. 512,128). Will be inferred by default.",
        cli::assign(optional_image_size)); // Throws

    opt("-p, --padding", "<size>", cli::no_attributes, spec,
        "Amount of padding specified as fraction of EM square. Can have one or two components (e.g. 2.2x1). Default "
        "is @V.",
        cli::assign(padding)); // Throws

    opt("-P, --position", "<pos>", cli::no_attributes, spec,
        "Relative position of text in image. Zero means left/top alignment. One means right/bottom alignment. Can "
        "have one or two components (e.g. 0.5x0). Default is @V.",
        cli::assign(position)); // Throws

    opt("-F, --scale-to-fit", "", cli::no_attributes, spec,
        "Scale layout to fit image size. Only applies when font face is scalable and grid-fitting is disabled.",
        cli::raise_flag(scale_to_fit)); // Throws

    opt("-u, --nonuniform-scaling", "", cli::no_attributes, spec,
        "Allow non-uniform scaling when scaling layout to fit image size.",
        cli::raise_flag(nonuniform_scaling)); // Throws

    opt("-o, --opacity", "<value>", cli::no_attributes, spec,
        "Opacity of rendered text. Nominal range is 0 to 1. Default is @V.",
        cli::assign(opacity)); // Throws

    opt("-v, --vertical", "", cli::no_attributes, spec,
        "Switch to vertical layout (top to bottom).",
        cli::raise_flag(vertical)); // Throws

    opt("-r, --reverse", "", cli::no_attributes, spec,
        "Use right-to-left layout direction instead of left-to-right, or bottom-to-top instead of top-to-bottom.",
        cli::raise_flag(reverse)); // Throws

    opt("-E, --secondary-reverse", "", cli::no_attributes, spec,
        "Reverse order of lines (vertical reversal for horizontal layout).",
        cli::raise_flag(major_reverse)); // Throws

    opt("-g, --disable-grid-fitting", "", cli::no_attributes, spec,
        "Disable grid fitting.",
        cli::lower_flag(grid_fitting)); // Throws

    opt("-k, --disable-kerning", "", cli::no_attributes, spec,
        "Disable kerning.",
        cli::lower_flag(kerning)); // Throws

    opt("-a, --alignment", "<value>", cli::no_attributes, spec,
        "Control line box alignment. For unreversed horizontal layout, 0 means left-alignment, 1 means "
        "right-alignment, 0.5 means centered. Default is @V.",
        cli::assign(alignment)); // Throws

    opt("-R, --rotate", "<angle>", cli::no_attributes, spec,
        "Rotate rendered text. Angle is specified in degrees. Direction is counter-clockwise. Rotation only applies "
        "when font face is scalable and grid-fitting is disabled.",
        cli::assign(rotate)); // Throws

    opt("-d, --line-distance", "<value>", cli::no_attributes, spec,
        "Scaling factor for baseline separation. Default is @V.",
        cli::assign(line_distance)); // Throws

    opt("-H, --half-leading", "", cli::no_attributes, spec,
        "Use \"half-leading\" line height model (like CSS).",
        cli::raise_flag(half_leading)); // Throws

    opt("-t, --tracking", "<value>", cli::no_attributes, spec,
        "Extra glyph separation specified as a fraction of width of EM square. Can be negative. Default is @V.",
        cli::assign(tracking)); // Throws

    opt("-e, --resolution", "<value>", cli::no_attributes, spec,
        "Physical resolution assumption in pixels per inch. Can have one or two components (e.g. 96.2x95.9). Default "
        "is @V.",
        cli::assign(resolution)); // Throws

    opt("-i, --implementation", "<name>", cli::no_attributes, spec,
        "Use specified font loader implementation instead of default implementation.",
        cli::assign(optional_implementation)); // Throws

    opt("-f, --font-file", "<path>", cli::no_attributes, spec,
        "Use particular font file. Ignored unless implementation is `freetype`.",
        cli::assign(optional_font_file)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set log level limit. Possible levels are @G. Default is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-L, --show-line-box", "", cli::no_attributes, spec,
        "Show line boxes. Applies only if rotation is zero or ignored.",
        cli::raise_flag(show_line_boxes)); // Throws

    opt("-B, --show-bearing-point", "", cli::no_attributes, spec,
        "Show leading bearing points. Applies only if rotation is zero or ignored.",
        cli::raise_flag(show_bearing_points)); // Throws

    opt("--freetype-force-autohint", "", cli::no_attributes, spec,
        "When using Freetype-based implementation, use auto-hinter on TrueType fonts.",
        cli::raise_flag(freetype_subconfig.force_autohint)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    if (list_implementations) {
        font::list_implementations(core::File::get_stdout(), locale); // Throws
        return EXIT_SUCCESS;
    }

    // FIXME: Logger used internally by command line processor should probably be a STDERR logger, not a STDOUT logger                                     
    log::FileLogger root_logger(core::File::get_stderr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    const font::implementation* impl;
    if (optional_implementation.has_value()) {
        std::string_view ident = optional_implementation.value();
        impl = font::lookup_implementation(ident);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("No such font loader implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
    }
    else {
        impl = &font::get_default_implementation();
    }
    logger.info("Font implementation: %s", impl->get_ident()); // Throws

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
    logger.info("Font resource path: %s", core::as_native_path(resource_path)); // Throws

    font::loader::config loader_config;
    loader_config.logger = &logger;
    loader_config.sub.register_(freetype_subconfig); // Throws
    std::unique_ptr<font::loader> loader;
    if (optional_font_file.has_value() && impl->get_ident() == font::get_freetype_implementation().get_ident()) {
        loader = font::new_freetype_loader_from_font_file(optional_font_file.value(), locale, loader_config); // Throws
    }
    else {
        loader = impl->new_loader(resource_path, locale, loader_config); // Throws
    }

    if (list_font_faces) {
        font::list_font_faces(*loader, core::File::get_stdout(), locale); // Throws
        return EXIT_SUCCESS;
    }

    std::unique_ptr<font::face> face = loader->load_default_face(); // Throws
    logger.info("Font family: %s", face->get_family_name()); // Throws
    logger.info("Font style: %s", face->get_style_name()); // Throws
    logger.info("Font face is bold: %s", (face->is_bold() ? "Yes" : "No")); // Throws
    logger.info("Font face is italic: %s", (face->is_italic() ? "Yes" : "No")); // Throws
    logger.info("Font face is monospace: %s", (face->is_monospace() ? "Yes" : "No")); // Throws
    logger.info("Font face is scalable: %s", (face->is_scalable() ? "Yes" : "No")); // Throws
    logger.info("Font face has color: %s", (face->has_color() ? "Yes" : "No")); // Throws
    if (logger.will_log(log::LogLevel::detail)) {
        int n = face->get_num_fixed_sizes();
        if (n == 0) {
            logger.detail("No fixed sizes in font face"); // Throws
        }
        else {
            std::vector<font::size> sizes;
            for (int i = 0; i < n; ++i)
                sizes.push_back(face->get_fixed_size(i)); // Throws
            logger.detail("Fixed sizes in font face: %s", core::as_list(sizes)); // Throws
        }
    }

    face->set_resolution(resolution);
    face->set_approx_size(font_size); // Throws
    font::size font_size_2 = face->get_size();
    logger.info("Effective font rendering size: %s", font_size_2); // Throws
    face->set_color_loading_enabled(enable_colored_glyphs); // Throws

    // If grid fitting is enabled or the font face is not scalable, rotation is not honored
    // by the font renderer
    if (grid_fitting || !face->is_scalable()) {
        scale_to_fit = false;
        rotate = 0;
    }

    if (enable_colored_glyphs)
        color = util::colors::black;

    using float_type  = font::face::float_type;
    using vector_type = font::face::vector_type;
    using matrix_type = font::face::matrix_type;
    float_type ascender = face->get_ascender(grid_fitting, vertical);
    float_type descender = face->get_descender(grid_fitting, vertical);
    float_type baseline_spacing = face->get_baseline_spacing(grid_fitting, vertical);

    float_type baseline_spacing_2 = line_distance * baseline_spacing;
    if (grid_fitting)
        baseline_spacing_2 = std::round(baseline_spacing_2);

    float_type ascender_2  = ascender;
    float_type descender_2 = descender;
    if (half_leading) {
        float_type line_gap = baseline_spacing_2 - (ascender - descender);
        ascender_2 = ascender + line_gap / 2;
        if (grid_fitting)
            ascender_2 = std::round(ascender_2);
        descender_2 = ascender_2 - baseline_spacing_2;
    }

    float_type tracking_2 = tracking * (vertical ? font_size_2.height : font_size_2.width);
    if (grid_fitting)
        tracking_2 = std::round(tracking_2);

    float_type horz_padding = padding.width  * font_size_2.width;
    float_type vert_padding = padding.height * font_size_2.height;

    logger.detail("Font face ascender: %s", core::with_reverted_numerics(ascender)); // Throws
    logger.detail("Font face descender: %s", core::with_reverted_numerics(descender)); // Throws
    logger.detail("Font face baseline spacing: %s", core::with_reverted_numerics(baseline_spacing)); // Throws
    logger.detail("Effective ascender: %s", core::with_reverted_numerics(ascender_2)); // Throws
    logger.detail("Effective descender: %s", core::with_reverted_numerics(descender_2)); // Throws
    logger.detail("Effective baseline spacing: %s", core::with_reverted_numerics(baseline_spacing_2)); // Throws
    logger.detail("Tracking: %s", core::with_reverted_numerics(tracking_2)); // Throws
    logger.detail("Padding: %s", core::with_reverted_numerics(font::size(horz_padding, vert_padding))); // Throws

    // Determine text layout
    struct Glyph {
        std::size_t index;
        float_type pos; // Along baseline
    };
    struct Line {
        std::size_t glyphs_begin, glyphs_end;
        float_type pos; // Of baseline
        float_type shift; // Along baseline
        float_type length; // Along baseline
    };
    std::vector<Glyph> glyphs;
    std::vector<Line> lines;
    {
        core::WideCharMapper char_mapper(locale); // Throws
        wchar_t newline = char_mapper.widen('\n'); // Throws
        float_type line_pos = 0;
        float_type glyph_pos = 0;
        bool nonempty_line = false;
        auto close_line = [&] {
            std::size_t glyphs_begin = (lines.empty() ? 0 : lines.back().glyphs_end);
            std::size_t glyphs_end = glyphs.size();
            float_type pos = line_pos;
            float_type shift = 0;
            float_type length = glyph_pos;
            Line line = {
                glyphs_begin,
                glyphs_end,
                pos,
                shift,
                length,
            };
            lines.push_back(line); // Throws
            line_pos += baseline_spacing_2;
            glyph_pos = 0;
            nonempty_line = false;
        };
        std::size_t prev_glyph_index = 0;
        for (wchar_t ch : text) {
            if (ARCHON_UNLIKELY(ch == newline)) {
                close_line(); // Throws
                continue;
            }
            std::size_t glyph_index = face->find_glyph(ch); // Throws
            if (ARCHON_LIKELY(nonempty_line)) {
                if (kerning) {
                    std::size_t glyph_index_1 = prev_glyph_index;
                    std::size_t glyph_index_2 = glyph_index;
                    if (reverse) {
                        using std::swap;
                        std::swap(glyph_index_1, glyph_index_2);
                    }
                    glyph_pos += face->get_kerning(glyph_index_1, glyph_index_2, grid_fitting, vertical); // Throws
                }
                glyph_pos += tracking_2;
            }
            bool success = face->try_load_glyph(glyph_index, grid_fitting, vertical); // Throws
            if (ARCHON_UNLIKELY(!success)) {
                glyph_index = 0; // Use replacement glyph
                success = face->try_load_glyph(glyph_index, grid_fitting, vertical); // Throws
                ARCHON_ASSERT(success);
            }
            glyphs.push_back({ glyph_index, glyph_pos }); // Throws
            glyph_pos += face->get_glyph_advance(vertical);
            nonempty_line = true;
            prev_glyph_index = glyph_index;
        }
        if (nonempty_line)
            close_line(); // Throws
    }

    // Perform line box alignment
    {
        float_type max = 0;
        for (const Line& line : lines)
            max = std::max(max, line.length);
        for (Line& line : lines)
            line.shift = alignment * (max - line.length);
    }

    // Reveal line box structure
    if (logger.will_log(log::LogLevel::debug)) {
        std::size_t n = lines.size();
        for (std::size_t i = 0; i < n; ++i) {
            const Line& line = lines[i];
            std::size_t num_glyphs = std::size_t(line.glyphs_end - line.glyphs_begin);
            core::Span glyphs_2 = { glyphs.data() + line.glyphs_begin, num_glyphs };
            auto format_glyph_pos = [](const Glyph& g) {
                return core::with_reverted_numerics(g.pos);
            };
            logger.debug("Line %s/%s: pos = %s, shift = %s, length = %s, glyphs = %s",
                         core::as_int(i + 1), core::as_int(n), core::with_reverted_numerics(line.pos),
                         core::with_reverted_numerics(line.shift), core::with_reverted_numerics(line.length),
                         core::as_rbr_list(glyphs_2, format_glyph_pos));
        }
    }

    // Find bounding box of rotated line boxes
    vector_type x_axis = { 1, 0 }; // Towards the right
    vector_type y_axis = { 0, 1 }; // Upwards
    vector_type minor_axis = x_axis; // Parallel to the baselines
    vector_type major_axis = y_axis; // Perpendicular to the baselines
    vector_type minor_direction =  x_axis; // Primary flow direction
    vector_type major_direction = -y_axis; // Secondary flow direction
    if (vertical) {
        using std::swap;
        swap(minor_axis, major_axis);
        swap(minor_direction, major_direction);
    }
    if (reverse)
        minor_direction = -minor_direction;
    if (major_reverse)
        major_direction = -major_direction;
    matrix_type rotation = math::rot(float_type(core::deg_to_rad(rotate)));
    bool have_bbox = false;
    vector_type bbox_pos_1 = {}, bbox_pos_2 = {};
    auto include = [&](vector_type pos) {
        vector_type pos_2 = rotation * pos;
        if (ARCHON_LIKELY(have_bbox)) {
            for (int i : { 0, 1 }) {
                if (ARCHON_UNLIKELY(pos_2[i] < bbox_pos_1[i])) {
                    bbox_pos_1[i] = pos_2[i];
                }
                else if (ARCHON_UNLIKELY(pos_2[i] > bbox_pos_2[i])) {
                    bbox_pos_2[i] = pos_2[i];
                }
            }
            return;
        }
        bbox_pos_1 = pos_2;
        bbox_pos_2 = pos_2;
        have_bbox = true;
    };
    for (const Line& line : lines) {
        vector_type major_1 = line.pos * major_direction + descender_2 * major_axis;
        vector_type major_2 = major_1 + (ascender_2 - descender_2) * major_axis;
        vector_type minor_1 = line.shift * minor_direction;
        vector_type minor_2 = minor_1 + line.length * minor_direction;
        include(major_1 + minor_1);
        include(major_1 + minor_2);
        include(major_2 + minor_1);
        include(major_2 + minor_2);
    }
    ARCHON_ASSERT(have_bbox);
    bbox_pos_1[0] -= horz_padding;
    bbox_pos_1[1] -= vert_padding;
    bbox_pos_2[0] += horz_padding;
    bbox_pos_2[1] += vert_padding;
    vector_type bbox_size = bbox_pos_2 - bbox_pos_1;

    // Determine image size
    image::Size image_size = {};
    if (optional_image_size.has_value()) {
        image_size = optional_image_size.value();
    }
    else {
        float_type width  = std::max(std::ceil(bbox_size[0]), float_type(1));
        float_type height = std::max(std::ceil(bbox_size[1]), float_type(1));
        core::float_to_int(width, image_size.width); // Throws
        core::float_to_int(height, image_size.height); // Throws
    }
    logger.detail("Image size: %s", image_size); // Throws

    // Determine scaling factors
    float_type horz_scaling = 1;
    float_type vert_scaling = 1;
    if (scale_to_fit) {
        float_type horz_scaling_2 = float_type(image_size.width)  / bbox_size[0];
        float_type vert_scaling_2 = float_type(image_size.height) / bbox_size[1];
        if (!nonuniform_scaling) {
            float_type scaling = std::min(horz_scaling_2, vert_scaling_2);
            horz_scaling = scaling;
            vert_scaling = scaling;
        }
        else {
            horz_scaling = horz_scaling_2;
            vert_scaling = vert_scaling_2;
        }
    }
    logger.detail("Scaling: %s", core::with_reverted_numerics(font::size(horz_scaling, vert_scaling))); // Throws
    matrix_type scaling = matrix_type::diag({ horz_scaling, vert_scaling, });
    matrix_type transformation = scaling * rotation;

    // Position of lower-left corner of scaled bounding box relative to upper-left corner of
    // image
    vector_type scaled_bbox_pos = {
        position.width  * (float_type(image_size.width) - horz_scaling * bbox_size[0]),
        position.height * (vert_scaling * bbox_size[1] - float_type(image_size.height)) - vert_scaling * bbox_size[1],
    };

    // Position of design space origin relative to upper-left corner of image
    vector_type design_pos = scaled_bbox_pos - scaling * bbox_pos_1;

    auto design_to_clamped_rounded_image_pos = [&](vector_type pos) noexcept {
        vector_type pos_2 = design_pos + transformation * pos;
        image::Pos pos_3 = {};
        core::clamped_float_to_int(std::round(+pos_2[0]), pos_3.x);
        core::clamped_float_to_int(std::round(-pos_2[1]), pos_3.y);
        return pos_3;
    };

    // Create image
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

    if (show_line_boxes && rotate == 0) {
        for (const Line& line : lines) {
            vector_type major_1 = line.pos * major_direction + descender_2 * major_axis;
            vector_type major_2 = major_1 + (ascender_2 - descender_2) * major_axis;
            vector_type minor_1 = line.shift * minor_direction;
            vector_type minor_2 = minor_1 + line.length * minor_direction;
            if (vertical != reverse) {
                using std::swap;
                swap(minor_1, minor_2);
            }
            image::Pos pos_1 = design_to_clamped_rounded_image_pos(major_1 + minor_1); // Lower-left corner
            image::Pos pos_2 = design_to_clamped_rounded_image_pos(major_2 + minor_2); // Upper-right corner
            image::Pos pos_3 = { pos_1.x, pos_2.y }; // Upper-left corner
            image::Pos pos_4 = { pos_2.x, pos_1.y }; // Lower-right corner
            writer.set_foreground_color(util::colors::yellow); // Throws
            writer.fill(image::Box(pos_3, pos_4 - pos_3)); // Throws
        }
    }

    if (show_bearing_points && rotate == 0) {
        writer.set_foreground_color(util::colors::brown); // Throws
        auto draw_vert_line = [&](int x, int w) {
            writer.fill(image::Box({ x, 0 }, { w, image_size.height })); // Throws
        };
        auto draw_horz_line = [&](int y, int h) {
            writer.fill(image::Box({ 0, y }, { image_size.width, h })); // Throws
        };
        image::Pos pos = design_to_clamped_rounded_image_pos({ 0, 0 });
        if (!vertical) {
            int x = pos.x;
            if (!reverse)
                x -= 1;
            draw_vert_line(x, 1); // Throws
        }
        else {
            int y = pos.y;
            if (!reverse)
                y -= 1;
            draw_horz_line(y, 1); // Throws
        }
        for (const Line& line : lines) {
            vector_type origin = line.pos * major_direction + line.shift * minor_direction;
            image::Pos pos = design_to_clamped_rounded_image_pos(origin);
            if (!vertical) {
                int y = pos.y;
                if (major_reverse)
                    y -= 1;
                draw_horz_line(y, 1); // Throws
            }
            else {
                int x = pos.x;
                if (major_reverse)
                    x -= 1;
                draw_vert_line(x, 1); // Throws
            }
        }
    }

    // Render glyphs
    face->set_transform(transformation);
    writer.enable_blending(); // Throws
    writer.set_background_color(util::colors::transparent); // Throws
    writer.set_foreground_color(color); // Throws
    writer.set_opacity(opacity); // Throws
    font::face::buffer_type buffer;
    for (const Line& line : lines) {
        vector_type origin = line.pos * major_direction + line.shift * minor_direction;
        std::size_t num_glyphs = std::size_t(line.glyphs_end - line.glyphs_begin);
        for (const Glyph& glyph : core::Span(glyphs.data() + line.glyphs_begin, num_glyphs)) {
            bool success = face->try_load_glyph(glyph.index, grid_fitting, vertical); // Throws
            ARCHON_ASSERT(success); // Must succeed after earlier success
            vector_type bearing_pos = face->get_glyph_bearing(vertical);
            if (!vertical) {
                if (reverse)
                    bearing_pos[0] += face->get_glyph_advance(false);
            }
            else {
                if (!reverse)
                    bearing_pos[1] += face->get_glyph_advance(true);
            }
            vector_type pos = origin + glyph.pos * minor_direction - bearing_pos;
            face->set_translat(design_pos + transformation * pos);
            if (!face->glyph_may_be_colored()) {
                face->render_glyph_mask(writer, buffer); // Throws
            }
            else {
                face->render_glyph_rgba(writer, buffer); // Throws
            }
        }
    }

    image::save(*image, path, locale); // Throws
}
