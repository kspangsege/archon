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
#include <cstdint>
#include <cmath>
#include <utility>
#include <algorithm>
#include <memory>
#include <string_view>
#include <string>
#include <stdexcept>
#include <locale>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/charenc_bridge.hpp>
#include <archon/core/format.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/log.hpp>
#include <archon/util/bit_medium.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image.hpp>
#include <archon/font/impl/config.h>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/implementation.hpp>
#include <archon/font/freetype_implementation.hpp>

#if ARCHON_FONT_HAVE_FREETYPE
#  include <ft2build.h>
#  include FT_FREETYPE_H
#  include FT_OUTLINE_H
#  include FT_GLYPH_H
#  include FT_DRIVER_H
#  include FT_MODULE_H
#  include FT_FONT_FORMATS_H
#  include FT_TRUETYPE_TABLES_H
#  include FT_COLOR_H
#endif


using namespace archon;


namespace {


#if ARCHON_FONT_HAVE_FREETYPE


#define IS_FREETYPE_VERSION_AT_LEAST(major, minor, patch)               \
    ((FREETYPE_MAJOR > (major)) ||                                      \
     (FREETYPE_MAJOR == (major) && FREETYPE_MINOR > (minor)) ||         \
     (FREETYPE_MAJOR == (major) && FREETYPE_MINOR == (minor) && FREETYPE_PATCH >= (patch)))


const char* get_freetype_error_string(FT_Error err) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s) case v: return s;
#define FT_ERROR_START_LIST switch (FT_ERROR_BASE(err)) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
    return "Unknown error";
}


auto get_freetype_error(const std::locale& locale, std::string_view message, FT_Error err) -> std::string
{
    const char* string = get_freetype_error_string(err);
    return core::format(locale, "FreeType: %s: %s", message, string); // Throws
}


[[noreturn]] void throw_freetype_error(const std::locale& locale, std::string_view message, FT_Error err)
{
    std::string msg = get_freetype_error(locale, message, err); // Throws
    throw std::runtime_error(std::move(msg));
}


struct library_guard {
    FT_Library library = nullptr;
    library_guard() noexcept = default;
    ~library_guard() noexcept
    {
        if (ARCHON_LIKELY(library)) {
            FT_Error err = FT_Done_FreeType(library);
            ARCHON_ASSERT(err == 0);
        }
    }
    library_guard(library_guard&&) noexcept = delete;
    auto operator=(library_guard&&) noexcept -> library_guard& = delete;
    void init(const std::locale& locale)
    {
        FT_Error err = FT_Init_FreeType(&library);
        if (ARCHON_UNLIKELY(err != 0))
            throw_freetype_error(locale, "Failed to initialize library", err); // Throws
    }
};


struct face_guard {
    const ::library_guard& library;
    FT_Face face = nullptr;
    face_guard(const ::library_guard& l) noexcept
        : library(l)
    {
    }
    ~face_guard() noexcept
    {
        if (ARCHON_LIKELY(face)) {
            FT_Error err = FT_Done_Face(face);
            ARCHON_ASSERT(err == 0);
        }
    }
    face_guard(face_guard&&) noexcept = delete;
    auto operator=(face_guard&&) noexcept -> face_guard& = delete;
    void init(const char* path, FT_Long face_index, const std::locale& locale)
    {
        FT_Error err = FT_New_Face(library.library, path, face_index, &face);
        if (ARCHON_LIKELY(err == 0))
            return;
        throw_freetype_error(locale, "Failed to load font face", err); // Throws
    }
};


class glyph_guard {
public:
    FT_Glyph glyph = {};
    glyph_guard() noexcept = default;
    ~glyph_guard() noexcept
    {
        discard();
    }
    glyph_guard(glyph_guard&& other) noexcept
    {
        assign(std::move(other));
    }
    auto operator=(glyph_guard&& other) noexcept -> glyph_guard&
    {
        discard();
        assign(std::move(other));
        return *this;
    }
    explicit operator bool() const noexcept
    {
        return bool(glyph);
    }
private:
    void discard() noexcept
    {
        if (glyph)
            FT_Done_Glyph(glyph);
    }
    void assign(glyph_guard&& other) noexcept
    {
        glyph = other.glyph;
        other.glyph = {};
    }
};


inline auto fixed_26p6_to_float(FT_F26Dot6 val) noexcept -> font::face::float_type
{
    return (font::face::float_type(1) / 64) * val;
}

inline auto float_to_fixed_26p6(font::face::float_type val) noexcept -> FT_F26Dot6
{
    return core::clamped_float_to_int<FT_F26Dot6>(std::round(64 * val));
}

inline auto float_to_fixed_16p16(font::face::float_type val) noexcept -> FT_Fixed
{
    return core::clamped_float_to_int<FT_Fixed>(std::round(65536 * val));
}


inline auto fixed_26p6_floor(FT_F26Dot6 val) noexcept -> FT_F26Dot6
{
    auto val_2 = core::to_unsigned(core::promote(val));
    using type = decltype(val_2);
    return core::cast_from_twos_compl<FT_F26Dot6>(val_2 & ~type(63));
}


inline auto fixed_26p6_ceil(FT_F26Dot6 val) noexcept -> FT_F26Dot6
{
    auto val_2 = core::to_unsigned(core::promote(val));
    using type = decltype(val_2);
    return core::cast_from_twos_compl<FT_F26Dot6>((val_2 + 63) & ~type(63));
}


inline auto fixed_26p6_round(FT_F26Dot6 val) noexcept -> FT_F26Dot6
{
    auto val_2 = core::to_unsigned(core::promote(val));
    using type = decltype(val_2);
    return core::cast_from_twos_compl<FT_F26Dot6>((val_2 + 32) & ~type(63));
}


inline auto fixed_26p6_to_int_floor(FT_F26Dot6 val) noexcept -> FT_Pos
{
    return FT_Pos(fixed_26p6_floor(val) / 64);
}


inline auto fixed_26p6_to_int_ceil(FT_F26Dot6 val) noexcept -> FT_Pos
{
    return FT_Pos(fixed_26p6_ceil(val) / 64);
}


class fixed_size_key {
public:
    FT_F26Dot6 width, height;

    constexpr auto operator<=>(const fixed_size_key&) const noexcept = default;
};


class size_spec {
public:
    // Must -1 or >= 0.
    int fixed_size_index = -1;

    // In points. Must be zero if fixed_size_index >= 0.
    FT_F26Dot6 width = 0, height = 0;

    // In pixels per inch. Must be zero if fixed_size_index >= 0
    FT_UInt horz_resol = 0, vert_resol = 0;

    size_spec() noexcept = default;

    // Fixed size specification
    explicit size_spec(int i) noexcept
        : fixed_size_index(i)
    {
    }

    // Scaled size specification
    explicit size_spec(FT_F26Dot6 w, FT_F26Dot6 h, FT_UInt hr, FT_UInt vr) noexcept
        : width(w)
        , height(h)
        , horz_resol(hr)
        , vert_resol(vr)
    {
    }

    constexpr auto operator<=>(const size_spec&) const noexcept = default;
};


struct size_properties {
    FT_F26Dot6 width, height;
    FT_F26Dot6 horz_ascender,    horz_descender,    horz_baseline_spacing;
    FT_F26Dot6 vert_ascender,    vert_descender,    vert_baseline_spacing;
    FT_F26Dot6 horz_ascender_gf, horz_descender_gf, horz_baseline_spacing_gf;
    FT_F26Dot6 vert_ascender_gf, vert_descender_gf, vert_baseline_spacing_gf;
};


struct glyph_load_params {
    ::size_spec size;
    FT_UInt glyph_index;
    bool color_loading_enabled;
    bool is_colored_outline;
    bool grid_fitting;
    bool vertical;
};


struct glyph_properties {
    FT_F26Dot6 horz_advance, vert_advance;
    FT_F26Dot6 vert_bearing_pos_x, vert_bearing_pos_y;
    FT_F26Dot6 x_1, x_2, y_1, y_2;
    FT_Glyph_Format orig_format;
    bool may_be_colored;
};


// In the case of uncolored scalable glyphs, if the transformation is degenerate (identity
// matrix), `substitute_translation_x` and `substitute_translation_y` specify the
// translation that must be applied to the glyph in the glyph slot. If the transformation is
// not degenerate, `substitute_translation_x` and `substitute_translation_y` must be zero
// and `glyph_clone` holds the substitute-transformed glyph. In both cases,
// `residual_translation_x` and `residual_translation_y` specify the residual translation
// that needs to be applied in addition to the substitute transformation in order to produce
// the full requested transformation.
//
// `transformed_x_1`, `transformed_x_2`, `transformed_y_1`, and `transformed_y_2` specify
// the bounding box of the substitute-transformed glyph.
//
struct glyph_transform {
    FT_F26Dot6 substitute_translation_x, substitute_translation_y;
    ::glyph_guard glyph_clone;
    int residual_translation_x, residual_translation_y;
    int transformed_x_1, transformed_x_2;
    int transformed_y_1, transformed_y_2;
};


struct raster_context {
    FT_BBox clip_box;
    font::face::tray_type::iter_type iter;
};


