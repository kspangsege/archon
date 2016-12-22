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

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <archon/core/generate.hpp>
#include <archon/core/random.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_table.hpp>
#include <archon/core/options.hpp>
#include <archon/util/rect_packer.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/util.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;


namespace {

class Glyph {
public:
    int index; // According to the FontFace instance
    int left, bottom; // Position in image
    int width, height; // Size in image
    int hori_bearing_x, hori_bearing_y;
    int vert_bearing_x, vert_bearing_y;
    int hori_advance, vert_advance;
    std::vector<wchar_t> code_points;

    Glyph(int i):
        index{i}
    {
    }
};


class GlyphHeightOrderCmp {
public:
    const std::vector<Glyph>& glyphs;

    bool operator()(int a, int b) const
    {
        return (glyphs[b].height < glyphs[a].height);
    }

    GlyphHeightOrderCmp(const std::vector<Glyph>& g):
        glyphs{g}
    {
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    FontConfig font_cfg;
    CommandlineOptions opts;
    opts.add_help("Utility to regenerate the fallback font", "RANGES");
    opts.check_num_args(0,1);
    opts.add_group(font_cfg, "font");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::unique_ptr<FontFace> face = load_font(file::dir_of(argv[0])+"../../", font_cfg);
    if (!face)
        return EXIT_FAILURE;

    std::string target_png  = "/tmp/fallback-font.png";
    std::string target_conf = "/tmp/fallback-font.conf";

    using CharRange = std::pair<wchar_t, wchar_t>;
    using CharRanges = std::vector<CharRange>;
    CharRanges char_ranges;
    if (argc < 2) {
        char_ranges.push_back(CharRange(0, 127));
    }
    else {
        std::istringstream in{argv[1]};
        Text::SimpleTokenizer<char> tokenizer(in, ",", Text::SimpleTokenizer<char>::incl_empty,
                                              std::locale::classic());
        wchar_t max = std::numeric_limits<wchar_t>::max();
        std::string range;
        while (tokenizer.generate(range)) {
            std::istringstream in2{range};
            unsigned long from, to;
            char c = '-';
            in2 >> from;
            if (in2.eof()) {
                to = from;
            }
            else {
                in2 >> c >> to;
            }
            if (  (in2.fail() || in2.bad() || !in2.eof() || c != '-' || to < from ||
                   int_less_than(max, from) || int_less_than(max, to)))
                throw std::runtime_error("Bad character range");
            char_ranges.push_back(CharRange(from, to));
        }
    }

    std::vector<Glyph> glyphs;
    glyphs.push_back(Glyph{0}); // Add replacement glyph first
    {
        using GlyphMap = std::map<int, int>;
        GlyphMap glyph_map;
        for (std::size_t i = 0; i < char_ranges.size(); ++i) {
            const CharRange& r = char_ranges[i];
            wchar_t c = r.first;
            for (;;) {
                int index = face->find_glyph(c);
                if (index != 0) {
                    auto r = glyph_map.insert(std::make_pair(index, -1));
                    Glyph* g;
                    if (r.second) { // New
                        r.first->second = glyphs.size();
                        glyphs.push_back(Glyph(index));
                        g = &glyphs.back();
                    }
                    else {
                        g = &glyphs[r.first->second];
                    }
                    g->code_points.push_back(c);
                }

                if (c == r.second)
                    break;
                ++c;
            }
        }
    }

    // Fetch glyph metrics
    for (Glyph& g: glyphs) {
        face->load_glyph(g.index, true); // Request grid fitting
        int left, right, bottom, top;
        face->get_glyph_pixel_box(left, right, bottom, top);
        g.width  = right - left;
        g.height = top - bottom;
        Vec2 hori_bearing = face->get_glyph_bearing(false);
        g.hori_bearing_x = hori_bearing[0];
        g.hori_bearing_y = hori_bearing[1];
        Vec2 vert_bearing = face->get_glyph_bearing(true);
        g.vert_bearing_x = vert_bearing[0];
        g.vert_bearing_y = vert_bearing[1];
        g.hori_advance = face->get_glyph_advance(false);
        g.vert_advance = face->get_glyph_advance(true);
    }

    // Sort according to height
    int n = glyphs.size();
    std::vector<int> glyph_order(n);
    generate(glyph_order.begin(), glyph_order.end(), make_inc_generator<int>());
    std::sort(glyph_order.begin(), glyph_order.end(), GlyphHeightOrderCmp(glyphs));

    long area = 0;
    int max_width = 0;
    for (const Glyph& g: glyphs) {
        if (max_width < g.width) max_width = g.width;
        area += g.height * long(g.width);
    }

    if (area <= 0) {
        std::cerr << "ERROR: No ink\n";
        return EXIT_FAILURE;
    }

    int width = std::max<int>(std::sqrt(double(area)), max_width);
    RectanglePacker packer{width};
    for (int i = 0; i < n; ++i) {
        int glyph_index = glyph_order[i];
        Glyph& g = glyphs[glyph_index];
        if (!packer.insert(g.width, g.height, g.left, g.bottom))
            throw std::runtime_error("Out of space in image");
    }

    int height = packer.get_height();

    std::cerr << "Number of glyphs:     "<<glyphs.size()<<"\n";
    std::cerr << "Font size:            "<<face->get_width()<<" x "<<face->get_height()<<"\n";
    std::cerr << "Image size:           "<<width<<" x "<<height<<"\n";
    std::cerr << "Image coverage:       "<<packer.get_coverage()<<"\n";
    std::cerr << "Glyphs per EM-square: "<<(face->get_width()*face->get_height()/(width*long(height)/glyphs.size()))<<"\n";


    Image::Ref img{Image::new_image(width, height, ColorSpace::get_Lum())};
    ImageWriter writer{img};
    writer.clear();
    for (const Glyph& g: glyphs) {
        face->load_glyph(g.index, true); // Request grid fitting
        face->set_target_origin(g.left, g.bottom);
        face->render_pixels_to(writer);
    }
    img->save(target_png);
    std::cout << "Saved: "<<target_png<<std::endl;

    {
        std::ofstream out{target_conf};
        out << face->get_family_name() << "\n";
        out << face->is_bold() << " " << face->is_italic() << " " << face->is_monospace() << "   ";
        out << face->get_width() << " " << face->get_height() << "   ";
        out << face->get_baseline_offset(false,  true) << " ";    // Grid fitting
        out << face->get_baseline_spacing(false, true) << "   ";  // Grid fitting
        out << face->get_baseline_offset(true,   true) << " ";    // Grid fitting
        out << face->get_baseline_spacing(true,  true) << "\n";   // Grid fitting
        Text::Table table{false};
        std::size_t n = glyphs.size();
        for (std::size_t j = 0; j < n; ++j) {
            const Glyph& g = glyphs[j];
            table.get_cell(j,  0).set_val(g.left);
            table.get_cell(j,  1).set_val(g.bottom);
            table.get_cell(j,  2).set_val(g.width);
            table.get_cell(j,  3).set_val(g.height);
            table.get_cell(j,  5).set_val(g.hori_bearing_x);
            table.get_cell(j,  6).set_val(g.hori_bearing_y);
            table.get_cell(j,  8).set_val(g.vert_bearing_x);
            table.get_cell(j,  9).set_val(g.vert_bearing_y);
            table.get_cell(j, 11).set_val(g.hori_advance);
            table.get_cell(j, 12).set_val(g.vert_advance);
            int m = g.code_points.size();
            if (m != 0) {
                std::ostringstream o;
                o << g.code_points[0];
                for (int i = 1; i < m; ++i)
                    o << " " << g.code_points[i];
                table.get_cell(j, 14).set_text(o.str());
            }
        }
        out << table.print();
    }
    std::cout << "Saved: "<<target_conf<<std::endl;
}
