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

#include <stdexcept>
#include <locale>
#include <sstream>

#include <archon/core/memory.hpp>
#include <archon/math/vector.hpp>
#include <archon/font/cache.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::Imaging;
using namespace archon::font;


namespace
{
  struct CacheImpl: FontCache
  {
    CacheImpl(FontList::Arg l):
      list(l), default_face_index(l->get_default_face()), num_faces(0), num_sizes(0)
    {
      l->get_init_size(default_width, default_height);
    }


    ~CacheImpl()
    {
      ARCHON_ASSERT_1(num_sizes == 0, "font::~CacheImpl: Found unreleased fonts");
    }


    int acquire_default_font()
    {
      return acquire_default_font(default_width, default_height);
    }


    int acquire_default_font(double width, double height)
    {
      FontList::SizeInfo size_info;
      list->find_default_size(width, height, size_info);
      if(!size_info.exact)
      {
        FontLoader::FaceInfo::FixedSize const &s =
          list->get_face_info(default_face_index).fixed_sizes[size_info.fixed_size_index];
        width  = s.first;
        height = s.second;
      }
//cerr << "MAP: DEFAULT --> '"<<list->get_face_info(default_face_index).family<<"'" << endl;
      return acquire_font(default_face_index, Vec2(width, height), size_info.fixed_size_index);
    }


    int acquire_font(FontDesc const &desc)
    {
      FontList::SizeInfo size_info;
      int const list_index =
        list->find_face(FontList::find_BestFace, desc.family,
                        0.5 <= desc.boldness, 0.5 <= desc.italicity,
                        desc.size[0], desc.size[1], &size_info);
      Vec2 size = desc.size;
      if(!size_info.exact)
      {
        FontLoader::FaceInfo::FixedSize const &s =
          list->get_face_info(list_index).fixed_sizes[size_info.fixed_size_index];
        size.set(s.first, s.second);
      }
//cerr << "MAP: family '"<<desc.family<<"' --> '"<<list->get_face_info(list_index).family<<"'" << endl;
      return acquire_font(list_index, size, size_info.fixed_size_index);
    }


    // 'size' is the true rendering size, not necessarily the requested size
    // fixed_size_index is negative if the size does not correspond to a fixed size
    int acquire_font(int face_index, Vec2 size, int fixed_size_index)
    {
      if(faces.size() <= size_t(face_index)) faces.resize(face_index + 1);
      FaceEntry *&e = faces[face_index];
      if(!e)
      {
        e = new FaceEntry(face_index);
        ++num_faces;
//cerr << "Adding face "<<face_index<<" for a total of "<<num_faces<<" faces" << endl;
      }
      pair<FaceEntry::SizeMap::iterator, bool> const r = e->size_map.insert(make_pair(size, 0));
      int i;
      if(r.second)
      {
        if(unused_size_entries.empty())
        {
          i = sizes.size();
          try
          {
            sizes.push_back(SizeEntry(e, size, fixed_size_index));
          }
          catch(...)
          {
            e->size_map.erase(size);
            throw;
          }
        }
        else
        {
          i = unused_size_entries.back();
          unused_size_entries.pop_back();
          SizeEntry &s = sizes[i];
          s.face = e;
          s.size = size;
          s.fixed_size_index = fixed_size_index;
          ++s.use_count;
        }
        r.first->second = i;
        ++num_sizes;
//cerr << "Adding size "<<size<<" to face "<<face_index<<" for a total of "<<num_sizes<<" sizes ("<<double(num_sizes)/num_faces<<" sizes per face on average)" << endl;
      }
      else
      {
        i = r.first->second;
        SizeEntry &s = sizes[i];
        ++s.use_count;
      }
//cerr << "Acquiring face "<<face_index<<" of size "<<size<<" for a total use count of "<<sizes[i].use_count << endl;
      return i;
    }


    void release_font(int font_id)
    {
      SizeEntry &s = get_size(font_id);
//cerr << "Releasing face "<<s.face->list_index<<" of size "<<s.size<<" for a total use count of "<<(s.use_count-1) << endl;
      if(0 < --s.use_count) return;
      --num_sizes;
      s.face->size_map.erase(s.size);
      unused_size_entries.push_back(font_id); // May throw
//cerr << "Removing size "<<s.size<<" from face "<<s.face->list_index<<" for a total of "<<num_sizes<<" sizes ("<<double(num_sizes)/num_faces<<" sizes per face on average)" << endl;
    }


    void get_font_desc(int font_id, FontDesc &desc)
    {
      SizeEntry &s = get_size(font_id);
      FaceEntry &f = *s.face;
      FontLoader::FaceInfo const &info = list->get_face_info(f.list_index);
      desc.family    = info.family;
      desc.boldness  = info.bold   ? 1 : 0;
      desc.italicity = info.italic ? 1 : 0;
      desc.size      = s.size;
    }


    void get_font_info(int font_id, FontInfo &info)
    {
      SizeEntry &s = get_size(font_id);
      FaceEntry &f = *s.face;
      FontLoader::FaceInfo const &i = list->get_face_info(f.list_index);
      ostringstream o;
      o.imbue(locale::classic());
      o << i.family <<
        (i.bold ? i.italic ? " bold italic" : " bold" : i.italic ? " italic" : "") << " " <<
        s.size[0] << "x" << s.size[1];
      info.name = o.str();
      if(!f.face) f.face.reset(list->load_face(f.list_index).release());
      info.num_glyphs = f.face->get_num_glyphs();
    };


