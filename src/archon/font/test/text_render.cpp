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
#include <archon/util/packed_trgb.hpp>
#include <archon/font/util.hpp>
#include <archon/font/text_render.hpp>
#include <archon/font/layout_cfg.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::Math;
using namespace archon::util;
using namespace archon::Imaging;
using namespace archon::Font;


int main(int argc, const char* argv[]) throw()
{
    int               opt_page             = 1;
    Series<2, double> opt_size(512,512);
    PackedTRGB        opt_color            = PackedTRGB(0x000000);
    PackedTRGB        opt_background_color = PackedTRGB(0xFFFFFF);
    PackedTRGB        opt_border_color     = PackedTRGB(0x000000);
    double            opt_margin           = 4;
    int               opt_border           = 1;
    bool              opt_grid_fitting     = true;
    bool              opt_debug            = false;
    bool              opt_mixed            = false;

    FontConfig font_cfg;
    LayoutConfig layout_cfg;
    CommandlineOptions opts;
    opts.add_help("Test application for the font rendering library", "TEXT");
    opts.check_num_args(0,1);
    opts.add_stop_opts();
    opts.add_param("p", "page", opt_page, "The number of the page to be rendered.");
    opts.add_param("S", "size", opt_size,
                   "Maximum page size in number of pixels (width,height). May be fractional. "
                   "If a component is less than or equal to zero, the page is unbounded in "
                   "that direction.");
    opts.add_param("c", "color", opt_color, "Set the text color using any valid CSS3 color value "
                   "(with or without alpha), or the obvious extentension of the hex notation for "
                   "RGBA values.");
    opts.add_param("u", "background-color", opt_background_color, "Set the nackground color using "
                   "any valid CSS3 color value (with or without alpha), or the obvious extentension "
                   "of the hex notation for RGBA values.");
    opts.add_param("o", "border-color", opt_border_color, "Set the border color using any valid "
                   "CSS3 color value (with or without alpha), or the obvious extentension of the "
                   "hex notation for RGBA values.");
    opts.add_param("m", "margin", opt_margin, "Set the width of the margin around the rendered text "
                   "in number of pixels. It does not need not be an integer.");
    opts.add_param("d", "border", opt_border, "Set the width of the border around the rendered text "
                   "in number of pixels.");
    opts.add_group(font_cfg, "font");
    opts.add_group(layout_cfg);
    opts.add_param("g", "grid-fitting", opt_grid_fitting, "Enable grid fitted layout. "
                   "This modifies each glyph slightly to improve the quality of small font sizes.");
    opts.add_param("D", "debug", opt_debug, "Display extra features "
                   "that are helpful when debugging.");
    opts.add_param("M", "mixed", opt_mixed, "Add extra text using a bouquet of font styles "
                   "and colors.");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? 0 : 1;

    FontList::Ptr list = make_font_list(File::dir_of(argv[0])+"../../", font_cfg);
    if (!list)
        return 1;

    wstring text = 1 < argc ? env_decode<wchar_t>(argv[1]) :
        L"The quick brown fox jumps over the lazy dog";
    TextRenderer renderer(new_font_cache(list));
    renderer.set_page_width(Interval(0, opt_size[0]));
    renderer.set_page_height(Interval(0, opt_size[1]));
    renderer.set_text_color(opt_color);
    renderer.set_background_color(opt_background_color);
    renderer.set_border_color(opt_border_color);
    renderer.set_padding(opt_margin, opt_margin, opt_margin, opt_margin);
    renderer.set_border_width(opt_border, opt_border, opt_border, opt_border);
    layout_cfg.apply_to(renderer);
    renderer.enable_grid_fitting(opt_grid_fitting);
    renderer.write(text);

    if (opt_mixed) {
        renderer.write(L" ");
        renderer.set_text_color(Color::red);
        renderer.set_font_size(35, 35);
        renderer.set_font_boldness(1);
        renderer.write(L"Kristian ");
        renderer.set_letter_spacing(10);
        renderer.write(L"Kristian ");

        renderer.set_text_color(Color::lime);
        renderer.set_font_size(25, 25);
        renderer.set_font_boldness(0);
        renderer.set_font_italicity(1);
        renderer.write(L"Spangsege ");

        renderer.set_text_color(Color::fuchsia);
        renderer.set_font_size(30, 30);
        renderer.set_font_italicity(0);
        renderer.set_font_family("URW Palladio L");
        renderer.write(L"h");
        renderer.set_line_spacing(2);
        renderer.write(L"I");
        renderer.set_line_spacing(1);
        renderer.write(L"gh ");

        renderer.set_text_color(Color::blue);
        renderer.set_font_size(45, 45);
        renderer.set_font_family("VL Gothic");
        renderer.write(L"Mandala");
    }

    Image::Ref img = renderer.render(opt_page-1, opt_debug);
    if (!img) {
        cerr << "ERROR: No image!" << endl;
        return 1;
    }

    string out_file = "/tmp/archon_font_text_render.png";
    img->save(out_file);
    cout << "Page "<<opt_page<<" of "<<renderer.get_num_pages()<<" saved to: " << out_file << endl;

    return 0;
}
