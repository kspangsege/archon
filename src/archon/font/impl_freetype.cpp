/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#include <cmath>
#include <cstdlib>
#include <limits>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <archon/core/types.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/file.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/font/loader.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;


namespace {

class LoaderImpl: public FontLoader {
public:
    std::unique_ptr<FontFace> load_default_face(double w, double h) const override
    {
        return load_face(default_file, default_index, w, h); // Throws
    }

    std::unique_ptr<FontFace> load_face(std::string, int, double, double) const override;

    void load_face_info(std::string font_file, int face_index, FaceInfo& info) const override
    {
        FT_Face f;
        if (FT_New_Face(library, font_file.c_str(), face_index, &f))
            throw BadFontFileException("Failed to load \""+font_file+"\"");
        try {
            info.family = (f->family_name ? std::string(f->family_name) : std::string());
            info.bold   = f->style_flags & FT_STYLE_FLAG_BOLD;
            info.italic = f->style_flags & FT_STYLE_FLAG_ITALIC;
            info.monospace = FT_IS_FIXED_WIDTH(f);
            info.scalable  = FT_IS_SCALABLE(f);
            info.fixed_sizes.reserve(f->num_fixed_sizes);
            for (int i = 0; i < f->num_fixed_sizes; ++i) {
                const FT_Bitmap_Size& s = f->available_sizes[i];
                info.fixed_sizes.push_back(FaceInfo::FixedSize(1/64.0*s.x_ppem, 1/64.0*s.y_ppem));
            }
        }
        catch (...) {
            FT_Done_Face(f);
            throw;
        }
    }

    int check_file(std::string f) const override
    {
        FT_Face face;
        if (FT_New_Face(library, f.c_str(), -1, &face))
            return 0;
        ARCHON_ASSERT_1(0 < face->num_faces, "No faces in font file");
        return face->num_faces;
    }

    std::string get_default_font_file() const override
    {
        return default_file;
    }

    int get_default_face_index() const override
    {
        return default_index;
    }

    LoaderImpl(FT_Library l, std::string resource_dir):
        library{l},
        default_file{resource_dir+"LiberationSerif-Regular.ttf"}
    {
    }

    ~LoaderImpl() noexcept override
    {
        FT_Done_FreeType(library);
    }

    const std::weak_ptr<LoaderImpl> weak_self;

    const FT_Library library;

private:
    const std::string default_file;
    const int default_index = 0;
};


class RenderTarget {
public:
    unsigned char* lower_left;
    int width, height;
    RenderTarget(unsigned char* p, int w, int h):
        lower_left(p),
        width(w),
        height(h)
    {
    }
};


void render_spans(int y, int count, const FT_Span* spans, void* user) throw()
{
    const RenderTarget* target = static_cast<RenderTarget*>(user);
    ARCHON_ASSERT_1(0 <= y && y < target->height, "render_spans: Bad y");
    unsigned char* ptr = target->lower_left + y * target->width;
    for (int i = 0; i < count; ++i) {
        const FT_Span& span = spans[i];
        int x1 = span.x, x2 = x1 + span.len;
        ARCHON_ASSERT_1(0 <= x1 && x2 <= target->width, "render_spans: Bad x");
        std::fill(ptr+x1, ptr+x2,
                  frac_adjust_bit_width(span.coverage, 8, std::numeric_limits<unsigned char>::digits));
    }
}



class FaceImpl: public FontFace {
public:
    std::string get_family_name() const override
    {
        return face->family_name ? std::string(face->family_name) : std::string();
    }

    bool is_bold() const override
    {
        return face->style_flags & FT_STYLE_FLAG_BOLD;
    }

    bool is_italic() const override
    {
        return face->style_flags & FT_STYLE_FLAG_ITALIC;
    }

    bool is_monospace() const override
    {
        return FT_IS_FIXED_WIDTH(face);
    }

    bool is_scalable() const override
    {
        return FT_IS_SCALABLE(face);
    }

    int get_num_fixed_sizes() const override
    {
        return face->num_fixed_sizes;
    }

    Vec2 get_fixed_size(int i) const override
    {
        if (i < 0 || face->num_fixed_sizes <= i)
            throw std::out_of_range("fixed_size_index");
        return 1/64.0 * Vec2(double(face->available_sizes[i].x_ppem),
                             double(face->available_sizes[i].y_ppem));
    }

