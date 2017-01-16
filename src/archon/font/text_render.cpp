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

#include <archon/core/memory.hpp>
#include <archon/font/text_render.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;


namespace archon {
namespace font {

class TextRenderer::TextProcessor: public TextHandler {
public:
    void handle(int style_id, int num_glyphs, const int* glyphs, const float* components)
    {
        const Style& s = renderer.m_styles[style_id - 1];
        set_color(s.text_color);
        renderer.m_cache->render_text(s.font_id, grid_fitting, layout_direction,
                                      num_glyphs, glyphs, components, img_writer);
    }

    TextProcessor(const TextRenderer& r, const SessionInfo& info, ImageWriter& w):
        renderer{r},
        grid_fitting{info.grid_fitting},
        layout_direction{info.layout_direction},
        img_writer{w}
    {
        w.enable_blending().enable_color_mapping();
        set_color(PackedTRGB(), true);
    }

    void set_color(PackedTRGB color, bool force = false)
    {
        if (force || color != last_color) {
            img_writer.set_background_color(PackedTRGB(color.value() | 0xFF000000));
            img_writer.set_foreground_color(color);
            last_color = color;
        }
    }

    const TextRenderer& renderer;
    bool grid_fitting;
    FontCache::Direction layout_direction;
    ImageWriter& img_writer;
    PackedTRGB last_color;
};


class TextRenderer::StructProcessor: public StructHandler {
public:
    void line_box(const Vec2& pos, const Vec2& size)
    {
        if (size[0] <= 0 || size[1] <= 0)
            return;
        fill_rect(pos, size, PackedTRGB(even_line ? 0xB0E0FF : 0xFFE0B0));
        even_line = !even_line;
        even_glyph = false;
    }

    void glyph_box(const Vec2& pos, const Vec2& size)
    {
        if (size[0] <= 0 || size[1] <= 0)
            return;
        fill_rect(pos, size, PackedTRGB(even_glyph ? 0xD0FFFF : 0xFFFFD0));
        even_glyph = !even_glyph;
    }

    void baseline(double pos, bool vertical, bool before, int type)
    {
        int x = 0, y = std::round(pos);
        int w = vertical ? img_writer.get_height() : img_writer.get_width(), h = 1;
        if (before)
            --y;
        if (vertical) {
            std::swap(x,y);
            std::swap(w,h);
        }
        unsigned long trgb = (type == 0 ? 0xFF0000 : type == 1 ? 0x0000FF : 0x00C000);
        img_writer.set_clip(x,y,w,h).set_foreground_color(PackedTRGB(trgb)).fill();
    }

    void fill_rect(const Vec2& pos, const Vec2& size, PackedTRGB color)
    {
        int x = std::round(pos[0]);
        int y = std::round(pos[1]);
        int w = int(std::round(pos[0] + size[0])) - x;
        int h = int(std::round(pos[1] + size[1])) - y;
        img_writer.set_clip(x,y,w,h).set_foreground_color(color).fill();
    }

    StructProcessor(ImageWriter& w):
        img_writer{w}
    {
    }

