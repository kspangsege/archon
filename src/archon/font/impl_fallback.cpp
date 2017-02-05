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

#include <locale>
#include <vector>
#include <map>
#include <fstream>

#include <archon/core/functions.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/text.hpp>
#include <archon/font/loader.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::image;
using namespace archon::font;


namespace {

class Glyph {
public:
    // Position and size in image, origin is at lower left corner of image.
    int left, bottom, width, height;

    // Position of bearing point of a left-to-right layout realtive to the lower
    // left corner of the bounding box of the glyph. The x-coordinate increases
    // towards the right and the y-coordinate increases upwards.
    int hori_bearing_x, hori_bearing_y;

    // Position of bearing point of a bottom-to-top layout realtive to the lower
    // left corner of the bounding box of the glyph. The x-coordinate increases
    // towards the right and the y-coordinate increases upwards.
    int vert_bearing_x, vert_bearing_y;

    // The glyph advance for horizontal and vertical layouts
    // respecively. Neither can ever be negative.
    int hori_advance, vert_advance;

    Glyph()
    {
    }

    Glyph(int l, int b, int w, int h,
          int hbx, int hby, int vbx, int vby, int ha, int va):
        left{l},
        bottom{b},
        width{w},
        height{h},
        hori_bearing_x{hbx},
        hori_bearing_y{hby},
        vert_bearing_x{vbx},
        vert_bearing_y{vby},
        hori_advance{ha},
        vert_advance{va}
    {
    }
};



class LoaderImpl: public FontLoader {
public:
    std::unique_ptr<FontFace> load_default_face(double, double) const;

    std::unique_ptr<FontFace> load_face(std::string file, int index, double, double) const
    {
        if (file != conf_file || index != 0)
            throw BadFontFileException("Unacceptable fallback font file");
        return load_default_face(0,0);
    }

    void load_face_info(std::string file, int index, FaceInfo& info) const
    {
        if (file != conf_file || index != 0)
            throw BadFontFileException("Unacceptable fallback font file");
        info.fixed_sizes.reserve(1);
        info.family    = family_name;
        info.bold      = bold;
        info.italic    = italic;
        info.monospace = monospace;
        info.scalable  = false;
        info.fixed_sizes.push_back(std::make_pair(render_width, render_height));
    }

    int check_file(std::string path) const
    {
        return (path == conf_file ? 1 : 0);
    }

    std::string get_default_font_file() const
    {
        return conf_file;
    }

    int get_default_face_index() const
    {
        return 0;
    }

    LoaderImpl(const std::string& resource_dir):
        conf_file{resource_dir+"fallback-font.conf"},
        glyph_image_reader{resource_dir+"fallback-font.png"}
    {
        std::wifstream in{conf_file};
        if (!in)
            throw std::runtime_error("Unable to open '"+conf_file+"' for reading");
        Text::LineReader<wchar_t> line_reader{in, std::locale{""}};
        std::wstring line;
        unsigned long line_num = 0;
        while (line_reader.generate(line)) {
            ++line_num;

            if (line_num == 1) {
                Text::WideTrimmer trimmer{std::locale{""}};
                family_name = env_encode(trimmer.trim(line));
                continue;
            }

            // FIXME: Adding space to work around bug in libc++'s implementation of std::ws
            std::wistringstream i{line + L" "};                              
            i.imbue(std::locale::classic());

            if (line_num == 2) {
                i >> bold >> italic >> monospace;
                i >> render_width >> render_height;
                i >> hori_baseline_offset >> hori_baseline_spacing;
                i >> vert_baseline_offset >> vert_baseline_spacing >> std::ws;
                if (i.bad() || i.fail() || !i.eof())
                    throw std::runtime_error("Failed to parse second line of '"+conf_file+"'");
                continue;
            }

            Glyph g;
            i >> g.left >> g.bottom >> g.width >> g.height;
            i >> g.hori_bearing_x >> g.hori_bearing_y;
            i >> g.vert_bearing_x >> g.vert_bearing_y;
            i >> g.hori_advance >> g.vert_advance;
            int glyph_index = glyphs.size();
            if (glyph_index == 0) {
                i >> std::ws;
                if (!i.eof()) {
                    throw std::runtime_error("Garbage after replacement/first glyph "
                                             "at '"+conf_file+":"+Text::print(line_num)+"'");
                }
            }
            else {
                unsigned long code_point;
                do {
                    i >> code_point >> std::ws;
                    if (i.bad() || i.fail()) {
                        throw std::runtime_error("Failed to parse line "
                                                 "at '"+conf_file+":"+Text::print(line_num)+"'");
                    }
                    if (!char_map.insert(std::make_pair(code_point, glyph_index)).second) {
                        throw std::runtime_error("Multiple glyphs for code point "+
                                                 Text::print(code_point));
                    }
                }
                while (!i.eof());
            }
            glyphs.push_back(g);
        }
        if (glyphs.size() == 0)
            throw std::runtime_error("Found no glyphs in '"+conf_file+"'");
    }


