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

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <archon/core/generate.hpp>
#include <archon/core/file.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/render/font_provider.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::render;


namespace {

constexpr int g_num_page_bits = 8; // Number of bits in index of glyph in page.
constexpr int g_glyphs_per_page = 1<<g_num_page_bits;

constexpr int g_texture_glyph_spacing = 3;
constexpr double g_texture_glyph_expand = 1.5;


class GlyphHeightOrderCmp { // Highest first
public:
    bool operator()(int a, int b) const
    {
        return glyphs[b].size[1] < glyphs[a].size[1];
    }
    GlyphHeightOrderCmp(const FontCache::GlyphBoxInfo* g):
        glyphs(g)
    {
    }
    const FontCache::GlyphBoxInfo* const glyphs;
};


/*
void dump_info(std::ostream& out) const
{
    out << "Direction: "<<(vertical?"Vertical":"Horizontal") << std::endl;
    out << "Size: "<<width<<"x"<<height << std::endl;
    out << "Glyphs:" << std::endl;
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        const Glyph& g = glyphs[i];
        out << "  "<<i<<": "
            "tex_lower="<<g.tex.lower_corner<<", tex_upper="<<g.tex.upper_corner<<", "
            "box_lower="<<g.box.lower_corner<<", box_upper="<<g.box.upper_corner << std::endl;
    }
    out << "Textures:" << std::endl;
    for (std::size_t i = 0; i < textures.size(); ++i) {
        Texture const &t = textures[i];
        out << "  "<<i<<": id="<<t.id<<", strips=" << std::endl;
        for (std::size_t j = 0; j < t.strips.size(); ++j) {
            const Strip& s = t.strips[j];
            out << "    "<<j<<": major_offset="<<s.major_offset<<", quads=" << std::endl;
            for (std::size_t k = 0; k < s.quads.size(); ++k) {
                const Quad& q = s.quads[k];
                out << "      "<<j<<": "
                    "minor_offset="<<q.minor_offset<<", "
                    "glyph_index="<<q.glyph_index << std::endl;
            }
        }
    }
}
*/

} // unnamed namespace


namespace archon {
namespace render {

class FontProvider::TextureFontSource: public TextureSource {
public:
    TextureFontSource(FontCache& font_cache, Texture* texture, std::string name, bool save):
        m_font_cache{font_cache},
        m_texture{texture},
        m_name{name},
        m_save{save}
    {
    }

    std::string get_name() const
    {
        return m_name;
    }

