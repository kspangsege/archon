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

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <archon/core/generate.hpp>
#include <archon/core/file.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/render/font_provider.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::Render;


namespace {

const int num_page_bits = 8; // Number of bits in index of glyph in page.
const int glyphs_per_page = 1<<num_page_bits;

const int texture_glyph_spacing = 3;
const double texture_glyph_expand = 1.5;


struct GlyphHeightOrderCmp { // Highest first
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
void dump_info(ostream& out) const
{
    out << "Direction: "<<(vertical?"Vertical":"Horizontal") << endl;
    out << "Size: "<<width<<"x"<<height << endl;
    out << "Glyphs:" << endl;
    for (size_t i = 0; i < glyphs.size(); ++i) {
        const Glyph& g = glyphs[i];
        out << "  "<<i<<": "
            "tex_lower="<<g.tex.lower_corner<<", tex_upper="<<g.tex.upper_corner<<", "
            "box_lower="<<g.box.lower_corner<<", box_upper="<<g.box.upper_corner << endl;
    }
    out << "Textures:" << endl;
    for (size_t i = 0; i < textures.size(); ++i) {
        Texture const &t = textures[i];
        out << "  "<<i<<": id="<<t.id<<", strips=" << endl;
        for (size_t j = 0; j < t.strips.size(); ++j) {
            const Strip& s = t.strips[j];
            out << "    "<<j<<": major_offset="<<s.major_offset<<", quads=" << endl;
            for (size_t k = 0; k < s.quads.size(); ++k) {
                const Quad& q = s.quads[k];
                out << "      "<<j<<": "
                    "minor_offset="<<q.minor_offset<<", "
                    "glyph_index="<<q.glyph_index << endl;
            }
        }
    }
}
*/

} // unnamed namespace


namespace archon {
namespace Render {

struct FontProvider::TextureFontSource: TextureSource {
    TextureFontSource(FontCache* c, Texture* t, string n, bool s):
        font_cache(c),
        texture(t),
        name(n),
        save(s)
    {
    }

    string get_name() const
    {
        return name;
    }