    void set_fixed_size(int i) override
    {
        if (i < 0 || face->num_fixed_sizes <= i)
            throw std::out_of_range("fixed_size_index");
        if (FT_Select_Size(face, i) != 0)
            throw std::runtime_error("Failed to set fixed size");
        const FT_Bitmap_Size& size = face->available_sizes[i];
        on_size_changed(size.x_ppem, size.y_ppem);
    }

    void set_scaled_size(double width, double height) override
    {
        if (!FT_IS_SCALABLE(face))
            throw std::logic_error("Font face is not scalable");
        if (width <= 0 || 16384 < width || height <= 0 || 16384 < height)
            throw std::invalid_argument("Bad font size");
        FT_F26Dot6 w = width*64, h = height*64;
        if (FT_Set_Char_Size(face, w, h, 0, 0))
            throw std::runtime_error("FT_Set_Char_Size failed");
        on_size_changed(w,h);
    }

    void set_approx_size(double width, double height) override
    {
        // Initialize on demand
        if (fixed_sizes.empty()) {
            if (face->num_fixed_sizes == 0) {
                set_scaled_size(width, height);
                return;
            }

            for (int i = 0; i < face->num_fixed_sizes; ++i) {
                const FT_Bitmap_Size& s = face->available_sizes[i];
                fixed_sizes[1/64.0 * Vec2{double(s.x_ppem), double(s.y_ppem)}] = i;
            }
        }

        // First check for an exact match
        Vec2 size{width, height};
        auto i = fixed_sizes.find(size);
        if (i != fixed_sizes.end()) {
            set_fixed_size(i->second);
            return;
        }

        if (FT_IS_SCALABLE(face)) {
            set_scaled_size(width, height);
            return;
        }

        // Now search for the best inexact match
        double min = std::numeric_limits<int>::max();
        int idx = 0;
        for (auto& entry: fixed_sizes)
        {
            double diff = sq_dist(entry.first, size);
            if (diff < min) {
                min = diff;
                idx = entry.second;
            }
        }
        set_fixed_size(idx);
    }

    void on_size_changed(FT_F26Dot6 width, FT_F26Dot6 height)
    {
        render_width  = width;
        render_height = height;

        const FT_Size_Metrics& metrics = face->size->metrics;
        double space_h = 1/64.0 * metrics.height;
        double space_v = 1/64.0 * metrics.max_advance;
        ARCHON_ASSERT_1(0 < space_h && 0 < space_v, "Zero baseline spacing");
        int space_h_gf = std::ceil(space_h);
        int space_v_gf = std::ceil(space_v);

        double min_h = 1/64.0 * metrics.descender, max_h = 1/64.0 * metrics.ascender;
        // Unfortunately Freetype cannot provide appropriate values for the the
        // descender and ascender equivalents in a vertical layout. We are
        // forced to make a guess that can easily be wrong. We will assume that
        // the vertical baseline is centered on the line.
        double min_v = -0.5 * space_v, max_v = min_v + space_v;

        int min_h_gf = std::floor(min_h);
        int max_h_gf =  std::ceil(max_h);
        int min_v_gf = std::floor(min_v);
        int max_v_gf =  std::ceil(max_v);

        hori_baseline_offset     = (space_h - max_h - min_h) / 2;
        hori_baseline_spacing    = space_h;
        vert_baseline_offset     = (space_v - max_v - min_v) / 2;
        vert_baseline_spacing    = space_v;
        hori_baseline_offset_gf  = std::round((space_h_gf - max_h_gf - min_h_gf) / 2.0);
        hori_baseline_spacing_gf = space_h_gf;
        vert_baseline_offset_gf  = std::round((space_v_gf - max_v_gf - min_v_gf) / 2.0);
        vert_baseline_spacing_gf = space_v_gf;
    }

    double get_width() const override
    {
        return 1/64.0 * render_width;
    }

    double get_height() const override
    {
        return 1/64.0 * render_height;
    }

