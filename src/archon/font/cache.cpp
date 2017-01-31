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

#include <stdexcept>
#include <locale>
#include <sstream>

#include <archon/core/memory.hpp>
#include <archon/math/vector.hpp>
#include <archon/font/cache.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::image;
using namespace archon::font;


namespace {

class CacheImpl: public FontCache {
public:
    std::weak_ptr<FontCache> weak_self;

    CacheImpl(std::shared_ptr<FontList> list):
        m_list{std::move(list)},
        m_default_face_index{m_list->get_default_face()}
    {
        m_list->get_init_size(m_default_width, m_default_height);
    }


    ~CacheImpl() noexcept override final
    {
        ARCHON_ASSERT(m_num_sizes == 0);
    }


    int acquire_default_font() override final
    {
        return acquire_default_font(m_default_width, m_default_height);
    }


    int acquire_default_font(double width, double height) override final
    {
        FontList::SizeInfo size_info;
        m_list->find_default_size(width, height, size_info);
        if (!size_info.exact) {
            const FontLoader::FaceInfo::FixedSize& s =
                m_list->get_face_info(m_default_face_index).fixed_sizes[size_info.fixed_size_index];
            width  = s.first;
            height = s.second;
        }
//std::cerr << "MAP: DEFAULT --> '"<<m_list->get_face_info(m_default_face_index).family<<"'\n";
        return acquire_font(m_default_face_index, Vec2(width, height), size_info.fixed_size_index);
    }


    int acquire_font(const FontDesc& desc) override final
    {
        FontList::SizeInfo size_info;
        int list_index =
            m_list->find_face(FontList::find_BestFace, desc.family,
                              0.5 <= desc.boldness, 0.5 <= desc.italicity,
                              desc.size[0], desc.size[1], &size_info);
        Vec2 size = desc.size;
        if (!size_info.exact) {
            const FontLoader::FaceInfo::FixedSize& s =
                m_list->get_face_info(list_index).fixed_sizes[size_info.fixed_size_index];
            size.set(s.first, s.second);
        }
//std::cerr << "MAP: family '"<<desc.family<<"' --> '"<<list->get_face_info(list_index).family<<"'\n";
        return acquire_font(list_index, size, size_info.fixed_size_index);
    }


    // 'size' is the true rendering size, not necessarily the requested size
    // fixed_size_index is negative if the size does not correspond to a fixed
    // size
    int acquire_font(int face_index, Vec2 size, int fixed_size_index)
    {
        if (m_faces.size() <= std::size_t(face_index))
            m_faces.resize(face_index + 1); // Throws
        std::unique_ptr<FaceEntry>& e = m_faces[face_index];
        if (!e) {
            e = std::make_unique<FaceEntry>(face_index); // Throws
            ++m_num_faces;
//std::cerr << "Adding face "<<face_index<<" for a total of "<<m_num_faces<<" faces"<<"\n";
        }
        auto r = e->size_map.insert(std::make_pair(size, 0));
        int i;
        if (r.second) {
            if (m_unused_size_entries.empty()) {
                i = m_sizes.size();
                try {
                    m_sizes.push_back(SizeEntry(e.get(), size, fixed_size_index));
                }
                catch (...) {
                    e->size_map.erase(size);
                    throw;
                }
            }
            else {
                i = m_unused_size_entries.back();
                m_unused_size_entries.pop_back();
                SizeEntry& s = m_sizes[i];
                s.face = e.get();
                s.size = size;
                s.fixed_size_index = fixed_size_index;
                ++s.use_count;
            }
            r.first->second = i;
            ++m_num_sizes;
//std::cerr << "Adding size "<<size<<" to face "<<face_index<<" for a total of "<<m_num_sizes<<" sizes ("<<double(m_num_sizes)/m_num_faces<<" sizes per face on average)\n";
        }
        else {
            i = r.first->second;
            SizeEntry& s = m_sizes[i];
            ++s.use_count;
        }
//std::cerr << "Acquiring face "<<face_index<<" of size "<<size<<" for a total use count of "<<m_sizes[i].use_count<<"\n";
        return i;
    }


