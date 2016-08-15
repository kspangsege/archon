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

#ifndef ARCHON_RENDER_TEXT_FORMATTER_HPP
#define ARCHON_RENDER_TEXT_FORMATTER_HPP

#include <vector>

#include <archon/math/vector.hpp>
#include <archon/font/text_format.hpp>
#include <archon/render/font_provider.hpp>


/*

About font size:

A certain desired rendering size is chosen. The rendering size is the
same as the size (or resolution) of the EM-square. It will probably be
set to 64x64 pixels.

When a particular font is used, the rendering size is chosen to match
the desired size as closly as possible, but in some cases the
effective size will be very different.

In any case, the size of rendered text will be such that the EM-square
corresponding with the desired resolution will appear as a square with
side length equal to 1 in OpenGL coordinates.

It makes no sense to ever have grid fitting enabled. Why?

*/

namespace archon {
namespace render {

class TextLayout;



class TextFormatter: public font::TextFormatter {
public:
    /// Ownership of the font provider remains with the caller.
    TextFormatter(FontProvider*);


    /// Set nominal width and height of individual glyphs. A value of
    /// (1,1) means that the EM-square corresponding with the desired
    /// glyph resolution would appear with the same size as a 1-by-1
    /// square rendered using ordinary OpenGL primitives.
    ///
    /// If \a height is less than or equal to 0, then it will be set
    /// equal to \a width.
    ///
    /// The default size is (1,1).
    void set_font_size(double width, double height = 0);


    /// Set the degree of boldness of the desired font. Zero means
    /// 'not bold', 1 means normal boldness. Any value is legal, but
    /// may not be honored. A negative value should be used to select
    /// a font that is lighter than normal.
    ///
    /// The default is 'not bold at all'.
    ///
    /// FIXME: Should be renamed to set_font_weight().
    void set_font_boldness(double frac = 1);


    /// Set the degree of italicity of the desired font. Zero means
    /// 'not italic', 1 means normal italicity. Any value is legal,
    /// but may not be honored.
    ///
    /// The default is 'not italic at all'.
    ///
    /// FIXME: Should be renamed to set_font_style().
    void set_font_italicity(double frac = 1);


    /// Set the family name of the desired font.
    ///
    /// The default family name is the empty string, which means that
    // an implementation given default is chosen.
    void set_font_family(std::string family_name);


    /// Reset the font size, style, and family to the default. The
    /// currently chosen text color is not affected.
    void reset_font();


    /// Set current text color (RGB + Alpha). This affects any new
    /// text that is written to the formatter.
    ///
    /// The default text color is white witch alpha = 1.
    void set_text_color(const math::Vec4F& rgba);


    /// Get the current text color.
    math::Vec4F get_text_color() const;


    /// Upon return, the TextLayout object will contain a reference to
    /// the FontProvider that is associated with this
    /// TextFormatter. That means you must make sure that the
    /// FontProvider instance is not destoyed before the TextLayout
    /// object is either destroyed, cleared, or overwitten.
    ///
    /// \param target Any previous contents is removed.
    void format(TextLayout& target, int page_index = 0);


    // Overriding font::TextFormatter::clear()
    void clear();

    ~TextFormatter();


private:
    class TextProcessor;

    int acquire_style();

    void get_style_info(int style_id, bool vertical, bool grid_fitting,
                        font::FontCache::FontMetrics& metrics);

    void get_glyph_info(int style_id, bool vertical, bool grid_fitting,
                        font::FontCache::KernType kern, int num_chars,
                        const wchar_t* chars, font::FontCache::GlyphInfo* glyphs);

    void release_used_styles() throw();


    FontProvider* const font_provider;

    FontProvider::StyleDesc style_desc;
    int style_id; // ID of currently used style
    std::vector<int> used_styles;
    int default_style; // ID of default style, also added to used_styles
};



class TextLayout {
public:
    /// Create an empty text layout.
    TextLayout();


    /// Create a layout of the specified text using the specified
    /// formatter.
    ///
    /// \sa TextFormatter::format()
    TextLayout(TextFormatter* formatter, std::wstring text, int page_idx = 0);


    void set(TextFormatter* formatter, std::wstring text, int page_idx = 0);


    double get_width()  const;
    double get_height() const;


    /// Render this text layout using the OpenGL context that is
    /// currently bound to the calling thread.
    ///
    /// You may embed this call in an OpenGL call list. If you do so,
    /// you must ensure that the text container remains unmodified for
    /// as long as the OpenGL call list.
    ///
    /// \note The current texture binding will be clobbered.
    void render() const;


    void clear();

private:
    friend class TextFormatter;
    FontProvider::TextContainer text;
    math::Vec2 size;
};








// Implementation:

inline void TextFormatter::set_font_size(double width, double height)
{
    if (height <= 0)
        height = width;
    if (width == style_desc.font_size[0] && height == style_desc.font_size[1])
        return;
    request_style_update(true);
    style_desc.font_size.set(width, height);
    style_id = 0; // Request new style
}


inline void TextFormatter::set_font_boldness(double boldness)
{
    if (boldness == style_desc.font_boldness)
        return;
    request_style_update(true);
    style_desc.font_boldness = boldness;
    style_id = 0; // Request new style
}


inline void TextFormatter::set_font_italicity(double italicity)
{
    if (italicity == style_desc.font_italicity)
        return;
    request_style_update(true);
    style_desc.font_italicity = italicity;
    style_id = 0; // Request new style
}


inline void TextFormatter::set_font_family(std::string family)
{
    if (family == style_desc.font_family)
        return;
    request_style_update(true);
    style_desc.font_family = family;
    style_id = 0; // Request new style
}


inline void TextFormatter::reset_font()
{
    FontProvider::StyleDesc desc;
    font_provider->get_style_desc(default_style, desc);
    if (desc == style_desc)
        return;
    request_style_update(true);
    style_desc = desc;
    style_id = 0; // Request new style
}


inline void TextFormatter::set_text_color(const math::Vec4F& rgba)
{
    if (rgba == style_desc.text_color)
        return;
    request_style_update(false);
    style_desc.text_color = rgba;
    style_id = 0; // Request new style
}


inline math::Vec4F TextFormatter::get_text_color() const
{
    return style_desc.text_color;
}


inline TextLayout::TextLayout()
{
    clear();
}


inline TextLayout::TextLayout(TextFormatter* formatter, std::wstring text, int page_idx)
{
    set(formatter, text, page_idx);
}


inline void TextLayout::set(TextFormatter* formatter, std::wstring text, int page_idx)
{
    formatter->clear();
    formatter->write(text);
    formatter->format(*this, page_idx);
}


inline double TextLayout::get_width()  const
{
    return size[0];
}


inline double TextLayout::get_height() const
{
    return size[1];
}


inline void TextLayout::render() const
{
    text.render();
}


inline void TextLayout::clear()
{
    text.clear();
    size.set(0,0);
}


} // namespace render
} // namespace archon

#endif // ARCHON_RENDER_TEXT_FORMATTER_HPP
