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
#include <type_traits>
#include <memory>
#include <string_view>
#include <string>
#include <stdexcept>
#include <locale>
#include <mutex>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/log.hpp>
#include <archon/font/impl/config.h>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/loader_freetype.hpp>

#if ARCHON_FONT_HAVE_FREETYPE
#  include <ft2build.h>
#  include FT_FREETYPE_H
#  include FT_OUTLINE_H
#  include FT_DRIVER_H
#  include FT_MODULE_H
#  include FT_FONT_FORMATS_H
#endif


using namespace archon;


namespace {


#if ARCHON_FONT_HAVE_FREETYPE


inline auto fixed_26p6_to_float(FT_F26Dot6 val) noexcept -> font::Face::float_type
{
    return (font::Face::float_type(1) / 64) * val;
}

inline auto float_to_fixed_26p6(font::Face::float_type val) noexcept -> FT_F26Dot6
{
    return core::clamped_float_to_int<FT_F26Dot6>(std::round(64 * val));
}


inline auto fixed_26p6_round(FT_F26Dot6 val) noexcept -> FT_F26Dot6
{
    auto val_2 = core::to_unsigned(core::promote(val));
    using type = decltype(val_2);
    if (val >= 0)
        return core::cast_from_twos_compl<FT_F26Dot6>((val_2 + 32) & ~type(63));
    return core::cast_from_twos_compl<FT_F26Dot6>((val_2 - 32) & ~type(63));
}


struct FixedSizeKey {
    FT_F26Dot6 width, height;

