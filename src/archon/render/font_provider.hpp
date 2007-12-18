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

#ifndef ARCHON_RENDER_FONT_PROVIDER_HPP
#define ARCHON_RENDER_FONT_PROVIDER_HPP

#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <archon/core/types.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/core/memory.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/hash_map.hpp>
#include <archon/util/rep_map_lookup_boost.hpp>
#include <archon/util/rect_packer.hpp>
#include <archon/font/cache.hpp>
#include <archon/render/texture_cache.hpp>


namespace Archon {
namespace Render {

/**
 * A provider of text fonts for rendering text in OpenGL. The fonts are provided
 * as a set of textures, each one holding a set of glyphs.
 *
 * \param save_textures_to_disk This is only a debugging feature. Each generated
 * texture is saved as a PNG file in the directory for temporary files.
 */
class FontProvider {
public:
    FontProvider(Font::FontCache::Arg, TextureCache&,
                 const Math::Vec2F& desired_glyph_resol = Math::Vec2F(64,64),
                 bool enable_mipmap = true, bool save_textures_to_disk = false);


    /**
     * Fetch the default font style. The result is the same as one would get by
     * calling acquire_style() passing a description of the default style.
     *
     * It is guaranteed that calling this method does not cause the associated
     * FontList to scan through the font path for further font files.
     *
     * \sa acquire_style()
     */
    int acquire_default_style();


    struct StyleDesc {
        std::string font_family;
        double font_boldness, font_italicity;
        Math::Vec2F font_size;
        Math::Vec4F text_color;
        bool operator==(const StyleDesc&) const throw();
    };

    /**
     * Will always succeed. The returned font, however, may not be exactly what
     * you requested. To the greatest possible extent, it will be the best match
     * among the available fonts.
     *
     * When the returned font ID is no longer needed, it must be released by
     * passing it to release_style().
     *
     * \return A numerical identifier for the specified style. The returned
     * value is never zero.
     */
    int acquire_style(const StyleDesc&);

    /**
     * Tell the font provider that you are no longer interested in the specified
     * font.
     */
    void release_style(int style_id)
    {
        release_style_fast(style_id);
    }


    /**
     * Get the descriptor for the specified style.
     */
    void get_style_desc(int style_id, StyleDesc& desc);


    /**
     * RAII scheme (Resource acquisition is initialization) for style IDs.
     */
    struct StyleOwner {
        StyleOwner(FontProvider* p, int style_id = 0) throw():
            provider(p),
            style(style_id)
        {
        }
        int get() const throw()
        {
            return style;
        }
        int release() throw()
        {
            int s = style;
            style = 0;
            return s;
        }
        void reset(int style_id = -1);
        ~StyleOwner()
        {
            if (style)
                provider->release_style(style);
        }
    private:
        StyleOwner(const StyleOwner&);
        FontProvider* const provider;
        int style;
    };


    void get_style_metrics(int style_id, bool vertical, Font::FontCache::FontMetrics& metrics);

    void get_glyph_info(int style_id, bool vertical, Font::FontCache::KernType kern,
                        int num_chars, const wchar_t* chars, Font::FontCache::GlyphInfo* glyphs);


    ~FontProvider() throw ();


    class TextContainer;
    class TextInserter;


private:
    friend class TextContainer;

    struct Style {
        int font_id; // As known to Font::FontCache
        Math::Vec2F font_size;
        Math::Vec4F text_color;
        Style()
        {
        }
        Style(int font_id, const Math::Vec2F& font_size, const Math::Vec4F& text_color);
        bool operator==(const Style& s) const;
    };

    struct StyleEntry {
        std::size_t use_count = 0;
        Style style;
        Math::Vec2F font_scaling;
    };

    struct StyleHasher {
        static int hash(const Style& s, int n);
    };

    typedef std::vector<StyleEntry> Styles;
    Styles styles;
    Util::HashMap<Style, int, StyleHasher> style_map; // Value is one plus index in 'styles'.
    std::vector<int> unused_styles; // One plus indexes into 'styles'


    struct TextureFontSource;
    class Texture;
    class Page;
    class FontEntry;

