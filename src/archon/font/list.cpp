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

#include <map>
#include <set>
#include <vector>
#include <sstream>

#include <archon/core/file.hpp>
#include <archon/core/dir_scan.hpp>
#include <archon/core/text.hpp>
#include <archon/math/vector.hpp>
#include <archon/font/list.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::font;


namespace {

class ListImpl: public FontList {
public:
    void find_default_size(double width, double height, SizeInfo& info) const
    {
        Vec2 size(width, height);
        const Entry& e = get_entry(-1);

        // First check for an exact match
        auto i = e.fixed_sizes.find(size);
        if (i != e.fixed_sizes.end()) {
            info.fixed_size_index = i->second;
            info.exact = true;
            return;
        }

        if (e.info.scalable) {
            info.fixed_size_index = -1;
            info.exact = true;
            return;
        }

        double min = std::numeric_limits<int>::max();
        int idx = 0;
        auto end = e.fixed_sizes.end();
        for (auto j = e.fixed_sizes.begin(); j != end; ++j) {
            double diff = sq_dist(j->first, size);
            if(diff < min)
            {
                min = diff;
                idx = j->second;
            }
        }

        info.fixed_size_index = idx;
        info.exact = false;
    }


    int find_face(FindType find_type, std::string family_name, bool bold, bool italic,
                  double width, double height, SizeInfo* size_info) const
    {
        int num_style = make_style(bold, italic);
        Vec2 size{width, height};
        Family* family = nullptr;
        Style* style = nullptr;

      family:
        // Find font family
        {
            auto i = family_map.find(family_name);
            if (i == family_map.end()) {
                if (!unbooted.empty() || !search_path.empty())
                    goto boot;
                if (find_type != find_BestFace)
                    return -1; // Not found
                ARCHON_ASSERT_1(!family_map.empty(), "No font families");
                // FIXME: We may want to search more intellignetly for an
                // appropriate family name.
                family = &family_map[get_entry(-1).info.family];
            }
            else {
                family = &i->second;
            }
        }

      style:
        // Find style (boldness and italicity)
        {
            auto i = family->styles.find(num_style);
            if (i == family->styles.end()) {
                if (!unbooted.empty() || !search_path.empty())
                    goto boot;
                if (find_type == find_Exact || find_type == find_BestSize)
                    return -1; // Not found
                ARCHON_ASSERT_1(!family->styles.empty(), "No styles for font family");
                int min = std::numeric_limits<int>::max();
                auto e = family->styles.end();
                for (auto j = family->styles.begin(); j != e; ++j) {
                    int diff = j->first ^ num_style;
                    if (diff < min) {
                        min = diff;
                        style = &j->second;
                    }
                }
            }
            else {
                style = &i->second;
            }
        }

      size:
        // Find size
        {
            // First check for an exact match
            auto i = style->fixed_sizes.find(size);
            if (i != style->fixed_sizes.end()) {
                const Style::FixedSize& s = i->second;
                if (size_info) {
                    size_info->fixed_size_index = s.second;
                    size_info->exact = true;
                }
                return s.first;
            }

            if (0 <= style->scalable) { // Face is scalable
                if (size_info) {
                    size_info->fixed_size_index = -1;
                    size_info->exact = true;
                }
                return style->scalable;
            }

            // Face is not scalable
            if (!unbooted.empty() || !search_path.empty())
                goto boot;
            if (find_type == find_Exact)
                return -1; // Not found
            ARCHON_ASSERT_1(!style->fixed_sizes.empty(), "No fixed sizes for font style");
            if (!style->multiple_fixed_faces && !size_info)
                return style->first_fixed_face;
            double min = std::numeric_limits<int>::max();
            const Style::FixedSize* s = nullptr;
            auto e = style->fixed_sizes.end();
            for (auto j = style->fixed_sizes.begin(); j != e; ++j) {
                double diff = sq_dist(j->first, size);
                if (diff < min) {
                    min = diff;
                    s = &j->second;
                }
            }

            if (size_info) {
                size_info->fixed_size_index = s->second;
                size_info->exact = false;
            }
            return s->first;
        }

      boot:
        // If there are unbooted entries, try to boot those first. Then, if
        // there is still no exact match and we have an outstanding scan, do
        // that scan now.
        if (unbooted.empty())
            scan();

        while (!unbooted.empty()) {
            Entry &e = entries[*unbooted.begin()];
            boot(e);
            if (e.info.family == family_name && e.info.bold == bold && e.info.italic == italic) {
                if (e.info.scalable) {
                    // If any matching fixed size existed at this point, we
                    // would already have found it, so it is not there.
                    if (size_info) {
                        size_info->fixed_size_index = -1;
                        size_info->exact = true;
                    }
                    return e.list_index;
                }
                auto i = e.fixed_sizes.find(size);
                if (i != e.fixed_sizes.end()) {
                    // Found exactly matching fixed size
                    if (size_info) {
                        size_info->fixed_size_index = i->second;
                        size_info->exact = true;
                    }
                    return e.list_index;
                }
            }
        }

        if (find_type == find_Exact)
            return -1;
        if (!family)
            goto family;
        if (!style)
            goto style;
        goto size;
    }


