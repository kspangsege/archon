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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_FONT_TEXT_RENDER_HPP
#define ARCHON_FONT_TEXT_RENDER_HPP

#include <algorithm>
#include <vector>

#include <archon/util/hash_map.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/image/image.hpp>
#include <archon/font/text_format.hpp>


namespace archon {
namespace Font {

/**
 * Not thread-safe.
 */
struct TextRenderer: TextFormatter {
    TextRenderer(FontCache::Arg cache);

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
    void set_text_color(Util::PackedTRGB color);

    // Set the background color. This only affects the final rendering. Default is white.
    void set_background_color(Util::PackedTRGB color);

    // Set the border color. This only affects the final rendering. Default is black.
    void set_border_color(Util::PackedTRGB color);

    // Components will be rounded to nearest integer if grid fitting is enabled for the active session.
    // The default is (4,4,4,4).
    void set_padding(double top, double right, double bottom, double left);

    // Default is (0,0,0,0)
    void set_border_width(int top, int right, int bottom, int left);

    /**
     * If a layout session is already initiated, this setting will
     * not have any effect until clear() is called and a new session
     * is started.
     *
     * Grid fitting is enabled by default.
     */
    void enable_grid_fitting(bool enabled);

    // Returns null if the image size would have been zero.
    // Set debug to true if you want this method to render extra features that are helpful when debugging.
    Imaging::Image::Ref render(int page_index = 0, bool debug = false);

    // Overriding TextFormatter::clear()
    void clear();

private:
    struct TextProcessor;
    struct StructProcessor;

    int acquire_style();

    void get_style_info(int style_id, bool vertical, bool grid_fitting,
                        FontCache::FontMetrics& info);

    void get_glyph_info(int style_id, bool vertical, bool grid_fitting, FontCache::KernType kern,
                        int num_chars, const wchar_t* chars, FontCache::GlyphInfo* glyphs);

    void release_used_fonts() throw();

    const FontCache::Ptr cache;

    FontCache::FontDesc font_desc;
    int font_id; // ID of currently used font, or -1 if none are used.
    std::vector<int> used_fonts;
    int default_font; // ID of default font, also added to used_fonts

    Util::PackedTRGB text_color, background_color, border_color;
    double padding_top, padding_right, padding_bottom, padding_left;
    int border_top, border_right, border_bottom, border_left;

    struct Style {
        int font_id;
        Util::PackedTRGB text_color; // TRGB
        Style(int f, Util::PackedTRGB c): font_id(f), text_color(c) {}
        bool operator==(const Style& s) const
        {
            return font_id == s.font_id && text_color == s.text_color;
        }
    };

    struct StyleHasher {
        static int hash(const Style& s, int n)
        {
            Util::Hash_FNV_1a_32 h;
            h.add_int(s.font_id);
            h.add_int(s.text_color.value());
            return h.get_hash(n);
        }
    };

    std::vector<Style> styles;
    Util::HashMap<Style, int, StyleHasher> style_map; // Value is one plus index in 'styles'.
};




// Implementation

inline void TextRenderer::set_font_size(double width, double height)
{
    if (width == font_desc.size[0] && height == font_desc.size[1])
        return;
    request_style_update(true);
    font_desc.size.set(width, height);
    font_id = -1; // Request new font
}

inline void TextRenderer::set_font_boldness(double boldness)
{
    if (boldness == font_desc.boldness)
        return;
    request_style_update(true);
    font_desc.boldness = boldness;
    font_id = -1; // Request new font
}

inline void TextRenderer::set_font_italicity(double italicity)
{
    if (italicity == font_desc.italicity)
        return;
    request_style_update(true);
    font_desc.italicity = italicity;
    font_id = -1; // Request new font
}

inline void TextRenderer::set_font_family(std::string family)
{
    if (family == font_desc.family)
        return;
    request_style_update(true);
    font_desc.family = family;
    font_id = -1; // Request new font
}

inline void TextRenderer::reset_font()
{
    FontCache::FontDesc desc;
    cache->get_font_desc(default_font, desc);
    if (desc == font_desc)
        return;
    request_style_update(true);
    font_desc = desc;
    font_id = -1; // Request new font
}

inline void TextRenderer::set_text_color(Util::PackedTRGB color)
{
    if (color == text_color)
        return;
    request_style_update(false);
    text_color = color;
}

inline void TextRenderer::set_background_color(Util::PackedTRGB color)
{
    background_color = color;
}

inline void TextRenderer::set_border_color(Util::PackedTRGB color)
{
    border_color = color;
}

inline void TextRenderer::set_padding(double top, double right, double bottom, double left)
{
    padding_top    = top;
    padding_right  = right;
    padding_bottom = bottom;
    padding_left   = left;
}

inline void TextRenderer::set_border_width(int top, int right, int bottom, int left)
{
    border_top    = std::max(top,    0);
    border_right  = std::max(right,  0);
    border_bottom = std::max(bottom, 0);
    border_left   = std::max(left,   0);
}

inline void TextRenderer::enable_grid_fitting(bool enabled)
{
    set_next_session_grid_fitting(enabled);
}

} // namespace Font
} // namespace archon

#endif // ARCHON_FONT_TEXT_RENDER_HPP