void render_spans_mask(int y, int count, const FT_Span* spans, void* user) noexcept
{
    const ::raster_context& context = *static_cast<::raster_context*>(user);
    ARCHON_ASSERT(y >= context.clip_box.yMin && y < context.clip_box.yMax);
    int y_2 = int((context.clip_box.yMax - 1) - y);
    for (int i = 0; i < count; ++i) {
        const FT_Span& span = spans[i];
        ARCHON_ASSERT(int(span.x) >= context.clip_box.xMin);
        ARCHON_ASSERT(int(span.x + span.len) <= context.clip_box.xMax);
        int x_1 = int(int(span.x) - context.clip_box.xMin);
        int x_2 = x_1 + int(span.len);
        // `span.coverage` expresses pixel coverage, which is linear (not gamma compressed)
        // and can therefore be used directly as an alpha channel component.
        using comp_type = font::face::comp_type;
        comp_type alpha = util::pack_int<comp_type, 8>(span.coverage);
        for (int x = x_1; x < x_2; ++x) {
            comp_type* pixel = context.iter(x, y_2);
            pixel[0] = alpha;
        }
    }
}


void render_spans_rgba(int y, int count, const FT_Span* spans, void* user) noexcept
{
    const ::raster_context& context = *static_cast<::raster_context*>(user);
    ARCHON_ASSERT(y >= context.clip_box.yMin && y < context.clip_box.yMax);
    int y_2 = int((context.clip_box.yMax - 1) - y);
    for (int i = 0; i < count; ++i) {
        const FT_Span& span = spans[i];
        ARCHON_ASSERT(int(span.x) >= context.clip_box.xMin);
        ARCHON_ASSERT(int(span.x + span.len) <= context.clip_box.xMax);
        int x_1 = int(int(span.x) - context.clip_box.xMin);
        int x_2 = x_1 + int(span.len);
        // `span.coverage` expresses pixel coverage, which is linear (not gamma compressed)
        // and can therefore be used directly as an alpha channel component.
        using comp_type = font::face::comp_type;
        comp_type alpha = util::pack_int<comp_type, 8>(span.coverage);
        for (int x = x_1; x < x_2; ++x) {
            comp_type* pixel = context.iter(x, y_2);
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = alpha;
        }
    }
}



class loader_impl final
    : public font::loader {
public:
    const std::locale locale;
    log::Logger& logger;
    const font::freetype_subconfig subconfig;

    loader_impl(core::FilesystemPathRef path, const std::locale& locale, log::Logger& logger,
                const font::freetype_subconfig& subconfig);

    auto load_default_face() -> std::unique_ptr<font::face> override;
    int get_num_faces() override;
    auto load_face(int) -> std::unique_ptr<font::face> override;

private:
    const std::string m_path;
    ::library_guard m_library;

    bool m_have_num_faces = false;
    int m_num_faces = {};

    void ensure_num_faces();
    auto do_load_face(FT_Long face_index) -> std::unique_ptr<font::face>;
};



class face_impl final
    : public font::face {
public:
    face_impl(const ::loader_impl& loader, const ::library_guard& library, const char* path, FT_Long face_index);

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
    const ::loader_impl& m_loader;
    ::face_guard m_face;
    FT_GlyphSlot m_glyph = {};
    int m_num_fixed_sizes = {};
    std::unique_ptr<char[]> m_string_owner;
    std::string_view m_family_name;
    std::string_view m_style_name;
    core::FlatMap<::fixed_size_key, int> m_fixed_sizes_map;
    font::size m_resolution = 72; // Equates pixels and points
    ::size_spec m_requested_size;
    ::size_spec m_current_size;
    ::size_properties m_size_properties = {};
    bool m_fixed_sizes_map_initialized = false;
    bool m_valid_current_size = false;
    bool m_color_loading_enabled = false;
    bool m_dirty_glyph_slot = {};
    bool m_dirty_glyph_transform = {};

    // Initialized by `try_load_glyph()`
    ::glyph_load_params m_glyph_load_params = {};
    ::glyph_properties m_glyph_properties = {};

    matrix_type m_transformation = matrix_type::identity();
    vector_type m_translation;
    image::Pos m_target_pos;

    // Initialized by update_glyph_transformation()
    ::glyph_transform m_glyph_transform = {};

    // The translation that is currently applied to the glyph in the glyph slot
    FT_F26Dot6 m_curr_glyph_translation_x = 0, m_curr_glyph_translation_y = 0;

    void ensure_fixed_sizes_map();
    void do_set_scaled_size(font::size size);
    void set_size(const ::size_spec& spec);
    void ensure_size(const ::size_spec& spec);
    void do_set_size(const ::size_spec& spec);
    bool do_try_load_glyph(std::size_t glyph_index, bool grid_fitting, bool vertical);
    bool get_colored_outline_bbox(FT_UInt glyph_index, bool& unsupported, FT_BBox& box);
    void mark_glyph_transform_dirty() noexcept;
    auto do_get_target_glyph_box() -> image::Box;
    void ensure_glyph();
    void load_glyph(const ::glyph_load_params& params);
    void ensure_glyph_transformation();
    void update_glyph_transformation();
    void ensure_translated_glyph(FT_F26Dot6 x, FT_F26Dot6 y);

    template<int N, bool H> static void read_mask_from_alpha_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                                                    const tray_type::iter_type& iter) noexcept;
    template<int N, bool H> static void read_rgba_from_alpha_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                                                    const tray_type::iter_type& iter) noexcept;
    static void read_mask_from_bgra_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                           const tray_type::iter_type& iter) noexcept;
    static void read_rgba_from_bgra_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                           const tray_type::iter_type& iter) noexcept;
};



loader_impl::loader_impl(core::FilesystemPathRef path, const std::locale& locale_2, log::Logger& logger_2,
                         const font::freetype_subconfig& subconfig_2)
    : locale(locale_2)
    , logger(logger_2)
    , subconfig(subconfig_2)
    , m_path(core::path_to_string_native(path, locale)) // Throws
{
    m_library.init(locale); // Throws
}


auto loader_impl::load_default_face() -> std::unique_ptr<font::face>
{
    FT_Long face_index = 0;
    return do_load_face(face_index); // Throws
}


int loader_impl::get_num_faces()
{
    ensure_num_faces(); // Throws
    return m_num_faces;
}


auto loader_impl::load_face(int face_index) -> std::unique_ptr<font::face>
{
    if (ARCHON_LIKELY(face_index == 0 || (face_index >= 0 && face_index < get_num_faces())))
        return do_load_face(FT_Long(face_index)); // Throws
    throw std::invalid_argument("Face index");
}


void loader_impl::ensure_num_faces()
{
    if (m_have_num_faces)
        return;
    ::face_guard face(m_library);
    FT_Long face_index = -1; // Produce a degenerate face object
    face.init(m_path.c_str(), face_index, locale); // Throws
    core::int_cast(face.face->num_faces, m_num_faces); // Throws
    if (ARCHON_LIKELY(m_num_faces >= 1)) {
        m_have_num_faces = true;
        return;
    }
    throw std::runtime_error("No font faces in font file");
}


inline auto loader_impl::do_load_face(FT_Long face_index) -> std::unique_ptr<font::face>
{
    return std::make_unique<::face_impl>(*this, m_library, m_path.c_str(), face_index); // Throws
}



face_impl::face_impl(const ::loader_impl& loader, const ::library_guard& library, const char* path, FT_Long face_index)
    : m_loader(loader)
    , m_face(library)
{
    m_face.init(path, face_index, m_loader.locale); // Throws
    m_glyph = m_face.face->glyph;

    core::int_cast(m_face.face->num_fixed_sizes, m_num_fixed_sizes); // Throws
    bool is_scalable_or_has_fixed_sizes = (FT_IS_SCALABLE(m_face.face) || m_num_fixed_sizes > 0);
    if (ARCHON_UNLIKELY(!is_scalable_or_has_fixed_sizes))
        throw std::runtime_error("FreeType: Font face is not scalable and has no fixed sizes");

    {
        core::charenc_bridge bridge(m_loader.locale); // Throws
        char seed_mem[64] = {};
        core::Buffer<char> buffer(seed_mem);
        std::size_t buffer_offset = 0;
        std::size_t family_name_offset = buffer_offset;
        if (ARCHON_LIKELY(m_face.face->family_name))
            bridge.ascii_to_native_mb_l(std::string_view(m_face.face->family_name), buffer, buffer_offset); // Throws
        std::size_t family_name_size = std::size_t(buffer_offset - family_name_offset);
        std::size_t style_name_offset = buffer_offset;
        if (ARCHON_LIKELY(m_face.face->style_name))
            bridge.ascii_to_native_mb_l(std::string_view(m_face.face->style_name), buffer, buffer_offset); // Throws
        std::size_t style_name_size = std::size_t(buffer_offset - style_name_offset);
        if (ARCHON_LIKELY(buffer_offset > 0)) {
            std::unique_ptr<char[]> string_owner = std::make_unique_for_overwrite<char[]>(buffer_offset); // Throws
            std::copy_n(buffer.data(), buffer_offset, string_owner.get());
            std::string_view family_name = { string_owner.get() + family_name_offset, family_name_size }; // Throws
            std::string_view style_name  = { string_owner.get() + style_name_offset,  style_name_size  }; // Throws
            m_string_owner = std::move(string_owner);
            m_family_name = family_name;
            m_style_name = style_name;
        }
    }

    const char* font_format = FT_Get_Font_Format(m_face.face);
    m_loader.logger.detail("Font format: %s", font_format); // Throws

    // Implementation is obliged to set initial rendering size as close to 16 x 16 as
    // possible.
    font::size size = 16;
    set_approx_size(size); // Throws

    // Implementations are obliged to load the replacement glyph initially with grid fitting
    // enabled for horizontal layout.
    std::size_t glyph_index = 0;
    bool vertical = false;
    bool grid_fitting = true;
    bool success = try_load_glyph(glyph_index, vertical, grid_fitting); // Throws
    ARCHON_ASSERT(success); // Replacement glyph can always be loaded
}