    double get_baseline_spacing(bool vertical, bool grid_fitting) const override
    {
        return (grid_fitting ?
                (vertical ? vert_baseline_spacing_gf : hori_baseline_spacing_gf) :
                (vertical ? vert_baseline_spacing    : hori_baseline_spacing));
    }

    double get_baseline_offset(bool vertical, bool grid_fitting) const override
    {
        return (grid_fitting ?
                (vertical ? vert_baseline_offset_gf : hori_baseline_offset_gf) :
                (vertical ? vert_baseline_offset    : hori_baseline_offset));
    }

    int get_num_glyphs() const override
    {
        return face->num_glyphs;
    }

    int find_glyph(wchar_t c) const override
    {
        return FT_Get_Char_Index(face, c);
    }

    double get_kerning(int glyph1, int glyph2, bool vertical, bool grid_fitting) const override
    {
        // FreeType only supports kerning for horizontal layouts
        if (!has_kerning || vertical || glyph1 == 0 || glyph2 == 0)
            return 0;
        FT_UInt kern_mode = (grid_fitting ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED);
        FT_Vector v;
        FT_Get_Kerning(face, glyph1, glyph2, kern_mode, &v);
        return v.x / 64.0;
    }

    void load_glyph(int i, bool grid_fitting) override
    {
        if (i < 0 || face->num_glyphs <= i)
            throw std::out_of_range("glyph_index");
        FT_Int32 flags = FT_LOAD_CROP_BITMAP;
        if (grid_fitting) {
            // Optimize grid fitting for gray-scale rendering (as opposed to
            // pure black and white)
            flags |= FT_LOAD_TARGET_NORMAL;
        }
        else {
            flags |= FT_LOAD_NO_HINTING;
        }
        if (FT_Load_Glyph(face, i, flags))
            throw std::runtime_error("FT_Load_Glyph failed");

        hori_glyph_advance = 1/64.0 * glyph->metrics.horiAdvance;

        // Freetype always loads a glyph such that the origin of the outline
        // description coincides with the bearing point pertaining to a
        // horizontal layout. Therefore, to acheive the direction neutral
        // position where the origin of the outline description is the lower
        // left corner of the bounding box, we need to make a correction.
        double left   = 1/64.0 *  glyph->metrics.horiBearingX;
        double top    = 1/64.0 *  glyph->metrics.horiBearingY;
        double right  = 1/64.0 * (glyph->metrics.horiBearingX + glyph->metrics.width);
        double bottom = 1/64.0 * (glyph->metrics.horiBearingY - glyph->metrics.height);

        // Grid fitting of the glyph metrics will normally already have been
        // done by FreeType, but since that behavior appears to be compile-time
        // configurable, the rounding is repeated here. Fortunately rounding is
        // an idempotent operation.
        if (grid_fitting) {
            hori_glyph_advance = std::round(hori_glyph_advance);
            left   = std::floor(left);
            bottom = std::floor(bottom);
            right  = std::ceil(right);
            top    = std::ceil(top);
        }

        // Vector from bearing point of vertical layout to bearing point of
        // horizontal layout
        //
        // FIXME: It seems that in some cases such as "Liberation Serif", the
        // vertical metrics are set to appropriate values even when the
        // underlying font face does not provide any. If that were always the
        // case, there would be no point in emulating those metrics
        // below. Problem is, according to the documentation, the vertical
        // metrics must be considered unrelibale when FT_HAS_VERTICAL(face)
        // returns false.
        Vec2 v2h;
        if (FT_HAS_VERTICAL(face)) {
            vert_glyph_advance = 1/64.0 * glyph->metrics.vertAdvance;
            v2h  = 1/64.0 * Vec2(glyph->metrics.vertBearingX - glyph->metrics.horiBearingX,
                                 glyph->metrics.vertAdvance -
                                 glyph->metrics.vertBearingY - glyph->metrics.horiBearingY);
            if (grid_fitting) {
                vert_glyph_advance = std::round(vert_glyph_advance);
                v2h[0] = std::round(v2h[0]);
                v2h[1] = std::round(v2h[1]);
            }
        }
        else { // Emulated vertical metrics
            if (grid_fitting) {
                vert_glyph_advance = hori_baseline_spacing_gf;
                v2h.set(std::round(-0.5 * hori_glyph_advance), hori_baseline_offset_gf);
            }
            else {
                vert_glyph_advance = hori_baseline_spacing;
                v2h.set(-0.5 * hori_glyph_advance, hori_baseline_offset);
            }
        }

        glyph_size.set(right - left, top - bottom);
        hori_glyph_bearing.set(-left, -bottom);
        vert_glyph_bearing = hori_glyph_bearing - v2h;
        prev_glyph_translation_x = -64 * hori_glyph_bearing[0];
        prev_glyph_translation_y = -64 * hori_glyph_bearing[1];
        glyph_translation.set(0);
    }

