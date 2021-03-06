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

#ifndef ARCHON_RENDER_FONT_PROVIDER_HPP
#define ARCHON_RENDER_FONT_PROVIDER_HPP

#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <archon/core/types.hpp>
#include <archon/core/memory.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/hash_map.hpp>
#include <archon/util/rep_map_lookup_boost.hpp>
#include <archon/util/rect_packer.hpp>
#include <archon/font/cache.hpp>
#include <archon/render/texture_cache.hpp>


namespace archon {
namespace render {

/// A provider of text fonts for rendering text in OpenGL. The fonts are
/// provided as a set of textures, each one holding a set of glyphs.
///
/// \param save_textures_to_disk This is only a debugging feature. Each
/// generated texture is saved as a PNG file in the directory for temporary
/// files.
class FontProvider {
public:
    /// The application must ensure that the specified font::FontCache object
    /// remains alive throughout the life of the new FontProvider object.
    FontProvider(font::FontCache&, TextureCache&,
                 const math::Vec2F& desired_glyph_resol = math::Vec2F(64,64),
                 bool enable_mipmap = true, bool save_textures_to_disk = false);

    /// Fetch the default font style. The result is the same as one would get by
    /// calling acquire_style() passing a description of the default style.
    ///
    /// It is guaranteed that calling this method does not cause the associated
    /// FontList to scan through the font path for further font files.
    ///
    /// \sa acquire_style()
    int acquire_default_style();

    struct StyleDesc {
        std::string font_family;
        double font_boldness, font_italicity;
        math::Vec2F font_size; // Size of EM-square in object coordinates
        math::Vec4F text_color;
        bool operator==(const StyleDesc&) const throw();
    };

    /// Will always succeed. The returned font, however, may not be exactly what
    /// you requested. To the greatest possible extent, it will be the best
    /// match among the available fonts.
    ///
    /// When the returned font ID is no longer needed, it must be released by
    /// passing it to release_style().
    ///
    /// \return A numerical identifier for the specified style. The returned
    /// value is never zero.
    int acquire_style(const StyleDesc&);

    /// Tell the font provider that you are no longer interested in the
    /// specified font.
    void release_style(int style_id);

    /// Get the descriptor for the specified style.
    void get_style_desc(int style_id, StyleDesc& desc);

    /// RAII scheme for style IDs.
    class StyleOwner {
    public:
        StyleOwner() noexcept;
        StyleOwner(FontProvider&, int style_id) noexcept;
        StyleOwner(const StyleOwner&) = delete;
        int get() const noexcept;
        int release() noexcept;
        void reset() noexcept;
        void reset(FontProvider&, int style_id) noexcept;
        ~StyleOwner();
    private:
        FontProvider* m_font_provider = nullptr;
        int m_style_id = 0;
    };

    void get_style_metrics(int style_id, bool vertical, font::FontCache::FontMetrics& metrics);

    void get_glyph_info(int style_id, bool vertical, font::FontCache::KernType kern,
                        int num_chars, const wchar_t* chars, font::FontCache::GlyphInfo* glyphs);

    ~FontProvider();

    class TextContainer;
    class TextInserter;

private:
    class Style {
    public:
        int font_id; // As known to font::FontCache
        math::Vec2F font_size;
        math::Vec4F text_color;
        Style();
        Style(int font_id, const math::Vec2F& font_size, const math::Vec4F& text_color);
        bool operator==(const Style& s) const;
    };

    class StyleEntry {
    public:
        std::size_t use_count = 0;
        Style style;
        math::Vec2F font_scaling; // Scaled texel size
    };

    class StyleHasher {
    public:
        static int hash(const Style& s, int n);
    };

    std::vector<StyleEntry> m_styles;
    util::HashMap<Style, int, StyleHasher> m_style_map; // Value is one plus index in `m_styles`.
    std::vector<int> m_unused_styles; // One plus indexes into `m_styles`

    class TextureFontSource;
    class Texture;
    class Page;
    class FontEntry;

    font::FontCache& m_font_cache;
    TextureCache& m_texture_cache;
    const math::Vec2F m_desired_glyph_resol; // Ask cache for this rendering size
    const bool m_enable_mipmap; // Do mipmapping on textures
    const bool m_save_textures; // Save each of the generated textures as a PNG file in /tmp/

    std::map<int, FontEntry*> m_font_map;
    std::vector<std::unique_ptr<FontEntry>> m_fonts;
    std::vector<std::unique_ptr<Texture>> m_textures;

