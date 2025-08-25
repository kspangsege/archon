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
#include <memory>
#include <utility>
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
#include <mutex>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
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
#include <archon/font/loader_fallback.hpp>


using namespace archon;


namespace {


constexpr std::string_view g_spec_file_name  = "fallback-font.txt";
constexpr std::string_view g_image_file_name = "fallback-font.png";


auto generate_file_path(core::FilesystemPathRef resource_dir, std::string_view file_name, const std::locale& loc,
                        std::string_view modifier = "") -> std::filesystem::path
{
    std::string file_name_2 = std::string(file_name); // Throws
    auto i = file_name_2.rfind('.');
    if (ARCHON_UNLIKELY(i == std::string::npos))
        i = file_name_2.size();
    file_name_2.insert(i, modifier); // Throws
    namespace fs = std::filesystem;
    fs::path path = core::make_fs_path_generic(file_name_2, loc); // Throws
    return resource_dir / path; // Throws
}


using char_type = font::CodePoint::char_type;


struct Glyph {
    // Position and size of glyph in image.
    image::Box box;

    // Position of bearing point of a left-to-right layout realtive to the lower left corner
    // of the bounding box of the glyph. The X-coordinate increases towards the right and
    // the Y-coordinate increases upwards.
    int horz_bearing_x, horz_bearing_y;

    // Position of bearing point of a bottom-to-top layout realtive to the lower left corner
    // of the bounding box of the glyph. The X-coordinate increases towards the right and
    // the Y-coordinate increases upwards.
    int vert_bearing_x, vert_bearing_y;

