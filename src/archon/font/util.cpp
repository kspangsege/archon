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

#include <iostream>

#include <archon/core/text_table.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/util.hpp>

using namespace archon::core;


namespace archon {
namespace font {

void print_font_list(const FontList& font_list, std::ostream& out, bool enable_ansi_term_attr)
{
    Text::Table table{enable_ansi_term_attr};
    table.get_odd_row_attr().set_bg_color(Term::color_White);
    table.get_odd_col_attr().set_bold();
    table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();
    table.get_cell(0,1).set_text("Family");
    table.get_cell(0,2).set_text("Bold");
    table.get_cell(0,3).set_text("Italic");
    table.get_cell(0,4).set_text("Monospace");
    table.get_cell(0,5).set_text("Scalable");
    int n = font_list.get_num_faces();
    for (int i = 0; i < n; ++i) {
        const FontLoader::FaceInfo& face_info = font_list.get_face_info(i);
        table.get_cell(i+1, 0).set_val(i);
        table.get_cell(i+1, 1).set_val(face_info.family);
        if (face_info.bold)
            table.get_cell(i+1, 2).set_text("B");
        if (face_info.italic)
            table.get_cell(i+1, 3).set_text("I");
        if (face_info.monospace)
            table.get_cell(i+1, 4).set_text("M");
        if (face_info.scalable)
            table.get_cell(i+1, 5).set_text("S");
    }
    out << table.print();
}


void ListConfig::populate(ConfigBuilder& cfg)
{
    cfg.add_param("s", "size", size,
                  "The default nominal glyph size in number of pixels for any font "
                  "in the list (may be fractional). Use zero to select the default size "
                  "of the implementation");
    cfg.add_param("l", "list", list, "List the known fonts and exit");
    cfg.add_param("P", "path", path,
                  "The font search path which is a colon separated list of directories "
                  "holding font files");
    cfg.add_param("F", "file", file, "Use the first font in this font file. "
                  "This option overrides --family, --bold, and --italic");
}


void FontConfig::populate(ConfigBuilder& cfg)
{
    ListConfig::populate(cfg);
    cfg.add_param("f", "family", family, "The family name of the font to use. "
                  "The family name of the default font will be used if this is empty");
    cfg.add_param("b", "bold", bold, "Use a bold font face");
    cfg.add_param("i", "italic", italic, "Use an italic font face");
}


ListConfig::ListConfig():
    size{0,0},
    path{"/usr/share/fonts"}
{
}


FontConfig::FontConfig()
{
}


std::unique_ptr<FontFace> load_font(const std::string& resource_dir, const FontConfig& cfg)
{
    if (std::unique_ptr<FontList> list = new_font_list(resource_dir, cfg))
        return list->load_face();
    return nullptr;
}


std::unique_ptr<FontList> new_font_list(const std::string& resource_dir, const FontConfig& cfg)
{
    std::shared_ptr<FontLoader> loader = new_font_loader(resource_dir);
    if (cfg.list) {
        std::unique_ptr<FontList> list;
        if (cfg.file.empty()) {
            list = new_font_list(std::move(loader), cfg.path);
        }
        else {
            list = new_font_list(std::move(loader), cfg.file, 0, 12, 12);
        }
        print_font_list(*list, std::cout);
        return nullptr;
    }

    double w = cfg.size[0], h = cfg.size[1];
    if (w <= 0 || h <= 0)
        w = h = 12;

    if (!cfg.file.empty())
        return new_font_list(std::move(loader), cfg.file, 0, w, h);

    std::unique_ptr<FontList> list =
        new_font_list(std::move(loader), cfg.path, FontList::find_BestSize,
                      cfg.family, cfg.bold, cfg.italic, w, h);
    if (list)
        return list;

    std::string name =
        (cfg.family.empty() ? "default family name" : "family name '"+cfg.family+"'");
    std::string style = (cfg.bold ? (cfg.italic ? "bold italic" : "bold") :
                         (cfg.italic ? "italic" : "regular"));
    std::cerr << "No font face with "<<name<<" and style '"<<style<<"'\n";
    return nullptr;
}

} // namespace font
} // namespace archon