    bool operator<(FixedSizeKey other) const noexcept
    {
        return (width < other.width || (width == other.width && height < other.height));
    }
};


struct RasterContext {
    FT_BBox clip_box;
    font::Face::comp_type* base;
    std::ptrdiff_t horz_stride, vert_stride;
};


void render_spans(int y, int count, const FT_Span* spans, void* user) throw()
{
    const RasterContext& context = *static_cast<RasterContext*>(user);
    ARCHON_ASSERT(y >= context.clip_box.yMin && y < context.clip_box.yMax);
    using comp_type = font::Face::comp_type;
    comp_type* base = context.base - (y - context.clip_box.yMin) * context.vert_stride;
    for (int i = 0; i < count; ++i) {
        const FT_Span& span = spans[i];
        static_assert(std::is_same_v<decltype(span.x), short>);
        static_assert(std::is_same_v<decltype(span.len), unsigned short>);
        ARCHON_ASSERT(span.len <= unsigned(core::int_max<int>() - span.x));
        ARCHON_ASSERT(span.x >= context.clip_box.xMin);
        ARCHON_ASSERT(int(int(span.x) + span.len) <= context.clip_box.xMax);
        int x_1 = int(span.x - context.clip_box.xMin);
        int x_2 = int(x_1 + span.len);
        // Judging from the implementation of `_gblender_spans_rgb24()` (note that this
        // function name is constructed from parts using a macro) in `graph/gblany.h` in the
        // "demo programs" package of FreeType 2.12.1, `span.coverage` is supposed to be
        // interpreted as a linearly encoded alpha value rather than as a gamma encoded
        // gray-level. Unfortunately, the FreeType documentation is not clear about it.
        comp_type alpha = image::pack_int<comp_type, 8>(span.coverage);
        for (int x = x_1; x < x_2; ++x)
            base[x * context.horz_stride] = alpha;
    }
}


struct LibraryGuard {
    FT_Library library = nullptr;
    mutable std::mutex mutex;
    void init()
    {
        FT_Error err = FT_Init_FreeType(&library);
        if (ARCHON_UNLIKELY(err != 0))
            throw std::runtime_error("FreeType: Failed to initialize library");
    }
    ~LibraryGuard() noexcept
    {
        if (ARCHON_LIKELY(library)) {
            FT_Error err = FT_Done_FreeType(library);
            ARCHON_ASSERT(err == 0);
        }
    }
};


struct FaceGuard {
    const LibraryGuard& library;
    FT_Face face = nullptr;
    FaceGuard(const LibraryGuard& l) noexcept
        : library(l)
    {
    }
    void init(const char* path, FT_Long face_index)
    {
        std::lock_guard lock(library.mutex);
        FT_Error err = FT_New_Face(library.library, path, face_index, &face);
        if (ARCHON_UNLIKELY(err != 0))
            throw std::runtime_error("FreeType: Failed to load font face");
    }
    ~FaceGuard() noexcept
    {
        if (ARCHON_LIKELY(face)) {
            std::lock_guard lock(library.mutex);
            FT_Error err = FT_Done_Face(face);
            ARCHON_ASSERT(err == 0);
        }
    }
};



class FaceImpl
    : public font::Face {
public:
    FaceImpl(const LibraryGuard& library, const char* path, FT_Long face_index)
        : m_face(library)
    {
        m_face.init(path, face_index); // Throws
        m_glyph = m_face.face->glyph;

        bool is_scalable_or_has_fixed_sizes = (FT_IS_SCALABLE(m_face.face) || m_face.face->num_fixed_sizes > 0);
        if (ARCHON_UNLIKELY(!is_scalable_or_has_fixed_sizes))
            throw std::runtime_error("Font face is not scalable and has no fixed sizes");

        // Implementation is obliged to set initial rendering size as close to 12 x 12 as
        // possible.
        font::Size size = { 12, 12 };
        set_approx_size(size); // Throws

        // Implementation is obliged to load the replacement glyph initially, and with grid
        // fitting enabled.
        std::size_t glyph_index = 0;
        bool grid_fitting = true;
        load_glyph(glyph_index, grid_fitting); // Throws
    }

    auto get_family_name() -> std::string_view override final
    {
        if (ARCHON_LIKELY(m_face.face->family_name))
            return { m_face.face->family_name };
        return {};
    }

    bool is_bold() noexcept override final
    {
        return ((m_face.face->style_flags & FT_STYLE_FLAG_BOLD) != 0);
    }

    bool is_italic() noexcept override final
    {
        return ((m_face.face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);
    }

    bool is_monospace() noexcept override final
    {
        return FT_IS_FIXED_WIDTH(m_face.face);
    }

    bool is_scalable() noexcept override final
    {
        return FT_IS_SCALABLE(m_face.face);
    }

    int get_num_fixed_sizes() override final
    {
        return core::int_cast<int>(m_face.face->num_fixed_sizes); // Throws
    }

    auto get_fixed_size(int fixed_size_index) -> font::Size override final
    {
        bool valid = (fixed_size_index >= 0 && fixed_size_index < m_face.face->num_fixed_sizes);
        if (ARCHON_LIKELY(valid)) {
            const FT_Bitmap_Size& entry = m_face.face->available_sizes[fixed_size_index];
            return {
                fixed_26p6_to_float(entry.x_ppem),
                fixed_26p6_to_float(entry.y_ppem),
            };
        }
        throw std::out_of_range("Fixed size index");
    }

    void set_fixed_size(int fixed_size_index) override final
    {
        bool valid = (fixed_size_index >= 0 && fixed_size_index < m_face.face->num_fixed_sizes);
        if (ARCHON_LIKELY(valid)) {
            do_set_fixed_size(fixed_size_index); // Throws
            return;
        }
        throw std::out_of_range("Fixed size index");
    }

    void set_scaled_size(font::Size size) override final
    {
        if (ARCHON_LIKELY(!FT_IS_SCALABLE(m_face.face))) {
            FT_F26Dot6 width  = float_to_fixed_26p6(size.width);
            FT_F26Dot6 height = float_to_fixed_26p6(size.height);
            do_set_scaled_size(width, height); // Throws
            return;
        }
        throw std::logic_error("Font face is not scalable");
    }

    void set_approx_size(font::Size size) override final
    {
        ensure_fixed_sizes_map(); // Throws

        // First, check for an exact match on a fixed size
        FT_F26Dot6 width  = float_to_fixed_26p6(size.width);
        FT_F26Dot6 height = float_to_fixed_26p6(size.height);
        FixedSizeKey key = { width, height };
        auto i = m_fixed_sizes_map.find(key);
        if (i != m_fixed_sizes_map.end()) {
            int fixed_size_index = i->second;
            do_set_fixed_size(fixed_size_index); // Throws
            return;
        }

        // Second, choose a scaled size if we can
        if (FT_IS_SCALABLE(m_face.face)) {
            do_set_scaled_size(width, height); // Throws
            return;
        }

        // Third, find the best matching fixed size
        float_type min = 0;
        int fixed_size_index = -1;
        int n = get_num_fixed_sizes(); // Throws
        for (int i = 0; i < n; ++i) {
            const FT_Bitmap_Size& entry = m_face.face->available_sizes[i];
            float_type diff = (core::square(size.width  - fixed_26p6_to_float(entry.x_ppem)) +
                               core::square(size.height - fixed_26p6_to_float(entry.y_ppem)));
            if (ARCHON_LIKELY(fixed_size_index < 0 && diff >= min))
                continue;
            min = diff;
            fixed_size_index = i;
        }
        ARCHON_ASSERT(fixed_size_index >= 0);
        do_set_fixed_size(fixed_size_index); // Throws
    }

    auto get_size() noexcept -> font::Size override final
    {
        return {
            fixed_26p6_to_float(m_render_width),
            fixed_26p6_to_float(m_render_height),
        };
    }

    auto get_baseline_spacing(bool vertical, bool grid_fitting) noexcept -> float_type override final
    {
        return (grid_fitting ?
                (!vertical ? m_horz_baseline_spacing_gf : m_vert_baseline_spacing_gf) :
                (!vertical ? m_horz_baseline_spacing    : m_vert_baseline_spacing));
    }

    auto get_baseline_offset(bool vertical, bool grid_fitting) noexcept -> float_type override final
    {
        return (grid_fitting ?
                (!vertical ? m_horz_baseline_offset_gf : m_vert_baseline_offset_gf) :
                (!vertical ? m_horz_baseline_offset    : m_vert_baseline_offset));
    }

    auto find_glyph(char_type ch) -> std::size_t override final
    {
        font::CodePoint cp_1;
        FT_ULong cp_2 = 0;
        if (ARCHON_LIKELY(cp_1.try_from_char(ch) && core::try_int_cast(cp_1.to_int(), cp_2))) {
            FT_UInt index = FT_Get_Char_Index(m_face.face, cp_2);
            return core::int_cast<std::size_t>(index); // Throws
        }
        return 0; // Index of replacement glyph
    }

    auto get_kerning(std::size_t glyph_index_1, std::size_t glyph_index_2,
                     bool vertical, bool grid_fitting) -> float_type override final
    {
        // According to the API reference, FreeType only supports kerning for horizontal
        // layouts
        FT_UInt glyph_index_3 = 0, glyph_index_4 = 0;
        if (ARCHON_LIKELY(!vertical &&
                          core::try_int_cast(glyph_index_1, glyph_index_3) &&
                          core::try_int_cast(glyph_index_2, glyph_index_4))) {
            FT_UInt kern_mode = (grid_fitting ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED);
            FT_Vector vec = {};
            FT_Error err = FT_Get_Kerning(m_face.face, glyph_index_3, glyph_index_4, kern_mode, &vec);
            // The existence of `vec.y`, i.e., a vertical component for the kerning
            // displacement, is weird. The FreeType documentation does not explain it, but
            // the implementation of `FTDemo_String_Load()` in `src/ftcommon.c` in the "demo
            // programs" package of FreeType 2.12.1 suggests that `vec.y` should displace
            // the cursor vertically in a horizontal layout. However, since such a
            // displacement would affect the rest of the line, it seems like an unreasonable
            // idea. For now, `vec.y` will be assumed to be zero.
            if (ARCHON_LIKELY(err == 0 && vec.y == 0))
                return fixed_26p6_to_float(vec.x);
            if (ARCHON_LIKELY(err != 0))
                throw std::runtime_error("FreeType: Failed to get kerning");
            throw std::runtime_error("FreeType: Got kerning with vertical displacement");
        }
        return 0;
    }

    void load_glyph(std::size_t glyph_index, bool grid_fitting) override final
    {
        FT_UInt glyph_index_2 = 0;
        if (ARCHON_UNLIKELY(!core::try_int_cast(glyph_index, glyph_index_2) ||
                            !core::int_less(glyph_index_2, m_face.face->num_glyphs)))
            throw std::out_of_range("glyph_index");
        FT_Int32 flags = 0;
        if (ARCHON_LIKELY(grid_fitting)) {
            flags |= FT_LOAD_TARGET_NORMAL;
            if (ARCHON_UNLIKELY(m_force_autohint))
                flags |= FT_LOAD_FORCE_AUTOHINT;
        }
        else {
            flags |= FT_LOAD_NO_HINTING;
        }
        FT_Error err = FT_Load_Glyph(m_face.face, glyph_index_2, flags);
        if (ARCHON_UNLIKELY(err != 0))
            throw std::runtime_error("FreeType: Failed to load glyph");

        m_horz_glyph_advance = fixed_26p6_to_float(m_glyph->metrics.horiAdvance);

        // FreeType always loads a glyph such that the origin of the outline description                                
        // coincides with the bearing point pertaining to a horizontal layout. Therefore, to
        // acheive the direction neutral position where the origin of the outline
        // description is the lower left corner of the bounding box, we need to make a
        // correction.
        float_type left   = fixed_26p6_to_float(m_glyph->metrics.horiBearingX);
        float_type top    = fixed_26p6_to_float(m_glyph->metrics.horiBearingY);
        float_type right  = fixed_26p6_to_float(m_glyph->metrics.horiBearingX + m_glyph->metrics.width);
        float_type bottom = fixed_26p6_to_float(m_glyph->metrics.horiBearingY - m_glyph->metrics.height);

        // Grid fitting of the glyph metrics will normally already have been done by                                  
        // FreeType, but since that behavior appears to be compile-time configurable, the
        // rounding is repeated here. Fortunately rounding is an idempotent operation.
        if (grid_fitting) {
            m_horz_glyph_advance = std::round(m_horz_glyph_advance);
            left   = std::floor(left);
            bottom = std::floor(bottom);                                    
            right  = std::ceil(right);
            top    = std::ceil(top);                                        
        }

        // Vector from bearing point of vertical layout to bearing point of horizontal
        // layout
        //
        // FIXME: It seems that in some cases such as "Liberation Serif", the vertical                              
        // metrics are set to appropriate values even when the underlying font face does not
        // provide any. If that were always the case, there would be no point in emulating
        // those metrics below. Problem is, according to the documentation, the vertical
        // metrics must be considered unrelibale when FT_HAS_VERTICAL(face) returns false.
        //
        // FIXME: Due to the assumptions made for font-level vertical layout metrics, it is
        // problematic to use glyph-level vertical metrics provided by FreeType, even if
        // FT_HAS_VERTICAL(m_face.face) is true.                                                          
        //
        vector_type vert_to_horz;
        if (FT_HAS_VERTICAL(m_face.face)) {                                                                                    
            m_vert_glyph_advance = fixed_26p6_to_float(m_glyph->metrics.vertAdvance);
            vert_to_horz = vector_type {
                fixed_26p6_to_float(m_glyph->metrics.vertBearingX - m_glyph->metrics.horiBearingX),
                fixed_26p6_to_float(m_glyph->metrics.vertAdvance - m_glyph->metrics.vertBearingY -
                                    m_glyph->metrics.horiBearingY),
            };
            if (grid_fitting) {
                m_vert_glyph_advance = std::round(m_vert_glyph_advance);
                vert_to_horz[0] = std::round(vert_to_horz[0]);
                vert_to_horz[1] = std::round(vert_to_horz[1]);
            }
        }
        else {                                   
            // Emulated vertical metrics
            float_type half = float_type(0.5);
            if (grid_fitting) {
                m_vert_glyph_advance = m_horz_baseline_spacing_gf;
                vert_to_horz = vector_type {
                    std::round(-half * m_horz_glyph_advance),
                    m_horz_baseline_offset_gf,
                };
            }
            else {
                m_vert_glyph_advance = m_horz_baseline_spacing;
                vert_to_horz = vector_type {
                    -half * m_horz_glyph_advance,
                    m_horz_baseline_offset,
                };
            }
        }

        m_glyph_size = vector_type(right - left, top - bottom);
        m_horz_glyph_bearing = vector_type(-left, -bottom);
        m_vert_glyph_bearing = m_horz_glyph_bearing - vert_to_horz;
        m_prev_glyph_translation_x = m_glyph->metrics.horiBearingX;
        m_prev_glyph_translation_y = m_glyph->metrics.horiBearingY - m_glyph->metrics.height;
        m_glyph_translation = vector_type(0, 0);
    }

    auto get_glyph_advance(bool vertical) noexcept -> float_type override final
    {
        return (!vertical ? m_horz_glyph_advance : m_vert_glyph_advance);
    }

    auto get_glyph_bearing(bool vertical) noexcept -> vector_type override final
    {
        return (!vertical ? m_horz_glyph_bearing : m_vert_glyph_bearing);
    }

    void translate_glyph(vector_type vec) override final
    {
        m_glyph_translation += vec;
    }

protected:
    void do_get_glyph_pa_box(int& left, int& right, int& bottom, int& top) override final
    {
        // FIXME: Tend to overflow in casts and arthmetic below                        
        if (m_glyph->format == FT_GLYPH_FORMAT_BITMAP) {
            left   = int(std::round(m_glyph_translation[0]));
            bottom = int(std::round(m_glyph_translation[1]));
            right  = int(left   + m_glyph->bitmap.width);
            top    = int(bottom + m_glyph->bitmap.rows);
        }
        else {
            left   = int(std::floor(m_glyph_translation[0]));
            bottom = int(std::floor(m_glyph_translation[1]));
            right  = int(std::ceil(m_glyph_translation[0] + m_glyph_size[0]));
            top    = int(std::ceil(m_glyph_translation[1] + m_glyph_size[1]));
        }
    }

    void do_render_glyph_mask(image::Pos pos, const iter_type& iter, image::Size size) override final
    {
        int left = 0, right = 0, bottom = 0, top = 0;
        do_get_glyph_pa_box(left, right, bottom, top);
        image::Pos pos_2 = pos;
        // Note the inversion of the Y-axis
        core::int_add(pos_2.x, left); // Throws
        core::int_sub(pos_2.y, top); // Throws
        image::Size glyph_size = { right - left, top - bottom };
        image::Box box = { pos_2, glyph_size };
        image::Box boundary = { size };
        if (ARCHON_UNLIKELY(!boundary.clip(box)))
            return;
        image::Iter iter_2 = iter + (box.pos - image::Pos());

        switch (m_glyph->format) {
            case FT_GLYPH_FORMAT_BITMAP:
                goto bitmap;
            case FT_GLYPH_FORMAT_OUTLINE:
                goto outline;
            default:
                break;
        }
        throw std::runtime_error("Unsupported glyph format");

      outline:
        // FIXME: Generally avoid translations (because it is wasteful) as long as they are all by integer amounts and no other transformations (rotations) have been specified                                    
        //
        // Translate glyph                             
        {
            FT_F26Dot6 x = float_to_fixed_26p6(m_glyph_translation[0]);
            FT_F26Dot6 y = float_to_fixed_26p6(m_glyph_translation[1]);
            if (x != m_prev_glyph_translation_x || y != m_prev_glyph_translation_y) {
                FT_Outline_Translate(&m_glyph->outline,
                                     x - m_prev_glyph_translation_x,
                                     y - m_prev_glyph_translation_y);
                m_prev_glyph_translation_x = x;
                m_prev_glyph_translation_y = y;
            }
        }

        {
            RasterContext context;
            FT_Raster_Params params;
            params.flags      = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_CLIP;
            params.gray_spans = &render_spans;
            params.user       = &context;
            // Note the inversion of the Y-axis
            params.clip_box.xMin = left + (box.pos.x - pos_2.x);
            params.clip_box.xMax = params.clip_box.xMin + box.size.width;
            params.clip_box.yMax = top - (box.pos.y - pos_2.y);
            params.clip_box.yMin = params.clip_box.yMax - box.size.height;
            context.clip_box = params.clip_box;
            ARCHON_ASSERT(box.size.height > 0);
            context.base = iter_2.base + (box.size.height - 1) * iter_2.vert_stride;
            context.horz_stride = iter_2.horz_stride;
            context.vert_stride = iter_2.vert_stride;
            FT_Error err = FT_Outline_Render(m_face.library.library, &m_glyph->outline, &params);
            if (ARCHON_LIKELY(err == 0))
                return;
            throw std::runtime_error("FreeType: Failed to render glyph");
        }

      bitmap:
/*
        ssize_t pitch = 1, stride = -glyph->bitmap.pitch;
        const unsigned char* src = (stride < 0 ? glyph->bitmap.buffer - (height-1)*stride : glyph->bitmap.buffer);
        unsigned char* dst = pix_buf.get();

        if (glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            int num_grays = glyph->bitmap.num_grays;
            ARCHON_ASSERT_1(0 < num_grays, "Unexpected number of gray levels");
            for (int y = 0; y < height; ++y, src += stride) {
                for (int x = 0; x < width; ++x, ++dst)
                    *dst = frac_adjust_denom<unsigned char>(src[x*pitch], num_grays, 0);
            }
        }
        else if(glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
            for (int y = 0; y < height; ++y, src += stride) {
                for (int x = 0; x < width; ++x, ++dst)
                    *dst = (src[(x>>3)*pitch] & 128>>(x&7) ? std::numeric_limits<unsigned char>::max() : 0);
            }
        }
        else {
            throw std::runtime_error("Unsupported pixel format of glyph");
        }
*/
        ARCHON_STEADY_ASSERT_UNREACHABLE();                              
    }

    void do_render_glyph_rgba(image::Pos pos, const iter_type& iter,
                              image::Size size) override final
    {
        static_cast<void>(pos);                      
        static_cast<void>(iter);                      
        static_cast<void>(size);                      
        ARCHON_STEADY_ASSERT_UNREACHABLE();                              
    }

private:
    FaceGuard m_face;
    FT_GlyphSlot m_glyph;
    bool m_force_autohint = false;
    bool m_fixed_sizes_map_initialized = false;
    core::FlatMap<FixedSizeKey, int> m_fixed_sizes_map;

    // These are initialized by `on_size_changed()`
    FT_F26Dot6 m_render_width, m_render_height;
    float_type m_horz_baseline_offset, m_horz_baseline_spacing;
    float_type m_vert_baseline_offset, m_vert_baseline_spacing;
    float_type m_horz_baseline_offset_gf, m_horz_baseline_spacing_gf;
    float_type m_vert_baseline_offset_gf, m_vert_baseline_spacing_gf;

    // These are initialized by `load_glyph()`
    float_type m_horz_glyph_advance, m_vert_glyph_advance;
    vector_type m_horz_glyph_bearing, m_vert_glyph_bearing;
    vector_type m_glyph_size, m_glyph_translation;
    FT_F26Dot6 m_prev_glyph_translation_x, m_prev_glyph_translation_y;

    void ensure_fixed_sizes_map()
    {
        if (ARCHON_LIKELY(m_fixed_sizes_map_initialized))
            return;

        int n = get_num_fixed_sizes(); // Throws
        for (int i = 0; i < n; ++i) {
            const FT_Bitmap_Size& entry = m_face.face->available_sizes[i];
            FixedSizeKey key = { entry.x_ppem, entry.y_ppem };
            m_fixed_sizes_map.emplace(key, i); // Throws
        }

        m_fixed_sizes_map_initialized = true;
    }

    void do_set_fixed_size(int fixed_size_index)
    {
        ARCHON_ASSERT(fixed_size_index >= 0 && fixed_size_index < m_face.face->num_fixed_sizes);
        FT_Error err = FT_Select_Size(m_face.face, FT_Int(fixed_size_index));
        if (ARCHON_LIKELY(err == 0)) {
            const FT_Bitmap_Size& entry = m_face.face->available_sizes[fixed_size_index];
            on_size_changed(entry.x_ppem, entry.y_ppem);
        }
        throw std::runtime_error("FreeType: Failed to select fixed size");
    }

    void do_set_scaled_size(FT_F26Dot6 width, FT_F26Dot6 height)
    {
        ARCHON_ASSERT(FT_IS_SCALABLE(m_face.face));
        // Passing zero for resolution sets the resolition to 72 DPI in both
        // directions. Since a point is 1 / 72 of an inch, this effectively equates a pixel
        // with a point.
        FT_UInt horz_resolution = 0;
        FT_UInt vert_resolution = 0;
        FT_Error err = FT_Set_Char_Size(m_face.face, width, height, horz_resolution, vert_resolution);
        if (ARCHON_LIKELY(err == 0)) {
            on_size_changed(width, height);
            return;
        }
        throw std::runtime_error("FreeType: Failed to set scaled size");
    }

    void on_size_changed(FT_F26Dot6 width, FT_F26Dot6 height)
    {
        m_render_width  = width;
        m_render_height = height;

        const FT_Size_Metrics& metrics = m_face.face->size->metrics;

        FT_Short raw_ascender     = m_face.face->ascender;
        FT_Short raw_descender    = m_face.face->descender;
        FT_Short raw_height       = m_face.face->height;
        FT_Short raw_max_advance  = m_face.face->max_advance_width;
        FT_Short adj_raw_ascender = (raw_ascender + raw_descender + raw_height) / 2;

        // Unfortunately FreeType cannot provide appropriate values for the the descender                                                                                     
        // and ascender equivalents in a vertical layout. We are forced to make a guess that
        // can easily be wrong. We will assume that the vertical baseline is centered on the
        // line.

        // FIXME: What if font is not scalable?                                                                                                                     
        {
            auto from_raw_x = [&](FT_Short val) {
                return fixed_26p6_to_float(FT_MulFix(val, metrics.x_scale));
            };
            auto from_raw_y = [&](FT_Short val) {
                return fixed_26p6_to_float(FT_MulFix(val, metrics.y_scale));
            };

            float_type height      = from_raw_y(raw_height);
            float_type max_advance = from_raw_x(raw_max_advance);

            m_horz_baseline_offset  = height - from_raw_y(adj_raw_ascender);
            m_horz_baseline_spacing = height;
            m_vert_baseline_offset  = max_advance - from_raw_x(raw_max_advance / 2);
            m_vert_baseline_spacing = max_advance;
        }

        const char* font_format = FT_Get_Font_Format(m_face.face);
        if (ARCHON_UNLIKELY(!font_format))
            throw std::runtime_error("FreeType: Failed to get font format");
        bool is_true_type_with_native_hinting = (std::string_view(font_format) == "TrueType" && !m_force_autohint);
        if (!is_true_type_with_native_hinting) {
            float_type ascender    = fixed_26p6_to_float(metrics.ascender);
            float_type descender   = fixed_26p6_to_float(metrics.descender);
            float_type height      = fixed_26p6_to_float(metrics.height);
            float_type max_advance = fixed_26p6_to_float(metrics.max_advance);

            m_horz_baseline_offset_gf  = std::round((height - (ascender + descender)) / 2);
            m_horz_baseline_spacing_gf = height;
            m_vert_baseline_offset_gf  = std::round(max_advance / 2);
            m_vert_baseline_spacing_gf = max_advance;
        }
        else {
            FT_Fixed x_scale = FT_DivFix(fixed_26p6_round(width),  FT_Long(m_face.face->units_per_EM));
            FT_Fixed y_scale = FT_DivFix(fixed_26p6_round(height), FT_Long(m_face.face->units_per_EM));

            auto from_raw_x = [&](FT_Short val) {
                return fixed_26p6_to_float(fixed_26p6_round(FT_MulFix(val, x_scale)));
            };
            auto from_raw_y = [&](FT_Short val) {
                return fixed_26p6_to_float(fixed_26p6_round(FT_MulFix(val, y_scale)));
            };

            float_type height      = from_raw_y(raw_height);
            float_type max_advance = from_raw_x(raw_max_advance);

            m_horz_baseline_offset_gf  = height - from_raw_y(adj_raw_ascender);
            m_horz_baseline_spacing_gf = height;
            m_vert_baseline_offset_gf  = max_advance - from_raw_x(raw_max_advance / 2);
            m_vert_baseline_spacing_gf = max_advance;
        }
    }
};



class LoaderImpl
    : public font::Loader {
public:
    LoaderImpl(core::FilesystemPathRef resource_dir, const std::locale& loc, log::Logger*)
    {
//        std::string_view file_name = "LiberationSans-Regular.ttf";                           
        std::string_view file_name = "LiberationMono-Regular.ttf";                           
        namespace fs = std::filesystem;
        fs::path path = resource_dir / core::make_fs_path_generic(file_name, loc); // Throws
        m_path = core::path_to_string_native(path, loc); // Throws
        m_library.init(); // Throws
    }

    auto load_default_face() const -> std::unique_ptr<font::Face> override final
    {
        FT_Long face_index = 0;
        return std::make_unique<FaceImpl>(m_library, m_path.c_str(), face_index); // Throws
    }

    auto get_implementation() const noexcept -> const Implementation& override final;

private:
    std::string m_path;
    LibraryGuard m_library;
};



class ImplementationImpl
    : public font::Loader::Implementation {
public:
    auto ident() const noexcept -> std::string_view override final
    {
        return "freetype";
    }

    auto new_loader(core::FilesystemPathRef resource_dir, const std::locale& loc,
                    font::Loader::Config config) const -> std::unique_ptr<font::Loader> override final
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


#endif // ARCHON_FONT_HAVE_FREETYPE


} // unnamed namespace


auto font::loader_freetype_impl() noexcept -> const font::Loader::Implementation*
{
#if ARCHON_FONT_HAVE_FREETYPE
    return &::get_implementation();
#else
    return nullptr;
#endif
}