    // The glyph advance for horizontal and vertical layouts respecively. Neither can be
    // negative.
    int horz_advance, vert_advance;
};


struct Spec {
    std::string family_name;
    std::vector<font::CodePointRange> code_point_ranges;
    image::Size image_size;
    bool bold, italic, monospace;
    font::Size render_size;
    int horz_baseline_offset, horz_baseline_spacing;
    int vert_baseline_offset, vert_baseline_spacing;
    core::Slab<Glyph> glyphs; // First glyph is fallback glyph
    std::map<char_type, std::size_t> glyph_map;
};


struct Font {
    std::unique_ptr<image::Image> image;
    Spec spec;
};


bool load_spec(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc, Spec& spec)
{
    namespace fs = std::filesystem;
    fs::path path = generate_file_path(resource_dir, g_spec_file_name, loc); // Throws
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
    std::vector<Glyph> glyphs;
    std::vector<font::CodePoint> code_points;
    bool have_family_name = false;
    bool have_code_point_ranges = false;
    bool have_font_metrics = false;
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
        if (ARCHON_LIKELY(have_font_metrics))
            goto glyph_spec;
        if (have_code_point_ranges)
            goto font_metrics_spec;
        if (have_family_name)
            goto code_point_ranges;
        spec.family_name = std::string(line_2); // Throws
        have_family_name = true;
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
                std::optional<font::CodePoint> prev_last;
                for (font::CodePointRange range : spec.code_point_ranges) {
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
      font_metrics_spec:
        have_font_metrics = true;
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
                text_parser.field(core::as_int(spec.horz_baseline_offset),  "horizontal baseline offset"sv),
                text_parser.field(core::as_int(spec.horz_baseline_spacing), "horizontal baseline spacing"sv),
                text_parser.field(core::as_int(spec.vert_baseline_offset),  "vertical baseline offset"sv),
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
            Glyph glyph;
            code_points.clear();
            using namespace std::literals;
            std::tuple fields = {
                text_parser.field(core::as_int(glyph.box.pos.x),       "left"sv),
                text_parser.field(core::as_int(glyph.box.pos.y),       "top"sv),
                text_parser.field(core::as_int(glyph.box.size.width),  "width"sv),
                text_parser.field(core::as_int(glyph.box.size.height), "height"sv),
                text_parser.field(core::as_int(glyph.horz_bearing_x),  "horizontal bearing x"sv),
                text_parser.field(core::as_int(glyph.horz_bearing_y),  "horizontal bearing y"sv),
                text_parser.field(core::as_int(glyph.vert_bearing_x),  "vertical bearing x"sv),
                text_parser.field(core::as_int(glyph.vert_bearing_y),  "vertical bearing y"sv),
                text_parser.field(core::as_int(glyph.horz_advance),    "horizontal advance"sv),
                text_parser.field(core::as_int(glyph.vert_advance),    "vertical advance"sv),
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
                    error_1("Negative glyph advance (horizontal %s, vertival %s)", glyph.horz_advance,
                            glyph.vert_advance); // Throws
                    goto error;
                }
                std::size_t glyph_index = glyphs.size();
                glyphs.push_back(glyph); // Throws
                for (font::CodePoint cp : code_points) {
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
    if (ARCHON_UNLIKELY(!have_font_metrics)) {
        ++line_num;
        error_1("Missing font metrics"); // Throws
        return false;
    }
    if (ARCHON_UNLIKELY(glyphs.size() < 1)) {
        ++line_num;
        error_1("Missing replacement glyph"); // Throws
        return false;
    }
    spec.glyphs = core::Slab<Glyph>(core::Span(glyphs)); // Throws
    return true;
}


bool load_image(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc,
                image::Size expected_image_size, std::unique_ptr<image::Image>& image)
{
    namespace fs = std::filesystem;
    fs::path path = generate_file_path(resource_dir, g_image_file_name, loc); // Throws
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
        load_logger.error("Image size  mismatch (was %s, expected %s)", image_2->get_size(),
                          expected_image_size); // Throws
        return false;
    }
    load_logger.error("%s", ec.message()); // Throws
    return false;
}


bool load_font(core::FilesystemPathRef resource_dir, log::Logger& logger, const std::locale& loc, Font& font)
{
    if (ARCHON_LIKELY(load_spec(resource_dir, logger, loc, font.spec))) // Throws
        return load_image(resource_dir, logger, loc, font.spec.image_size, font.image); // Throws
    return false;
}


auto make_file_logger(const std::locale& loc) -> std::unique_ptr<log::FileLogger>
{
    return std::make_unique<log::FileLogger>(core::File::get_stdout(), loc); // Throws
}


auto get_logger(const std::locale& loc, font::Loader::Config config,
                std::unique_ptr<log::FileLogger>& file_logger) -> log::Logger&
{
    if (config.logger)
        return *config.logger;
    file_logger = std::make_unique<log::FileLogger>(core::File::get_stdout(), loc); // Throws
    return *file_logger;
}



class FaceImpl final
    : public font::Face {
public:
    FaceImpl(const Font& font) noexcept
        : m_font(font)
        , m_image_reader(*font.image) // Throws
    {
        ARCHON_ASSERT(m_font.spec.glyphs.size() > 0);
        m_glyph = &m_font.spec.glyphs[0]; // Replacement glyph
    }

    auto get_family_name() -> std::string_view override
    {
        return m_font.spec.family_name;
    }

    bool is_bold() noexcept override
    {
        return m_font.spec.bold;
    }

    bool is_italic() noexcept override
    {
        return m_font.spec.italic;
    }

    bool is_monospace() noexcept override
    {
        return m_font.spec.monospace;
    }

    bool is_scalable() noexcept override
    {
        return false;
    }

    int get_num_fixed_sizes() override
    {
        return 1;
    }

    auto get_fixed_size(int fixed_size_index) -> font::Size override
    {
        if (ARCHON_LIKELY(fixed_size_index == 0))
            return m_font.spec.render_size;
        throw std::out_of_range("Fixed size index");
    }

    void set_fixed_size(int fixed_size_index) override
    {
        if (ARCHON_LIKELY(fixed_size_index == 0))
            return;
        throw std::out_of_range("Fixed size index");
    }

    void set_scaled_size(font::Size) override
    {
        throw std::logic_error("Font face is not scalable");
    }

    void set_approx_size(font::Size) override
    {
        // No-op since there is only one size in the first place
    }

    auto get_size() noexcept -> font::Size override
    {
        return m_font.spec.render_size;
    }

    auto get_baseline_spacing(bool vertical, bool) noexcept -> float_type override
    {
        const Spec& spec = m_font.spec;
        return (vertical ? float_type(spec.vert_baseline_spacing) : float_type(spec.horz_baseline_spacing));
    }

    auto get_baseline_offset(bool vertical, bool) noexcept -> float_type override
    {
        const Spec& spec = m_font.spec;
        return (vertical ? float_type(spec.vert_baseline_offset) : float_type(spec.horz_baseline_offset));
    }

    auto find_glyph(char_type ch) -> std::size_t override
    {
        auto i = m_font.spec.glyph_map.find(ch); // Throws
        if (ARCHON_LIKELY(i != m_font.spec.glyph_map.end()))
            return i->second;
        return 0;
    }

    auto get_kerning(std::size_t, std::size_t, bool, bool) -> float_type override
    {
        return 0;
    }

    void load_glyph(std::size_t glyph_index, bool) override
    {
        if (ARCHON_LIKELY(glyph_index < m_font.spec.glyphs.size())) {
            m_glyph = &m_font.spec.glyphs[glyph_index];
            m_glyph_translation = vector_type(); // Throws
            return;
        }
        throw std::out_of_range("glyph index");
    }

    auto get_glyph_advance(bool vertical) noexcept -> float_type override
    {
        if (ARCHON_LIKELY(!vertical))
            return float_type(m_glyph->horz_advance);
        return float_type(m_glyph->vert_advance);
    }

    auto get_glyph_bearing(bool vertical) noexcept -> vector_type override
    {
        if (ARCHON_LIKELY(!vertical))
            return { float_type(m_glyph->horz_bearing_x), float_type(m_glyph->horz_bearing_y) };
        return { float_type(m_glyph->vert_bearing_x), float_type(m_glyph->vert_bearing_y) };
    }

    void translate_glyph(vector_type dist) override
    {
        m_glyph_translation += dist;
    }

protected:
    void do_get_glyph_pa_box(int& left, int& right, int& bottom, int& top) override
    {
        // FIXME: Tend to overflow in arithmetic     
        left   = get_glyph_translation_x(); // Throws
        bottom = get_glyph_translation_y(); // Throws
        right  = left   + m_glyph->box.size.width;
        top    = bottom + m_glyph->box.size.height;
    }

    void do_render_glyph_mask(image::Pos pos, const iter_type& iter, image::Size size) override
    {
        // FIXME: Would be better if glyph image had been loaded into tray-type buffer. Then this function could be a simple memory copy for each scan line.                                                          
        ARCHON_ASSERT(m_glyph);
        image::Pos pos_2 = pos;
        // Note the inversion of the Y-axis
        int left   = get_glyph_translation_x(); // Throws
        int bottom = get_glyph_translation_y(); // Throws
        int top    = bottom + m_glyph->box.size.height;
        core::int_add(pos_2.x, left); // Throws
        core::int_sub(pos_2.y, top); // Throws
        image::Box target_box = { pos_2, m_glyph->box.size };
        image::Box bounding_box = { size };
        if (ARCHON_LIKELY(bounding_box.clip(target_box))) {
            image::Iter iter_2 = iter + (target_box.pos - image::Pos());
            image::Pos source_pos = m_glyph->box.pos + (target_box.pos - pos_2);
            m_image_reader.get_block_lum(source_pos, { iter_2, target_box.size }); // Throws
        }
    }

    void do_render_glyph_rgba(image::Pos pos, const iter_type& iter, image::Size size) override
    {
        // FIXME: Implement this                                                                       
        static_cast<void>(pos);                
        static_cast<void>(iter);                
        static_cast<void>(size);                
        ARCHON_STEADY_ASSERT_UNREACHABLE();                  
    }

private:
    const Font& m_font;
    image::Reader m_image_reader;
    const Glyph* m_glyph = nullptr;
    vector_type m_glyph_translation;

    int get_glyph_translation_x() const
    {
        // FIXME: Tend to overflow in conversion     
        return int(std::round(m_glyph_translation[0])); // Throws
    }

    int get_glyph_translation_y() const
    {
        // FIXME: Tend to overflow in conversion     
        return int(std::round(m_glyph_translation[1])); // Throws
    }
};



class LoaderImpl final
    : public font::Loader {
public:
    LoaderImpl(core::FilesystemPathRef resource_dir, const std::locale& loc, log::Logger* logger)
        : m_resource_dir(resource_dir) // Throws
        , m_locale(loc)
        , m_file_logger(logger ? nullptr : make_file_logger(loc)) // Throws
        , m_logger(logger ? *logger : *m_file_logger)
    {
    }

    auto load_default_face() const -> std::unique_ptr<font::Face> override
    {
        const Font& font = ensure_font(); // Throws
        return std::make_unique<FaceImpl>(font); // Throws
    }

    auto get_implementation() const noexcept -> const Implementation& override;

private:
    const std::filesystem::path m_resource_dir;
    const std::locale m_locale;
    const std::unique_ptr<log::FileLogger> m_file_logger;
    log::Logger& m_logger;

    mutable std::mutex m_mutex;
    mutable std::unique_ptr<Font> m_font; // Protected by `m_mutex`

    auto ensure_font() const -> const Font&
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (ARCHON_LIKELY(m_font))
            goto have;
        m_font = load_font(); // Throws
      have:
        return *m_font;
    }

    auto load_font() const -> std::unique_ptr<Font>
    {
        auto font = std::make_unique<Font>(); // Throws
        if (ARCHON_LIKELY(::load_font(m_resource_dir, m_logger, m_locale, *font))) { // Throws
            std::size_t num_glyphs = font->spec.glyphs.size();
            std::size_t num_code_points = font->spec.glyph_map.size();
            core::NumOfSpec glyphs_spec = { "glyph", "glyphs" };
            core::NumOfSpec code_points_spec = { "code point", "code points" };
            m_logger.detail("Fallback font loaded: %s (%s, %s)", font->spec.family_name,
                            core::as_num_of(num_glyphs, glyphs_spec),
                            core::as_num_of(num_code_points, code_points_spec)); // Throws
            return font;
        }
        throw std::runtime_error("Failed to load fallback font");
    }
};



class ImplementationImpl final
    : public font::Loader::Implementation {
public:
    auto ident() const noexcept -> std::string_view override
    {
        return "fallback";
    }

    auto new_loader(core::FilesystemPathRef resource_dir, const std::locale& loc,
                    font::Loader::Config config) const -> std::unique_ptr<font::Loader> override
    {
        return std::make_unique<LoaderImpl>(resource_dir, loc, config.logger); // Throws
    }
};

inline auto get_implementation() noexcept -> const ImplementationImpl&
{
    static ImplementationImpl impl;
    return impl;
}



inline auto LoaderImpl::get_implementation() const noexcept -> const Implementation&
{
    return ::get_implementation();
}


} // unnamed namespace


