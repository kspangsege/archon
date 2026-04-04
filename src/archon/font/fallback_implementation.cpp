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
#include <utility>
#include <algorithm>
#include <memory>
#include <array>
#include <tuple>
#include <optional>
#include <string_view>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <system_error>
#include <locale>
#include <filesystem>
#include <ios>

#include <archon/core/features.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/string.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/core/buffered_text_file.hpp>
#include <archon/core/text_parser.hpp>
#include <archon/log.hpp>
#include <archon/util/rectangle_packer.hpp>
#include <archon/image.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/implementation.hpp>
#include <archon/font/fallback_implementation.hpp>


using namespace archon;


namespace {


// The fallback font is specified by an image file (`fallback-font.png`) and a spec file
// (`fallback-font.txt`). The image file contains all the rasterized glyphs packed closely
// together. The spec file is a text file and specifies glyph metrics (one glyph per line),
// including the positions of the glyphs in the image.
//
// NOTE: Glyphs are not stored conventionally in the image. The PNG image has one channel of
// 8-bit components. While the PNG format believes these are gamma-compressed luminance
// values, they are in fact linear coverage values making up an alpha mask (as produced by
// font::face::render_glyph_mask_a()). This is in order to best preserve the linear alpha
// values while only using one channel. The obvious alternative would have been to use a
// two-channel image with an alpha channel, and then let the alpha channel fully specify the
// glyph, but this would have been wasteful.
//
// The fact that a linear alpha channel masquerades as gamma-compressed luminance channel
// means that if the image file is opened in an image viewer, the anti-aliased borders of
// the glyphs will appear different and gamma-incorrect compared to a proper rendering of
// the glyph.
//
constexpr std::string_view spec_file_name  = "fallback-font.txt";
constexpr std::string_view image_file_name = "fallback-font.png";


auto generate_file_path(core::FilesystemPathRef resource_dir, std::string_view file_name, const std::locale& loc,
                        std::string_view qual = "") -> std::filesystem::path
{
    std::string file_name_2 = std::string(file_name); // Throws
    auto i = file_name_2.rfind('.');
    if (ARCHON_UNLIKELY(i == std::string::npos))
        i = file_name_2.size();
    file_name_2.insert(i, qual); // Throws
    namespace fs = std::filesystem;
    fs::path path = core::make_fs_path_generic(file_name_2, loc); // Throws
    return resource_dir / path; // Throws
}


using char_type = font::code_point::char_type;


struct glyph {
    // Position and size of glyph in image.
    image::Box box;

    // Position in design space of lower-left corner of glyphs bounding box.
    int pos_x, pos_y;

    // Position in design space of the bearing point for horizontal layout.
    int horz_bearing_x, horz_bearing_y;

    // Position in design space of the bearing point for vertical layout.
    int vert_bearing_x, vert_bearing_y;