    std::unique_ptr<FontFace> load_face(int i) const
    {
        const Entry& entry = get_entry(i);
        return loader->load_face(entry.file_path, entry.file_face_index, init_width, init_height);
    }


    int get_num_faces() const
    {
        if (!search_path.empty())
            scan();
        return entries.size();
    }


    const FontLoader::FaceInfo& get_face_info(int i) const
    {
        return get_entry(i).info;
    }


    int get_num_families() const
    {
        if (!search_path.empty())
            scan();
        while (!unbooted.empty()) {
            Entry& e = entries[*unbooted.begin()];
            boot(e);
        }
        while (family_check_end < entries.size()) {
            auto i = family_map.find(entries[family_check_end++].info.family);
            if (i->second.fresh) {
                families.push_back(i);
                i->second.fresh = false;
            }
        }
        return families.size();
    }


    std::string get_family_name(int family_index) const
    {
        if (int(families.size()) <= family_index)
            get_num_families(); // Force update of 'families'
        return families.at(family_index)->first;
    }


    void add_face(std::string f, int i)
    {
        if (!search_path.empty())
            scan();
        int n = loader->check_file(f);
        if (n < 1)
            throw BadFontFileException("Failed to recognize \""+f+"\" as a font file");
        if (i < 0) {
            for (int j = 0; j < n; ++j)
                add_face_unchecked(f,j);
        }
        else {
            if (n <= i)
                throw std::out_of_range("font file face index out of range");
            add_face_unchecked(f,i);
        }
    }