auto font::loader_fallback_impl() noexcept -> const font::Loader::Implementation&
{
    return ::get_implementation();
}


void font::regen_fallback_font(font::Face& face, bool try_keep_orig_font_size,
                               core::Span<const font::CodePointRange> ranges, core::FilesystemPathRef resource_dir,
                               const std::locale& loc, font::Loader::Config config)
{
    std::vector<font::CodePointRange> fallback_ranges;
    core::Span<const font::CodePointRange> ranges_2 = ranges;
    if (ranges_2.empty() || try_keep_orig_font_size) {
        Spec spec = {};
        log::Logger& logger = log::Logger::get_null();
        if (ARCHON_LIKELY(load_spec(resource_dir, logger, loc, spec))) { // Throws
            fallback_ranges = std::move(spec.code_point_ranges);
            if (try_keep_orig_font_size)
                face.set_approx_size(spec.render_size); // Throws
        }
        else {
            font::CodePoint first, last;
            bool success = (first.try_from_int(0) && last.try_from_int(127));
            ARCHON_ASSERT(success);
            fallback_ranges = {
                { first, last },
            };
        }
        if (ranges_2.empty())
            ranges_2 = fallback_ranges;
    }
    std::unique_ptr<log::FileLogger> file_logger;
    log::Logger& logger = get_logger(loc, config, file_logger); // Throws

    struct Glyph2 {
        std::size_t index; // In sourcing font face
        image::Box box; // Position and size in image
        int horz_bearing_x, horz_bearing_y;
        int vert_bearing_x, vert_bearing_y;
        int horz_advance, vert_advance;
        std::vector<font::CodePoint> code_points;
    };
    std::vector<Glyph2> glyphs;
    std::size_t num_code_points = 0;

    // Load glyph metrics
    bool grid_fitting = true;
    {
        auto add_glyph = [&](std::size_t index) {
            face.load_glyph(index, grid_fitting); // Throws
            Glyph2 glyph = {};
            glyph.index = index;
            glyph.box.size = face.get_glyph_pa_size(); // Throws
            font::Face::vector_type horz_bearing = face.get_glyph_bearing(false); // Throws
            glyph.horz_bearing_x = int(horz_bearing[0]);
            glyph.horz_bearing_y = int(horz_bearing[1]);
            font::Face::vector_type vert_bearing = face.get_glyph_bearing(true); // Throws
            glyph.vert_bearing_x = int(vert_bearing[0]);
            glyph.vert_bearing_y = int(vert_bearing[1]);
            glyph.horz_advance = int(face.get_glyph_advance(false)); // Throws
            glyph.vert_advance = int(face.get_glyph_advance(true)); // Throws
            glyphs.push_back(std::move(glyph)); // Throws
        };
        // Add replacement glyph first
        add_glyph(0); // Throws
        // Map index of glyph in sourcing font face to index of glyph in generated fallback
        // font face
        std::map<std::size_t, std::size_t> map;
        for (font::CodePointRange range : ranges_2) {
            char_type ch = range.first().to_char();
            for (;;) {
                font::CodePoint cp;
                if (ARCHON_LIKELY(cp.try_from_char(ch))) {
                    std::size_t index = face.find_glyph(ch);
                    if (index != 0) {
                        auto p = map.emplace(index, glyphs.size()); // Throws
                        bool was_inserted = p.second;
                        if (was_inserted)
                            add_glyph(index); // Throws
                        Glyph2& glyph = glyphs[p.first->second];
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
        for (const Glyph2& glyph : glyphs)
            packer.add_rect(glyph.box.size.width, glyph.box.size.height); // Throws
        int max_width = packer.suggest_bin_width();
        if (ARCHON_LIKELY(packer.pack(max_width))) { // Throws
            ARCHON_ASSERT(packer.get_num_bins() == 1);
            image_size.width  = packer.get_utilized_width();
            image_size.height = packer.get_utilized_height();
            std::size_t n = glyphs.size();
            for (std::size_t i = 0; i < n; ++i) {
                Glyph2& glyph = glyphs[i];
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
        fs::path path = generate_file_path(resource_dir, g_spec_file_name, loc, "-new"); // Throws
        core::BufferedTextFile file(path, core::File::Mode::write); // Throws
        core::SeedMemoryOutputStream out; // Throws
        out.imbue(loc); // Throws
        out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        auto format = [&](const char* message, const auto&... params) {
            core::format(out, message, params...); // Throws
            file.write(out.view()); // Throws
            out.full_clear();
        };
        format("%s\n", face.get_family_name()); // Throws
        format("%s\n", core::as_words(ranges_2)); // Throws
        font::Size font_size = face.get_size();
        format("%s %s   %s %s %s   %s %s   %s %s   %s %s\n",
               core::as_int(image_size.width), core::as_int(image_size.height),
               core::as_int(face.is_bold()), core::as_int(face.is_italic()),
               core::as_int(face.is_monospace()),
               font_size.width, font_size.height,
               face.get_baseline_offset(false,  grid_fitting),
               face.get_baseline_spacing(false, grid_fitting),
               face.get_baseline_offset(true,   grid_fitting),
               face.get_baseline_spacing(true,  grid_fitting)); // Throws
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
            0, // before left
            2, // before top
            2, // before width
            2, // before height
            4, // before horizontal bearing x
            2, // before horizontal bearing y
            4, // before vertical bearing x
            2, // before vertical bearing y
            4, // before horizontal advance
            2, // before vertical advance
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
        for (const Glyph2& glyph : glyphs) {
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
        fs::path path = generate_file_path(resource_dir, g_image_file_name, loc, "-new"); // Throws
        image::BufferedImage_Lum_8 image(image_size); // Throws
        std::ptrdiff_t horz_stride = 1;
        std::ptrdiff_t vert_stride = std::ptrdiff_t(image_size.width);
        image::Iter iter = { image.get_buffer().data(), horz_stride, vert_stride };
        for (const Glyph2& glyph : glyphs) {
            image::Pos pos = glyph.box.pos;
            pos.y += glyph.box.size.height;
            face.set_target_pos(pos);
            face.load_glyph(glyph.index, grid_fitting); // Throws
            face.render_glyph_mask_a(iter, image_size); // Throws
        }
        image::save(image, path, loc); // Throws
        logger.info("Image file generated: %s", core::as_native_path(path)); // Throws
    }

    core::Vector<std::string_view, 3> font_style_keywords;
    if (face.is_bold())
        font_style_keywords.push_back("bold"); // Throws
    if (face.is_italic())
        font_style_keywords.push_back("italic"); // Throws
    if (face.is_monospace())
        font_style_keywords.push_back("monospace"); // Throws
    font::Size font_size = face.get_size();
    std::size_t num_glyphs = glyphs.size();
    double em_area = double(font_size.width) * double(font_size.height);
    double image_area = double(image_size.width) * double(image_size.height);
    double accum_glyph_area = 0;
    for (Glyph2 glyph : glyphs)
        accum_glyph_area += double(glyph.box.size.width) * double(glyph.box.size.height);
    double coverage = accum_glyph_area / image_area;
    double gplyps_per_em = em_area / (image_area / num_glyphs);
    logger.info("Fallback font successfully generated"); // Throws
    logger.info("Font family: %s", face.get_family_name()); // Throws
    logger.info("Font style: %s", core::as_list(font_style_keywords)); // Throws
    logger.info("Font size: %s", font_size); // Throws
    logger.info("Code point ranges: %s", core::as_list(ranges_2)); // Throws
    logger.info("Number of glyphs: %s", num_glyphs); // Throws
    logger.info("Number of code points: %s", num_code_points); // Throws
    logger.info("Image size: %s", image_size); // Throws
    logger.info("Image coverage: %s", core::as_percent(coverage, 1)); // Throws
    logger.info("Glyphs per EM-square: %s", core::with_fixed(gplyps_per_em, 2)); // Throws
}