    // The glyph advance for horizontal and vertical layouts respectively. Neither can be
    // negative.
    int horz_advance, vert_advance;
};


struct spec {
    std::string family_name;
    std::string style_name;
    std::vector<font::code_point_range> code_point_ranges;
    image::Size image_size;
    bool bold, italic, monospace;
    font::size render_size;
    int horz_ascender, horz_descender, horz_baseline_spacing;
    int vert_ascender, vert_descender, vert_baseline_spacing;
    core::Slab<::glyph> glyphs; // First glyph is fallback glyph
    std::map<char_type, std::size_t> glyph_map;
};


struct fallback_font {
    std::unique_ptr<image::Image> image;
    ::spec spec;
};


bool load_spec(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc, ::spec& spec)
{
    namespace fs = std::filesystem;
    fs::path path = generate_file_path(resource_dir, ::spec_file_name, loc); // Throws
    core::BufferedTextFile file(path, loc); // Throws
    std::array<char, 96> seed_memory;
    core::Buffer<char> buffer(seed_memory);
    std::string_view line, line_2;
    std::locale loc_2(loc, std::locale::classic(), std::locale::numeric); // Throws
    core::ValueParser value_parser(loc_2); // Throws
    core::TextParser text_parser(value_parser); // Throws
    core::CharMapper char_mapper(loc); // Throws
    char delim = char_mapper.widen(' '); // Throws
    char hash = char_mapper.widen('#'); // Throws
    std::vector<::glyph> glyphs;
    std::vector<font::code_point> code_points;
    bool have_family_name = false;
    bool have_style_name = false;
    bool have_code_point_ranges = false;
    bool have_global_metrics = false;
    bool have_fallback_glyph = false;
    long line_num = 0;
    bool have_error = false;
    auto error_1 = [&](const char* msg, const auto&... params) {
        logger.error("%s:%s: %s", core::as_native_path(path), core::as_int(line_num),
                     core::formatted(msg, params...)); // Throws
    };
    auto error_2 = [&](std::size_t pos, const char* msg, const auto&... params) {
        logger.error("%s:%s:%s: %s", core::as_native_path(path), core::as_int(line_num),
                     core::as_int(pos), core::formatted(msg, params...)); // Throws
    };
    while (ARCHON_LIKELY(file.read_line(buffer, line))) { // Throws
        ++line_num;
        line = line.substr(0, line.find(hash));
        line_2 = core::trim_a(line, delim);
        if (line_2.empty())
            continue;
        if (ARCHON_LIKELY(have_global_metrics))
            goto glyph_spec;
        if (have_code_point_ranges)
            goto global_metrics_spec;
        if (have_style_name)
            goto code_point_ranges;
        if (have_family_name)
            goto style_name;
        spec.family_name = std::string(line_2); // Throws
        have_family_name = true;
        continue;
      style_name:
        spec.style_name = std::string(line_2); // Throws
        have_style_name = true;
        continue;
      code_point_ranges:
        have_code_point_ranges = true;
        {
            using namespace std::literals;
            std::tuple fields = {};
            auto field_seq = text_parser.field_seq(spec.code_point_ranges, "code point ranges"sv);
            std::size_t min = 1, max = std::size_t(-1);
            core::TextParser::Error error = {};
            std::string_view value;
            std::string_view label;
            std::size_t pos = 0;
            bool success = text_parser.parse(line, delim, fields, field_seq, min, max, error, value, label,
                                             pos); // Throws
            if (ARCHON_LIKELY(success)) {
                std::optional<font::code_point> prev_last;
                for (font::code_point_range range : spec.code_point_ranges) {
                    if (ARCHON_LIKELY(!prev_last.has_value() || range.first().to_int() > prev_last.value().to_int())) {
                        prev_last = range.last();
                        continue;
                    }
                    error_1("Overlapping code point ranges"); // Throws
                    goto error;
                }
                continue;
            }
            switch (error) {
                case core::TextParser::Error::missing_value:
                    error_2(pos, "Missing code point range"); // Throws
                    break;
                case core::TextParser::Error::bad_value:
                    error_2(pos, "Bad code point range %s", core::quoted(value)); // Throws
                    break;
                case core::TextParser::Error::too_many_values:
                    error_2(pos, "Too many code point ranges"); // Throws
                    break;
            }
            goto error;
        }
      global_metrics_spec:
        have_global_metrics = true;
        {
            using namespace std::literals;
            std::tuple fields = {
                text_parser.field(core::as_int(spec.image_size.width),      "image width"sv),
                text_parser.field(core::as_int(spec.image_size.height),     "image height"sv),
                text_parser.field(core::as_int(spec.bold),                  "bold"sv),
                text_parser.field(core::as_int(spec.italic),                "italic"sv),
                text_parser.field(core::as_int(spec.monospace),             "monospace"sv),
                text_parser.field(spec.render_size.width,                   "render width"sv),
                text_parser.field(spec.render_size.height,                  "render height"sv),
                text_parser.field(core::as_int(spec.horz_ascender),         "horizontal ascender"sv),
                text_parser.field(core::as_int(spec.horz_descender),        "horizontal descender"sv),
                text_parser.field(core::as_int(spec.horz_baseline_spacing), "horizontal baseline spacing"sv),
                text_parser.field(core::as_int(spec.vert_ascender),         "vertical ascender"sv),
                text_parser.field(core::as_int(spec.vert_descender),        "vertical descender"sv),
                text_parser.field(core::as_int(spec.vert_baseline_spacing), "vertical baseline spacing"sv),
            };
            core::TextParser::Error error = {};
            std::string_view value;
            std::string_view label;
            std::size_t pos = 0;
            bool success = text_parser.parse(line, delim, fields, error, value, label, pos); // Throws
            if (ARCHON_LIKELY(success))
                continue;
            switch (error) {
                case core::TextParser::Error::missing_value:
                    error_2(pos, "Missing value for field '%s' in font metrics specification", label); // Throws
                    break;
                case core::TextParser::Error::bad_value:
                    error_2(pos, "Bad value %s for field '%s' in font metrics specification",
                            core::quoted(value), label); // Throws
                    break;
                case core::TextParser::Error::too_many_values:
                    error_2(pos, "Too many values in font metrics specification"); // Throws
                    break;
            }
            goto error;
        }
      glyph_spec:
        {
            ::glyph glyph;
            code_points.clear();
            using namespace std::literals;
            std::tuple fields = {
                text_parser.field(core::as_int(glyph.box.pos.x),       "image position x"sv),
                text_parser.field(core::as_int(glyph.box.pos.y),       "image position y"sv),
                text_parser.field(core::as_int(glyph.box.size.width),  "width"sv),
                text_parser.field(core::as_int(glyph.box.size.height), "height"sv),
                text_parser.field(core::as_int(glyph.horz_bearing_x),  "horizontal bearing x"sv),
                text_parser.field(core::as_int(glyph.horz_bearing_y),  "horizontal bearing y"sv),
                text_parser.field(core::as_int(glyph.vert_bearing_x),  "vertical bearing x"sv),
                text_parser.field(core::as_int(glyph.vert_bearing_y),  "vertical bearing y"sv),
                text_parser.field(core::as_int(glyph.horz_advance),    "horizontal advance"sv),
                text_parser.field(core::as_int(glyph.vert_advance),    "vertical advance"sv),
                text_parser.field(core::as_int(glyph.pos_x),           "position x"sv),
                text_parser.field(core::as_int(glyph.pos_y),           "position y"sv),
            };
            auto field_seq = text_parser.field_seq(code_points, "code point"sv);
            std::size_t min = 0, max = 0;
            bool is_fallback_glyph = !have_fallback_glyph;
            if (ARCHON_LIKELY(!is_fallback_glyph)) {
                min = 1;
                max = std::size_t(-1);
            }
            else {
                have_fallback_glyph = true;
            }
            core::TextParser::Error error = {};
            std::string_view value;
            std::string_view label;
            std::size_t pos = 0;
            bool success = text_parser.parse(line, delim, fields, field_seq, min, max, error, value, label,
                                             pos); // Throws
            if (ARCHON_LIKELY(success)) {
                if (ARCHON_UNLIKELY(!glyph.box.contained_in(spec.image_size))) {
                    error_1("Glyph box (%s; %s) escapes image boundary (%s)", glyph.box.pos, glyph.box.size,
                            spec.image_size); // Throws
                    goto error;
                }
                if (ARCHON_UNLIKELY(glyph.horz_advance < 0 || glyph.vert_advance < 0)) {
                    error_1("Negative glyph advance (horizontal %s, vertical %s)", glyph.horz_advance,
                            glyph.vert_advance); // Throws
                    goto error;
                }
                std::size_t glyph_index = glyphs.size();
                glyphs.push_back(glyph); // Throws
                for (font::code_point cp : code_points) {
                    char_type ch = cp.to_char();
                    auto p = spec.glyph_map.emplace(ch, glyph_index); // Throws
                    bool was_inserted = p.second;
                    if (ARCHON_LIKELY(was_inserted))
                        continue;
                    error_2(pos, "Multiple glyphs for code point %s", cp); // Throws
                    goto error;
                }
                continue;
            }
            std::string_view qual = (is_fallback_glyph ? "fallback glyph" : "glyph");
            switch (error) {
                case core::TextParser::Error::missing_value:
                    error_2(pos, "Missing value for field '%s' in %s specification", label, qual); // Throws
                    break;
                case core::TextParser::Error::bad_value:
                    error_2(pos, "Bad value %s for field '%s' in %s specification", core::quoted(value), label,
                            qual); // Throws
                    break;
                case core::TextParser::Error::too_many_values:
                    error_2(pos, "Too many values in %s specification", qual); // Throws
                    break;
            }
            goto error;
        }
      error:
        have_error = true;
    }
    if (ARCHON_UNLIKELY(have_error))
        return false;
    if (ARCHON_UNLIKELY(!have_family_name)) {
        ++line_num;
        error_1("Missing family name"); // Throws
        return false;
    }
    if (ARCHON_UNLIKELY(!have_style_name)) {
        ++line_num;
        error_1("Missing style name"); // Throws
        return false;
    }
    if (ARCHON_UNLIKELY(!have_code_point_ranges)) {
        ++line_num;
        error_1("Missing code point ranges"); // Throws
        return false;
    }
    if (ARCHON_UNLIKELY(!have_global_metrics)) {
        ++line_num;
        error_1("Missing font metrics"); // Throws
        return false;
    }
    if (ARCHON_UNLIKELY(glyphs.size() < 1)) {
        ++line_num;
        error_1("Missing replacement glyph"); // Throws
        return false;
    }
    spec.glyphs = core::Slab<::glyph>(core::Span(glyphs)); // Throws
    return true;
}


bool load_image(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc,
                image::Size expected_image_size, std::unique_ptr<image::Image>& image)
{
    namespace fs = std::filesystem;
    fs::path path = generate_file_path(resource_dir, ::image_file_name, loc); // Throws
    log::PrefixLogger load_logger(logger, "Load image with glyphs of fallback font: "); // Throws
    image::LoadConfig config;
    config.logger = &load_logger;
    std::unique_ptr<image::WritableImage> image_2;
    std::error_code ec;
    if (ARCHON_LIKELY(image::try_load(path, image_2, loc, config, ec))) { // Throws
        if (ARCHON_LIKELY(image_2->get_size() == expected_image_size)) {
            image = std::move(image_2);
            return true;
        }
        load_logger.error("Image size mismatch (was %s, expected %s)", image_2->get_size(),
                          expected_image_size); // Throws
        return false;
    }
    load_logger.error("%s", ec.message()); // Throws
    return false;
}


bool load_font(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc, ::fallback_font& font)
{
    if (ARCHON_LIKELY(::load_spec(resource_dir, logger, loc, font.spec))) // Throws
        return load_image(resource_dir, logger, loc, font.spec.image_size, font.image); // Throws
    return false;
}


auto get_logger(const std::locale& loc, const font::loader::config& config,
                std::unique_ptr<log::FileLogger>& file_logger) -> log::Logger&
{
    if (config.logger)
        return *config.logger;
    file_logger = std::make_unique<log::FileLogger>(core::File::get_stdout(), loc); // Throws
    return *file_logger;
}



class face_impl final
    : public font::face {
public:
    face_impl(const ::fallback_font& font) noexcept;

    auto get_family_name() noexcept -> std::string_view override;
    auto get_style_name() noexcept -> std::string_view override;
    bool is_bold() noexcept override;
    bool is_italic() noexcept override;
    bool is_monospace() noexcept override;
    bool is_scalable() noexcept override;
    bool has_color() noexcept override;
    void set_resolution(font::size) noexcept override;
    int get_num_fixed_sizes() noexcept override;
    auto get_fixed_size(int) -> font::size override;
    void set_fixed_size(int) override;
    void set_scaled_size(font::size) override;
    void set_approx_size(font::size) override;
    auto get_size() noexcept -> font::size override;
    auto get_ascender(bool, bool) noexcept -> float_type override;
    auto get_descender(bool, bool) noexcept -> float_type override;
    auto get_baseline_spacing(bool, bool) noexcept -> float_type override;
    void set_color_loading_enabled(bool) override;
    auto find_glyph(char_type) -> std::size_t override;
    auto get_kerning(std::size_t, std::size_t, bool, bool) -> float_type override;
    [[nodiscard]] bool try_load_glyph(std::size_t, bool, bool) override;
    auto get_glyph_advance(bool) noexcept -> float_type override;
    auto get_glyph_bearing(bool) noexcept -> vector_type override;
    auto get_glyph_pos() noexcept -> vector_type override;
    auto get_glyph_size() noexcept -> vector_type override;
    bool glyph_may_be_colored() noexcept override;
    void set_transform(const matrix_type&) noexcept override;
    void set_translat(vector_type) noexcept override;
    void reset_transform_translat() noexcept override;
    void set_target_pos(image::Pos) noexcept override;
    auto get_target_glyph_box() -> image::Box override;
    void render_glyph_mask_a(image::Pos, const tray_type&) override;
    void render_glyph_rgba_a(image::Pos, const tray_type&) override;

private:
    const ::fallback_font& m_font;
    image::Reader m_image_reader;
    const ::glyph* m_glyph = nullptr;
    vector_type m_translation;
    image::Pos m_target_pos;

    auto do_get_target_glyph_box() -> image::Box;
};



class loader_impl final
    : public font::loader {
public:
    loader_impl(core::FilesystemPathRef resource_dir, const std::locale& loc, log::Logger* logger);

    auto load_default_face() -> std::unique_ptr<font::face> override;
    int get_num_faces() override;
    auto load_face(int) -> std::unique_ptr<font::face> override;

private:
    const std::filesystem::path m_resource_dir;
    const std::locale m_locale;
    log::Logger& m_logger;

    std::unique_ptr<::fallback_font> m_font;

    auto do_load_face() -> std::unique_ptr<font::face>;
    auto ensure_font() -> const ::fallback_font&;
    auto load_font() const -> std::unique_ptr<::fallback_font>;
};



class implementation_impl final
    : public font::implementation {
public:
    auto get_ident() const noexcept -> std::string_view override;
    auto get_descr() const noexcept -> std::string_view override;
    bool is_available() const noexcept override;
    auto new_loader(core::FilesystemPathRef, const std::locale&, const font::loader::config&) const ->
        std::unique_ptr<font::loader> override;
};

// Making this `constexpr` triggers a bug in GCC 12 and below
constinit ::implementation_impl g_implementation;



face_impl::face_impl(const ::fallback_font& font) noexcept
    : m_font(font)
    , m_image_reader(*font.image) // Throws
{
    ARCHON_ASSERT(m_font.spec.glyphs.size() > 0);
    m_glyph = &m_font.spec.glyphs[0]; // Replacement glyph
}


auto face_impl::get_family_name() noexcept -> std::string_view
{
    return m_font.spec.family_name;
}


auto face_impl::get_style_name() noexcept -> std::string_view
{
    return m_font.spec.style_name;
}


bool face_impl::is_bold() noexcept
{
    return m_font.spec.bold;
}


bool face_impl::is_italic() noexcept
{
    return m_font.spec.italic;
}


bool face_impl::is_monospace() noexcept
{
    return m_font.spec.monospace;
}


bool face_impl::is_scalable() noexcept
{
    return false;
}


bool face_impl::has_color() noexcept
{
    return false;
}


void face_impl::set_resolution(font::size) noexcept
{
    // No-op since this as a non-scalable font.
}


int face_impl::get_num_fixed_sizes() noexcept
{
    return 1;
}


auto face_impl::get_fixed_size(int fixed_size_index) -> font::size
{
    if (ARCHON_LIKELY(fixed_size_index == 0))
        return m_font.spec.render_size;
    throw std::out_of_range("Fixed size index");
}


void face_impl::set_fixed_size(int fixed_size_index)
{
    if (ARCHON_LIKELY(fixed_size_index == 0))
        return;
    throw std::out_of_range("Fixed size index");
}


void face_impl::set_scaled_size(font::size)
{
    throw std::logic_error("Font face is not scalable");
}


void face_impl::set_approx_size(font::size)
{
    // No-op since there is only one size in the first place
}


auto face_impl::get_size() noexcept -> font::size
{
    return m_font.spec.render_size;
}


auto face_impl::get_ascender(bool, bool vertical) noexcept -> float_type
{
    const ::spec& spec = m_font.spec;
    return (vertical ? float_type(spec.vert_ascender) : float_type(spec.horz_ascender));
}


auto face_impl::get_descender(bool, bool vertical) noexcept -> float_type
{
    const ::spec& spec = m_font.spec;
    return (vertical ? float_type(spec.vert_descender) : float_type(spec.horz_descender));
}


auto face_impl::get_baseline_spacing(bool, bool vertical) noexcept -> float_type
{
    const ::spec& spec = m_font.spec;
    return (vertical ? float_type(spec.vert_baseline_spacing) : float_type(spec.horz_baseline_spacing));
}


void face_impl::set_color_loading_enabled(bool)
{
    // No-op since implementation has no support for color
}


auto face_impl::find_glyph(char_type ch) -> std::size_t
{
    auto i = m_font.spec.glyph_map.find(ch); // Throws
    if (ARCHON_LIKELY(i != m_font.spec.glyph_map.end()))
        return i->second;
    return 0;
}


auto face_impl::get_kerning(std::size_t, std::size_t, bool, bool) -> float_type
{
    return 0;
}


bool face_impl::try_load_glyph(std::size_t glyph_index, bool, bool)
{
    if (ARCHON_LIKELY(glyph_index < m_font.spec.glyphs.size())) {
        m_glyph = &m_font.spec.glyphs[glyph_index];
        return true;
    }
    throw std::out_of_range("glyph index");
}


auto face_impl::get_glyph_advance(bool vertical) noexcept -> float_type
{
    if (ARCHON_LIKELY(!vertical))
        return float_type(m_glyph->horz_advance);
    return float_type(m_glyph->vert_advance);
}


auto face_impl::get_glyph_bearing(bool vertical) noexcept -> vector_type
{
    if (ARCHON_LIKELY(!vertical))
        return { float_type(m_glyph->horz_bearing_x), float_type(m_glyph->horz_bearing_y) };
    return { float_type(m_glyph->vert_bearing_x), float_type(m_glyph->vert_bearing_y) };
}


auto face_impl::get_glyph_pos() noexcept -> vector_type
{
    return { float_type(m_glyph->pos_x), float_type(m_glyph->pos_y) };
}


auto face_impl::get_glyph_size() noexcept -> vector_type
{
    return { float_type(m_glyph->box.size.width), float_type(m_glyph->box.size.height) };
}


bool face_impl::glyph_may_be_colored() noexcept
{
    return false;
}


void face_impl::set_transform(const matrix_type&) noexcept
{
    // Any configured glyph transformation is supposed to be ignored for a non-scalable font face.
}


void face_impl::set_translat(vector_type translat) noexcept
{
    m_translation = translat;
}


void face_impl::reset_transform_translat() noexcept
{
    m_translation = {};
}


void face_impl::set_target_pos(image::Pos pos) noexcept
{
    m_target_pos = pos;
}


auto face_impl::get_target_glyph_box() -> image::Box
{
    return do_get_target_glyph_box(); // Throws
}


void face_impl::render_glyph_mask_a(image::Pos pos, const tray_type& tray)
{
    image::Box target_box = { pos, tray.size };
    image::Box box = do_get_target_glyph_box(); // Throws
    image::Pos orig_pos = box.pos;
    if (ARCHON_UNLIKELY(!target_box.clip(box)))
        return;
    image::Pos pos_2 = m_glyph->box.pos + (box.pos - orig_pos);
    tray_type::iter_type iter = tray.iter + (box.pos - pos);
    //
    // NOTE: The component values in the image are in fact linear coverage values
    // masquerading as gamma-compressed luminance values. Therefore,
    // image::Image::get_block_lum() effectively extracts an alpha mask, despite the
    // functions name.
    //
    m_image_reader.get_block_lum(pos_2, { iter, box.size }); // Throws
}


void face_impl::render_glyph_rgba_a(image::Pos pos, const tray_type& tray)
{
    image::Box target_box = { pos, tray.size };
    image::Box box = do_get_target_glyph_box(); // Throws
    image::Pos orig_pos = box.pos;
    if (ARCHON_UNLIKELY(!target_box.clip(box)))
        return;
    image::Pos pos_2 = m_glyph->box.pos + (box.pos - orig_pos);
    tray_type::iter_type iter = tray.iter + (box.pos - pos);
    //
    // First read coverage values from the image directly into the alpha channel of the
    // callers tray, then set the color channels to BLACK.
    //
    // NOTE: The component values in the image are in fact linear coverage values
    // masquerading as gamma-compressed luminance values. Therefore,
    // image::Image::get_block_lum() effectively extracts an alpha mask, despite the
    // functions name.
    //
    m_image_reader.get_block_lum(pos_2, { iter.shift(3), box.size }); // Throws
    for (int y = 0; y < box.size.height; ++y) {
        for (int x = 0; x < box.size.width; ++x) {
            tray_type::comp_type* pixel = iter(x, y);
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
        }
    }
}


auto face_impl::do_get_target_glyph_box() -> image::Box
{
    int translation_x = {}, translation_y = {};
    core::float_to_int(std::floor(m_translation[0]), translation_x); // Throws
    core::float_to_int(std::floor(m_translation[1]), translation_y); // Throws

    image::Pos pos = m_target_pos;
    core::int_add(pos.x, m_glyph->pos_x); // Throws
    core::int_sub(pos.y, m_glyph->pos_y + m_glyph->box.size.height); // Throws
    core::int_add(pos.x, translation_x); // Throws
    core::int_sub(pos.y, translation_y); // Throws

    return { pos, m_glyph->box.size };
}



loader_impl::loader_impl(core::FilesystemPathRef resource_dir, const std::locale& loc, log::Logger* logger)
    : m_resource_dir(resource_dir) // Throws
    , m_locale(loc)
    , m_logger(log::Logger::or_null(logger))
{
}


auto loader_impl::load_default_face() -> std::unique_ptr<font::face>
{
    return do_load_face(); // Throws
}


int loader_impl::get_num_faces()
{
    return 1;
}


auto loader_impl::load_face(int face_index) -> std::unique_ptr<font::face>
{
    if (ARCHON_LIKELY(face_index == 0))
        return do_load_face(); // Throws
    throw std::invalid_argument("Face index");
}


auto loader_impl::do_load_face() -> std::unique_ptr<font::face>
{
    const ::fallback_font& font = ensure_font(); // Throws
    return std::make_unique<::face_impl>(font); // Throws
}


inline auto loader_impl::ensure_font() -> const ::fallback_font&
{
    if (ARCHON_LIKELY(m_font))
        goto have;
    m_font = load_font(); // Throws
  have:
    return *m_font;
}


auto loader_impl::load_font() const -> std::unique_ptr<::fallback_font>
{
    auto font = std::make_unique<::fallback_font>(); // Throws
    if (ARCHON_LIKELY(::load_font(m_resource_dir, m_logger, m_locale, *font))) { // Throws
        std::size_t num_glyphs = font->spec.glyphs.size();
        std::size_t num_code_points = font->spec.glyph_map.size();
        core::NumOfSpec glyphs_spec = { "glyph", "glyphs" };
        core::NumOfSpec code_points_spec = { "code point", "code points" };
        m_logger.detail("Fallback font loaded: %s %s (%s, %s)", font->spec.family_name, font->spec.style_name,
                        core::as_num_of(num_glyphs, glyphs_spec),
                        core::as_num_of(num_code_points, code_points_spec)); // Throws
        return font;
    }
    throw std::runtime_error("Failed to load fallback font");
}



inline auto new_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                       const font::loader::config& config) -> std::unique_ptr<font::loader>
{
    return std::make_unique<::loader_impl>(resource_dir, locale, config.logger); // Throws
}