    int acquire_style(Font::FontCache::FontOwner& font, const Math::Vec2F& font_size,
                      const Math::Vec4F& text_color);
    void release_style_fast(int style_id);
    void provide(int style_id, int num_glyphs, const int* glyphs, const float* components,
                 TextInserter& inserter);
    FontEntry* new_font(int font_id);
    Page* new_page(FontEntry* font, int page_idx);
    void new_texture(FontEntry* font, int page_idx, int tex_ord,
                     Core::UniquePtr<Util::RectanglePacker>& packer, Core::UIntMin16& tex_idx);
    void render(const TextContainer& text) const;
    void release(TextContainer& text);

    const Font::FontCache::Ptr font_cache;
    TextureCache& texture_cache;
    const Math::Vec2F desired_glyph_resol; // Ask cache for this rendering size
    const Math::Vec2 size_of_pixel; // Inverse of desired glyph resolution
    const bool enable_mipmap; // Do mipmapping on textures
    const bool save_textures; // Save each of the generated textures as a PNG file in /tmp/

    typedef std::map<int, FontEntry*> FontMap;
    FontMap font_map;
    Core::DeletingVector<FontEntry> fonts;
    Core::DeletingVector<Texture> textures;

    std::size_t used_pages = 0, used_textures = 0;
};



/**
 * Holds strips of glyphs.
 *
 * An instance of this class remains associated with the font provider that was
 * last passed to the TextInserter constructor alongside this text instance. For
 * this reason, the application must ensure that this instance is destroyed (or
 * cleared) before the font provider is.
 */
class FontProvider::TextContainer {
public:
    /**
     * Render this text using the OpenGL context that is currently bound to the
     * calling thread.
     *
     * You may embed this call in an OpenGL call list. If you do so, you must
     * ensure that the text container remains unmodified for as long as the
     * OpenGL call list.
     */
    void render() const
    {
        if (provider)
            provider->render(*this);
    }

    void clear();

    ~TextContainer()
    {
        clear();
    }

private:
    friend class FontProvider;
    FontProvider* provider = nullptr;
    Font::FontCache::Direction layout_direction;

    typedef std::pair<int, int> PageRef; // Page of glyps (cache_font_id, page_index)
    typedef std::vector<PageRef> PageRefs;
    PageRefs page_refs;

    struct Strip {
        int style_idx; // Index in 'provider->styles' of style of this strip
        float lateral_pos; // Position of baseline
        int num_glyphs = 0;
        Strip(int s, float p):
            style_idx(s),
            lateral_pos(p)
        {
        }
    };
    struct Glyph {
        int index; // Index in texture
        float position;
        Glyph(int i, float p):
            index(i),
            position(p)
        {
        }
    };
    typedef std::vector<Strip> Strips;
    typedef std::vector<Glyph> Glyphs;
    struct Texture {
        FontProvider::Texture* texture;
        Strips  strips;
        Glyphs glyphs;
        Texture(FontProvider::Texture* t):
            texture(t)
        {
        }
    };
    typedef std::list<Texture> Textures;
    Textures textures;
};



class FontProvider::TextInserter {
public:
    /**
     * This clears the specified text container.
     *
     * Application must ensure that neither the font provider nor the text
     * object is destroyed before this object is. Ownership of both the provider
     * and the text remains with the caller.
     *
     * \note You must make sure that two inserters never exist for the same
     * container at the same time.
     */
    TextInserter(FontProvider*, TextContainer*, Font::FontCache::Direction);

    /**
     * \param font_id The ID of the style that the glyph indices refer to. The
     * ID must have previously been obtained by calling
     * FontProvider::acquire_style() in the associated FontProvider instance.
     */
    void insert_strip(int style_id, int num_glyphs, const int* glyphs, const float* components);

private:
    friend class FontProvider;

    FontProvider* const font_provider;
    TextContainer* const text;
    const bool vertical; // True iff the layout is vertical

    typedef std::vector<bool> PageSet;
    std::map<int, PageSet> page_sets;
    // Only to speed up page_set lookups
    int last_font_id = -1;
    FontEntry* last_font;
    PageSet* last_page_set;

