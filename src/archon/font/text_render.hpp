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

#ifndef ARCHON_FONT_TEXT_RENDER_HPP
#define ARCHON_FONT_TEXT_RENDER_HPP

#include <algorithm>
#include <vector>

#include <archon/util/hash_map.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/image/image.hpp>
#include <archon/font/text_format.hpp>


namespace archon {
namespace font {

/// Not thread-safe.
class TextRenderer: public TextFormatter {
public:
    /// The application must ensure that the specified FontCache object remains
    /// alive throughout the life of the new TextRenderer object.
    TextRenderer(FontCache&);

    ~TextRenderer();

    // Set nominal width and height of individual glyphs in pixels.
    // If either component is less than or equal to zero, the effective size is the default size of the selected font.
    // The default size is (0,0).
    void set_font_size(double width, double height);

    // Zero means 'not bold', 1 means normal boldness
    // The default value is 0.
    void set_font_boldness(double frac = 1);

    // Zero means 'not italic', 1 means normal italicity
    // The default value is 0.
    void set_font_italicity(double frac = 1);

    // The default family name is the empty string, which means that an implementation given default is chosen.
    void set_font_family(std::string family_name);

    // Reset the font size, style, and family to the default.
    // The current text color is not affected.
    void reset_font();

    // Set current text color. This affects any new text that is
    // written to the renderer. Default is black.
    void set_text_color(util::PackedTRGB color);

    // Set the background color. This only affects the final rendering. Default is white.
    void set_background_color(util::PackedTRGB color);

    // Set the border color. This only affects the final rendering. Default is black.
    void set_border_color(util::PackedTRGB color);

    // Components will be rounded to nearest integer if grid fitting is enabled for the active session.
    // The default is (4,4,4,4).
    void set_padding(double top, double right, double bottom, double left);

    // Default is (0,0,0,0)
    void set_border_width(int top, int right, int bottom, int left);

    /// If a layout session is already initiated, this setting will not have any
    /// effect until clear() is called and a new session is started.
    ///
    /// Grid fitting is enabled by default.
    void enable_grid_fitting(bool enabled);

    // Returns null if the image size would have been zero.
    // Set debug to true if you want this method to render extra features that are helpful when debugging.
    image::Image::Ref render(int page_index = 0, bool debug = false);

    void clear() override;

private:
    class TextProcessor;
    class StructProcessor;

    int acquire_style() override;

    void get_style_info(int style_id, bool vertical, bool grid_fitting,
                        FontCache::FontMetrics& info) override;

    void get_glyph_info(int style_id, bool vertical, bool grid_fitting, FontCache::KernType kern,
                        int num_chars, const wchar_t* chars,
                        FontCache::GlyphInfo* glyphs) override;

    void release_used_fonts() throw();

    FontCache& m_font_cache;

    FontCache::FontDesc m_font_desc;
    int m_font_id = -1; // ID of currently used font, or -1 if none are used.
    std::vector<int> m_used_fonts;
    int m_default_font; // ID of default font, also added to used_fonts

    util::PackedTRGB m_text_color       = util::PackedTRGB{0x000000};
    util::PackedTRGB m_background_color = util::PackedTRGB{0xFFFFFF};
    util::PackedTRGB m_border_color     = util::PackedTRGB{0x000000};
    double m_padding_top = 4, m_padding_right = 4, m_padding_bottom = 4, m_padding_left = 4;
    int m_border_top = 0, m_border_right = 0, m_border_bottom = 0, m_border_left = 0;

    struct Style {
        int font_id;
        util::PackedTRGB text_color; // TRGB
        Style(int f, util::PackedTRGB c): font_id(f), text_color(c) {}
        bool operator==(const Style& s) const
        {
            return font_id == s.font_id && text_color == s.text_color;
        }
    };

    struct StyleHasher {
        static int hash(const Style& s, int n)
        {
            util::Hash_FNV_1a_32 h;
            h.add_int(s.font_id);
            h.add_int(s.text_color.value());
            return int(h.get_hash(n));
        }
    };

    std::vector<Style> m_styles;
    util::HashMap<Style, int, StyleHasher> m_style_map; // Value is one plus index in 'styles'.
};




// Implementation

inline void TextRenderer::set_font_size(double width, double height)
{
    if (width == m_font_desc.size[0] && height == m_font_desc.size[1])
        return;
    request_style_update(true);
    m_font_desc.size.set(width, height);
    m_font_id = -1; // Request new font
}

inline void TextRenderer::set_font_boldness(double boldness)
{
    if (boldness == m_font_desc.boldness)
        return;
    request_style_update(true);
    m_font_desc.boldness = boldness;
    m_font_id = -1; // Request new font
}

inline void TextRenderer::set_font_italicity(double italicity)
{
    if (italicity == m_font_desc.italicity)
        return;
    request_style_update(true);
    m_font_desc.italicity = italicity;
    m_font_id = -1; // Request new font
}

inline void TextRenderer::set_font_family(std::string family)
{
    if (family == m_font_desc.family)
        return;
    request_style_update(true);
    m_font_desc.family = family;
    m_font_id = -1; // Request new font
}

inline void TextRenderer::reset_font()
{
    FontCache::FontDesc desc;
    m_font_cache.get_font_desc(m_default_font, desc);
    if (desc == m_font_desc)
        return;
    request_style_update(true);
    m_font_desc = desc;
    m_font_id = -1; // Request new font
}

inline void TextRenderer::set_text_color(util::PackedTRGB color)
{
    if (color == m_text_color)
        return;
    request_style_update(false);
    m_text_color = color;
}

inline void TextRenderer::set_background_color(util::PackedTRGB color)
{
    m_background_color = color;
}

inline void TextRenderer::set_border_color(util::PackedTRGB color)
{
    m_border_color = color;
}

inline void TextRenderer::set_padding(double top, double right, double bottom, double left)
{
    m_padding_top    = top;
    m_padding_right  = right;
    m_padding_bottom = bottom;
    m_padding_left   = left;
}

inline void TextRenderer::set_border_width(int top, int right, int bottom, int left)
{
    m_border_top    = std::max(top,    0);
    m_border_right  = std::max(right,  0);
    m_border_bottom = std::max(bottom, 0);
    m_border_left   = std::max(left,   0);
}

inline void TextRenderer::enable_grid_fitting(bool enabled)
{
    set_next_session_grid_fitting(enabled);
}

} // namespace font
} // namespace archon

#endif // ARCHON_FONT_TEXT_RENDER_HPP
