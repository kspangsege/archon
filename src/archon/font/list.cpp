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

#include <map>
#include <set>
#include <vector>
#include <sstream>

#include <archon/core/file.hpp>
#include <archon/core/dir_scan.hpp>
#include <archon/core/text.hpp>
#include <archon/math/vector.hpp>
#include <archon/font/list.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::font;


namespace
{
  struct ListImpl: FontList
  {
    void find_default_size(double width, double height, SizeInfo &info) const
    {
      Vec2 const size(width, height);
      Entry const &e = get_entry(-1);

      // First check for an exact match
      Entry::FixedSizes::const_iterator const i = e.fixed_sizes.find(size);
      if(i != e.fixed_sizes.end())
      {
        info.fixed_size_index = i->second;
        info.exact = true;
        return;
      }

      if(e.info.scalable)
      {
        info.fixed_size_index = -1;
        info.exact = true;
        return;
      }

      double min = numeric_limits<int>::max();
      int idx = 0;
      Entry::FixedSizes::const_iterator const end = e.fixed_sizes.end();
      for(Entry::FixedSizes::const_iterator j = e.fixed_sizes.begin(); j != end; ++j)
      {
        double const diff = sq_dist(j->first, size);
        if(diff < min)
        {
          min = diff;
          idx = j->second;
        }
      }

      info.fixed_size_index = idx;
      info.exact = false;
    }


    int find_face(FindType find_type, string family_name, bool bold, bool italic,
                  double width, double height, SizeInfo *size_info) const
    {
      int const num_style = make_style(bold, italic);
      Vec2 const size(width, height);
      Family *family = 0;
      Style *style = 0;

    family:
      // Find font family
      {
        FamilyMap::iterator const i = family_map.find(family_name);
        if(i == family_map.end())
        {
          if(!unbooted.empty() || !search_path.empty()) goto boot;
          if(find_type != find_BestFace) return -1; // Not found
          ARCHON_ASSERT_1(!family_map.empty(), "No font families");
          // FIXME: We may want to search more intellignetly for an
          // appropriate family name.
          family = &family_map[get_entry(-1).info.family];
        }
        else family = &i->second;
      }

    style:
      // Find style (boldness and italicity)
      {
        StyleMap::iterator const i = family->styles.find(num_style);
        if(i == family->styles.end())
        {
          if(!unbooted.empty() || !search_path.empty()) goto boot;
          if(find_type == find_Exact || find_type == find_BestSize) return -1; // Not found
          ARCHON_ASSERT_1(!family->styles.empty(), "No styles for font family");
          int min = numeric_limits<int>::max();
          StyleMap::iterator const e = family->styles.end();
          for(StyleMap::iterator j = family->styles.begin(); j != e; ++j)
          {
            int const diff = j->first ^ num_style;
            if(diff < min)
            {
              min = diff;
              style = &j->second;
            }
          }
        }
        else style = &i->second;
      }

    size:
      // Find size
      {
        // First check for an exact match
        Style::FixedSizes::iterator const i = style->fixed_sizes.find(size);
        if(i != style->fixed_sizes.end())
        {
          Style::FixedSize const &s = i->second;
          if(size_info)
          {
            size_info->fixed_size_index = s.second;
            size_info->exact = true;
          }
          return s.first;
        }

        if(0 <= style->scalable) // Face is scalable
        {
          if(size_info)
          {
            size_info->fixed_size_index = -1;
            size_info->exact = true;
          }
          return style->scalable;
        }

        // Face is not scalable
        if(!unbooted.empty() || !search_path.empty()) goto boot;
        if(find_type == find_Exact) return -1; // Not found
        ARCHON_ASSERT_1(!style->fixed_sizes.empty(), "No fixed sizes for font style");
        if(!style->multiple_fixed_faces && !size_info) return style->first_fixed_face;
        double min = numeric_limits<int>::max();
        Style::FixedSize const *s = nullptr;
        Style::FixedSizes::iterator const e = style->fixed_sizes.end();
        for(Style::FixedSizes::iterator j = style->fixed_sizes.begin(); j != e; ++j)
        {
          double const diff = sq_dist(j->first, size);
          if(diff < min)
          {
            min = diff;
            s = &j->second;
          }
        }

        if(size_info)
        {
          size_info->fixed_size_index = s->second;
          size_info->exact = false;
        }
        return s->first;
      }

    boot:
      // If there are unbooted entries, try to boot those first. Then,
      // if there is still no exact match and we have an outstanding
      // scan, do that scan now.
      if(unbooted.empty()) scan();

      while(!unbooted.empty())
      {
        Entry &e = entries[*unbooted.begin()];
        boot(e);
        if(e.info.family == family_name && e.info.bold == bold && e.info.italic == italic)
        {
          if(e.info.scalable)
          {
            // If any matching fixed size existed at this point, we
            // would already have found it, so it is not there.
            if(size_info)
            {
              size_info->fixed_size_index = -1;
              size_info->exact = true;
            }
            return e.list_index;
          }
          Entry::FixedSizes::iterator const i = e.fixed_sizes.find(size);
          if(i != e.fixed_sizes.end())
          {
            // Found exactly matching fixed size
            if(size_info)
            {
              size_info->fixed_size_index = i->second;
              size_info->exact = true;
            }
            return e.list_index;
          }
        }
      }

      if(find_type == find_Exact) return -1;
      if(!family) goto family;
      if(!style) goto style;
      goto size;
    }