auto implementation_impl::get_ident() const noexcept -> std::string_view
{
    return "fallback";
}


auto implementation_impl::get_descr() const noexcept -> std::string_view
{
    return "Fallback font implementation";
}


bool implementation_impl::is_available() const noexcept
{
    return true;
}


auto implementation_impl::new_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                                     const font::loader::config& config) const -> std::unique_ptr<font::loader>
{
    return ::new_loader(resource_dir, locale, config); // Throws
}


} // unnamed namespace


auto font::get_fallback_implementation() noexcept -> const font::implementation&
{
    return ::g_implementation;
}


auto font::new_fallback_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                               const font::loader::config& config) -> std::unique_ptr<font::loader>
{
    return ::new_loader(resource_dir, locale, config); // Throws
}


void font::regen_fallback_font(font::face& face, core::Span<const font::code_point_range> ranges,
                               core::FilesystemPathRef resource_dir, std::string_view file_name_qual,
                               const std::locale& locale, const font::loader::config& config)
{
    if (ARCHON_UNLIKELY(ranges.empty()))
        throw std::invalid_argument("Codepoint ranges");

    std::unique_ptr<log::FileLogger> file_logger;
    log::Logger& logger = get_logger(locale, config, file_logger); // Throws

    struct glyph_entry {
        std::size_t index; // In sourcing font face
        image::Box box; // Position and size in image
        int horz_bearing_x, horz_bearing_y;
        int vert_bearing_x, vert_bearing_y;
        int horz_advance, vert_advance;
        int pos_x, pos_y;
        std::vector<font::code_point> code_points;
    };
    std::vector<glyph_entry> glyphs;
    std::size_t num_code_points = 0;

    // Load glyph metrics
    bool grid_fitting = true;
    bool vertical = false;
    {
        auto add_glyph = [&](std::size_t index) {
            glyph_entry glyph = {};
            glyph.index = index;
            ARCHON_ASSERT(grid_fitting);
            bool success = face.try_load_glyph(index, grid_fitting, vertical); // Throws
            if (ARCHON_UNLIKELY(!success))
                throw std::invalid_argument("Unsupported glyph format");
            font::face::vector_type size = face.get_glyph_size();
            glyph.box.size = { int(size[0]), int(size[1]) };
            font::face::vector_type horz_bearing = face.get_glyph_bearing(false);
            glyph.horz_bearing_x = int(horz_bearing[0]);
            glyph.horz_bearing_y = int(horz_bearing[1]);
            font::face::vector_type vert_bearing = face.get_glyph_bearing(true);
            glyph.vert_bearing_x = int(vert_bearing[0]);
            glyph.vert_bearing_y = int(vert_bearing[1]);
            glyph.horz_advance = int(face.get_glyph_advance(false));
            glyph.vert_advance = int(face.get_glyph_advance(true));
            font::face::vector_type pos = face.get_glyph_pos();
            glyph.pos_x = int(pos[0]);
            glyph.pos_y = int(pos[1]);
            glyphs.push_back(std::move(glyph)); // Throws
        };
        // Add replacement glyph first
        add_glyph(0); // Throws
        // Map index of glyph in sourcing font face to index of glyph in generated fallback
        // font face
        std::map<std::size_t, std::size_t> map;
        for (font::code_point_range range : ranges) {
            char_type ch = range.first().to_char();
            for (;;) {
                font::code_point cp;
                if (ARCHON_LIKELY(cp.try_from_char(ch))) {
                    std::size_t index = face.find_glyph(ch);
                    if (index != 0) {
                        auto p = map.emplace(index, glyphs.size()); // Throws
                        bool was_inserted = p.second;
                        if (was_inserted)
                            add_glyph(index); // Throws
                        glyph_entry& glyph = glyphs[p.first->second];
                        glyph.code_points.push_back(cp); // Throws
                        core::int_add(num_code_points, 1); // Throws
                    }
                }
                if (ch == range.last().to_char())
                    break;
                ++ch;
            }
        }
    }

    // Pack glyphs into image area
    image::Size image_size;
    {
        util::RectanglePacker<int> packer;
        for (const glyph_entry& glyph : glyphs)
            packer.add_rect(glyph.box.size.width, glyph.box.size.height); // Throws
        int max_width = packer.suggest_bin_width();
        if (ARCHON_LIKELY(packer.pack(max_width))) { // Throws
            ARCHON_ASSERT(packer.get_num_bins() == 1);
            image_size.width  = packer.get_utilized_width();
            image_size.height = packer.get_utilized_height();
            std::size_t n = glyphs.size();
            for (std::size_t i = 0; i < n; ++i) {
                glyph_entry& glyph = glyphs[i];
                packer.get_rect_pos(i, glyph.box.pos.x, glyph.box.pos.y);
            }
        }
        else {
            throw std::runtime_error("Out of image space");
        }
    }

    // Generate new spec file
    {
        namespace fs = std::filesystem;
        fs::path path = generate_file_path(resource_dir, ::spec_file_name, locale, file_name_qual); // Throws
        core::BufferedTextFile file(path, core::File::Mode::write); // Throws
        core::SeedMemoryOutputStream out; // Throws
        out.imbue(locale); // Throws
        out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        auto format = [&](const char* message, const auto&... params) {
            core::format(out, message, params...); // Throws
            file.write(out.view()); // Throws
            out.full_clear();
        };
        format("%s\n", face.get_family_name()); // Throws
        format("%s\n", face.get_style_name()); // Throws
        format("%s\n", core::as_words(ranges)); // Throws
        font::size font_size = face.get_size();
        format("%s %s   %s %s %s   %s %s   %s %s %s   %s %s %s\n",
               core::as_int(image_size.width), core::as_int(image_size.height),
               core::as_int(face.is_bold()), core::as_int(face.is_italic()),
               core::as_int(face.is_monospace()),
               font_size.width, font_size.height,
               face.get_ascender(grid_fitting, false),
               face.get_descender(grid_fitting, false),
               face.get_baseline_spacing(grid_fitting, false),
               face.get_ascender(grid_fitting, true),
               face.get_descender(grid_fitting, true),
               face.get_baseline_spacing(grid_fitting, true)); // Throws
        using namespace std::literals;
        std::string_view padding = "        "sv;
        auto pad = [&](std::size_t n) {
            std::size_t n_2 = n;
            while (n_2 > padding.size()) {
                file.write(padding); // Throws
                n_2 -= padding.size();
            }
            file.write(padding.substr(0, n_2)); // Throws
        };
        std::array col_spacings = {
            0, // before image position x
            2, // before image position y
            2, // before width
            2, // before height
            4, // before horizontal bearing x
            2, // before horizontal bearing y
            4, // before vertical bearing x
            2, // before vertical bearing y
            4, // before horizontal advance
            2, // before vertical advance
            4, // before position x
            2, // before position y
            4, // before code points
        };
        std::vector<std::size_t> cell_ends;
        std::vector<std::size_t> row_ends;
        std::vector<std::size_t> col_widths;
        std::size_t col_index = 0;
        auto format_cell = [&](const char* message, const auto&... params) {
            std::size_t begin = out.view().size();
            core::format(out, message, params...); // Throws
            std::size_t end = out.view().size();
            cell_ends.push_back(end); // Throws
            std::size_t col_width = std::size_t(end - begin);
            if (ARCHON_UNLIKELY(col_index == col_widths.size()))
                col_widths.push_back(0); // Throws
            if (ARCHON_UNLIKELY(col_width > col_widths[col_index]))
                col_widths[col_index] = col_width;
            ++col_index;
        };
        for (const glyph_entry& glyph : glyphs) {
            format_cell("%s", core::as_int(glyph.box.pos.x)); // Throws
            format_cell("%s", core::as_int(glyph.box.pos.y)); // Throws
            format_cell("%s", core::as_int(glyph.box.size.width)); // Throws
            format_cell("%s", core::as_int(glyph.box.size.height)); // Throws
            format_cell("%s", core::as_int(glyph.horz_bearing_x)); // Throws
            format_cell("%s", core::as_int(glyph.horz_bearing_y)); // Throws
            format_cell("%s", core::as_int(glyph.vert_bearing_x)); // Throws
            format_cell("%s", core::as_int(glyph.vert_bearing_y)); // Throws
            format_cell("%s", core::as_int(glyph.horz_advance)); // Throws
            format_cell("%s", core::as_int(glyph.vert_advance)); // Throws
            format_cell("%s", core::as_int(glyph.pos_x)); // Throws
            format_cell("%s", core::as_int(glyph.pos_y)); // Throws
            format_cell("%s", core::as_words(glyph.code_points)); // Throws
            row_ends.push_back(cell_ends.size()); // Throws
            col_index = 0;
        }
        const char* base = out.view().data();
        std::size_t prev_cell_end = 0;
        std::size_t prev_row_end = 0;
        for (std::size_t row_end : row_ends) {
            std::size_t cursor = 0;
            std::size_t offset = 0;
            std::size_t num_cells = std::size_t(row_end - prev_row_end);
            for (std::size_t i = 0; i < num_cells; ++i) {
                std::size_t col_spacing = std::size_t(i < col_spacings.size() ? col_spacings[i] : 1);
                offset += col_spacing;
                std::size_t cell_end = cell_ends[prev_row_end + i];
                std::size_t cell_width = std::size_t(cell_end - prev_cell_end);
                if (cell_width > 0) {
                    ARCHON_ASSERT(cursor <= offset);
                    pad(offset - cursor); // Throws
                    file.write(core::Span(base + prev_cell_end, cell_width)); // Throws
                    cursor = offset + cell_width;
                }
                ARCHON_ASSERT(i < col_widths.size());
                offset += col_widths[i];
                prev_cell_end = cell_end;
            }
            file.write("\n"sv); // Throws
            prev_row_end = row_end;
        }
        file.flush(); // Throws
        logger.info("Spec file generated: %s", core::as_native_path(path)); // Throws
    }

    // Create image with glyphs
    {
        namespace fs = std::filesystem;
        fs::path path = generate_file_path(resource_dir, ::image_file_name, locale, file_name_qual); // Throws
        image::BufferedImage_Lum_8 image(image_size); // Throws
        std::ptrdiff_t horz_stride = 1;
        std::ptrdiff_t vert_stride = std::ptrdiff_t(image_size.width);
        image::Iter iter = { image.get_buffer().data(), horz_stride, vert_stride };
        image::Tray tray = { iter, image_size };
        image::Pos pos = { 0, 0 };
        for (const glyph_entry& glyph : glyphs) {
            using float_type = font::face::float_type;
            face.set_translat({ -float_type(glyph.pos_x), -float_type(glyph.pos_y) });
            face.set_target_pos(glyph.box.pos + glyph.box.size.proj_y());
            bool success = face.try_load_glyph(glyph.index, grid_fitting, vertical); // Throws
            ARCHON_ASSERT(success);
            // NOTE: The component values extracted by font::face::render_glyph_mask_a() are
            // linear coverage values, but these are stored directly into a pixel buffer
            // which forms a single gamma-compressed luminance channel. The effect of this
            // is that the linear coverage values, now masquerading as gamma-compress
            // luminance values get transported into and stored in the image file without
            // loss of information. The reader must know, however, that when extracting the
            // gamma-compressed luminance channel, what comes out is really a linear alpha
            // mask.
            face.render_glyph_mask_a(pos, tray); // Throws
        }
        image::save(image, path, locale); // Throws
        logger.info("Image file generated: %s", core::as_native_path(path)); // Throws
    }

    font::size font_size = face.get_size();
    std::size_t num_glyphs = glyphs.size();
    double em_area = double(font_size.width) * double(font_size.height);
    double image_area = double(image_size.width) * double(image_size.height);
    double accum_glyph_area = 0;
    for (glyph_entry glyph : glyphs)
        accum_glyph_area += double(glyph.box.size.width) * double(glyph.box.size.height);
    double coverage = accum_glyph_area / image_area;
    double glyphs_per_em = em_area / (image_area / num_glyphs);
    logger.info("Fallback font successfully generated"); // Throws
    logger.info("Font family: %s", face.get_family_name()); // Throws
    logger.info("Font style: %s", face.get_style_name()); // Throws
    logger.info("Font size: %s", font_size); // Throws
    logger.info("Code point ranges: %s", core::as_list(ranges)); // Throws
    logger.info("Number of glyphs: %s", core::as_int(num_glyphs)); // Throws
    logger.info("Number of code points: %s", core::as_int(num_code_points)); // Throws
    logger.info("Image size: %s", image_size); // Throws
    logger.info("Image coverage: %s", core::as_percent(coverage, 1)); // Throws
    logger.info("Glyphs per EM-square: %s", core::with_fixed(glyphs_per_em, 2)); // Throws
}