auto face_impl::get_family_name() noexcept -> std::string_view
{
    return m_family_name;
}


auto face_impl::get_style_name() noexcept -> std::string_view
{
    return m_style_name;
}


bool face_impl::is_bold() noexcept
{
    return ((m_face.face->style_flags & FT_STYLE_FLAG_BOLD) != 0);
}


bool face_impl::is_italic() noexcept
{
    return ((m_face.face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);
}


bool face_impl::is_monospace() noexcept
{
    return FT_IS_FIXED_WIDTH(m_face.face);
}


bool face_impl::is_scalable() noexcept
{
    return FT_IS_SCALABLE(m_face.face);
}


bool face_impl::has_color() noexcept
{
    return FT_HAS_COLOR(m_face.face);
}


void face_impl::set_resolution(font::size resol) noexcept
{
    m_resolution = resol;
}


int face_impl::get_num_fixed_sizes() noexcept
{
    return m_num_fixed_sizes;
}


auto face_impl::get_fixed_size(int fixed_size_index) -> font::size
{
    bool valid = (fixed_size_index >= 0 && fixed_size_index < m_num_fixed_sizes);
    if (ARCHON_LIKELY(valid)) {
        const FT_Bitmap_Size& entry = m_face.face->available_sizes[fixed_size_index];
        return {
            fixed_26p6_to_float(entry.x_ppem),
            fixed_26p6_to_float(entry.y_ppem),
        };
    }
    throw std::out_of_range("Fixed size index");
}


void face_impl::set_fixed_size(int fixed_size_index)
{
    bool valid = (fixed_size_index >= 0 && fixed_size_index < m_num_fixed_sizes);
    if (ARCHON_LIKELY(valid)) {
        set_size(::size_spec(fixed_size_index)); // Throws
        return;
    }
    throw std::out_of_range("Fixed size index");
}


void face_impl::set_scaled_size(font::size size)
{
    if (ARCHON_LIKELY(FT_IS_SCALABLE(m_face.face))) {
        do_set_scaled_size(size); // Throws
        return;
    }
    throw std::logic_error("Font face is not scalable");
}


void face_impl::set_approx_size(font::size size)
{
    ensure_fixed_sizes_map(); // Throws

    // First, check for an exact match on a fixed size
    FT_F26Dot6 width  = float_to_fixed_26p6(size.width);
    FT_F26Dot6 height = float_to_fixed_26p6(size.height);
    ::fixed_size_key key = { width, height };
    auto i = m_fixed_sizes_map.find(key);
    if (i != m_fixed_sizes_map.end()) {
        int fixed_size_index = i->second;
        set_size(::size_spec(fixed_size_index)); // Throws
        return;
    }

    // Second, choose a scaled size if we can
    if (FT_IS_SCALABLE(m_face.face)) {
        do_set_scaled_size(size); // Throws
        return;
    }

    // Third, find the best matching fixed size
    float_type min = 0;
    int fixed_size_index = -1;
    int n = m_num_fixed_sizes;
    for (int i = 0; i < n; ++i) {
        const FT_Bitmap_Size& entry = m_face.face->available_sizes[i];
        float_type diff = (core::square(float_type(size.width)  - fixed_26p6_to_float(entry.x_ppem)) +
                           core::square(float_type(size.height) - fixed_26p6_to_float(entry.y_ppem)));
        if (ARCHON_LIKELY(fixed_size_index >= 0 && diff >= min))
            continue;
        min = diff;
        fixed_size_index = i;
    }
    ARCHON_ASSERT(fixed_size_index >= 0);
    set_size(::size_spec(fixed_size_index)); // Throws
}


auto face_impl::get_size() noexcept -> font::size
{
    return {
        fixed_26p6_to_float(m_size_properties.width),
        fixed_26p6_to_float(m_size_properties.height),
    };
}


auto face_impl::get_ascender(bool grid_fitting, bool vertical) noexcept -> float_type
{
    const ::size_properties& properties = m_size_properties;
    FT_F26Dot6 val = (grid_fitting ?
                      (!vertical ? properties.horz_ascender_gf : properties.vert_ascender_gf) :
                      (!vertical ? properties.horz_ascender    : properties.vert_ascender));
    return fixed_26p6_to_float(val);
}


auto face_impl::get_descender(bool grid_fitting, bool vertical) noexcept -> float_type
{
    const ::size_properties& properties = m_size_properties;
    FT_F26Dot6 val = (grid_fitting ?
                      (!vertical ? properties.horz_descender_gf : properties.vert_descender_gf) :
                      (!vertical ? properties.horz_descender    : properties.vert_descender));
    return fixed_26p6_to_float(val);
}


auto face_impl::get_baseline_spacing(bool grid_fitting, bool vertical) noexcept -> float_type
{
    const ::size_properties& properties = m_size_properties;
    FT_F26Dot6 val = (grid_fitting ?
                      (!vertical ? properties.horz_baseline_spacing_gf : properties.vert_baseline_spacing_gf) :
                      (!vertical ? properties.horz_baseline_spacing    : properties.vert_baseline_spacing));
    return fixed_26p6_to_float(val);
}


void face_impl::set_color_loading_enabled(bool on)
{
    m_color_loading_enabled = on;
}


auto face_impl::find_glyph(char_type ch) -> std::size_t
{
    font::code_point cp_1;
    FT_ULong cp_2 = 0;
    if (ARCHON_LIKELY(cp_1.try_from_char(ch) && core::try_int_cast(cp_1.to_int(), cp_2))) {
        FT_UInt index = FT_Get_Char_Index(m_face.face, cp_2);
        return core::int_cast<std::size_t>(index); // Throws
    }
    return 0; // Index of replacement glyph
}


auto face_impl::get_kerning(std::size_t glyph_index_1, std::size_t glyph_index_2,
                 bool grid_fitting, bool vertical) -> float_type
{
    // According to the API reference, FreeType only supports kerning for horizontal layouts
    FT_UInt glyph_index_3 = 0, glyph_index_4 = 0;
    if (ARCHON_LIKELY(!vertical &&
                      core::try_int_cast(glyph_index_1, glyph_index_3) &&
                      core::try_int_cast(glyph_index_2, glyph_index_4))) {
        FT_UInt kern_mode = (grid_fitting ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED);
        FT_Vector vec = {};
        FT_Error err = FT_Get_Kerning(m_face.face, glyph_index_3, glyph_index_4, kern_mode, &vec);
        // The existence of `vec.y`, i.e., a vertical component for the kerning
        // displacement, is weird. The FreeType documentation does not explain it, but the
        // implementation of `FTDemo_String_Load()` in `src/ftcommon.c` in the "demo
        // programs" package of FreeType 2.12.1 suggests that `vec.y` should displace the
        // cursor vertically in a horizontal layout. However, since such a displacement
        // would affect the rest of the line, it seems like an unreasonable idea. For now,
        // `vec.y` will be assumed to be zero.
        if (ARCHON_LIKELY(err == 0 && vec.y == 0))
            return fixed_26p6_to_float(vec.x);
        if (ARCHON_LIKELY(err != 0))
            throw_freetype_error(m_loader.locale, "Failed to get kerning", err); // Throws
        throw_freetype_error(m_loader.locale, "Got kerning with vertical displacement", err); // Throws
    }
    return 0;
}


bool face_impl::try_load_glyph(std::size_t glyph_index, bool grid_fitting, bool vertical)
{
    bool success = do_try_load_glyph(glyph_index, grid_fitting, vertical); // Throws
    if (ARCHON_LIKELY(success))
        return true;
    if (glyph_index == 0)
        throw std::runtime_error("FreeType: Replacement glyph has unsupported format");
    return false;
}


auto face_impl::get_glyph_advance(bool vertical) noexcept -> float_type
{
    const ::glyph_properties& properties = m_glyph_properties;
    return fixed_26p6_to_float(!vertical ? properties.horz_advance : properties.vert_advance);
}


auto face_impl::get_glyph_bearing(bool vertical) noexcept -> vector_type
{
    if (!vertical) {
        vector_type horz_bearing = { 0, 0 };
        return horz_bearing;
    }
    const ::glyph_properties& properties = m_glyph_properties;
    return {
        fixed_26p6_to_float(properties.vert_bearing_pos_x),
        fixed_26p6_to_float(properties.vert_bearing_pos_y),
    };
}


auto face_impl::get_glyph_pos() noexcept -> vector_type
{
    const ::glyph_properties& properties = m_glyph_properties;
    return {
        fixed_26p6_to_float(properties.x_1),
        fixed_26p6_to_float(properties.y_1),
    };
}


auto face_impl::get_glyph_size() noexcept -> vector_type
{
    const ::glyph_properties& properties = m_glyph_properties;
    return {
        fixed_26p6_to_float(properties.x_2 - properties.x_1),
        fixed_26p6_to_float(properties.y_2 - properties.y_1),
    };
}


bool face_impl::glyph_may_be_colored() noexcept
{
    return m_glyph_properties.may_be_colored;
}


void face_impl::set_transform(const matrix_type& transform) noexcept
{
    m_transformation = transform;
    mark_glyph_transform_dirty();
}


void face_impl::set_translat(vector_type translat) noexcept
{
    m_translation = translat;
    mark_glyph_transform_dirty();
}


void face_impl::reset_transform_translat() noexcept
{
    m_transformation = matrix_type::identity();
    m_translation = {};
    mark_glyph_transform_dirty();
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
    image::Box orig_box = box;
    if (ARCHON_UNLIKELY(!target_box.clip(box)))
        return;
    image::Size adjust = box.pos - orig_box.pos;
    tray_type::iter_type iter = tray.iter + (box.pos - pos);

    const ::glyph_transform& transform = m_glyph_transform;
    if (m_glyph->format == FT_GLYPH_FORMAT_OUTLINE)
        goto outline;
    if (ARCHON_LIKELY(m_glyph->format == FT_GLYPH_FORMAT_BITMAP))
        goto bitmap;
    ARCHON_ASSERT_UNREACHABLE();
    return;

  outline:
    {
        ARCHON_ASSERT(m_glyph_properties.orig_format == FT_GLYPH_FORMAT_OUTLINE);
        FT_Outline* outline = &m_glyph->outline;
        if (ARCHON_LIKELY(!transform.glyph_clone)) {
            ensure_translated_glyph(transform.substitute_translation_x, transform.substitute_translation_y); // Throws
        }
        else {
            ARCHON_ASSERT(transform.glyph_clone.glyph->format == FT_GLYPH_FORMAT_OUTLINE);
            FT_OutlineGlyph glyph = reinterpret_cast<FT_OutlineGlyph>(m_glyph_transform.glyph_clone.glyph);
            outline = &glyph->outline;
        }
        ::raster_context context = {};
        FT_Raster_Params params = {};
        params.flags      = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_CLIP;
        params.gray_spans = &render_spans_mask;
        params.user       = &context;
        // Note the Y-axis inversion
        params.clip_box.xMin = FT_Pos(transform.transformed_x_1 + adjust.width);
        params.clip_box.xMax = FT_Pos(params.clip_box.xMin + box.size.width);
        params.clip_box.yMax = FT_Pos(transform.transformed_y_2 - adjust.height);
        params.clip_box.yMin = FT_Pos(params.clip_box.yMax - box.size.height);
        context.clip_box = params.clip_box;
        context.iter = iter;
        FT_Error err = FT_Outline_Render(m_face.library.library, outline, &params);
        if (ARCHON_LIKELY(err == 0))
            return;
        throw_freetype_error(m_loader.locale, "Failed to rasterize uncolored FreeType glyph", err); // Throws
    }

  bitmap:
    {
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap_left, transform.transformed_x_1));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap_top, transform.transformed_y_2));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap.width, orig_box.size.width));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap.rows, orig_box.size.height));
        image::Box box_2 = { image::Pos() + adjust, box.size };
        switch (m_glyph->bitmap.pixel_mode) {
            case FT_PIXEL_MODE_MONO:
                read_mask_from_alpha_bitmap<1, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY:
                read_mask_from_alpha_bitmap<8, true>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY2:
                read_mask_from_alpha_bitmap<2, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY4:
                read_mask_from_alpha_bitmap<4, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_BGRA:
                read_mask_from_bgra_bitmap(m_glyph->bitmap, box_2, iter);
                return;
        }
        throw std::runtime_error("Unsupported pixel mode of FreeType bitmap glyph");
    }
}


