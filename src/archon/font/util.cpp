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

#include <iostream>

#include <archon/core/text_table.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/util.hpp>

using namespace std;
using namespace Archon::Core;


namespace Archon
{
  namespace Font
  {
    void print_font_list(FontList::ConstArg l, ostream &out, bool enable_ansi_term_attr)
    {
      Text::Table table(enable_ansi_term_attr);
      table.get_odd_row_attr().set_bg_color(Term::color_White);
      table.get_odd_col_attr().set_bold();
      table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();
      table.get_cell(0,1).set_text("Family");
      table.get_cell(0,2).set_text("Bold");
      table.get_cell(0,3).set_text("Italic");
      table.get_cell(0,4).set_text("Monospace");
      table.get_cell(0,5).set_text("Scalable");
      int const n = l->get_num_faces();
      for(int i=0; i<n; ++i)
      {
        FontLoader::FaceInfo const&f = l->get_face_info(i);
        table.get_cell(i+1, 0).set_val(i);
        table.get_cell(i+1, 1).set_val(f.family);
        if(f.bold)      table.get_cell(i+1, 2).set_text("B");
        if(f.italic)    table.get_cell(i+1, 3).set_text("I");
        if(f.monospace) table.get_cell(i+1, 4).set_text("M");
        if(f.scalable)  table.get_cell(i+1, 5).set_text("S");
      }
      out << table.print();
    }


    void ListConfig::populate(ConfigBuilder &cfg)
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


    void FontConfig::populate(ConfigBuilder &cfg)
    {
      ListConfig::populate(cfg);
      cfg.add_param("f", "family", family, "The family name of the font to use. "
                    "The family name of the default font will be used if this is empty");
      cfg.add_param("b", "bold", bold, "Use a bold font face");
      cfg.add_param("i", "italic", italic, "Use an italic font face");
    }


    ListConfig::ListConfig(): size(0,0), list(false), path("/usr/share/fonts") {}


    FontConfig::FontConfig(): bold(false), italic(false) {}


    UniquePtr<FontFace> load_font(string resource_dir, FontConfig const &cfg)
    {
      FontList::Ptr const list = make_font_list(resource_dir, cfg);
      UniquePtr<FontFace> face(list ? list->load_face().release() : 0);
      return face;
    }


    FontList::Ptr make_font_list(string resource_dir, FontConfig const &cfg)
    {
      FontLoader::Ptr const loader = new_font_loader(resource_dir);
      if(cfg.list)
      {
        FontList::Ptr const list = cfg.file.empty() ? new_font_list(loader, cfg.path) :
          new_font_list(loader, cfg.file, 0, 12, 12);
        print_font_list(list, cout);
        return FontList::Ptr(); // Null
      }

      double w = cfg.size[0], h = cfg.size[1];
      if(w <= 0 || h <= 0) w = h = 12;

      if(!cfg.file.empty())
        return new_font_list(loader, cfg.file, 0, w, h);

      FontList::Ptr const list = new_font_list(loader, cfg.path, FontList::find_BestSize,
                                               cfg.family, cfg.bold, cfg.italic, w, h);
      if(list) return list;

      string const name =
        cfg.family.empty() ? "default family name" : "family name '"+cfg.family+"'";
      string const style = cfg.bold ? cfg.italic ? "bold italic" : "bold" :
        cfg.italic ? "italic" : "regular";
      cerr << "No font face with "<<name<<" and style '"<<style<<"'"<< endl;
      return FontList::Ptr(); // Null
    }
  }
}