bool font::try_get_fallback_font_params(core::FilesystemPathRef resource_dir, const std::locale& locale,
                                        std::vector<font::code_point_range>& ranges, font::fallback_font_params& params,
                                        std::unique_ptr<char[]>& string_owner, font::size& size)
{
    log::Logger& logger = log::Logger::get_null();
    ::spec spec = {};
    if (ARCHON_LIKELY(::load_spec(resource_dir, logger, locale, spec))) { // Throws
        char seed_mem[64] = {};
        core::Buffer<char> buffer(seed_mem);
        std::size_t buffer_offset = 0;
        std::size_t family_name_offset = buffer_offset;
        buffer.append(spec.family_name, buffer_offset); // Throws
        std::size_t family_name_size = std::size_t(buffer_offset - family_name_offset);
        std::size_t style_name_offset = buffer_offset;
        buffer.append(spec.style_name, buffer_offset); // Throws
        std::size_t style_name_size = std::size_t(buffer_offset - style_name_offset);
        std::unique_ptr<char[]> string_owner_2;
        std::string_view family_name;
        std::string_view style_name;
        if (ARCHON_LIKELY(buffer_offset > 0)) {
            string_owner_2 = std::make_unique_for_overwrite<char[]>(buffer_offset); // Throws
            std::copy_n(buffer.data(), buffer_offset, string_owner_2.get());
            family_name = { string_owner_2.get() + family_name_offset, family_name_size }; // Throws
            style_name  = { string_owner_2.get() + style_name_offset,  style_name_size  }; // Throws
        }
        string_owner = std::move(string_owner_2);
        ranges = std::move(spec.code_point_ranges);
        params = {
            family_name,
            style_name,
            spec.bold,
            spec.italic,
            spec.monospace,
        };
        size = spec.render_size;
        return true;
    }
    return false;
}