void face_impl::render_glyph_rgba_a(image::Pos pos, const tray_type& tray)
{
    image::Box target_box = { pos, tray.size };
    image::Box box = do_get_target_glyph_box(); // Throws
    image::Box orig_box = box;
    if (ARCHON_UNLIKELY(!target_box.clip(box)))
        return;
    image::Size adjust = box.pos - orig_box.pos;
    tray_type::iter_type iter = tray.iter + (box.pos - pos);

    const ::glyph_transform& transform = m_glyph_transform;
    if (m_glyph->format == FT_GLYPH_FORMAT_OUTLINE)
        goto outline;
    if (ARCHON_LIKELY(m_glyph->format == FT_GLYPH_FORMAT_BITMAP))
        goto bitmap;
    ARCHON_ASSERT_UNREACHABLE();
    return;

  outline:
    {
        ARCHON_ASSERT(m_glyph_properties.orig_format == FT_GLYPH_FORMAT_OUTLINE);
        FT_Outline* outline = &m_glyph->outline;
        if (ARCHON_LIKELY(!transform.glyph_clone)) {
            ensure_translated_glyph(transform.substitute_translation_x, transform.substitute_translation_y); // Throws
        }
        else {
            ARCHON_ASSERT(transform.glyph_clone.glyph->format == FT_GLYPH_FORMAT_OUTLINE);
            FT_OutlineGlyph glyph = reinterpret_cast<FT_OutlineGlyph>(transform.glyph_clone.glyph);
            outline = &glyph->outline;
        }
        ::raster_context context = {};
        FT_Raster_Params params = {};
        params.flags      = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_CLIP;
        params.gray_spans = &render_spans_rgba;
        params.user       = &context;
        // Note the Y-axis inversion
        params.clip_box.xMin = FT_Pos(transform.transformed_x_1 + adjust.width);
        params.clip_box.xMax = FT_Pos(params.clip_box.xMin + box.size.width);
        params.clip_box.yMax = FT_Pos(transform.transformed_y_2 - adjust.height);
        params.clip_box.yMin = FT_Pos(params.clip_box.yMax - box.size.height);
        context.clip_box = params.clip_box;
        context.iter = iter;
        FT_Error err = FT_Outline_Render(m_face.library.library, outline, &params);
        if (ARCHON_LIKELY(err == 0))
            return;
        throw_freetype_error(m_loader.locale, "Failed to rasterize uncolored FreeType glyph", err); // Throws
    }

  bitmap:
    {
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap_left, transform.transformed_x_1));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap_top, transform.transformed_y_2));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap.width, orig_box.size.width));
        ARCHON_ASSERT(core::int_equal(m_glyph->bitmap.rows, orig_box.size.height));
        image::Box box_2 = { image::Pos() + adjust, box.size };
        switch (m_glyph->bitmap.pixel_mode) {
            case FT_PIXEL_MODE_MONO:
                read_rgba_from_alpha_bitmap<1, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY:
                read_rgba_from_alpha_bitmap<8, true>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY2:
                read_rgba_from_alpha_bitmap<2, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_GRAY4:
                read_rgba_from_alpha_bitmap<4, false>(m_glyph->bitmap, box_2, iter);
                return;
            case FT_PIXEL_MODE_BGRA:
                read_rgba_from_bgra_bitmap(m_glyph->bitmap, box_2, iter);
                return;
        }
        throw std::runtime_error("Unsupported pixel mode of FreeType bitmap glyph");
    }
}


void face_impl::ensure_fixed_sizes_map()
{
    if (ARCHON_LIKELY(m_fixed_sizes_map_initialized))
        return;

    int n = m_num_fixed_sizes;
    for (int i = 0; i < n; ++i) {
        const FT_Bitmap_Size& entry = m_face.face->available_sizes[i];
        ::fixed_size_key key = { entry.x_ppem, entry.y_ppem };
        m_fixed_sizes_map.emplace(key, i); // Throws
    }

    m_fixed_sizes_map_initialized = true;
}


void face_impl::do_set_scaled_size(font::size size)
{
    ARCHON_ASSERT(FT_IS_SCALABLE(m_face.face));

    // FreeType accepts only integer numbers of pixels per inch. However, because the
    // rounded resolution is used as a basis for calculating the physical font size, any
    // error introduced by rounding becomes automatically compensated for in the calculated
    // physical font size. The net result is that the specified font size is respected
    // precisely, but the resolution is generally slightly off. Since the resolution
    // generally has only subtle effects on the finally rendered glyph, the problem is
    // small.
    FT_UInt horz_pixels_per_inch = {};
    FT_UInt vert_pixels_per_inch = {};
    core::float_to_int(std::round(m_resolution.width),  horz_pixels_per_inch); // Throws
    core::float_to_int(std::round(m_resolution.height), vert_pixels_per_inch); // Throws
    if (horz_pixels_per_inch < 1)
        horz_pixels_per_inch = 1;
    if (vert_pixels_per_inch < 1)
        vert_pixels_per_inch = 1;
    float_type points_per_inch = 72;
    float_type horz_points_per_pixel = points_per_inch / float_type(horz_pixels_per_inch);
    float_type vert_points_per_pixel = points_per_inch / float_type(vert_pixels_per_inch);
    FT_F26Dot6 width  = float_to_fixed_26p6(size.width  * horz_points_per_pixel);
    FT_F26Dot6 height = float_to_fixed_26p6(size.height * vert_points_per_pixel);
    set_size(::size_spec(width, height, horz_pixels_per_inch, vert_pixels_per_inch)); // Throws
}