    std::size_t m_used_pages = 0, m_used_textures = 0;

    int acquire_style(font::FontCache::FontOwner& font, const math::Vec2F& font_size,
                      const math::Vec4F& text_color);
    void release_style_fast(int style_id);
    void provide(int style_id, int num_glyphs, const int* glyphs, const float* components,
                 TextInserter& inserter);
    FontEntry* new_font(int font_id);
    Page* new_page(FontEntry* font, int page_idx);
    void new_texture(FontEntry* font, int page_idx, int tex_ord,
                     std::unique_ptr<util::RectanglePacker>& packer, core::UIntMin16& tex_idx);
    void render(const TextContainer& text) const;
    void release(TextContainer& text);
};



/// Holds strips of glyphs.
///
/// An instance of this class remains associated with the font provider that was
/// last passed to the TextInserter constructor alongside this text
/// instance. For this reason, the application must ensure that this instance is
/// destroyed (or cleared) before the font provider is destroyed.
class FontProvider::TextContainer {
public:
    /// Render this text using the OpenGL context that is currently bound to the
    /// calling thread.
    ///
    /// You may embed this call in an OpenGL call list. If you do so, you must
    /// ensure that the text container remains unmodified for as long as the
    /// OpenGL call list.
    void render() const
    {
        if (m_provider)
            m_provider->render(*this);
    }

    void clear();

    ~TextContainer()
    {
        clear();
    }

private:
    FontProvider* m_provider = nullptr;
    font::FontCache::Direction m_layout_direction;

    using PageRef = std::pair<int, int>; // Page of glyps (cache_font_id, page_index)
    std::vector<PageRef> m_page_refs;

    class Strip {
    public:
        int style_idx; // Index in 'm_provider->m_styles' of style of this strip
        float lateral_pos; // Position of baseline
        int num_glyphs = 0;
        Strip(int s, float p):
            style_idx(s),
            lateral_pos(p)
        {
        }
    };
    class Glyph {
    public:
        int index; // Index in texture
        float position;
        Glyph(int i, float p):
            index(i),
            position(p)
        {
        }
    };
    class Texture {
    public:
        FontProvider::Texture* texture;
        std::vector<Strip> strips;
        std::vector<Glyph> glyphs;
        Texture(FontProvider::Texture* t):
            texture(t)
        {
        }
    };
    std::list<Texture> m_textures;

    friend class FontProvider;
};



class FontProvider::TextInserter {
public:
    /// This clears the specified text container.
    ///
    /// Application must ensure that neither the font provider nor the text
    /// object is destroyed before this object is.
    ///
    /// \note You must make sure that two inserters never exist for the same
    /// container at the same time.
    TextInserter(FontProvider&, TextContainer&, font::FontCache::Direction);

    /// \param font_id The ID of the style that the glyph indices refer to. The
    /// ID must have previously been obtained by calling
    /// FontProvider::acquire_style() in the associated FontProvider instance.
    void insert_strip(int style_id, int num_glyphs, const int* glyphs, const float* components);

private:
    FontProvider& m_font_provider;
    TextContainer& m_text_container;
    const bool m_vertical; // True iff the layout is vertical

    using PageSet = std::vector<bool>;
    std::map<int, PageSet> m_page_sets;
    // Only to speed up page_set lookups
    int m_last_font_id = -1;
    FontEntry* m_last_font;
    PageSet* m_last_page_set;

    using Textures = std::map<int, TextContainer::Texture*>;
    Textures m_textures;
    util::RepMapLookupBooster<Textures, 4> m_texture_lookup;
    using StripTexture = std::pair<int, TextContainer::Texture*>;
    std::vector<StripTexture> m_strip_textures;