    Image::ConstRef get_image()
    {
        const FontEntry* f = m_texture->font;
        Image::Ref img(Image::new_image(f->texture_width, f->texture_height,
                                        ColorSpace::get_Lum(), true));
        ImageWriter writer{img};
        writer.set_foreground_color(color::white);
        writer.set_background_color(PackedTRGB(0xFFFFFFFF)); // Fully transparent white
        writer.clear();
        writer.enable_color_mapping();

        const FontEntry* font = m_texture->font;
        int num_glyphs = m_texture->glyphs.size();
        const int max_glyphs_per_chunk = 128;
        int glyphs[max_glyphs_per_chunk];
        float components[2*max_glyphs_per_chunk];
        int begin = 0;
        while (begin < num_glyphs) {
            int end = std::min(begin + max_glyphs_per_chunk, num_glyphs);
            int n = end - begin;
            int* g = glyphs;
            float* c = components;
            for (int i = begin; i < end; ++i) {
                const Texture::Glyph& h = m_texture->glyphs[i];
                *g++ = h.index;
                *c++ = h.img_x;
                *c++ = h.img_y;
            }
            m_font_cache.render_glyphs(font->id, font->grid_fitting,
                                       FontCache::bearing_None, FontCache::coord_Cloud,
                                       n, glyphs, components, writer);
            begin = end;
        }

        if (m_save) {
            std::string p = file::get_temp_dir()+m_name+".png";
            img->save(p);
            std::cout << "Saved '"<<p<<"'" << std::endl;
        }
        return img;
    }

private:
    FontCache& m_font_cache;
    const FontProvider::Texture* const m_texture;
    const std::string m_name;
    bool m_save;
};



FontProvider::FontProvider(FontCache& font_cache, TextureCache& texture_cache,
                           const math::Vec2F& desired_glyp_resol, bool enable_mipmap,
                           bool save_textures):
    m_font_cache{font_cache},
    m_texture_cache{texture_cache},
    m_desired_glyph_resol{desired_glyp_resol},
    m_enable_mipmap{enable_mipmap},
    m_save_textures{save_textures}
{
}


FontProvider::~FontProvider()
{
    ARCHON_ASSERT(m_used_pages == 0);
    ARCHON_ASSERT(m_used_textures == 0);
    ARCHON_ASSERT(m_unused_styles.size() >= m_styles.size());
}



int FontProvider::acquire_default_style()
{
    Vec2F size = m_desired_glyph_resol;
    FontCache::FontOwner font{m_font_cache, m_font_cache.acquire_default_font(size[0], size[1])};
    return acquire_style(font, Vec2F(1), Vec4F(1));
}


int FontProvider::acquire_style(const StyleDesc& desc)
{
    FontCache::FontDesc font_desc;
    font_desc.family    = desc.font_family;
    font_desc.boldness  = desc.font_boldness;
    font_desc.italicity = desc.font_italicity;
    font_desc.size      = m_desired_glyph_resol;
    FontCache::FontOwner font{m_font_cache, m_font_cache.acquire_font(font_desc)};
    return acquire_style(font, desc.font_size, desc.text_color);
}


int FontProvider::acquire_style(FontCache::FontOwner& font, const Vec2F& font_size,
                                const Vec4F& text_color)
{
    Style style{font.get(), font_size, text_color};
    int& i = m_style_map[style];
    StyleEntry* s;
    if (i != 0) {
        s = &m_styles[i-1];
    }
    else { // New
        if (m_unused_styles.empty()) {
            std::size_t n = m_styles.size() + 1;
            m_styles.reserve(n);
            m_unused_styles.push_back(n);
            m_styles.resize(n);
        }
        i = m_unused_styles.back();
        s = &m_styles[i-1];
        s->style = style;
        Vec2 font_resol = m_font_cache.get_font_size(font.get());
        s->font_scaling.set(font_size[0] / font_resol[0],
                            font_size[1] / font_resol[1]);
        font.release();
        m_unused_styles.pop_back();
    }
    ++s->use_count;
    return i;
}


void FontProvider::get_style_desc(int style_id, StyleDesc& desc)
{
    const Style& style = m_styles[style_id-1].style;
    FontCache::FontDesc font_desc;
    m_font_cache.get_font_desc(style.font_id, font_desc);
    desc.font_family    = font_desc.family;
    desc.font_boldness  = font_desc.boldness;
    desc.font_italicity = font_desc.italicity;
    desc.font_size      = style.font_size;
    desc.text_color     = style.text_color;
}



void FontProvider::get_style_metrics(int style_id, bool vertical,
                                     FontCache::FontMetrics& metrics)
{
    const StyleEntry& style = m_styles[style_id-1];
    bool grid_fitting = false;
    m_font_cache.get_font_metrics(style.style.font_id, vertical, grid_fitting, metrics);
    metrics.lateral_span *= style.font_scaling[vertical?0:1];
}


void FontProvider::get_glyph_info(int style_id, bool vertical, FontCache::KernType kern,
                                  int num_chars, const wchar_t* chars,
                                  FontCache::GlyphInfo* glyphs)
{
    const StyleEntry& style = m_styles[style_id-1];
    bool grid_fitting = false;
    m_font_cache.get_glyph_info(style.style.font_id, vertical, grid_fitting, kern,
                                num_chars, chars, glyphs);

    double scaling = style.font_scaling[vertical?1:0];
    for (int i = 0; i < num_chars; ++i) {
        FontCache::GlyphInfo& info = glyphs[i];
        info.advance *= scaling;
        info.kerning *= scaling;
    }
}



void FontProvider::provide(int style_id, int num_glyphs, const int* glyphs,
                           const float* components, TextInserter& inserter)
{
    // Make sure we only continue if at least one glyph is going to
    // be added
    int first = 0;
    for (;;) {
        if (first == num_glyphs)
            return;
        if (0 <= glyphs[first])
            break;
        ++first;
    }

    auto& strip_textures = inserter.m_strip_textures;
    strip_textures.clear();
    strip_textures.reserve(4);

    int style_idx = style_id - 1;
    StyleEntry& style = m_styles[style_idx];
    int font_id = style.style.font_id;

    FontEntry* font;
    TextInserter::PageSet* page_set;
    if (font_id == inserter.m_last_font_id) {
        font     = inserter.m_last_font;
        page_set = inserter.m_last_page_set;
    }
    else {
        // We are going to add at least one glyph for this font,
        // which will guarantee that this font entry will remain
        // in existence, and therefore that the pointer stored in
        // inserter.last_font will remain valid.
        font = m_font_map[font_id];
        if (!font) {
            // Initialize font entry
            font = new_font(font_id);
        }

        page_set = &inserter.m_page_sets[font_id];
        if (page_set->empty()) {
            // Initialize page set
            page_set->resize(font->num_glyphs);
        }

        inserter.m_last_font_id  = font_id;
        inserter.m_last_font     = font; // This is safe since num_glyphs > 0 so the Font entry   
        inserter.m_last_page_set = page_set;
    }

    // Process each glyph in turn
    int num_font_glyphs = font->num_glyphs;
    TextContainer& text = inserter.m_text_container;
    float lateral_pos = *components++;
    for (int i = first; i < num_glyphs; ++i) {
        int glyph_idx = glyphs[i];
        if (glyph_idx < 0)
            continue;
        if (num_font_glyphs <= glyph_idx)
            throw std::invalid_argument("Bad glyph index");
        int page_idx = glyph_idx >> g_num_page_bits;
        Page* page = font->pages[page_idx].get();
        if (!page) {
            // Create page on demand
            page = new_page(font, page_idx); // Throws
        }
        if (!(*page_set)[page_idx]) {
            text.m_page_refs.push_back(std::make_pair(font_id, page_idx));
            if (++page->text_use_count == 1)
                ++m_used_pages;
            (*page_set)[page_idx] = true;
        }
        const Page::Glyph& glyph = page->glyphs[glyph_idx & g_glyphs_per_page-1];

        int tex_idx = glyph.texture;
        Texture* texture = m_textures[tex_idx].get();

        // Find corresponding texture entry in TextContainer
        TextContainer::Texture* texture2;
        {
            std::size_t n = strip_textures.size();
            if (0 < n) {
                TextInserter::StripTexture* t = &strip_textures.front();
                if (t->first == tex_idx) {
                    texture2 = t->second;
                    goto got_texture;
                }
                for (std::size_t i = 1; i < n; ++i) {
                    t = &strip_textures[i];
                    if (t->first == tex_idx) {
                        texture2 = t->second;
                        swap(*t, strip_textures[i-1]); // Move one step closer to the front
                        goto got_texture;
                    }
                }
            }

            TextContainer::Texture** t = &inserter.m_texture_lookup[tex_idx];
            if (!*t) {
                // Start new texture in text container
                text.m_textures.push_back(TextContainer::Texture(texture));
                if (++texture->text_use_count == 1) {
                    texture->use = texture->decl.acquire();
                    ++m_used_textures;
                }
                *t = &text.m_textures.back();
            }
            texture2 = *t;

            // Start new strip in texture of text container
            texture2->strips.push_back(TextContainer::Strip(style_idx, lateral_pos));
            ++style.use_count;

            // Remember this texture for remaining iterations
            strip_textures.push_back(std::make_pair(tex_idx, texture2));
        }

      got_texture:
        texture2->glyphs.push_back(TextContainer::Glyph(glyph.index, components[i]));
        ++texture2->strips.back().num_glyphs;
    }
}



FontProvider::FontEntry* FontProvider::new_font(int font_id)
{
    FontCache::FontInfo info;
    m_font_cache.get_font_info(font_id, info);
    std::unique_ptr<FontEntry> font = std::make_unique<FontEntry>(font_id, info.name); // Throws
    font->pages.resize((info.num_glyphs + (g_glyphs_per_page-1)) >> g_num_page_bits);
    font->grid_fitting = false;
    font->texture_width = font->texture_height = 512;                 // FIXME: Needs to depend of the selected rendering size
    font->texture_scale.set(1.0/font->texture_width, 1.0/font->texture_height);
    font->num_glyphs = info.num_glyphs;
    FontEntry* f = font.get();
    m_fonts.reserve(m_fonts.size()+1); // Throws
    m_font_map[font_id] = f; // Throws
    m_fonts.emplace_back(std::move(font));
    return f;
}



FontProvider::Page* FontProvider::new_page(FontEntry* font, int page_idx)
{
    int num_glyphs;
    if (page_idx+1 < int(font->pages.size())) {
        num_glyphs = g_glyphs_per_page;
    }
    else {
        int n = font->num_glyphs & g_glyphs_per_page-1;
        num_glyphs = 0 < n ? n : g_glyphs_per_page;
    }
    int begin = page_idx << g_num_page_bits;

    std::unique_ptr<int[]> glyphs = std::make_unique<int[]>(num_glyphs); // Throws
    generate(glyphs.get(), glyphs.get()+num_glyphs, make_inc_generator(begin));
    std::unique_ptr<FontCache::GlyphBoxInfo[]> info =
        std::make_unique<FontCache::GlyphBoxInfo[]>(num_glyphs); // Throws
    m_font_cache.get_glyph_box_info(font->id, false, num_glyphs, glyphs.get(), info.get());

    // Sort according to glyph height
    std::vector<int> glyph_order(num_glyphs);
    generate(glyph_order.begin(), glyph_order.end(), make_inc_generator<int>());
    std::sort(glyph_order.begin(), glyph_order.end(), GlyphHeightOrderCmp(info.get()));

    int tex_ord = 0;
    if (!font->packer)
        new_texture(font, page_idx, tex_ord++, font->packer, font->open_texture_index);

    std::unique_ptr<RectanglePacker> secondary_packer;
    UIntMin16 secondary_texture_index; // Defined only if 'secondary_packer' is not null
    bool primary_dirty = false, secondary_dirty = false;

    std::unique_ptr<Page> page = std::make_unique<Page>(); // Throws
    page->glyphs.resize(num_glyphs);
    Vec2 tex_scale = font->texture_scale;
    for (int i = 0; i < num_glyphs; ++i) {
        int glyph_idx = glyph_order[i];
        // Allocate texture space for this glyph
        const FontCache::GlyphBoxInfo& box = info[glyph_idx];
        int w = ceil(box.size[0]), h = ceil(box.size[1]);
        if (font->texture_width < w || font->texture_height < h)
            throw std::runtime_error("Glyph image too big");
        int x = 0, y = 0;
        UIntMin16 tex_idx;
        if (font->packer->insert(w,h,x,y)) {
            tex_idx = font->open_texture_index;
            primary_dirty = true;
        }
        else {
            if (!secondary_packer)
                new_texture(font, page_idx, tex_ord++, secondary_packer, secondary_texture_index);
            if (!secondary_packer->insert(w,h,x,y)) {
                if (primary_dirty)
                    m_textures[font->open_texture_index]->decl.refresh();
                font->packer = std::move(secondary_packer);
                secondary_packer.reset();
                font->open_texture_index = secondary_texture_index;
                primary_dirty = secondary_dirty;
                new_texture(font, page_idx, tex_ord++, secondary_packer, secondary_texture_index);
                secondary_packer->insert(w,h,x,y); // Must succeed since target texture is empty
            }
            tex_idx = secondary_texture_index;
            secondary_dirty = true;
        }

        Texture& texture = *m_textures[tex_idx];
        UIntMin16 idx = texture.glyphs.size();
        texture.glyphs.push_back(Texture::Glyph());
        Page::Glyph& g = page->glyphs[glyph_idx];
        g.texture = tex_idx;
        g.index   = idx;
        Texture::Glyph& glyph = texture.glyphs.back();
        glyph.index = begin + glyph_idx;
        glyph.img_x = x;
        glyph.img_y = y;
        glyph.quad_info = box;
        glyph.quad_info.size     += Vec2F(2*g_texture_glyph_expand);
        glyph.quad_info.hori_pos -= Vec2F(  g_texture_glyph_expand);
        glyph.quad_info.vert_pos -= Vec2F(  g_texture_glyph_expand);
        glyph.quad_info.rev_pos  -= Vec2F(  g_texture_glyph_expand);
        Vec2F p(x - g_texture_glyph_expand, y - g_texture_glyph_expand);
        glyph.tex_lower_left.set(tex_scale[0] * p[0], tex_scale[1] * p[1]);
        glyph.tex_upper_right.set(tex_scale[0] * (p[0] + glyph.quad_info.size[0]),
                                  tex_scale[1] * (p[1] + glyph.quad_info.size[1]));
    }

    if (primary_dirty)
        m_textures[font->open_texture_index]->decl.refresh();
    if (secondary_packer) {
        font->packer = std::move(secondary_packer);
        font->open_texture_index = secondary_texture_index;
        m_textures[secondary_texture_index]->decl.refresh();
    }

    Page* page_2 = page.get();
    font->pages[page_idx] = std::move(page);
    return page_2;
}



void FontProvider::new_texture(FontEntry* font, int page_idx, int tex_ord,
                               std::unique_ptr<RectanglePacker>& packer, UIntMin16& tex_idx)
{
    std::unique_ptr<RectanglePacker> p =
        std::make_unique<RectanglePacker>(font->texture_width, font->texture_height,
                                          g_texture_glyph_spacing); // Throws
    std::unique_ptr<Texture> t = std::make_unique<Texture>(font); // Throws
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << font->name << " " << page_idx << ":" << tex_ord;
    std::unique_ptr<TextureSource> src =
        std::make_unique<TextureFontSource>(m_font_cache, t.get(), out.str(), m_save_textures);
    using FilterMode = TextureCache::FilterMode;
    FilterMode filter_mode = (m_enable_mipmap ? FilterMode::mipmap : FilterMode::interp);
    bool wait_for_refresh = true;
    bool fast_image_retrieval = true;
    t->decl = m_texture_cache.declare(std::move(src), GL_CLAMP, GL_CLAMP, filter_mode,
                                      wait_for_refresh, fast_image_retrieval);
    UIntMin16 i = m_textures.size();
    m_textures.emplace_back(std::move(t));
    packer  = std::move(p);
    tex_idx = i;
}



void FontProvider::render(const TextContainer& text) const
{
    bool vert =
        text.m_layout_direction == FontCache::dir_BottomToTop ||
        text.m_layout_direction == FontCache::dir_TopToBottom;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_COLOR_MATERIAL);
    glAlphaFunc(GL_GREATER, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glNormal3f(0, 0, 1);

    int prev_style = -1;
    Vec4F prev_color;
    Vec2 scaling;
    for (const TextContainer::Texture& t: text.m_textures) {
        const Texture& texture = *t.texture;
        texture.use.bind();
        glBegin(GL_QUADS);
        auto glyphs_begin = t.glyphs.begin();
        for (const TextContainer::Strip& s: t.strips) {
            // FIXME: Skip rendering this strip if the color is completely
            // transparent.
            if (s.style_idx != prev_style) {
                const StyleEntry& style = m_styles[s.style_idx];
                scaling = style.font_scaling;
                if (prev_style < 0 || style.style.text_color != prev_color) {
                    Vec4F color = style.style.text_color;
                    glColor4f(color[0], color[1], color[2], color[3]);
                    prev_color = color;
                }
                prev_style = s.style_idx;
            }
            double lateral_pos = s.lateral_pos;
            auto glyphs_end = glyphs_begin + s.num_glyphs;
            for (auto g = glyphs_begin; g != glyphs_end; ++g) {
                const Texture::Glyph& glyph = texture.glyphs[g->index];

                Vec2 p_0{g->position, lateral_pos};
                if (vert)
                    std::swap(p_0[0], p_0[1]);
                const FontCache::GlyphBoxInfo& i = glyph.quad_info;
                Vec2F q;
                switch (text.m_layout_direction) {
                    case FontCache::dir_LeftToRight:
                        q.set(i.hori_pos[0], i.hori_pos[1]);
                        break;
                    case FontCache::dir_RightToLeft:
                        q.set(i.rev_pos[0],  i.hori_pos[1]);
                        break;
                    case FontCache::dir_BottomToTop:
                        q.set(i.vert_pos[0], i.vert_pos[1]);
                        break;
                    case FontCache::dir_TopToBottom:
                        q.set(i.vert_pos[0], i.rev_pos[1]);
                        break;
                }

                Vec2 p_1 = p_0 + Vec2(scaling[0] *      q[0], scaling[1] *      q[1]);
                Vec2 p_2 = p_1 + Vec2(scaling[0] * i.size[0], scaling[1] * i.size[1]);
                Vec2F t_1 = glyph.tex_lower_left;
                Vec2F t_2 = glyph.tex_upper_right;

                glTexCoord2f (t_2[0], t_2[1]);
                glVertex2d   (p_2[0], p_2[1]);

                glTexCoord2f (t_1[0], t_2[1]);
                glVertex2d   (p_1[0], p_2[1]);

                glTexCoord2f (t_1[0], t_1[1]);
                glVertex2d   (p_1[0], p_1[1]);

                glTexCoord2f (t_2[0], t_1[1]);
                glVertex2d   (p_2[0], p_1[1]);
            }
            glyphs_begin = glyphs_end;
        }
        glEnd();
    }

    glPopAttrib();
}



void FontProvider::release(TextContainer& text)
{
    // Release pages
    {
        RepMapLookupBooster<decltype(m_font_map)> booster{m_font_map};
        for (TextContainer::PageRef r: text.m_page_refs) {
            if (--booster[r.first]->pages[r.second]->text_use_count == 0)
                --m_used_pages;
        }
    }
    // Release textures
    {
        for (const TextContainer::Texture& t: text.m_textures) {
            if (--t.texture->text_use_count == 0) {
                --m_used_textures;
                t.texture->use.clear();
            }

            for (const TextContainer::Strip& s: t.strips)
                release_style_fast(s.style_idx+1);
        }
    }
}

} // namespace render
} // namespace archon