    UniquePtr<FontFace> load_face(int i) const
    {
      Entry const &entry = get_entry(i);
      UniquePtr<FontFace> face(loader->load_face(entry.file_path, entry.file_face_index,
                                                 init_width, init_height).release());
      return face;
    }


    int get_num_faces() const
    {
      if(!search_path.empty()) scan();
      return entries.size();
    }


    FontLoader::FaceInfo const &get_face_info(int i) const
    {
      return get_entry(i).info;
    }


    int get_num_families() const
    {
      if(!search_path.empty()) scan();
      while(!unbooted.empty())
      {
        Entry &e = entries[*unbooted.begin()];
        boot(e);
      }
      while(family_check_end < entries.size())
      {
        FamilyMap::iterator const i = family_map.find(entries[family_check_end++].info.family);
        if(i->second.fresh)
        {
          families.push_back(i);
          i->second.fresh = false;
        }
      }
      return families.size();
    }


    string get_family_name(int family_index) const
    {
      if(int(families.size()) <= family_index) get_num_families(); // Force update of 'families'
      return families.at(family_index)->first;
    }


    void add_face(string f, int i)
    {
      if(!search_path.empty()) scan();
      int const n = loader->check_file(f);
      if(n < 1) throw BadFontFileException("Failed to recognize \""+f+"\" as a font file");
      if(i < 0) for(int j=0; j<n; ++j) add_face_unchecked(f,j);
      else
      {
        if(n <= i) throw out_of_range("font file face index out of range");
        add_face_unchecked(f,i);
      }
    }


    void scan_dir(string dir, bool recurse)
    {
      if(!search_path.empty()) scan();
      if(dir.empty()) dir = "./";
      else if(*dir.end() != '/') dir.append(1, '/');
      UniquePtr<DirScanner> scanner(DirScanner::new_dir_scanner(dir).release());
      for(;;)
      {
        string name = scanner->next_entry();
        if(name.empty()) break;
        string path = dir+name;
        file::Stat stat(path);
        switch(stat.get_type())
        {
        case file::Stat::type_Regular:
          {
            int const n = loader->check_file(path);
            for(int i=0; i<n; ++i) add_face_unchecked(path, i);
          }
          break;
        case file::Stat::type_Directory:
          if(recurse)
            try
            {
              scan_dir(path+"/", true);
            }
            catch(file::AccessException &) {}
          break;
        default:
          break;
        }
      }
    }


    void get_init_size(double &width, double &height) const
    {
      width  = init_width;
      height = init_height;
    }


    int get_default_face() const
    {
      return default_index;
    }


    ListImpl(FontLoader::Ptr l, double w, double h):
      loader(l), init_width(w), init_height(h), family_check_end(0), default_index(0) {}


    void set_search_path(string p) { search_path = p; }
    void set_default_index(int i) { default_index = i; }


  private:
    struct Style;

    struct Entry
    {
      int list_index;
      string file_path;
      int file_face_index;
      Style *style; // Null until booted
      FontLoader::FaceInfo info; // Undefined untill booted
      typedef std::map<Vec2, int> FixedSizes; // Value is fixed size index
      FixedSizes fixed_sizes;
      Entry(int i, string f, int j):
        list_index(i), file_path(f), file_face_index(j), style(0) {}
    };

    typedef vector<Entry> Entries;