    void release_font(int font_id) override final
    {
        SizeEntry& s = get_size(font_id);
//std::cerr << "Releasing face "<<s.face->list_index<<" of size "<<s.size<<" for a total use count of "<<(s.use_count-1)<<"\n";
        if (0 < --s.use_count)
            return;
        --m_num_sizes;
        s.face->size_map.erase(s.size);
        m_unused_size_entries.push_back(font_id); // May throw
//std::cerr << "Removing size "<<s.size<<" from face "<<s.face->list_index<<" for a total of "<<m_num_sizes<<" sizes ("<<double(m_num_sizes)/m_num_faces<<" sizes per face on average)\n";
    }


    void get_font_desc(int font_id, FontDesc& desc) override final
    {
        SizeEntry& s = get_size(font_id);
        FaceEntry& f = *s.face;
        const FontLoader::FaceInfo& info = m_list->get_face_info(f.list_index);
        desc.family    = info.family;
        desc.boldness  = info.bold   ? 1 : 0;
        desc.italicity = info.italic ? 1 : 0;
        desc.size      = s.size;
    }


    void get_font_info(int font_id, FontInfo& info) override final
    {
        SizeEntry& s = get_size(font_id);
        FaceEntry& f = *s.face;
        const FontLoader::FaceInfo& i = m_list->get_face_info(f.list_index);
        std::ostringstream o;
        o.imbue(std::locale::classic());
        o << i.family <<
            (i.bold ? i.italic ? " bold italic" : " bold" : i.italic ? " italic" : "") << " " <<
            s.size[0] << "x" << s.size[1];
        info.name = o.str();
        if (!f.face)
            f.face = m_list->load_face(f.list_index);
        info.num_glyphs = f.face->get_num_glyphs();
    };


    void get_font_metrics(int font_id, bool vertical, bool grid_fitting,
                          FontMetrics& metrics) override final
    {
        FontFace* face = get_face(font_id);
        metrics.lateral_span.begin = -face->get_baseline_offset(vertical, grid_fitting);
        metrics.lateral_span.end =
            metrics.lateral_span.begin + face->get_baseline_spacing(vertical, grid_fitting);
    }


    void get_glyph_info(int font_id, bool vertical, bool grid_fitting, KernType kern,
                        int num_chars, const wchar_t* chars, GlyphInfo* glyphs) override final
    {
        FontFace* face = get_face(font_id);
        int prev_glyph = 0;
        for (int i = 0; i < num_chars; ++i) {
            int glyph_index = face->find_glyph(chars[i]);
            face->load_glyph(glyph_index, grid_fitting);
            GlyphInfo& info = glyphs[i];
            info.index = glyph_index;
            info.advance = face->get_glyph_advance(vertical);
            if (kern == kern_No) {
                info.kerning = 0;
            }
            else {
                info.kerning = kern == kern_Inc ?
                    face->get_kerning(prev_glyph, glyph_index, vertical, grid_fitting) :
                    face->get_kerning(glyph_index, prev_glyph, vertical, grid_fitting);
                prev_glyph = glyph_index;
            }
        }
    }


    void render_glyphs(int font_id, bool grid_fitting,
                       BearingType bearing_type, CoordType coord_type,
                       int num_glyphs, const int* glyphs, const float* components,
                       ImageWriter& img_writer) override final
    {
        FontFace* face = get_face(font_id);

        double cursor_x = 0, cursor_y = 0;
        if (coord_type == coord_Vert) {
            cursor_x = *components++;
        }
        else if (coord_type == coord_Hori) {
            cursor_y = *components++;
        }

        for (int i = 0; i < num_glyphs; ++i) {
            if (coord_type != coord_Vert)
                cursor_x = *components++; // hori / cloud
            if (coord_type != coord_Hori)
                cursor_y = *components++; // vert / cloud

            int glyph = *glyphs++;
            if (glyph < 0)
                continue;

            face->load_glyph(glyph, grid_fitting);

            Vec2 p(cursor_x, cursor_y);
            switch (bearing_type) {
                case bearing_None:
                    break;
                case bearing_Right:
                    p[0] -= face->get_glyph_advance(false);
                    // Intended fall-through
                case bearing_Left:
                    p -= face->get_glyph_bearing(false);
                    break;
                case bearing_Above:
                    p[1] -= face->get_glyph_advance(true);
                    // Intended fall-through
                case bearing_Below:
                    p -= face->get_glyph_bearing(true);
                    break;
            }

            face->translate_glyph(p);
            face->render_pixels_to(img_writer);
        }
    }