    void get_font_metrics(int font_id, bool vertical, bool grid_fitting, FontMetrics &metrics)
    {
      FontFace *const face = get_face(font_id);
      metrics.lateral_span.begin = -face->get_baseline_offset(vertical, grid_fitting);
      metrics.lateral_span.end =
        metrics.lateral_span.begin + face->get_baseline_spacing(vertical, grid_fitting);
    }


    int get_num_glyphs(int font_id)
    {
      return get_face(font_id)->get_num_glyphs();
    }


    void get_glyph_info(int font_id, bool vertical, bool grid_fitting, KernType kern,
                        int num_chars, wchar_t const *chars, GlyphInfo *glyphs)
    {
      FontFace *const face = get_face(font_id);
      int prev_glyph = 0;
      for(int i=0; i<num_chars; ++i)
      {
        int const glyph_index = face->find_glyph(chars[i]);
        face->load_glyph(glyph_index, grid_fitting);
        GlyphInfo &info = glyphs[i];
        info.index = glyph_index;
        info.advance = face->get_glyph_advance(vertical);
        if(kern == kern_No) info.kerning = 0;
        else
        {
          info.kerning = kern == kern_Inc ?
            face->get_kerning(prev_glyph, glyph_index, vertical, grid_fitting) :
            face->get_kerning(glyph_index, prev_glyph, vertical, grid_fitting);
          prev_glyph = glyph_index;
        }
      }
    }


    void render_glyphs(int font_id, bool grid_fitting,
                       BearingType bearing_type, CoordType coord_type,
                       int num_glyphs, int const *glyphs, float const *components,
                       ImageWriter &img_writer)
    {
      FontFace *const face = get_face(font_id);

      double cursor_x = 0, cursor_y = 0;
      if      (coord_type == coord_Vert) cursor_x = *components++;
      else if (coord_type == coord_Hori) cursor_y = *components++;

      for(int i=0; i<num_glyphs; ++i)
      {
        if(coord_type != coord_Vert) cursor_x = *components++; // hori / cloud
        if(coord_type != coord_Hori) cursor_y = *components++; // vert / cloud

        int const glyph = *glyphs++;
        if(glyph < 0) continue;

        face->load_glyph(glyph, grid_fitting);

        Vec2 p(cursor_x, cursor_y);
        switch(bearing_type)
        {
        case bearing_None: break;
        case bearing_Right: p[0] -= face->get_glyph_advance(false); // Fall through
        case bearing_Left:  p    -= face->get_glyph_bearing(false); break;
        case bearing_Above: p[1] -= face->get_glyph_advance(true);  // Fall through
        case bearing_Below: p    -= face->get_glyph_bearing(true);  break;
        }

        face->translate_glyph(p);
        face->render_pixels_to(img_writer);
      }
    }


    void get_glyph_box_info(int font_id, bool grid_fitting, int num_glyphs,
                            int const *glyphs, GlyphBoxInfo *info)
    {
      FontFace *const face = get_face(font_id);
      for(int i=0; i<num_glyphs; ++i, ++glyphs, ++info)
      {
        int const glyph = *glyphs;
        face->load_glyph(glyph, grid_fitting);
        Vec2 const size     =  face->get_glyph_size();
        Vec2 const hori_pos = -face->get_glyph_bearing(false);
        Vec2 const vert_pos = -face->get_glyph_bearing(true);
        info->size.set(size[0], size[1]);
        info->hori_pos.set(hori_pos[0], hori_pos[1]);
        info->vert_pos.set(vert_pos[0], vert_pos[1]);
        info->rev_pos.set(hori_pos[0] - face->get_glyph_advance(false),
                          vert_pos[1] - face->get_glyph_advance(true));
      }
    }


    FontFace *get_face(int font_id)
    {
      SizeEntry &s = get_size(font_id);
      FaceEntry &f = *s.face;
      if(!f.face) f.face.reset(list->load_face(f.list_index).release());
      if(!f.current_size_valid || f.current_size != s.size)
      {
        if(s.fixed_size_index < 0) f.face->set_scaled_size(s.size[0], s.size[1]);
        else f.face->set_fixed_size(s.fixed_size_index);
        f.current_size = s.size;
        f.current_size_valid = true;
      }
      return f.face.get();
    }



    struct FaceEntry
    {
      int const list_index;
      UniquePtr<FontFace> face;
      Vec2 current_size;
      bool current_size_valid;
      typedef std::map<Vec2, int> SizeMap;
      SizeMap size_map; // Values are indices into CacheImpl::sizes

      FaceEntry(int i): list_index(i), current_size_valid(false) {}
    };

    struct SizeEntry
    {
      FaceEntry *face;
      Vec2 size;
      int fixed_size_index; // -1 if the size is not a fixed size
      int use_count;

      SizeEntry(FaceEntry *f, Vec2 const &s, int i):
        face(f), size(s), fixed_size_index(i), use_count(1) {}
    };


    SizeEntry &get_size(int font_id)
    {
      if(font_id < 0 || sizes.size() <= size_t(font_id)) throw invalid_argument("Bad font ID");
      SizeEntry &s = sizes[font_id];
      if(s.use_count == 0) throw invalid_argument("Bad font ID");
      return s;
    }


    FontList::Ptr const list;

    int const default_face_index; // The index of the default face of 'list'.

    double default_width, default_height; // The initial rendering size that the list applies to any face after loading it.

    DeletingVector<FaceEntry> faces; // Indexed as 'list'

    std::vector<SizeEntry> sizes;

    std::vector<int> unused_size_entries; // Indices od unused entries in 'sizes'

    int num_faces, num_sizes;
  };
}


namespace archon
{
  namespace font
  {
    FontCache::Ptr new_font_cache(FontList::Arg l)
    {
      FontCache::Ptr c(new CacheImpl(l));
      return c;
    }
  }
}
