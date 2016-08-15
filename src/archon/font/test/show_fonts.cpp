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

#include <archon/core/char_enc.hpp>
#include <archon/core/file.hpp>
#include <archon/core/options.hpp>
#include <archon/font/util.hpp>
#include <archon/font/text_render.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::Imaging;
using namespace archon::Font;


int main(int argc, char const *argv[]) throw()
{
  FontConfig font_cfg;
  CommandlineOptions opts;
  opts.add_help("Render the specified text using each available font", "TEXT");
  opts.check_num_args(0,1);
  opts.add_stop_opts();
  opts.add_group<ListConfig>(font_cfg, "font");
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  FontList::Ptr const list = make_font_list(File::dir_of(argv[0])+"../../", font_cfg);
  if(!list) return 1;

  wstring const text = 1 < argc ? env_decode<wchar_t>(argv[1]) :
    L"0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";

  TextRenderer renderer(new_font_cache(list));

  int const n = list->get_num_families();
  for(int i=0; i<n; ++i)
  {
    wostringstream o;
    o << i+1 << ": ";
    renderer.write(o.str());
    string name = list->get_family_name(i);
    renderer.write(env_decode<wchar_t>(name.empty() ? "(no name)" : name));
    renderer.write(L" \"");
    renderer.set_font_family(name);
    renderer.write(text);
    renderer.reset_font();
    renderer.write(L"\"\n");
  }

  Image::Ref const img = renderer.render();
  if(!img)
  {
    cerr << "ERROR: No image!" << endl;
    return 1;
  }

  string const out_file = "/tmp/archon_fonts.png";
  img->save(out_file);
  cout << "Result saved as: " << out_file << endl;

  return 0;
}