    friend class FontProvider;
};




// Implementation

inline bool FontProvider::StyleDesc::operator==(const StyleDesc& d) const throw()
{
    return font_family == d.font_family && font_boldness == d.font_boldness &&
        font_italicity == d.font_italicity && font_size == d.font_size &&
        text_color == d.text_color;
}

inline void FontProvider::release_style(int style_id)
{
    release_style_fast(style_id);
}

inline FontProvider::StyleOwner::StyleOwner() noexcept
{
}

inline FontProvider::StyleOwner::StyleOwner(FontProvider& fp, int style_id) noexcept
{
    reset(fp, style_id);
}

inline int FontProvider::StyleOwner::get() const noexcept
{
    return m_style_id;
}

inline int FontProvider::StyleOwner::release() noexcept
{
    m_font_provider = nullptr;
    return m_style_id;
}

inline void FontProvider::StyleOwner::reset() noexcept
{
    if (m_font_provider) {
        m_font_provider->release_style(m_style_id);
        m_font_provider = nullptr;
    }
}

inline FontProvider::StyleOwner::~StyleOwner()
{
    reset();
}

inline void FontProvider::StyleOwner::reset(FontProvider& fp, int style_id) noexcept
{
    reset();
    m_font_provider = &fp;
    m_style_id = style_id;
}

inline FontProvider::Style::Style()
{
}

inline FontProvider::Style::Style(int fi, const math::Vec2F& fs, const math::Vec4F& tc):
    font_id{fi},
    font_size{fs},
    text_color{tc}
{
}

inline bool FontProvider::Style::operator==(const Style& s) const
{
    return font_id == s.font_id && font_size == s.font_size && text_color == s.text_color;
}

inline int FontProvider::StyleHasher::hash(const Style& s, int n)
{
    util::Hash_FNV_1a_32 h;
    h.add_int(s.font_id);
    h.add_float(s.font_size[0]);
    h.add_float(s.font_size[1]);
    h.add_float(s.text_color[0]);
    h.add_float(s.text_color[1]);
    h.add_float(s.text_color[2]);
    h.add_float(s.text_color[3]);
    return int(h.get_hash(n));
}

class FontProvider::Texture {
public:
    const FontEntry* const font;
    struct Glyph {
        int index; // Index of glyph in font as known to font::FontCache.
        int img_x, img_y; // Position of glyph in texture image.
        font::FontCache::GlyphBoxInfo quad_info; // Size and position of GL quad. All distances specified relative to EM-square.
        math::Vec2F tex_lower_left, tex_upper_right; // Position in relative coordinates of glyph in texture
    };
    std::vector<Glyph> glyphs;
    TextureDecl decl; // A hook into the texture cache
    TextureUse use; // Must be non-null when, and only when text_use_count > 0
    int page_use_count = 0; // One for each page with a glyph in this texture
    int text_use_count = 0; // One for each TextContainer that refers to this texture.
    // Invariant: page_use_count == 0  ==>  text_use_count == 0
    Texture(const FontEntry* f):
        font{f}
    {
    }
};


class FontProvider::Page {
public:
    struct Glyph {
        core::UIntMin16 texture; // Index of texture
        core::UIntMin16 index; // Index of glyph in texture
    };
    std::vector<Glyph> glyphs;
    int text_use_count = 0; // One for each TextContainer that refers to a glyph from this page.
};


class FontProvider::FontEntry {
public:
    const int id; // ID as known to font::FontCache
    std::string name;
    int num_glyphs = 0;
    bool grid_fitting;
    int texture_width, texture_height;
    math::Vec2 texture_scale; // Inverse of texture resolution
    std::vector<std::unique_ptr<Page>> pages;

    std::unique_ptr<util::RectanglePacker> packer;
    core::UIntMin16 open_texture_index; // Defined only if 'packer' is not null
    bool dirty_texture = false; // If new glyphs were rendered into the open texture, but the texture is not yet refreshed

    FontEntry(int i, std::string n):
        id{i},
        name{n}
    {
    }
};



inline void FontProvider::release_style_fast(int style_id)
{
    StyleEntry& style = m_styles[style_id-1];
    if (0 < --style.use_count)
        return;
    m_style_map.erase(style.style);
    m_font_cache.release_font(style.style.font_id);
    m_unused_styles.push_back(style_id);
}


inline void FontProvider::TextContainer::clear()
{
    if (m_provider) {
        m_provider->release(*this);
        m_provider = nullptr;
        m_page_refs.clear();
        m_textures.clear();
    }
}


inline FontProvider::TextInserter::TextInserter(FontProvider& fp, TextContainer& tc,
                                                font::FontCache::Direction d):
    m_font_provider{fp},
    m_text_container{tc},
    m_vertical{d == font::FontCache::dir_BottomToTop || d == font::FontCache::dir_TopToBottom},
    m_texture_lookup{m_textures}
{
    tc.clear();
    tc.m_layout_direction = d;
    tc.m_provider = &fp;
}


inline void FontProvider::TextInserter::insert_strip(int style_id, int num_glyphs,
                                                     const int* glyphs,
                                                     const float* components)
{
    m_font_provider.provide(style_id, num_glyphs, glyphs, components, *this);
}

} // namespace render
} // namespace archon

#endif // ARCHON_RENDER_FONT_PROVIDER_HPP