void face_impl::set_size(const ::size_spec& spec)
{
    ensure_size(spec); // Throws

    FT_F26Dot6 width = {}, height = {};
    ARCHON_ASSERT(m_valid_current_size);
    bool fixed_size_selected = (spec.fixed_size_index >= 0);
    if (!fixed_size_selected) {
        width  = m_current_size.width;
        height = m_current_size.height;
    }
    else {
        int fixed_size_index = m_current_size.fixed_size_index;
        ARCHON_ASSERT(fixed_size_index >= 0 && fixed_size_index < m_num_fixed_sizes);
        const FT_Bitmap_Size& entry = m_face.face->available_sizes[fixed_size_index];
        width  = entry.x_ppem;
        height = entry.y_ppem;
    }

    FT_F26Dot6 horz_ascender,    horz_descender,    horz_baseline_spacing;
    FT_F26Dot6 vert_ascender,    vert_descender,    vert_baseline_spacing;
    FT_F26Dot6 horz_ascender_gf, horz_descender_gf, horz_baseline_spacing_gf;
    FT_F26Dot6 vert_ascender_gf, vert_descender_gf, vert_baseline_spacing_gf;

    const FT_Size_Metrics& metrics = m_face.face->size->metrics;

    if (FT_IS_SCALABLE(m_face.face) && !fixed_size_selected) {
        // The scaled metrics provided by FreeType have been rounded to integer values, so,
        // for maximum precision, we need to do the scaling manually
        horz_ascender         = FT_MulFix(m_face.face->ascender, metrics.y_scale);
        horz_descender        = FT_MulFix(m_face.face->descender, metrics.y_scale);
        horz_baseline_spacing = FT_MulFix(m_face.face->height, metrics.y_scale);

        bool have_native_vertical_metrics = false;
        if (FT_HAS_VERTICAL(m_face.face)) {
            if (TT_VertHeader* vhea = static_cast<TT_VertHeader*>(FT_Get_Sfnt_Table(m_face.face, FT_SFNT_VHEA))) {
                vert_ascender         = FT_MulFix(vhea->Ascender, metrics.x_scale);
                vert_descender        = FT_MulFix(vhea->Descender, metrics.x_scale);
                vert_baseline_spacing = FT_MulFix(vhea->Ascender - vhea->Descender + vhea->Line_Gap, metrics.x_scale);
                have_native_vertical_metrics = true;
            }
        }

        // Synthesize vertical metrics
        if (!have_native_vertical_metrics) {
            FT_F26Dot6 vert_size = FT_MulFix(m_face.face->ascender - m_face.face->descender, metrics.x_scale);
            vert_ascender         = vert_size / 2;
            vert_descender        = vert_ascender - vert_size;
            vert_baseline_spacing = FT_MulFix(m_face.face->height, metrics.x_scale);
        }

        const char* font_format = FT_Get_Font_Format(m_face.face);
        if (ARCHON_UNLIKELY(!font_format))
            throw std::runtime_error("FreeType: Failed to get font format");
        bool is_true_type_with_native_hinting = (std::string_view(font_format) == "TrueType" &&
                                                 !m_loader.subconfig.force_autohint);
        if (!is_true_type_with_native_hinting) {
            // In this case, we use the scaled and hinted metrics provided by
            // FreeType. These are guaranteed to have integer values. The hope is that these
            // will incorporate the effect of glyph hinting in a way that would not be
            // possible to emulate by simple rounding.

            ARCHON_ASSERT(fixed_26p6_round(metrics.ascender) == metrics.ascender);
            ARCHON_ASSERT(fixed_26p6_round(metrics.descender) == metrics.descender);
            ARCHON_ASSERT(fixed_26p6_round(metrics.height) == metrics.height);

            horz_ascender_gf         = metrics.ascender;
            horz_descender_gf        = metrics.descender;
            horz_baseline_spacing_gf = metrics.height;

            vert_ascender_gf         = fixed_26p6_round(vert_ascender);
            vert_descender_gf        = fixed_26p6_round(vert_descender);
            vert_baseline_spacing_gf = fixed_26p6_round(vert_baseline_spacing);
        }
        else {
            // For TrueType fonts when using native hinting, the hinted metrics provided by
            // FreeType are assumed to be broken. For this reason, we are forced to emulate
            // the hinted metrics using simple rounding.

            horz_ascender_gf         = fixed_26p6_ceil(horz_ascender);
            horz_descender_gf        = fixed_26p6_floor(horz_descender);
            horz_baseline_spacing_gf = fixed_26p6_round(horz_baseline_spacing);

            vert_ascender_gf         = fixed_26p6_ceil(vert_ascender);
            vert_descender_gf        = fixed_26p6_floor(vert_descender);
            vert_baseline_spacing_gf = fixed_26p6_round(vert_baseline_spacing);
        }
    }
    else {
        // For non-scalable font faces, the global metrics in the FT_Face object are
        // unavailable. They need to be fetched from the FT_Size_Metrics object. FreeType
        // guarantees that these metrics have integer values.

        ARCHON_ASSERT(fixed_26p6_round(metrics.ascender) == metrics.ascender);
        ARCHON_ASSERT(fixed_26p6_round(metrics.descender) == metrics.descender);
        ARCHON_ASSERT(fixed_26p6_round(metrics.height) == metrics.height);

        // Synthesize vertical metrics
        FT_F26Dot6 vert_size = fixed_26p6_round(FT_MulDiv(metrics.ascender - metrics.descender,
                                                          metrics.x_ppem, metrics.y_ppem));

        horz_ascender         = metrics.ascender;
        horz_descender        = metrics.descender;
        horz_baseline_spacing = metrics.height;

        vert_ascender         = fixed_26p6_round(vert_size / 2);
        vert_descender        = vert_ascender - vert_size;
        vert_baseline_spacing = fixed_26p6_round(FT_MulDiv(metrics.height, metrics.x_ppem, metrics.y_ppem));

        horz_ascender_gf         = horz_ascender;
        horz_descender_gf        = horz_descender;
        horz_baseline_spacing_gf = horz_baseline_spacing;

        vert_ascender_gf         = vert_ascender;
        vert_descender_gf        = vert_descender;
        vert_baseline_spacing_gf = vert_baseline_spacing;
    }

    ::size_properties properties = {
        width, height,
        horz_ascender,    horz_descender,    horz_baseline_spacing,
        vert_ascender,    vert_descender,    vert_baseline_spacing,
        horz_ascender_gf, horz_descender_gf, horz_baseline_spacing_gf,
        vert_ascender_gf, vert_descender_gf, vert_baseline_spacing_gf,
    };

    m_size_properties = properties;
    m_requested_size = spec;
}


inline void face_impl::ensure_size(const ::size_spec& spec)
{
    if (ARCHON_LIKELY(m_valid_current_size && spec == m_current_size))
        return;
    do_set_size(spec); // Throws
}


void face_impl::do_set_size(const ::size_spec& spec)
{
    m_valid_current_size = false;
    bool scaled_size = (spec.fixed_size_index < 0);
    if (scaled_size) {
        ARCHON_ASSERT(FT_IS_SCALABLE(m_face.face));
        FT_Error err = FT_Set_Char_Size(m_face.face, spec.width, spec.height, spec.horz_resol, spec.vert_resol);
        if (ARCHON_UNLIKELY(err != 0))
            throw_freetype_error(m_loader.locale, "Failed to set scaled size", err); // Throws
    }
    else {
        ARCHON_ASSERT(spec.fixed_size_index >= 0 && spec.fixed_size_index < m_num_fixed_sizes);
        FT_Error err = FT_Select_Size(m_face.face, FT_Int(spec.fixed_size_index));
        if (ARCHON_UNLIKELY(err != 0))
            throw_freetype_error(m_loader.locale, "Failed to select fixed size", err); // Throws
    }
    m_current_size = spec;
    m_valid_current_size = true;
}


