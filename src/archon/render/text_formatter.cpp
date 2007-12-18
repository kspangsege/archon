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

#include <archon/core/memory.hpp>
#include <archon/render/text_formatter.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Math;
using namespace Archon::Font;


/*

Considerations:

Chars are maped to glyph indices/adv/kern using the associated font cache.

desired_glyphs_per_texture

desired_em_squares_per_texture = average_glyphs_per_em_square * desired_glyphs_per_texture

desired_texture_size = sqrt(desired_em_squares_per_texture * (horizontal__font_resolution + spacing) * (vertical_font_resolution + spacing)) + spacing


use desired_em_squares_per_texture and effective rendering size to determine texture size;

Per font:

  List of textures



For each glyph to be rendered by the TextProcessor:

  compute page index for glyph index

  look up page in page vector

  if page is not yet generated, generate it now

  look up glyph in page, yielding texture index and metrics



========= New attempt ==========

Make a new cache class in this namespace. Use it in this class almost the same way Font::TextRender uses Font::FontCache
The new cache class shall be responsible for generating and managing textures containing glyphs. It will require a reference to a Font::FontCache.
The new cache renderes a strip of glyphs into an abstract placeholder which will be implemented by the renderer.

Problem: how to represent the text layout in the placeholder such that a minimum number of texture switches are needed. Ideally we could achieve this while at the same time keep the intended glyph rendering order. Maybe this is impossible by definition.

The cache associates a number of textures with each font.
The glyphs of the font are divided into pages, and each page is generated when one of the glyphs of that page is needed.
The page of glyphs are sorted according to height of the glyph. then added into one of the textures.
for each font, the cache maintains up to two textures that are currently open to glyph additions. Each one has associated with it a RectPacker object that help utilize the texture "real estate".
When a glyph needs to be added to a texture:
  If there are no open textures, make a new one which becomes the primary texture.
  Try to add the glyph to the primary texture.
  If it fails:
    If there is no secondary texture, make one now.
    Attempt to add the glyph to the secondary texture.
    If it fails:
      Close the primary texture
      Use the current secondary texture as the new primary texture
      Open a new secondary texture
      Add the glyph to the secondary texture.
      It must succeeed since the texture is empty.

The texture size should be fixed and chosen based on font rendering size and desired number of glyphs per texture.
It seems like a good compromise to assume that the area of a glyph is about half the area of the EM-square on average.
In practice the average varies wildly between latin and chineese characters.
For chineese characters it often approaches one glyph per EM-square, and for the ASCII subset, it is somtimes as much as 4 glyphs per EM-square on average.




There is generally always one open texture. During the addition of a new page of glyphs, two open textures may exists simultaneously


Can glyph pages be discarded as follows?

When page becomes unused:
  For each texture, decrement the num_used_pages. If it drops to zero, drop the texture.
- Mark all glyphs of page unused. If any closed texture ends up having only unused glyphs, drop that texture.
- When a page is needed that was previously marked as unused
    Scan existing textures for ones that has glyphs for this page.
    Add all existing glyphs to a set (likely a paged set)
    Generate all remaining glyphs.

Font:
  size_t num_glyphs;
  Page: // Each one dynamically allocated
    size_t text_use_count; // One for each layout placeholder object that uses this page
    vector<Glyph> glyphs;
    Glyph:
      Texture *texture;
      index of glyph in texture

Texture: // Each one dynamically allocated
  vector<int> page_indicess; // Indices of all pages that contribute glyphs to this texture
  size_t text_use_count; // One for each layout placeholder object that uses this texture, when droppping to zero, texture_use must be nullified.
  int page_use_count // When it drops to zero, the texture is dropped
  TextureDecl texture_decl;
  TextureUse texture_use; // Reference broken unless 0 < text_use_count
  Glyph:
    glyph_index
    Position of lower left corner in relative texture coordinates
    size and bearing metrics

Want to mix at most two pages present in a single texture. This improves page separation and the ability to discard data associated with abandoned pages

render_glyphs must stash the result in a placeholder from which it can later be rendered proper using OpenGL. This is necessary beacause one might want to add the rendering to a call list and render_glyphs might need to call OpenGL functions that should not go into the call list.

In the text-layout placeholder, each used page of each used font must be recorded, such that when the placeholder is destroyed, the page use counters can be updated.


LayoutPlaceholder:
  List of used (font_id,counted_page_ref) pairs: Each of these must be reported as broken when the placeholder is cleared or destroyed.
  Texture: // Each one dynamically allocated
    Texture *const texture;
    num_strips
    Strip
      type: hori/vert
      num_glyphs    
    glyphs: Each one is an index into the specified texture


No no not at all anyway: I probably want to count the texture usage rather than the page usage, then when all textures of a page get unused, the page can go. Then all glyph metrics should go to the glyph list in the texture object.

*/




namespace Archon {
namespace Render {

class TextFormatter::TextProcessor: public TextHandler {
public:
    void handle(int style_id, int num_glyphs, const int* glyphs, const float* components)
    {
        inserter->insert_strip(style_id, num_glyphs, glyphs, components);
    }

    TextProcessor(FontProvider::TextInserter* i): inserter(i) {}

    FontProvider::TextInserter* const inserter;
};



TextFormatter::TextFormatter(FontProvider* p): font_provider(p)
{
    used_styles.reserve(8);
    FontProvider::StyleOwner style(font_provider, font_provider->acquire_default_style());
    font_provider->get_style_desc(style.get(), style_desc);
    used_styles.push_back(style.get());
    style_id = default_style = style.release();
    set_next_session_grid_fitting(false);
}


TextFormatter::~TextFormatter()
{
    style_id = 0; // Release everything
    release_used_styles();
}



void TextFormatter::format(TextLayout& layout, int page_index)
{
    SessionInfo info;
    get_session_info(info);

    layout.size = get_page_size(page_index);

    FontProvider::TextInserter inserter(font_provider, &layout.text, info.layout_direction);
    TextProcessor proc(&inserter);
    process_page(page_index, Vec2(0,0), proc);
}



void TextFormatter::clear()
{
    release_used_styles();
    clear_vector(used_styles, 8, 32);
    if (style_id)
        used_styles.push_back(style_id);
    Archon::Font::TextFormatter::clear();
}



// Overriding Archon::Font::TextFormatter::acquire_style
int TextFormatter::acquire_style()
{
    if (!style_id) {
        FontProvider::StyleOwner style(font_provider, font_provider->acquire_style(style_desc));
        used_styles.push_back(style.get());
        style_id = style.release();
    }
    return style_id;
}



void TextFormatter::get_style_info(int style_id, bool vertical, bool /*grid_fitting*/,
                                   FontCache::FontMetrics& metrics)
{
    font_provider->get_style_metrics(style_id, vertical, metrics);
}


void TextFormatter::get_glyph_info(int style_id, bool vertical, bool /*grid_fitting*/,
                                   FontCache::KernType kern, int num_chars, const wchar_t* chars,
                                   FontCache::GlyphInfo* glyphs)
{
    font_provider->get_glyph_info(style_id, vertical, kern, num_chars, chars, glyphs);
}



// Does not release the one that is currently used
void TextFormatter::release_used_styles() throw()
{
    while (!used_styles.empty()) {
        int id = used_styles.back();
        used_styles.pop_back();
        if (id != style_id)
            font_provider->release_style(id);
    }
}


} // namespace Render
} // namespace Archon