    typedef std::map<int, TextContainer::Texture*> Textures;
    Textures textures;
    typedef Util::RepMapLookupBooster<Textures, 4> TextureLookup;
    TextureLookup texture_lookup;
    typedef std::pair<int, TextContainer::Texture*> StripTexture;
    typedef std::vector<StripTexture> StripTextures;
    StripTextures strip_textures;
};




// Implementation

inline bool FontProvider::StyleDesc::operator==(const StyleDesc& d) const throw()
{
    return font_family == d.font_family && font_boldness == d.font_boldness &&
        font_italicity == d.font_italicity && font_size == d.font_size &&
        text_color == d.text_color;
}

inline void FontProvider::StyleOwner::reset(int style_id)
{
    int s = style;
    style = style_id;
    if (s)
        provider->release_style(s);
}

inline FontProvider::Style::Style(int i, const Math::Vec2F& s, const Math::Vec4F& c):
    font_id(i),
    font_size(s),
    text_color(c)
{
}

inline bool FontProvider::Style::operator==(const Style& s) const
{
    return font_id == s.font_id && font_size == s.font_size && text_color == s.text_color;
}

inline int FontProvider::StyleHasher::hash(const Style& s, int n)
{
    Util::Hash_FNV_1a_32 h;
    h.add_int(s.font_id);
    h.add_float(s.font_size[0]);
    h.add_float(s.font_size[1]);
    h.add_float(s.text_color[0]);
    h.add_float(s.text_color[1]);
    h.add_float(s.text_color[2]);
    h.add_float(s.text_color[3]);
    return h.get_hash(n);
}

class FontProvider::Texture {
public:
    const FontEntry* const font;
    struct Glyph {
        int index; // Index of glyph in font as known to Font::FontCache.
        int img_x, img_y; // Position of glyph in texture image.
        Font::FontCache::GlyphBoxInfo quad_info; // Size and position of GL quad. All distances specified relative to EM-square.
        Math::Vec2F tex_lower_left, tex_upper_right; // Position in relative coordinates of glyph in texture
    };
    std::vector<Glyph> glyphs;
    TextureDecl decl; // A hook into the texture cache
    TextureUse use; // Must be non-null when, and only when text_use_count > 0
    int page_use_count = 0; // One for each page with a glyph in this texture
    int text_use_count = 0; // One for each TextContainer that refers to this texture.
    // Invariant: page_use_count == 0  ==>  text_use_count == 0
    Texture(const FontEntry* f):
        font(f)
    {
    }
};


class FontProvider::Page {
public:
    struct Glyph {
        Core::UIntMin16 texture; // Index of texture
        Core::UIntMin16 index; // Index of glyph in texture
    };
    std::vector<Glyph> glyphs;
    int text_use_count = 0; // One for each TextContainer that refers to a glyph from this page.
};


class FontProvider::FontEntry {
public:
    const int id; // ID as known to Font::FontCache
    std::string name;
    int num_glyphs = 0;
    bool grid_fitting;
    int texture_width, texture_height;
    Math::Vec2 texture_scale; // Inverse of texture resolution
    Core::DeletingVector<Page> pages;

    Core::UniquePtr<Util::RectanglePacker> packer;
    Core::UIntMin16 open_texture_index; // Defined only if 'packer' is not null
    bool dirty_texture = false; // If new glyphs were rendered into the open texture, but the texture is not yet refreshed

    FontEntry(int i, std::string n):
        id(i),
        name(n)
    {
    }
};



inline void FontProvider::release_style_fast(int style_id)
{
    StyleEntry& style = styles[style_id-1];
    if (0 < --style.use_count)
        return;
    style_map.erase(style.style);
    font_cache->release_font(style.style.font_id);
    unused_styles.push_back(style_id);
}


inline void FontProvider::TextContainer::clear()
{
    if (provider) {
        provider->release(*this);
        provider = 0;
        page_refs.clear();
        textures.clear();
    }
}


inline FontProvider::TextInserter::TextInserter(FontProvider* p, TextContainer* t,
                                                Font::FontCache::Direction d):
    font_provider(p),
    text(t),
    vertical(d == Font::FontCache::dir_BottomToTop || d == Font::FontCache::dir_TopToBottom),
    texture_lookup(&textures)
{
    t->clear();
    t->layout_direction = d;
    t->provider = p;
}


inline void FontProvider::TextInserter::insert_strip(int style_id, int num_glyphs,
                                                     const int* glyphs,
                                                     const float* components)
{
    font_provider->provide(style_id, num_glyphs, glyphs, components, *this);
}

} // namespace Render
} // namespace Archon

#endif // ARCHON_RENDER_FONT_PROVIDER_HPP