    Image::ConstRef get_image()
    {
        const FontEntry* f = texture->font;
        Image::Ref img(Image::new_image(f->texture_width, f->texture_height, ColorSpace::get_Lum(), true));
        ImageWriter writer(img);
        writer.set_foreground_color(Color::white);
        writer.set_background_color(PackedTRGB(0xFFFFFFFF)); // Fully transparent white
        writer.clear();
        writer.enable_color_mapping();

        const FontEntry* font = texture->font;
        int num_glyphs = texture->glyphs.size();
        const int max_glyphs_per_chunk = 128;
        int glyphs[max_glyphs_per_chunk];
        float components[2*max_glyphs_per_chunk];
        int begin = 0;
        while (begin < num_glyphs) {
            int end = min(begin + max_glyphs_per_chunk, num_glyphs);
            int n = end - begin;
            int* g = glyphs;
            float* c = components;
            for (int i = begin; i < end; ++i) {
                const Texture::Glyph& h = texture->glyphs[i];
                *g++ = h.index;
                *c++ = h.img_x;
                *c++ = h.img_y;
            }
            font_cache->render_glyphs(font->id, font->grid_fitting,
                                      FontCache::bearing_None, FontCache::coord_Cloud,
                                      n, glyphs, components, writer);
            begin = end;
        }

        if (save) {
            string p = File::get_temp_dir()+name+".png";
            img->save(p);
            cout << "Saved '"<<p<<"'" << endl;
        }
        return img;
    }

private:
    FontCache* const font_cache;
    const FontProvider::Texture* const texture;
    const std::string name;
    bool save;
};



FontProvider::FontProvider(font::FontCache::Arg f, TextureCache& t,
                           const math::Vec2F& r, bool m, bool s):
    font_cache(f),
    texture_cache(t),
    desired_glyph_resol(r),
    size_of_pixel(1/r[0], 1/r[1]),
    enable_mipmap(m),
    save_textures(s)
{
}


FontProvider::~FontProvider() throw ()
{
    if (0 < used_pages)
        throw runtime_error("~FontProvider: Unreleased pages detected");
    if (0 < used_textures)
        throw runtime_error("~FontProvider: Unreleased textures detected");
    if (unused_styles.size() < styles.size())
        throw runtime_error("~FontProvider: Unreleased styles detected");
}



int FontProvider::acquire_default_style()
{
    Vec2F size = desired_glyph_resol;
    FontCache::FontOwner font(font_cache, font_cache->acquire_default_font(size[0], size[1]));
    return acquire_style(font, Vec2F(1), Vec4F(1));
}


int FontProvider::acquire_style(const StyleDesc& desc)
{
    FontCache::FontDesc font_desc;
    font_desc.family    = desc.font_family;
    font_desc.boldness  = desc.font_boldness;
    font_desc.italicity = desc.font_italicity;
    font_desc.size.set(desired_glyph_resol[0], desired_glyph_resol[1]);
    FontCache::FontOwner font(font_cache, font_cache->acquire_font(font_desc));
    return acquire_style(font, desc.font_size, desc.text_color);
}


int FontProvider::acquire_style(FontCache::FontOwner& font, const Vec2F& font_size,
                                const Vec4F& text_color)
{
    Vec2F scaling;
    Style style(font.get(), font_size, text_color);
    int& i = style_map[style];
    StyleEntry* s;
    if (i) {
        s = &styles[i-1];
    }
    else { // New
        if (unused_styles.empty()) {
            size_t n = styles.size() + 1;
            styles.reserve(n);
            unused_styles.push_back(n);
            styles.resize(n);
        }
        i = unused_styles.back();
        s = &styles[i-1];
        s->style = style;
        s->font_scaling.set(font_size[0] * size_of_pixel[0],
                            font_size[1] * size_of_pixel[1]);
        font.release();
        unused_styles.pop_back();
    }
    ++s->use_count;
    return i;
}


void FontProvider::get_style_desc(int style_id, StyleDesc& desc)
{
    const Style& style = styles[style_id-1].style;
    FontCache::FontDesc font_desc;
    font_cache->get_font_desc(style.font_id, font_desc);
    desc.font_family    = font_desc.family;
    desc.font_boldness  = font_desc.boldness;
    desc.font_italicity = font_desc.italicity;
    desc.font_size      = style.font_size;
    desc.text_color     = style.text_color;
}



void FontProvider::get_style_metrics(int style_id, bool vertical,
                                     FontCache::FontMetrics& metrics)
{
    const StyleEntry& style = styles[style_id-1];
    bool grid_fitting = false;
    font_cache->get_font_metrics(style.style.font_id, vertical, grid_fitting, metrics);
    metrics.lateral_span *= style.font_scaling[vertical?0:1];
}


void FontProvider::get_glyph_info(int style_id, bool vertical, FontCache::KernType kern,
                                  int num_chars, const wchar_t* chars,
                                  FontCache::GlyphInfo* glyphs)
{
    const StyleEntry& style = styles[style_id-1];
    bool grid_fitting = false;
    font_cache->get_glyph_info(style.style.font_id, vertical, grid_fitting, kern,
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

    TextInserter::StripTextures& strip_textures = inserter.strip_textures;
    strip_textures.clear();
    strip_textures.reserve(4);

    int style_idx = style_id - 1;
    StyleEntry& style = styles[style_idx];
    int font_id = style.style.font_id;

    FontEntry* font;
    TextInserter::PageSet* page_set;
    if (font_id == inserter.last_font_id) {
        font     = inserter.last_font;
        page_set = inserter.last_page_set;
    }
    else {
        // We are going to add at least one glyph for this font,
        // which will guarantee that this font entry will remain
        // in existence, and therefore that the pointer stored in
        // inserter.last_font will remain valid.
        font = font_map[font_id];
        if (!font) {
            // Initialize font entry
            font = new_font(font_id);
        }

        page_set = &inserter.page_sets[font_id];
        if (page_set->empty()) {
            // Initialize page set
            page_set->resize(font->num_glyphs);
        }

        inserter.last_font_id  = font_id;
        inserter.last_font     = font; // This is safe since num_glyphs > 0 so the Font entry   
        inserter.last_page_set = page_set;
    }

    // Process each glyph in turn
    int num_font_glyphs = font->num_glyphs;
    TextContainer& text = *inserter.text;
    float lateral_pos = *components++;
    for (int i = first; i < num_glyphs; ++i) {
        int glyph_idx = glyphs[i];
        if (glyph_idx < 0)
            continue;
        if (num_font_glyphs <= glyph_idx)
            throw invalid_argument("Bad glyph index");
        int page_idx = glyph_idx >> num_page_bits;
        Page* page = font->pages[page_idx];
        if (!page) {
            // Create page on demand
            page = new_page(font, page_idx);
        }
        if (!(*page_set)[page_idx]) {
            text.page_refs.push_back(make_pair(font_id, page_idx));
            if (++page->text_use_count == 1)
                ++used_pages;
            (*page_set)[page_idx] = true;
        }
        const Page::Glyph& glyph = page->glyphs[glyph_idx & glyphs_per_page-1];

        int tex_idx = glyph.texture;
        Texture* texture = textures[tex_idx];

        // Find corresponding texture entry in TextContainer
        TextContainer::Texture* texture2;
        {
            size_t n = strip_textures.size();
            if (0 < n) {
                TextInserter::StripTexture* t = &strip_textures.front();
                if (t->first == tex_idx) {
                    texture2 = t->second;
                    goto got_texture;
                }
                for (size_t i = 1; i < n; ++i) {
                    t = &strip_textures[i];
                    if (t->first == tex_idx) {
                        texture2 = t->second;
                        swap(*t, strip_textures[i-1]); // Move one step closer to the front
                        goto got_texture;
                    }
                }
            }

            TextContainer::Texture** t = &inserter.texture_lookup[tex_idx];
            if (!*t) {
                // Start new texture in text container
                text.textures.push_back(TextContainer::Texture(texture));
                if (++texture->text_use_count == 1) {
                    texture->use = texture->decl.acquire();
                    ++used_textures;
                }
                *t = &text.textures.back();
            }
            texture2 = *t;

            // Start new strip in texture of text container
            texture2->strips.push_back(TextContainer::Strip(style_idx, lateral_pos));
            ++style.use_count;

            // Remember this texture for remaining iterations
            strip_textures.push_back(make_pair(tex_idx, texture2));
        }

      got_texture:
        texture2->glyphs.push_back(TextContainer::Glyph(glyph.index, components[i]));
        ++texture2->strips.back().num_glyphs;
    }
}



FontProvider::FontEntry* FontProvider::new_font(int font_id)
{
    FontCache::FontInfo info;
    font_cache->get_font_info(font_id, info);
    UniquePtr<FontEntry> font(new FontEntry(font_id, info.name));
    font->pages.resize((info.num_glyphs + (glyphs_per_page-1)) >> num_page_bits);
    font->grid_fitting = false;
    font->texture_width = font->texture_height = 512;                 // FIXME: Needs to depend of the selected rendering size
    font->texture_scale.set(1.0/font->texture_width, 1.0/font->texture_height);
    font->num_glyphs = info.num_glyphs;
    FontEntry* f = font.get();
    fonts.push_back(font);
    font_map[font_id] = f;
    return f;
}



FontProvider::Page* FontProvider::new_page(FontEntry* font, int page_idx)
{
    int num_glyphs;
    if (page_idx+1 < int(font->pages.size())) {
        num_glyphs = glyphs_per_page;
    }
    else {
        int n = font->num_glyphs & glyphs_per_page-1;
        num_glyphs = 0 < n ? n : glyphs_per_page;
    }
    int begin = page_idx << num_page_bits;

    Array<int> glyphs(num_glyphs);
    generate(glyphs.get(), glyphs.get()+num_glyphs, make_inc_generator(begin));
    Array<FontCache::GlyphBoxInfo> info(num_glyphs);
    font_cache->get_glyph_box_info(font->id, false, num_glyphs, glyphs.get(), info.get());

    // Sort according to glyph height
    vector<int> glyph_order(num_glyphs);
    generate(glyph_order.begin(), glyph_order.end(), make_inc_generator<int>());
    sort(glyph_order.begin(), glyph_order.end(), GlyphHeightOrderCmp(info.get()));

    int tex_ord = 0;
    if (!font->packer)
        new_texture(font, page_idx, tex_ord++, font->packer, font->open_texture_index);

    UniquePtr<RectanglePacker> secondary_packer;
    UIntMin16 secondary_texture_index; // Defined only if 'secondary_packer' is not null
    bool primary_dirty = false, secondary_dirty = false;

    UniquePtr<Page> page(new Page);
    page->glyphs.resize(num_glyphs);
    Vec2 tex_scale = font->texture_scale;
    for (int i = 0; i < num_glyphs; ++i) {
        int glyph_idx = glyph_order[i];
        // Allocate texture space for this glyph
        const FontCache::GlyphBoxInfo& box = info[glyph_idx];
        int w = ceil(box.size[0]), h = ceil(box.size[1]);
        if (font->texture_width < w || font->texture_height < h)
            throw runtime_error("Glyph image too big");
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
                    textures[font->open_texture_index]->decl.refresh();
                font->packer = secondary_packer;
                font->open_texture_index = secondary_texture_index;
                primary_dirty = secondary_dirty;
                new_texture(font, page_idx, tex_ord++, secondary_packer, secondary_texture_index);
                secondary_packer->insert(w,h,x,y); // Must succeed since target texture is empty
            }
            tex_idx = secondary_texture_index;
            secondary_dirty = true;
        }

        Texture* texture = textures[tex_idx];
        UIntMin16 idx = texture->glyphs.size();
        texture->glyphs.push_back(Texture::Glyph());
        Page::Glyph& g = page->glyphs[glyph_idx];
        g.texture = tex_idx;
        g.index   = idx;
        Texture::Glyph& glyph = texture->glyphs.back();
        glyph.index = begin + glyph_idx;
        glyph.img_x = x;
        glyph.img_y = y;
        glyph.quad_info = box;
        glyph.quad_info.size     += Vec2F(2*texture_glyph_expand);
        glyph.quad_info.hori_pos -= Vec2F(  texture_glyph_expand);
        glyph.quad_info.vert_pos -= Vec2F(  texture_glyph_expand);
        glyph.quad_info.rev_pos  -= Vec2F(  texture_glyph_expand);
        Vec2F p(x - texture_glyph_expand, y - texture_glyph_expand);
        glyph.tex_lower_left.set(tex_scale[0] * p[0], tex_scale[1] * p[1]);
        glyph.tex_upper_right.set(tex_scale[0] * (p[0] + glyph.quad_info.size[0]),
                                  tex_scale[1] * (p[1] + glyph.quad_info.size[1]));
    }