    void get_glyph_box_info(int font_id, bool grid_fitting, int num_glyphs,
                            const int* glyphs, GlyphBoxInfo* info) override final
    {
        FontFace* face = get_face(font_id);
        for (int i = 0; i < num_glyphs; ++i, ++glyphs, ++info) {
            int glyph = *glyphs;
            face->load_glyph(glyph, grid_fitting);
            Vec2 size     =  face->get_glyph_size();
            Vec2 hori_pos = -face->get_glyph_bearing(false);
            Vec2 vert_pos = -face->get_glyph_bearing(true);
            info->size.set(size[0], size[1]);
            info->hori_pos.set(hori_pos[0], hori_pos[1]);
            info->vert_pos.set(vert_pos[0], vert_pos[1]);
            info->rev_pos.set(hori_pos[0] - face->get_glyph_advance(false),
                              vert_pos[1] - face->get_glyph_advance(true));
        }
    }


    FontFace* get_face(int font_id)
    {
        SizeEntry& s = get_size(font_id);
        FaceEntry& f = *s.face;
        if (!f.face)
            f.face = m_list->load_face(f.list_index);
        if (!f.current_size_valid || f.current_size != s.size) {
            if (s.fixed_size_index < 0) {
                f.face->set_scaled_size(s.size[0], s.size[1]);
            }
            else {
                f.face->set_fixed_size(s.fixed_size_index);
            }
            f.current_size = s.size;
            f.current_size_valid = true;
        }
        return f.face.get();
    }



    class FaceEntry {
    public:
        const int list_index;
        std::unique_ptr<FontFace> face;
        Vec2 current_size;
        bool current_size_valid = false;
        std::map<Vec2, int> size_map; // Values are indices into CacheImpl::m_sizes

        FaceEntry(int i):
            list_index(i)
        {
        }
    };

    class SizeEntry {
    public:
        FaceEntry* face;
        Vec2 size;
        int fixed_size_index; // -1 if the size is not a fixed size
        int use_count = 1;

        SizeEntry(FaceEntry* f, const Vec2& s, int i):
            face(f),
            size(s),
            fixed_size_index(i)
        {
        }
    };


    SizeEntry& get_size(int font_id)
    {
        if (font_id < 0 || m_sizes.size() <= std::size_t(font_id))
            throw std::invalid_argument("Bad font ID");
        SizeEntry& s = m_sizes[font_id];
        if (s.use_count == 0)
            throw std::invalid_argument("Bad font ID");
        return s;
    }


    const std::shared_ptr<FontList> m_list;

    const int m_default_face_index; // The index of the default face of 'm_list'.

    double m_default_width, m_default_height; // The initial rendering size that the list applies to any face after loading it.

    std::vector<std::unique_ptr<FaceEntry>> m_faces; // Indexed as 'm_list'

    std::vector<SizeEntry> m_sizes;

    std::vector<int> m_unused_size_entries; // Indices od unused entries in 'm_sizes'

    int m_num_faces = 0, m_num_sizes = 0;
};

} // unnamed namespace


namespace archon {
namespace font {

std::shared_ptr<FontCache> new_font_cache(std::shared_ptr<FontList> list)
{
    auto cache = std::make_shared<CacheImpl>(std::move(list)); // Throws
    cache->weak_self = cache;
    return cache;
}

} // namespace font
} // namespace archon