    double get_glyph_advance(bool vertical) const override
    {
        return (vertical ? vert_glyph_advance : hori_glyph_advance);
    }

    Vec2 get_glyph_bearing(bool vertical) const override
    {
        return (vertical ? vert_glyph_bearing : hori_glyph_bearing);
    }

    Vec2 get_glyph_size() const override
    {
        return glyph_size;
    }

    void translate_glyph(Vec2 v) override
    {
        glyph_translation += v;
    }

    void get_glyph_pixel_box(int& left, int& right, int& bottom, int& top) const override
    {
        if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
            left   = std::round(glyph_translation[0]);
            bottom = std::round(glyph_translation[1]);
            right = left   + glyph->bitmap.width;
            top   = bottom + glyph->bitmap.rows;
        }
        else {
            left   = std::floor(glyph_translation[0]);
            bottom = std::floor(glyph_translation[1]);
            right  = std::ceil(glyph_translation[0] + glyph_size[0]);
            top    = std::ceil(glyph_translation[1] + glyph_size[1]);
        }
    }

    void set_target_origin(int x, int y) override
    {
        target_origin_x = x;
        target_origin_y = y;
    }

    void render_pixels_to(ImageWriter& image_writer) const override
    {
        int left, right, bottom, top;
        get_glyph_pixel_box(left, right, bottom, top);
        image_writer.set_pos(target_origin_x + left, target_origin_y + bottom);

        int width = right - left, height = top - bottom;

        // First check if we can render the glyph directly and bypass the
        // intermediate buffer
        if (  (glyph->format == FT_GLYPH_FORMAT_BITMAP &&
               glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY &&
               0 < glyph->bitmap.num_grays &&
               unsigned(glyph->bitmap.num_grays-1) ==
               unsigned(std::numeric_limits<unsigned char>::max()))) {
            ssize_t pitch = 1, stride = -glyph->bitmap.pitch;
            const unsigned char* src =
                (stride < 0 ? glyph->bitmap.buffer - (height-1)*stride : glyph->bitmap.buffer);
            image_writer.put_block(src, pitch, stride, width, height, color_space_lum, false);
            return;
        }

        // Make sure our buffer is big enough to hold the affected pixel
        // block
        {
            std::size_t s = width * size_t(height);
            if (pix_buf_size < s) {
                std::size_t t = std::max(s, pix_buf_size + pix_buf_size/4); // Increase by at least 25%
                pix_buf = std::make_unique<unsigned char[]>(t); // Throws
                pix_buf_size = t;
            }
        }

        // Clear the buffer
        std::fill(pix_buf.get(), pix_buf.get()+pix_buf_size, 0);

        // Render into intermediate buffer
        if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
            ssize_t pitch = 1, stride = -glyph->bitmap.pitch;
            const unsigned char* src =
                (stride < 0 ? glyph->bitmap.buffer - (height-1)*stride : glyph->bitmap.buffer);
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
                        *dst = (src[(x>>3)*pitch] & 128>>(x&7) ?
                                std::numeric_limits<unsigned char>::max() : 0);
                }
            }
            else {
                throw std::runtime_error("Unsupported pixel format of glyph");
            }
        }
        else { // Is scalable outline
            // Translate glyph
            {
                FT_Pos x = 64 * (glyph_translation[0] -   left);
                FT_Pos y = 64 * (glyph_translation[1] - bottom);
                if (x != prev_glyph_translation_x || y != prev_glyph_translation_y) {
                    FT_Outline_Translate(&glyph->outline,
                                         x - prev_glyph_translation_x,
                                         y - prev_glyph_translation_y);
                    prev_glyph_translation_x = x;
                    prev_glyph_translation_y = y;
                }
            }

            RenderTarget target(pix_buf.get(), width, height);
            FT_Raster_Params params;
            params.flags      = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
            params.gray_spans = &render_spans;
            params.user       = &target;
            FT_Outline_Render(loader->library, &glyph->outline, &params);
        }

        image_writer.put_block(pix_buf.get(), width, height, color_space_lum, false);
    }


    FaceImpl(const LoaderImpl* l, FT_Face f, double w, double h):
        loader(l->weak_self),
        face(f),
        glyph(face->glyph),
        has_kerning(FT_HAS_KERNING(f)),
        color_space_lum(ColorSpace::get_Lum())
    {
        ARCHON_ASSERT_1(0 < get_num_fixed_sizes() || is_scalable(),
                        "No fixed sizes in non-scalable font");

/*
        if (!face->charmap || face->charmap->encoding != FT_ENCODING_UNICODE)
            throw std::runtime_error("No Unicode charmap available in font face");
*/

        set_approx_size(w,h);

        // Implementation is obliged to load the replacement glyph initially
        int glyph_index = 0;
        bool grid_fitting = true;
        load_glyph(glyph_index, grid_fitting);
    }

    ~FaceImpl() noexcept override
    {
        FT_Done_Face(face);
    }