    ImageWriter& img_writer;
    bool first_line = true, even_line = false, even_glyph;
};


Image::Ref TextRenderer::render(int page_index, bool debug)
{
    SessionInfo info;
    get_session_info(info);

    Vec2 page_size = get_page_size(page_index);
    Vec2 offset;
    int width, height;
    if (info.grid_fitting) {
        offset[0] = std::round(m_padding_left);
        offset[1] = std::round(m_padding_bottom);
        width  = offset[0] + page_size[0] + std::round(m_padding_right);
        height = offset[1] + page_size[1] + std::round(m_padding_top);
    }
    else {
        double w = m_padding_left   + page_size[0] + m_padding_right;
        double h = m_padding_bottom + page_size[1] + m_padding_top;
        width  = std::ceil(w);
        height = std::ceil(h);
        offset[0] = m_padding_left   + 0.5*(width  - w);
        offset[1] = m_padding_bottom + 0.5*(height - h);
    }

    int full_width = width, full_height = height;
    int border_hori = m_border_left   + m_border_right;
    int border_vert = m_border_bottom + m_border_top;
    bool border = 0 < border_hori || 0 < border_vert;
    if (border) {
        offset[0] += m_border_left;
        offset[1] += m_border_bottom;
        full_width  += border_hori;
        full_height += border_vert;
    }

    if (full_width < 1 || full_height < 1)
        return Image::Ref{}; // Null

    bool has_alpha = m_background_color.value() & 0xFF000000;
    Image::Ref img = Image::new_image(full_width, full_height, ColorSpace::get_RGB(), has_alpha);
    ImageWriter writer{img};

    if (width < 1 || height < 1) {
        writer.set_foreground_color(m_border_color).fill();
        return img;
    }

    writer.set_background_color(m_background_color).clear();

    if (debug) {
        StructProcessor proc{writer};
        process_page_struct(page_index, offset, proc);
        writer.set_clip(); // Reset
    }

    if (border) {
        writer.set_foreground_color(m_border_color);
        if (m_border_bottom)
            writer.set_clip(0, 0, full_width, m_border_bottom).fill();
        if (m_border_top)
            writer.set_clip(0, full_height - m_border_top, full_width, m_border_top).fill();
        if (m_border_left)
            writer.set_clip(0, m_border_bottom, m_border_left, height).fill();
        if (m_border_right)
            writer.set_clip(full_width - m_border_right, m_border_bottom, m_border_right, height).fill();
        writer.set_clip(m_border_left, m_border_bottom, width, height);
    }

    {
        TextProcessor proc{*this, info, writer};
        process_page(page_index, offset, proc);
    }

    return img;
}



void TextRenderer::clear()
{
    release_used_fonts();
    clear_vector(m_used_fonts, 8, 32);
    if (0 <= m_font_id)
        m_used_fonts.push_back(m_font_id);
    TextFormatter::clear();
}



TextRenderer::TextRenderer(std::shared_ptr<FontCache> c):
    m_cache{std::move(c)}
{
    m_used_fonts.reserve(8);
    FontCache::FontOwner font{m_cache, m_cache->acquire_default_font()};
    m_cache->get_font_desc(font.get(), m_font_desc);
    m_used_fonts.push_back(font.get());
    m_default_font = font.release();
}


TextRenderer::~TextRenderer()
{
    m_font_id = -1; // Release everything
    release_used_fonts();
}



// Overriding TextFormatter::acquire_style()
int TextRenderer::acquire_style()
{
    if (m_font_id < 0) {
        FontCache::FontOwner f{m_cache, m_cache->acquire_font(m_font_desc)};
        m_used_fonts.push_back(f.get());
        m_font_id = f.release();
    }

    Style s{m_font_id, m_text_color};
    int& id = m_style_map[s];
    if (id == 0) { // New
        m_styles.push_back(s);
        id = m_styles.size(); // One plus index
    }

    return id;
}


// Overriding TextFormatter::get_style_info()
void TextRenderer::get_style_info(int style_id, bool vertical, bool grid_fitting,
                                  FontCache::FontMetrics& info)
{
    m_cache->get_font_metrics(m_styles[style_id-1].font_id, vertical, grid_fitting, info);
}


// Overriding TextFormatter::get_glyph_info()
void TextRenderer::get_glyph_info(int style_id, bool vertical, bool grid_fitting,
                                  FontCache::KernType kern, int num_chars,
                                  const wchar_t* chars, FontCache::GlyphInfo* glyphs)
{
    m_cache->get_glyph_info(m_styles[style_id-1].font_id, vertical, grid_fitting, kern,
                            num_chars, chars, glyphs);
}


// Does not release the one that is currently used
void TextRenderer::release_used_fonts() noexcept
{
    while (!m_used_fonts.empty()) {
        int id = m_used_fonts.back();
        m_used_fonts.pop_back();
        if (id != m_font_id)
            m_cache->release_font(id);
    }
}

} // namespace font
} // namespace archon