    struct Style
    {
      int scalable; // Index of scalable face, or negative if no scalable face has been encountered.
      typedef pair<int, int> FixedSize; // (face index, fixed size index)
      typedef std::map<Vec2, FixedSize> FixedSizes;
      FixedSizes fixed_sizes;
      int first_fixed_face; // First face that contributed a fixed size, or negative if none have been added yet.
      bool multiple_fixed_faces; // True if more than one face has contributed fixed sizes.
      Style(): scalable(-1), first_fixed_face(-1), multiple_fixed_faces(false) {}
    };

    typedef std::map<int, Style> StyleMap;

    struct Family
    {
      bool fresh; // True until this family is added to 'families'
      StyleMap styles;
      Family(): fresh(true) {}
    };

    typedef std::map<string, Family> FamilyMap;


    FontLoader::Ptr const loader;
    double const init_width, init_height;

    Entries mutable entries;
    set<int> mutable unbooted;
    FamilyMap mutable family_map;
    vector<FamilyMap::iterator> mutable families;

    size_t mutable family_check_end; // Next entry to check for addition of family to 'families'

    int default_index;
    string mutable search_path; // A scan is pending if this is non-empty


    static int make_style(bool bold, bool italic)
    {
      return (bold ? 1 : 0) | (italic ? 2 : 0);
    }


    Entry const &get_entry(int i) const
    {
      if(i < 0) i = default_index;
      else if(!search_path.empty() && entries.size() <= size_t(i)) scan();
      Entry &entry = entries.at(i);
      if(!entry.style) boot(entry);
      return entry;
    }


    void add_face_unchecked(string f, int i)
    {
      int const j = entries.size();
      set<int>::iterator const k = unbooted.insert(j).first;
      try
      {
        entries.push_back(Entry(j,f,i));
      }
      catch(...)
      {
        unbooted.erase(k);
        throw;
      }
    }


    void boot(Entry &e) const
    {
      loader->load_face_info(e.file_path, e.file_face_index, e.info);

      Family &family = family_map[e.info.family];
      Style &style = family.styles[make_style(e.info.bold, e.info.italic)];
      if(e.info.scalable) style.scalable = e.list_index;
      if(!e.info.fixed_sizes.empty())
      {
        if(style.first_fixed_face < 0) style.first_fixed_face = e.list_index;
        else if(style.first_fixed_face != e.list_index) style.multiple_fixed_faces = true;
      }

      {
        size_t const n = e.info.fixed_sizes.size();
        for(size_t i=0; i<n; ++i)
        {
          FontLoader::FaceInfo::FixedSize const &s = e.info.fixed_sizes[i];
          Vec2 const v(s.first, s.second);
          e.fixed_sizes[v] = i;
          style.fixed_sizes[v] = Style::FixedSize(e.list_index, i);
        }
      }

      e.style = &style;
      unbooted.erase(e.list_index);
    }


    void scan() const
    {
      string p;
      swap(p, search_path);
      const_cast<ListImpl *>(this)->scan_dirs(p, true);
    }
  };
}


namespace archon
{
  namespace font
  {
    void FontList::scan_dirs(string dir_paths, bool recursive)
    {
      istringstream in(dir_paths);
      Text::SimpleTokenizer<char> tokenizer(in, ":", Text::SimpleTokenizer<char>::incl_empty,
                                            locale::classic());
      string dir;
      while(tokenizer.generate(dir)) if(file::is_dir(dir)) scan_dir(dir, recursive);
    }


    FontList::Ptr new_font_list(FontLoader::Ptr loader, string font_file, int face_index,
                                double width, double height)
    {
      FontList::Ptr list(new ListImpl(loader, width, height));
      list->add_face(font_file, face_index);
      return list;
    }


    FontList::Ptr new_font_list(FontLoader::Ptr loader, string font_search_path,
                                double width, double height)
    {
      FontList::Ptr list(new ListImpl(loader, width, height));
      list->add_face(loader->get_default_font_file(), loader->get_default_face_index());
      static_cast<ListImpl *>(list.get())->set_search_path(font_search_path);
      return list;
    }


    FontList::Ptr new_font_list(FontLoader::Ptr loader, string font_search_path,
                                FontList::FindType find_type, string family,
                                bool bold, bool italic, double width, double height)
    {
      FontList::Ptr list(new ListImpl(loader, width, height));
      list->add_face(loader->get_default_font_file(), loader->get_default_face_index());
      static_cast<ListImpl *>(list.get())->set_search_path(font_search_path);
      if(family.empty()) family = list->get_face_info().family;
      int const i = list->find_face(find_type, family, bold, italic, width, height);
      if(i < 0) list.reset();
      else static_cast<ListImpl *>(list.get())->set_default_index(i);
      return list;
    }
  }
}