    void scan_dir(std::string dir, bool recurse)
    {
        if (!search_path.empty())
            scan();
        if (dir.empty()) {
            dir = "./";
        }
        else if(*dir.end() != '/') {
            dir.append(1, '/');
        }
        std::unique_ptr<DirScanner> scanner = DirScanner::new_dir_scanner(dir);
        for (;;) {
            std::string name = scanner->next_entry();
            if (name.empty())
                break;
            std::string path = dir+name;
            file::Stat stat{path};
            switch (stat.get_type()) {
                case file::Stat::type_Regular: {
                    int n = loader->check_file(path);
                    for (int i = 0; i < n; ++i)
                        add_face_unchecked(path, i);
                    break;
                }
                case file::Stat::type_Directory:
                    if (recurse) {
                        try {
                            scan_dir(path+"/", true);
                        }
                        catch(file::AccessException&) {
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }


    void get_init_size(double& width, double& height) const
    {
        width  = init_width;
        height = init_height;
    }


    int get_default_face() const
    {
        return default_index;
    }


    ListImpl(std::shared_ptr<FontLoader> l, double w, double h):
        loader{l},
        init_width{w},
        init_height{h}
    {
    }


    void set_search_path(std::string p)
    {
        search_path = p;
    }
    void set_default_index(int i)
    {
        default_index = i;
    }


private:
    struct Style;

    class Entry {
    public:
        int list_index;
        std::string file_path;
        int file_face_index;
        Style* style = nullptr; // Null until booted
        FontLoader::FaceInfo info; // Undefined untill booted
        using FixedSizes = std::map<Vec2, int>; // Value is fixed size index
        FixedSizes fixed_sizes;
        Entry(int i, std::string f, int j):
            list_index{i},
            file_path{f},
            file_face_index{j}
        {
        }
    };

    using Entries = std::vector<Entry>;

    struct Style {
        int scalable = -1; // Index of scalable face, or negative if no scalable face has been encountered.
        using FixedSize = std::pair<int, int>; // (face index, fixed size index)
        using FixedSizes = std::map<Vec2, FixedSize>;
        FixedSizes fixed_sizes;
        int first_fixed_face = -1; // First face that contributed a fixed size, or negative if none have been added yet.
        bool multiple_fixed_faces = false; // True if more than one face has contributed fixed sizes.
    };

    using StyleMap = std::map<int, Style>;

    struct Family {
        bool fresh = true; // True until this family is added to 'families'
        StyleMap styles;
    };

    using FamilyMap = std::map<std::string, Family>;


    const std::shared_ptr<FontLoader> loader;
    const double init_width, init_height;

    mutable Entries entries;
    mutable std::set<int> unbooted;
    mutable FamilyMap family_map;
    mutable std::vector<FamilyMap::iterator> families;

    mutable std::size_t family_check_end = 0; // Next entry to check for addition of family to 'families'

    int default_index = 0;
    mutable std::string search_path; // A scan is pending if this is non-empty


    static int make_style(bool bold, bool italic)
    {
        return (bold ? 1 : 0) | (italic ? 2 : 0);
    }


    const Entry& get_entry(int i) const
    {
        if (i < 0) {
            i = default_index;
        }
        else if(!search_path.empty() && entries.size() <= std::size_t(i)) {
            scan();
        }
        Entry& entry = entries.at(i);
        if (!entry.style)
            boot(entry);
        return entry;
    }


    void add_face_unchecked(std::string f, int i)
    {
        int j = entries.size();
        auto k = unbooted.insert(j).first;
        try {
            entries.push_back(Entry(j,f,i));
        }
        catch (...) {
            unbooted.erase(k);
            throw;
        }
    }


    void boot(Entry& e) const
    {
        loader->load_face_info(e.file_path, e.file_face_index, e.info);

        Family& family = family_map[e.info.family];
        Style& style = family.styles[make_style(e.info.bold, e.info.italic)];
        if (e.info.scalable)
            style.scalable = e.list_index;
        if (!e.info.fixed_sizes.empty()) {
            if (style.first_fixed_face < 0) {
                style.first_fixed_face = e.list_index;
            }
            else if(style.first_fixed_face != e.list_index) {
                style.multiple_fixed_faces = true;
            }
        }

        {
            std::size_t n = e.info.fixed_sizes.size();
            for (std::size_t i = 0; i < n; ++i) {
                const FontLoader::FaceInfo::FixedSize& s = e.info.fixed_sizes[i];
                Vec2 v(s.first, s.second);
                e.fixed_sizes[v] = i;
                style.fixed_sizes[v] = Style::FixedSize(e.list_index, i);
            }
        }

        e.style = &style;
        unbooted.erase(e.list_index);
    }


    void scan() const
    {
        std::string p;
        swap(p, search_path);
        const_cast<ListImpl*>(this)->scan_dirs(p, true);
    }
};

} // unnamed namespace


namespace archon {
namespace font {

void FontList::scan_dirs(std::string dir_paths, bool recursive)
{
    std::istringstream in(dir_paths);
    Text::SimpleTokenizer<char> tokenizer(in, ":", Text::SimpleTokenizer<char>::incl_empty,
                                          std::locale::classic());
    std::string dir;
    while (tokenizer.generate(dir)) {
        if (file::is_dir(dir))
            scan_dir(dir, recursive);
    }
}


std::shared_ptr<FontList> new_font_list(std::shared_ptr<FontLoader> loader, std::string font_file,
                                        int face_index, double width, double height)
{
    std::shared_ptr<FontList> list = std::make_shared<ListImpl>(loader, width, height);
    list->add_face(font_file, face_index);
    return list;
}


std::shared_ptr<FontList> new_font_list(std::shared_ptr<FontLoader> loader,
                                        std::string font_search_path,
                                        double width, double height)
{
    std::shared_ptr<FontList> list = std::make_shared<ListImpl>(loader, width, height);
    list->add_face(loader->get_default_font_file(), loader->get_default_face_index());
    static_cast<ListImpl*>(list.get())->set_search_path(font_search_path);
    return list;
}


std::shared_ptr<FontList> new_font_list(std::shared_ptr<FontLoader> loader,
                                        std::string font_search_path,
                                        FontList::FindType find_type, std::string family,
                                        bool bold, bool italic, double width, double height)
{
    std::shared_ptr<FontList> list = std::make_shared<ListImpl>(loader, width, height);
    list->add_face(loader->get_default_font_file(), loader->get_default_face_index());
    static_cast<ListImpl*>(list.get())->set_search_path(font_search_path);
    if (family.empty())
        family = list->get_face_info().family;
    int i = list->find_face(find_type, family, bold, italic, width, height);
    if (i < 0) {
        list.reset();
    }
    else {
        static_cast<ListImpl*>(list.get())->set_default_index(i);
    }
    return list;
}

} // namespace font
} // namespace archon