bool face_impl::do_try_load_glyph(std::size_t glyph_index, bool grid_fitting, bool vertical)
{
    FT_UInt glyph_index_2 = 0;
    if (ARCHON_UNLIKELY(!core::try_int_cast(glyph_index, glyph_index_2) ||
                        !core::int_less(glyph_index_2, m_face.face->num_glyphs)))
        throw std::out_of_range("glyph_index");

    ensure_size(m_requested_size); // Throws

    FT_Set_Transform(m_face.face, nullptr, nullptr);

    // Figure out if the glyph is a colored outline glyph (COLRv0 or COLRv1). This needs to
    // be known before the loading of the glyph, because colored outline glyphs are
    // incompatible with hinting, so it needs to be known whether hinting has to be disabled
    // for that reason.
    //
    // Because the determination of the bounding box of a colored outline glyph may clobber
    // the glyph slot, that determination is also performed before the loading of the
    // glyph. Note that this determination is performed even when a fixed size has been
    // selected. If the glyph is ultimately loaded from a bitmap strike, the bounding box
    // determination goes unused, but that is deemed to be an acceptable trade-off.
    //
    FT_BBox colored_outline_box = {};
    bool is_colored_outline_glyph = false;
    if (m_color_loading_enabled && FT_IS_SCALABLE(m_face.face) && FT_HAS_COLOR(m_face.face)) {
        bool unsupported = {};
        if (get_colored_outline_bbox(glyph_index_2, unsupported, colored_outline_box)) { // Throws
            if (ARCHON_UNLIKELY(unsupported))
                return false;
            is_colored_outline_glyph = true;
        }
    }

    ::glyph_load_params params = {
        m_requested_size,
        glyph_index_2,
        m_color_loading_enabled,
        is_colored_outline_glyph,
        grid_fitting,
        vertical,
    };

    load_glyph(params); // Throws

    // Determine glyph metrics and whether glyph may be colored
    FT_F26Dot6 width          = m_glyph->metrics.width;
    FT_F26Dot6 height         = m_glyph->metrics.height;
    FT_F26Dot6 horz_bearing_x = m_glyph->metrics.horiBearingX;
    FT_F26Dot6 horz_bearing_y = m_glyph->metrics.horiBearingY;
    FT_F26Dot6 vert_bearing_x = m_glyph->metrics.vertBearingX;
    FT_F26Dot6 vert_bearing_y = m_glyph->metrics.vertBearingY;
    FT_F26Dot6 horz_advance   = m_glyph->metrics.horiAdvance;
    FT_F26Dot6 vert_advance   = m_glyph->metrics.vertAdvance;
    bool glyph_may_be_colored = false;
    switch (m_glyph->format) {
        case FT_GLYPH_FORMAT_BITMAP:
            // FreeType apparently does not always synthesize vertical metrics for bitmap strikes
            // that lack genuine vertical metrics.
            if (ARCHON_UNLIKELY(vert_advance == 0)) {
                bool fixed_size_selected = (m_current_size.fixed_size_index >= 0);
                ARCHON_ASSERT(fixed_size_selected);
                FT_F26Dot6 baseline_spacing =  m_face.face->size->metrics.height;
                vert_bearing_x = FT_F26Dot6(horz_bearing_x - (horz_advance / 2));
                vert_bearing_y = FT_F26Dot6((baseline_spacing - height) / 2);
                vert_advance = baseline_spacing;
            }

            glyph_may_be_colored = (m_glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);
            break;

        case FT_GLYPH_FORMAT_OUTLINE:
            // For colored outline glyphs, FreeType bases the regular metrics on the
            // uncolored fallback glyphs. In many cases, however, those fallback glyphs are
            // left empty or mostly empty, causing the regular metrics to be useless in a
            // colored context. `metrics.horiAdvance` can be trusted, however, and so can
            // `metrics.vertAdvance` if FT_HAS_VERTICAL() evaluates to true.
            if (is_colored_outline_glyph) {
                width          = colored_outline_box.xMax - colored_outline_box.xMin;
                height         = colored_outline_box.yMax - colored_outline_box.yMin;
                horz_bearing_x = colored_outline_box.xMin;
                horz_bearing_y = colored_outline_box.yMax;
                if (!FT_HAS_VERTICAL(m_face.face)) {
                    FT_F26Dot6 baseline_spacing =  m_face.face->size->metrics.height;
                    vert_advance = baseline_spacing;
                }
                vert_bearing_x = FT_F26Dot6(horz_bearing_x - (horz_advance / 2));
                vert_bearing_y = FT_F26Dot6((vert_advance - height) / 2);
                glyph_may_be_colored = true;
            }
            break;

        default:
            // Unexpected unsupported glyph format
            return false;
    }

    // Determine the absolute position of the bearing point for vertical layout. The bearing
    // point is the point on the baseline that corresponds to the cursor position to the
    // left of the glyph in the case of horizontal layout, or below the glyph in case of
    // vertical layout.
    //
    // FreeType always loads a glyph such that the bearing point for horizontal layout is at
    // (0, 0). Therefore, we do not need to store the position of that bearing point.
    //
    FT_F26Dot6 vert_bearing_pos_x = horz_bearing_x - vert_bearing_x;
    FT_F26Dot6 vert_bearing_pos_y = horz_bearing_y + vert_bearing_y - vert_advance;

    FT_F26Dot6 x_1 = horz_bearing_x;
    FT_F26Dot6 x_2 = horz_bearing_x + width;
    FT_F26Dot6 y_1 = horz_bearing_y - height;
    FT_F26Dot6 y_2 = horz_bearing_y;

    // Grid fitting (rounding to integer values) of the glyph metrics will normally already
    // have been done by FreeType, but since it is not guaranteed, it is repeated
    // here. Fortunately, rounding is an idempotent operation.
    //
    if (grid_fitting) {
        horz_advance = fixed_26p6_round(horz_advance);
        vert_advance = fixed_26p6_round(vert_advance);

        vert_bearing_pos_x = fixed_26p6_round(vert_bearing_pos_x);
        vert_bearing_pos_y = fixed_26p6_round(vert_bearing_pos_y);

        x_1 = fixed_26p6_floor(x_1);
        x_2 = fixed_26p6_ceil(x_2);
        y_1 = fixed_26p6_floor(y_1);
        y_2 = fixed_26p6_ceil(y_2);
    }

    ::glyph_properties properties = {
        horz_advance, vert_advance,
        vert_bearing_pos_x, vert_bearing_pos_y,
        x_1, x_2, y_1, y_2,
        m_glyph->format,
        glyph_may_be_colored,
    };

    m_glyph_load_params = params;
    m_glyph_properties = properties;
    m_dirty_glyph_slot = false;

    mark_glyph_transform_dirty();
    return true;
}


bool face_impl::get_colored_outline_bbox(FT_UInt glyph_index, bool& unsupported, FT_BBox& box)
{
    // WARNING: This function may clobber the glyph slot!

    // Check for COLRv1 via ClipBox (available in FreeType >= 2.11.1)
#if IS_FREETYPE_VERSION_AT_LEAST(2, 11, 1)
    {
        FT_ClipBox box_2;
        if (FT_Get_Color_Glyph_ClipBox(m_face.face, glyph_index, &box_2)) {
            // As of version 2.14, FreeType is not able to rasterize COLRv1 glyphs
            unsupported = true;
            return true;
        }
    }
#endif

    // Check for COLRv0 via Layer Iteration (available in FreeType >= 2.10.0)
    FT_UInt glyph_index_2 = {};
    FT_UInt color_index = {}; // Dummy
    FT_LayerIterator iterator = {};
    if (FT_Get_Color_Glyph_Layer(m_face.face, glyph_index, &glyph_index_2, &color_index, &iterator)) {
        FT_Pos min = core::int_min<FT_Pos>();
        FT_Pos max = core::int_max<FT_Pos>();
        FT_Pos x_min = max;
        FT_Pos y_min = max;
        FT_Pos x_max = min;
        FT_Pos y_max = min;
        do {
            FT_Int32 flags = FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
            FT_Error err = FT_Load_Glyph(m_face.face, glyph_index_2, flags);
            if (ARCHON_UNLIKELY(err != 0))
                throw_freetype_error(m_loader.locale, "Failed to load glyph layer", err); // Throws
            FT_BBox box_2 = {};
            FT_Outline_Get_CBox(&m_glyph->outline, &box_2);
            x_min = std::min(x_min, box_2.xMin);
            y_min = std::min(y_min, box_2.yMin);
            x_max = std::max(x_max, box_2.xMax);
            y_max = std::max(y_max, box_2.yMax);
        }
        while (FT_Get_Color_Glyph_Layer(m_face.face, glyph_index, &glyph_index_2, &color_index, &iterator));
        box = { x_min, y_min, x_max, y_max };
        return true;
    }

    // No colored outline glyph found
    return false;
}


void face_impl::mark_glyph_transform_dirty() noexcept
{
    m_dirty_glyph_transform = true;
    m_glyph_transform.glyph_clone = {};
}


auto face_impl::do_get_target_glyph_box() -> image::Box
{
    ensure_glyph_transformation(); // Throws

    const ::glyph_transform& transform = m_glyph_transform;

    image::Pos pos = m_target_pos;
    core::int_add(pos.x, transform.transformed_x_1); // Throws
    core::int_sub(pos.y, transform.transformed_y_2); // Throws
    core::int_add(pos.x, transform.residual_translation_x); // Throws
    core::int_sub(pos.y, transform.residual_translation_y); // Throws

    image::Size size = {
        transform.transformed_x_2 - transform.transformed_x_1,
        transform.transformed_y_2 - transform.transformed_y_1,
    };

    return { pos, size };
}


inline void face_impl::ensure_glyph()
{
    if (ARCHON_LIKELY(!m_dirty_glyph_slot))
        return;
    FT_Set_Transform(m_face.face, nullptr, nullptr);
    load_glyph(m_glyph_load_params); // Throws
    m_dirty_glyph_slot = false;
}


void face_impl::load_glyph(const ::glyph_load_params& params)
{
    ensure_size(params.size); // Throws
    bool fixed_size_selected = (params.size.fixed_size_index >= 0);

    m_dirty_glyph_slot = true;

    FT_Int32 flags = 0;
    if (FT_IS_SCALABLE(m_face.face)) {
        // Colored outline glyphs are not compatible with hinting. Glyph metrics must still
        // be grid-fitted when grid-fitting is requested, however.
        bool hinting = (params.grid_fitting && !params.is_colored_outline);
        if (hinting) {
            if (m_loader.subconfig.force_autohint)
                flags |= FT_LOAD_FORCE_AUTOHINT;
            if (params.vertical)
                flags |= FT_LOAD_VERTICAL_LAYOUT;
        }
        else {
            flags |= FT_LOAD_NO_HINTING;
        }

        // It is important that bitmap glyphs are not loaded when transformation might be
        // needed. Transformation might be needed when the font face is scalable and
        // grid-fitting is disabled.
        //
        // We also do not want to get a bitmap glyph from a scalable font face unless a
        // specific fixed size bitmap strike has been selected. This ensures that the manual
        // fixed size selection logic is strictly respected.
        //
        // Note that this logic allows bitmap glyphs even when hinting is disabled because
        // the glyph is colored and scalable. This allows for a bitmap strike to be used by
        // FT_Load_Glyph() even when the glyph is a colored and scalable.
        //
        bool allow_bitmap = (params.grid_fitting && fixed_size_selected);
        if (!allow_bitmap)
            flags |= FT_LOAD_NO_BITMAP;
    }

    if (params.color_loading_enabled)
        flags |= FT_LOAD_COLOR;

    FT_Error err = FT_Load_Glyph(m_face.face, params.glyph_index, flags);
    if (ARCHON_UNLIKELY(err != 0))
        throw_freetype_error(m_loader.locale, "Failed to load glyph", err); // Throws

    m_curr_glyph_translation_x = 0;
    m_curr_glyph_translation_y = 0;
}


inline void face_impl::ensure_glyph_transformation()
{
    if (ARCHON_LIKELY(!m_dirty_glyph_transform))
        return;
    update_glyph_transformation(); // Throws
}