    std::string family_name;
    bool bold, italic, monospace;
    std::string conf_file;
    mutable ImageReader glyph_image_reader;
    double render_width, render_height;
    int hori_baseline_offset, hori_baseline_spacing;
    int vert_baseline_offset, vert_baseline_spacing;

    using CharMap = std::map<wchar_t, int>;
    CharMap char_map;

    using Glyphs = std::vector<Glyph>;
    Glyphs glyphs;

    const std::weak_ptr<LoaderImpl> weak_self;
};



class FaceImpl: public FontFace {
public:
    std::string get_family_name() const override
    {
        return loader->family_name;
    }

    bool is_bold() const override
    {
        return loader->bold;
    }

    bool is_italic() const override
    {
        return loader->italic;
    }

    bool is_monospace() const override
    {
        return loader->monospace;
    }

    bool is_scalable() const override
    {
        return false;
    }

    int get_num_fixed_sizes() const override
    {
        return 1;
    }

    Vec2 get_fixed_size(int i) const override
    {
        if (i != 0)
            throw std::out_of_range{"fixed_size_index"};
        return Vec2{loader->render_width, loader->render_height};
    }

    void set_fixed_size(int i) override
    {
        if (i != 0)
            throw std::out_of_range{"fixed_size_index"};
    }

    void set_scaled_size(double, double) override
    {
        throw std::logic_error{"Fallback font loader cannot scale glyphs"};
    }

    void set_approx_size(double, double) override
    {
        // No-op since there is only one size in the first place
    }

    double get_width() const override
    {
        return loader->render_width;
    }

    double get_height() const override
    {
        return loader->render_height;
    }

    double get_baseline_spacing(bool vertical, bool) const override
    {
        return (vertical ? loader->vert_baseline_spacing : loader->hori_baseline_spacing);
    };

    double get_baseline_offset(bool vertical, bool) const override
    {
        return (vertical ? loader->vert_baseline_offset : loader->hori_baseline_offset);
    };

    int get_num_glyphs() const override
    {
        return int(loader->glyphs.size());
    }

    int find_glyph(wchar_t c) const override
    {
        LoaderImpl::CharMap::const_iterator i = loader->char_map.find(c);
        return (i == loader->char_map.end() ? 0 : i->second);
    }

    double get_kerning(int, int, bool, bool) const override
    {
        return 0;
    }

    void load_glyph(int i, bool) override
    {
        if (i < 0 || int(loader->glyphs.size()) <= i)
            throw std::out_of_range{"glyph_index"};
        glyph = &loader->glyphs[i];
        glyph_translation = Vec2::zero();
    }

    double get_glyph_advance(bool vertical) const override
    {
        return (vertical ? glyph->vert_advance : glyph->hori_advance);
    }

    Vec2 get_glyph_bearing(bool vertical) const override
    {
        return (vertical ?
                Vec2{double(glyph->vert_bearing_x), double(glyph->vert_bearing_y)} :
                Vec2{double(glyph->hori_bearing_x), double(glyph->hori_bearing_y)});
    }

    Vec2 get_glyph_size() const override
    {
        return Vec2{double(glyph->width), double(glyph->height)};
    }

    void translate_glyph(Vec2 v) override
    {
        glyph_translation += v;
    }

    void get_glyph_pixel_box(int& left, int& right, int& bottom, int& top) const override
    {
        left   = std::round(glyph_translation[0]);
        bottom = std::round(glyph_translation[1]);
        right = left   + glyph->width;
        top   = bottom + glyph->height;
    }

    void set_target_origin(int x, int y) override
    {
        target_origin_x = x;
        target_origin_y = y;
    }

    void render_pixels_to(ImageWriter& image_writer) const override
    {
        loader->glyph_image_reader.set_pos(glyph->left, glyph->bottom);
        image_writer.set_pos(target_origin_x + std::round(glyph_translation[0]),
                             target_origin_y + std::round(glyph_translation[1]));
        image_writer.put_image(loader->glyph_image_reader, glyph->width, glyph->height);
    }

    FaceImpl(const LoaderImpl* l):
        loader{l->weak_self},
        glyph{&loader->glyphs[0]}
    {
    }

    const std::shared_ptr<const LoaderImpl> loader;

    int target_origin_x = 0, target_origin_y = 0;
    Vec2 glyph_translation = Vec2::zero();
    const Glyph* glyph;
};



std::unique_ptr<FontFace> LoaderImpl::load_default_face(double, double) const
{
    return std::make_unique<FaceImpl>(this); // Throws
}

} // unnamed namespace


namespace archon {
namespace font {

std::shared_ptr<FontLoader> new_font_loader(const std::string& resource_dir)
{
    std::shared_ptr<LoaderImpl> l = std::make_shared<LoaderImpl>(resource_dir);
    const_cast<std::weak_ptr<LoaderImpl> &>(l->weak_self) = l;
    return l;
}

} // namespace font
} // namespace archon