    if (primary_dirty)
        textures[font->open_texture_index]->decl.refresh();
    if (secondary_packer) {
        font->packer             = secondary_packer;
        font->open_texture_index = secondary_texture_index;
        textures[secondary_texture_index]->decl.refresh();
    }

    font->pages[page_idx] = page.get();
    return page.release();
}



void FontProvider::new_texture(FontEntry* font, int page_idx, int tex_ord,
                               UniquePtr<RectanglePacker>& packer, UIntMin16& tex_idx)
{
    UniquePtr<RectanglePacker> p(new RectanglePacker(font->texture_width, font->texture_height,
                                                     texture_glyph_spacing));
    UniquePtr<Texture> t(new Texture(font));
    ostringstream o;
    o << font->name << " " << page_idx << ":" << tex_ord;
    UniquePtr<TextureSource> src(new TextureFontSource(font_cache.get(), t.get(), o.str(), save_textures));
    TextureCache::FilterMode filter_mode =
        enable_mipmap ? TextureCache::filter_mode_Mipmap : TextureCache::filter_mode_Interp;
    bool wait_for_refresh = true;
    bool fast_image_retrieval = true;
    t->decl = texture_cache.declare(src, GL_CLAMP, GL_CLAMP, filter_mode,
                                    wait_for_refresh, fast_image_retrieval);
    UIntMin16 i = textures.size();
    textures.push_back(t);
    packer  = p;
    tex_idx = i;
}



void FontProvider::render(const TextContainer& text) const
{
    bool vert =
        text.layout_direction == FontCache::dir_BottomToTop ||
        text.layout_direction == FontCache::dir_TopToBottom;

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
    TextContainer::Textures::const_iterator textures_end = text.textures.end();
    for (TextContainer::Textures::const_iterator t = text.textures.begin(); t != textures_end; ++t) {
        const Texture* texture = t->texture;
        texture->use.bind();
        glBegin(GL_QUADS);
        TextContainer::Glyphs::const_iterator glyphs_begin = t->glyphs.begin();
        TextContainer::Strips::const_iterator strips_end = t->strips.end();
        for (TextContainer::Strips::const_iterator s = t->strips.begin(); s != strips_end; ++s) {
            // FIXME: Skip rendering this strip if the color is completely
            // transparent.
            if (s->style_idx != prev_style) {
                const StyleEntry& style = styles[s->style_idx];
                scaling.set(style.font_scaling[0], style.font_scaling[1]);
                if (prev_style < 0 || style.style.text_color != prev_color) {
                    Vec4F color = style.style.text_color;
                    glColor4f(color[0], color[1], color[2], color[3]);
                    prev_color = color;
                }
                prev_style = s->style_idx;
            }
            double lateral_pos = s->lateral_pos;
            TextContainer::Glyphs::const_iterator glyphs_end = glyphs_begin + s->num_glyphs;
            for (TextContainer::Glyphs::const_iterator g = glyphs_begin; g != glyphs_end; ++g) {
                const Texture::Glyph& glyph = texture->glyphs[g->index];

                Vec2 p(g->position, lateral_pos);
                if (vert)
                    swap(p[0], p[1]);
                const FontCache::GlyphBoxInfo& i = glyph.quad_info;
                Vec2F q;
                switch (text.layout_direction) {
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

                Vec2 p0 = p + Vec2(scaling[0] * q[0], scaling[1] * q[1]);
                Vec2 p1 = p0 + Vec2(scaling[0] * i.size[0], scaling[1] * i.size[1]);
                Vec2F t0 = glyph.tex_lower_left;
                Vec2F t1 = glyph.tex_upper_right;

                glTexCoord2f (t1[0], t1[1]);
                glVertex2d   (p1[0], p1[1]);

                glTexCoord2f (t0[0], t1[1]);
                glVertex2d   (p0[0], p1[1]);

                glTexCoord2f (t0[0], t0[1]);
                glVertex2d   (p0[0], p0[1]);

                glTexCoord2f (t1[0], t0[1]);
                glVertex2d   (p1[0], p0[1]);
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
        RepMapLookupBooster<FontMap> booster(&font_map);
        TextContainer::PageRefs::const_iterator e = text.page_refs.end();
        for (TextContainer::PageRefs::const_iterator i = text.page_refs.begin(); i != e; ++i) {
            if (--booster[i->first]->pages[i->second]->text_use_count == 0)
                --used_pages;
        }
    }
    // Release textures
    {
        TextContainer::Textures::const_iterator texture_end = text.textures.end();
        for (TextContainer::Textures::const_iterator t = text.textures.begin(); t != texture_end; ++t) {
            if (--t->texture->text_use_count == 0) {
                --used_textures;
                t->texture->use.clear();
            }

            TextContainer::Strips::const_iterator strips_end = t->strips.end();
            for (TextContainer::Strips::const_iterator s = t->strips.begin(); s != strips_end; ++s)
                release_style_fast(s->style_idx+1);
        }
    }
}

} // namespace Render
} // namespace archon