void face_impl::update_glyph_transformation()
{
    ARCHON_ASSERT(m_dirty_glyph_transform);

    FT_F26Dot6 substitute_translation_x = 0, substitute_translation_y = 0;
    ::glyph_guard glyph_clone;
    int residual_translation_x = {}, residual_translation_y = {};
    int transformed_x_1, transformed_x_2;
    int transformed_y_1, transformed_y_2;

    if (m_glyph_properties.orig_format == FT_GLYPH_FORMAT_OUTLINE) {
        matrix_type transformation = m_transformation;
        vector_type translation = m_translation;
        if (m_glyph_load_params.grid_fitting) {
            translation = {
                std::round(translation[0]),
                std::round(translation[1]),
            };
            // When grid fitting is enabled, transformation is effectively disabled
            transformation = matrix_type::identity();
        }
        if (!m_glyph_properties.may_be_colored) {
            //
            // Case: Un-colored scalable glyph
            //
            // In this case, a degenerate transformation is one that only involves
            // translation. Such a transformation is perfectly reversible, so the glyph does
            // not have to be cloned.
            //
            bool degen_transform = (transformation == matrix_type::identity());
            if (degen_transform) {
                // Subtraction of 1 is safety margin of 1 pixel for coordinate
                // non-negativity during rasterization
                residual_translation_x = int(fixed_26p6_to_int_floor(m_glyph_properties.x_1)) - 1;
                residual_translation_y = int(fixed_26p6_to_int_floor(m_glyph_properties.y_1)) - 1;
                int translation_x = {}, translation_y = {};
                core::float_to_int(std::floor(translation[0]), translation_x); // Throws
                core::float_to_int(std::floor(translation[1]), translation_y); // Throws
                core::int_add(residual_translation_x, translation_x); // Throws
                core::int_add(residual_translation_y, translation_y); // Throws
                substitute_translation_x = float_to_fixed_26p6(translation[0] - float_type(residual_translation_x));
                substitute_translation_y = float_to_fixed_26p6(translation[1] - float_type(residual_translation_y));
                transformed_x_1 = int(fixed_26p6_to_int_floor(m_glyph_properties.x_1 + substitute_translation_x));
                transformed_x_2 = int(fixed_26p6_to_int_ceil(m_glyph_properties.x_2 + substitute_translation_x));
                transformed_y_1 = int(fixed_26p6_to_int_floor(m_glyph_properties.y_1 + substitute_translation_y));
                transformed_y_2 = int(fixed_26p6_to_int_ceil(m_glyph_properties.y_2 + substitute_translation_y));
            }
            else {
                ensure_translated_glyph(0, 0); // Throws
                FT_Error err = FT_Get_Glyph(m_glyph, &glyph_clone.glyph);
                if (ARCHON_UNLIKELY(err != 0))
                    throw_freetype_error(m_loader.locale, "FT_Get_Glyph() failed", err); // Throws
                ARCHON_ASSERT(glyph_clone.glyph->format == FT_GLYPH_FORMAT_OUTLINE);
                FT_OutlineGlyph glyph_2 = reinterpret_cast<FT_OutlineGlyph>(glyph_clone.glyph);
                FT_Matrix matrix = {
                    float_to_fixed_16p16(transformation[0][0]),
                    float_to_fixed_16p16(transformation[0][1]),
                    float_to_fixed_16p16(transformation[1][0]),
                    float_to_fixed_16p16(transformation[1][1]),
                };
                FT_Outline_Transform(&glyph_2->outline, &matrix);
                FT_BBox box = {};
                FT_Outline_Get_CBox(&glyph_2->outline, &box);
                // Subtraction of 1 is safety margin of 1 pixel for coordinate
                // non-negativity during rasterization
                residual_translation_x = int(fixed_26p6_to_int_floor(box.xMin)) - 1;
                residual_translation_y = int(fixed_26p6_to_int_floor(box.yMin)) - 1;
                int translation_x = {}, translation_y = {};
                core::float_to_int(std::floor(translation[0]), translation_x); // Throws
                core::float_to_int(std::floor(translation[1]), translation_y); // Throws
                core::int_add(residual_translation_x, translation_x); // Throws
                core::int_add(residual_translation_y, translation_y); // Throws
                FT_F26Dot6 substitute_translation_x_2 =
                    float_to_fixed_26p6(translation[0] - float_type(residual_translation_x));
                FT_F26Dot6 substitute_translation_y_2 =
                    float_to_fixed_26p6(translation[1] - float_type(residual_translation_y));
                transformed_x_1 = int(fixed_26p6_to_int_floor(box.xMin + substitute_translation_x_2));
                transformed_x_2 = int(fixed_26p6_to_int_ceil(box.xMax + substitute_translation_x_2));
                transformed_y_1 = int(fixed_26p6_to_int_floor(box.yMin + substitute_translation_y_2));
                transformed_y_2 = int(fixed_26p6_to_int_ceil(box.yMax + substitute_translation_y_2));
                FT_Outline_Translate(&glyph_2->outline, substitute_translation_x_2, substitute_translation_y_2);
            }
        }
        else {
            //
            // Case: Colored scalable glyph
            //
            // In this case, a degenerate transformation is one that only involves
            // translation, and only by integer amounts along both axes. Such a
            // transformation commutes with rasterization, so the glyph does not have to be
            // reloaded with a new load-time transformation here.
            //
            core::float_to_int(std::floor(translation[0]), residual_translation_x); // Throws
            core::float_to_int(std::floor(translation[1]), residual_translation_y); // Throws
            FT_F26Dot6 substitute_translation_x_2 =
                float_to_fixed_26p6(translation[0] - float_type(residual_translation_x));
            FT_F26Dot6 substitute_translation_y_2 =
                float_to_fixed_26p6(translation[1] - float_type(residual_translation_y));
            bool degen_transform = (transformation == matrix_type::identity() &&
                                    substitute_translation_x_2 == 0 && substitute_translation_y_2 == 0);
            if (degen_transform) {
                if (m_glyph->format != FT_GLYPH_FORMAT_OUTLINE)
                    m_dirty_glyph_slot = true;
                ensure_glyph(); // Throws
            }
            else {
                FT_Matrix matrix = {
                    float_to_fixed_16p16(transformation[0][0]),
                    float_to_fixed_16p16(transformation[0][1]),
                    float_to_fixed_16p16(transformation[1][0]),
                    float_to_fixed_16p16(transformation[1][1]),
                };
                FT_Vector delta = { substitute_translation_x_2, substitute_translation_y_2 };
                FT_Set_Transform(m_face.face, &matrix, &delta);
                load_glyph(m_glyph_load_params); // Throws
            }
            FT_Error err = FT_Render_Glyph(m_glyph, FT_RENDER_MODE_NORMAL);
            if (ARCHON_UNLIKELY(err != 0))
                throw_freetype_error(m_loader.locale, "Failed to rasterize colored glyph", err); // Throws
            transformed_x_1 = int(m_glyph->bitmap_left);
            transformed_x_2 = int(m_glyph->bitmap_left + m_glyph->bitmap.width);
            transformed_y_1 = int(m_glyph->bitmap_top - m_glyph->bitmap.rows);
            transformed_y_2 = int(m_glyph->bitmap_top);
        }
    }
    else {
        //
        // Case: Non-scalable glyph
        //
        ARCHON_ASSERT(m_glyph_properties.orig_format == FT_GLYPH_FORMAT_BITMAP);
        core::float_to_int(std::round(m_translation[0]), residual_translation_x); // Throws
        core::float_to_int(std::round(m_translation[1]), residual_translation_y); // Throws
        transformed_x_1 = int(fixed_26p6_to_int_floor(m_glyph_properties.x_1));
        transformed_x_2 = int(fixed_26p6_to_int_ceil(m_glyph_properties.x_2));
        transformed_y_1 = int(fixed_26p6_to_int_floor(m_glyph_properties.y_1));
        transformed_y_2 = int(fixed_26p6_to_int_ceil(m_glyph_properties.y_2));
    }

    ::glyph_transform transform = {
        substitute_translation_x, substitute_translation_y,
        std::move(glyph_clone),
        residual_translation_x, residual_translation_y,
        transformed_x_1, transformed_x_2,
        transformed_y_1, transformed_y_2,
    };

    m_glyph_transform = std::move(transform);
    m_dirty_glyph_transform = false;
}


void face_impl::ensure_translated_glyph(FT_F26Dot6 x, FT_F26Dot6 y)
{
    ARCHON_ASSERT(!m_glyph_properties.may_be_colored);
    ensure_glyph(); // Throws
    ARCHON_ASSERT(m_glyph->format == FT_GLYPH_FORMAT_OUTLINE);
    FT_F26Dot6 x_2 = x - m_curr_glyph_translation_x;
    FT_F26Dot6 y_2 = y - m_curr_glyph_translation_y;
    if (x_2 != 0 || y_2 != 0) {
        FT_Outline_Translate(&m_glyph->outline, x_2, y_2);
        m_curr_glyph_translation_x = x_2;
        m_curr_glyph_translation_y = y_2;
    }
}