private:
    const std::shared_ptr<const LoaderImpl> loader;
    const FT_Face face;
    const FT_GlyphSlot glyph;
    bool has_kerning;
    const ColorSpace::ConstRef color_space_lum;

    // Values are fixed size indices. Used only by set_approx_size, and
    // initialized on demand.
    mutable std::map<Vec2, int> fixed_sizes;

    FT_F26Dot6 render_width, render_height;

    double hori_baseline_offset, hori_baseline_spacing;
    double vert_baseline_offset, vert_baseline_spacing;

    int hori_baseline_offset_gf, hori_baseline_spacing_gf; // Grid fitted
    int vert_baseline_offset_gf, vert_baseline_spacing_gf; // Grid fitted

    double hori_glyph_advance, vert_glyph_advance;

    Vec2 glyph_size;
    Vec2 hori_glyph_bearing, vert_glyph_bearing;

    mutable FT_Pos prev_glyph_translation_x, prev_glyph_translation_y;
    Vec2 glyph_translation;

    // Position in target image of design tablet origin (in integer pixels)
    int target_origin_x = 0, target_origin_y = 0;

    // These are used only when copying glyphs with one bit per pixel.
    mutable std::size_t pix_buf_size = 0;
    mutable std::unique_ptr<unsigned char[]> pix_buf;
};



std::unique_ptr<FontFace> LoaderImpl::load_face(std::string f, int i, double w, double h) const
{
    FT_Face face;
    if (FT_New_Face(library, f.c_str(), i, &face))
        throw BadFontFileException("Failed to load \""+f+"\"");
    try {
        // Some font files have extra "strap on" files with metrics and
        // kerning information
        std::string suffix = file::suffix_of(f);
        if (suffix == "pfa" || suffix == "pfb") { // Type 1 fonts (a.k.a. PostScript fonts)
            std::string stem = file::dir_of(f)+file::stem_of(f)+".";
            std::string afm = stem + "afm";
            // We don't care if it fails, this is entirely opportunistic
            if (file::is_regular(afm))
                FT_Attach_File(face, afm.c_str());
        }
        return std::make_unique<FaceImpl>(this, face, w, h); // Throws
    }
    catch (...) {
        FT_Done_Face(face);
        throw;
    }
}

} // unnamed namespace


namespace archon {
namespace font {

std::shared_ptr<FontLoader> new_font_loader(std::string resource_dir)
{
    FT_Library library;
    if (FT_Init_FreeType(&library))
        throw std::runtime_error("Error initializing FreeType library");
    std::shared_ptr<LoaderImpl> loader;
    try {
        loader = std::make_shared<LoaderImpl>(library, resource_dir); // Throws
    }
    catch (...) {
        FT_Done_FreeType(library);
        throw;
    }
    const_cast<std::weak_ptr<LoaderImpl>&>(loader->weak_self) = loader;
    return loader;
}

} // namespace font
} // namespace archon