template<int N, bool H> void face_impl::read_mask_from_alpha_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                                                    const tray_type::iter_type& iter) noexcept
{
    constexpr int bits_per_pixel = N;
    constexpr bool has_num_grays = H;
    constexpr int pixels_per_byte = 8 / bits_per_pixel;
    static_assert(8 % bits_per_pixel == 0);
    constexpr int mask = core::int_mask<int>(bits_per_pixel);
    int max = mask;
    if (has_num_grays) {
        ARCHON_ASSERT(bitmap.num_grays >= 2 && bitmap.num_grays - 1 <= mask);
        max = int(bitmap.num_grays - 1);
    }
    for (int y = 0; y < box.size.height; ++y) {
        unsigned char* base = bitmap.buffer + (box.pos.y + y) * bitmap.pitch;
        for (int x = 0; x < box.size.width; ++x) {
            int x_2 = box.pos.x + x;
            int byte_offset = x_2 / pixels_per_byte;
            int pixel_index = x_2 % pixels_per_byte;
            // FreeType pixel modes use "big-endian" bit ordering
            int rev_pixel_index = (pixels_per_byte - 1 - pixel_index);
            int bit_position = rev_pixel_index * bits_per_pixel;
            int value = (int(base[byte_offset]) >> bit_position) & mask;
            namespace uf = util::unit_frac;
            int alpha = uf::int_to_int<8, 8>(value, max, 255);
            comp_type* pixel = iter(x, y);
            pixel[0] = util::pack_int<comp_type, 8>(alpha);
        }
    }
}


template<int N, bool H> void face_impl::read_rgba_from_alpha_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                                                    const tray_type::iter_type& iter) noexcept
{
    constexpr int bits_per_pixel = N;
    constexpr bool has_num_grays = H;
    constexpr int pixels_per_byte = 8 / bits_per_pixel;
    static_assert(8 % bits_per_pixel == 0);
    constexpr int mask = core::int_mask<int>(bits_per_pixel);
    int max = mask;
    if (has_num_grays) {
        ARCHON_ASSERT(bitmap.num_grays >= 2 && bitmap.num_grays - 1 <= mask);
        max = int(bitmap.num_grays - 1);
    }
    for (int y = 0; y < box.size.height; ++y) {
        unsigned char* base = bitmap.buffer + (box.pos.y + y) * bitmap.pitch;
        for (int x = 0; x < box.size.width; ++x) {
            int x_2 = box.pos.x + x;
            int byte_offset = x_2 / pixels_per_byte;
            int pixel_index = x_2 % pixels_per_byte;
            // FreeType pixel modes use "big-endian" bit ordering
            int rev_pixel_index = (pixels_per_byte - 1 - pixel_index);
            int bit_position = rev_pixel_index * bits_per_pixel;
            int value = (int(base[byte_offset]) >> bit_position) & mask;
            namespace uf = util::unit_frac;
            int alpha = uf::int_to_int<8, 8>(value, max, 255);
            comp_type* pixel = iter(x, y);
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = util::pack_int<comp_type, 8>(alpha);
        }
    }
}


void face_impl::read_mask_from_bgra_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                           const tray_type::iter_type& iter) noexcept
{
    // The following is a precise imitation of the integer-arithmetic-only conversion from
    // BGRA to coverage value as it happens inside FreeType 2.14.2 (in
    // ft_gray_for_premultiplied_srgb_bgra() in src/base/ftbitmap.c).
    //
    // Except for the fact that it replaces the sRGB "gamma curve" (approx 2.2) with a
    // simple squaring operation (gamma of 2), this implementation is equivalent to a
    // process that converts the specified integer components to floating-point form, undoes
    // alpha-premultiplication (FreeType applies alpha pre-multiplication in the
    // gamma-compressed domain), undoes sRGB gamma compression, converts from linear RGB to
    // luminance, computes a darkness value as one minus luminance, computes an final alpha
    // value as the original alpha value times the darkness value, and finally converts the
    // floating-point value back to integer form.

    using uint32_type = std::uint_fast32_t;
    using uint64_type = std::uint_fast64_t;

    // High-precision sRGB luminance coefficients for red and blue channels derived by Bruce
    // Lindbloom, multiplied by 1'000'000 to avoid floating-point arithmetic.
    constexpr uint32_type r_coef = 212656;
    constexpr uint32_type b_coef =  72186;

    // Convert to 16.16 fixed point form. Green, because it has the largest value, is
    // computed by subtracting the others from 65'536. This ensures that the three add up to
    // 65'536, and it ensures that we recover exactly the values used by FreeType.
    constexpr uint32_type r_weight = uint32_type((uint64_type(r_coef) * 65'536 + 500'000) / 1'000'000); // 13'937
    constexpr uint32_type b_weight = uint32_type((uint64_type(b_coef) * 65'536 + 500'000) / 1'000'000); //  4'731
    constexpr uint32_type g_weight = uint32_type(65'536 - r_weight - b_weight);                         // 46'868

    auto convert = [](int r, int g, int b, int a) noexcept -> int {
        if (ARCHON_LIKELY(a == 0))
            return 0;
        uint32_type r_2 = uint32_type(r) * r;
        uint32_type g_2 = uint32_type(g) * g;
        uint32_type b_2 = uint32_type(b) * b;
        uint32_type l = (r_weight * r_2 + g_weight * g_2 + b_weight * b_2) / 65'536;
        return int(a - l / a);
    };

    for (int y = 0; y < box.size.height; ++y) {
        unsigned char* base = bitmap.buffer + (box.pos.y + y) * bitmap.pitch;
        for (int x = 0; x < box.size.width; ++x) {
            unsigned char* pixel_1 = base + (box.pos.x + x) * 4;
            int blue  = int(util::unpack_int<8>(pixel_1[0]));
            int green = int(util::unpack_int<8>(pixel_1[1]));
            int red   = int(util::unpack_int<8>(pixel_1[2]));
            int alpha = int(util::unpack_int<8>(pixel_1[3]));
            int alpha_2 = convert(red, green, blue, alpha);
            comp_type* pixel_2 = iter(x, y);
            pixel_2[0] = util::pack_int<comp_type, 8>(alpha_2);
        }
    }
}


void face_impl::read_rgba_from_bgra_bitmap(const FT_Bitmap& bitmap, const image::Box& box,
                                           const tray_type::iter_type& iter) noexcept
{
    for (int y = 0; y < box.size.height; ++y) {
        unsigned char* base = bitmap.buffer + (box.pos.y + y) * bitmap.pitch;
        for (int x = 0; x < box.size.width; ++x) {
            unsigned char* pixel_1 = base + (box.pos.x + x) * 4;
            // Alpha pre-multiplication needs to be undone. FreeType applies it in the
            // gamma-compressed domain.
            using type = unsigned;
            type blue  = type(util::unpack_int<8>(pixel_1[0]));
            type green = type(util::unpack_int<8>(pixel_1[1]));
            type red   = type(util::unpack_int<8>(pixel_1[2]));
            type alpha = type(util::unpack_int<8>(pixel_1[3]));
            auto unpremultiply = [&](type val) noexcept -> type {
                if (ARCHON_LIKELY(alpha == 0))
                    return 0;
                if (ARCHON_LIKELY(alpha == 255))
                    return val;
                static_assert(255 * 255 <= core::int_max<type>());
                return core::int_div_round_half_up(val * 255, alpha);
            };
            comp_type* pixel_2 = iter(x, y);
            pixel_2[0] = util::pack_int<comp_type, 8>(unpremultiply(red));
            pixel_2[1] = util::pack_int<comp_type, 8>(unpremultiply(green));
            pixel_2[2] = util::pack_int<comp_type, 8>(unpremultiply(blue));
            pixel_2[3] = util::pack_int<comp_type, 8>(alpha);
        }
    }
}



auto new_loader_from_font_file(core::FilesystemPathRef file, const std::locale& locale,
                               const font::loader::config& config) -> std::unique_ptr<::loader_impl>
{
    log::Logger& logger = log::Logger::or_null(config.logger);
    font::freetype_subconfig subconfig;
    config.sub.get(subconfig);
    return std::make_unique<::loader_impl>(file, locale, logger, subconfig); // Throws
}


auto new_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                const font::loader::config& config) -> std::unique_ptr<font::loader>
{
//    std::string_view file_name = "liberation-sans-regular.ttf";                           
    std::string_view file_name = "liberation-mono-regular.ttf";                           
    namespace fs = std::filesystem;
    fs::path file = resource_dir / core::make_fs_path_generic(file_name, locale); // Throws
    return ::new_loader_from_font_file(file, locale, config); // Throws
}


#endif // ARCHON_FONT_HAVE_FREETYPE


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


auto implementation_impl::get_ident() const noexcept -> std::string_view
{
    return "freetype";
}


auto implementation_impl::get_descr() const noexcept -> std::string_view
{
    return "FreeType font rendering library";
}


bool implementation_impl::is_available() const noexcept
{
#if ARCHON_FONT_HAVE_FREETYPE
    return true;
#else
    return false;
#endif
}


auto implementation_impl::new_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                                     const font::loader::config& config) const -> std::unique_ptr<font::loader>
{
#if ARCHON_FONT_HAVE_FREETYPE
    return ::new_loader(resource_dir, locale, config); // Throws
#else
    static_cast<void>(resource_dir);
    static_cast<void>(locale);
    static_cast<void>(config);
    throw std::runtime_error("FreeType implementation is unavailable");
#endif
}


} // unnamed namespace


auto font::get_freetype_implementation() noexcept -> const font::implementation&
{
    return g_implementation;
}


auto font::new_freetype_loader_from_font_file(core::FilesystemPathRef file, const std::locale& locale,
                                              const font::loader::config& config) -> std::unique_ptr<font::loader>
{
#if ARCHON_FONT_HAVE_FREETYPE
    return ::new_loader_from_font_file(file, locale, config); // Throws
#else
    static_cast<void>(file);
    static_cast<void>(locale);
    static_cast<void>(config);
    throw std::runtime_error("FreeType implementation is unavailable");
#endif
}
