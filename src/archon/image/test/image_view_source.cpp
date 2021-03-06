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

#include <cstdlib>
#include <iostream>

#include <archon/core/cxx.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/string.hpp>
#include <archon/core/text.hpp>
#include <archon/core/options.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/oper.hpp>
#include <archon/image/imageio.hpp>
#include <archon/image/writer.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace {

std::string opt_view = "ident";
std::string opt_palette = "";


bool check_view(std::string name, std::string opt, std::vector<int>& params)
{
    if (!Text::is_prefix(name, opt))
        return false;
    if (name.size() == opt.size())
        return true;
    if (opt[name.size()] != ',')
        throw std::invalid_argument("Syntax error in view params");
    std::istringstream i(opt.substr(name.size()+1));
    Text::SimpleTokenizer<char> tokenizer(i, ",", Text::SimpleTokenizer<char>::incl_empty);
    std::string s;
    while (tokenizer.generate(s))
        params.push_back(parse_value<int>(s));
    if (params.empty())
        throw std::invalid_argument("Syntax error in view params");
    return true;
}


Image::ConstRef get_palette()
{
    static Image::ConstRef palette;
    if (!palette) {
        if (!opt_palette.empty()) {
            palette = Image::load(opt_palette);
        }
        else {
            Image::Ref p = Image::new_image(256, 1);
            ImageWriter w(p);
            for (int i = 0; i < 256; ++i) {
                w.set_pos(i,0).put_pixel_rgb(frac_n_bit_int_to_float<int, float>(255-i, 8),
                                             frac_n_bit_int_to_float<int, float>(i,     8),
                                             frac_n_bit_int_to_float<int, float>(128,   8));
            }
            palette = p;
        }
    }
    return palette;
}

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    std::set_terminate(&cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "image/test/");

    CommandlineOptions opts;
    opts.add_help("Test application for image operator compositions as sources");
    opts.check_num_args();
    opts.add_param("v", "view", opt_view,
                   "Choose from:\n"
                   "ident\n"
//                   "cut,left,bottom,width,height[,horizontalrepeat,verticalrepeat]\n"
                   "flip,horizontal,vertical\n"
                   "diagflip,even,odd\n"
                   "rot,ninety,oneeighty\n"
                   "invert[,index]\n"
                   "channel,index[,preserve-alpha]\n"
                   "colormap");
    opts.add_param("p", "palette", opt_palette, "Path to file holding palette image");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    Image::Ref image = load_image(*make_stdin_stream(), "stream:in:std");
    Image::ConstRef view;

    std::vector<int> params;
    if (check_view("ident", opt_view, params)) {
        if (0 < params.size())
            throw std::invalid_argument("Wrong number of view params");
        view = image;
    }
/*
    else if (check_view("cut", opt_view, params)) {
        if (params.size() != 4 && params.size() != 6)
            throw std::invalid_argument("Wrong number of view params");
        if (params.size() == 4) {
            view = image->get_sub_view(params[0], params[1], params[2], params[3]);
        }
        else {
            view = image->get_sub_view(params[0], params[1], params[2], params[3], params[4], params[5]);
        }
    }
*/
    else if (check_view("flip", opt_view, params)) {
        if (params.size() != 2)
            throw std::invalid_argument("Wrong number of view params");
        view = Oper::flip(image, params[0], params[1]);
    }
    else if (check_view("diagflip", opt_view, params)) {
        if (params.size() != 2)
            throw std::invalid_argument("Wrong number of view params");
        view = Oper::flip_diag(image, params[0], params[1]);
    }
    else if (check_view("rot", opt_view, params)) {
        if (params.size() != 2)
            throw std::invalid_argument("Wrong number of view params");
        view = Oper::rotate(image, params[0], params[1]);
    }
    else if (check_view("invert", opt_view, params)) {
        if (params.size() != 0 && params.size() != 1)
            throw std::invalid_argument("Wrong number of view params");
        view = params.size() == 0 ? Oper::invert(image) : Oper::invert(image, params[0]);
    }
    else if (check_view("channel", opt_view, params)) {
        if (params.size() != 1 && params.size() != 2)
            throw std::invalid_argument("Wrong number of view params");
        view = params.size() == 1 ? Oper::pick_channel(image, params[0]) :
            Oper::pick_channel(image, params[0], params[1]);
    }
    else if (check_view("colormap", opt_view, params)) {
        if (params.size() != 0)
            throw std::invalid_argument("Wrong number of view params");
        view = Oper::color_map(image, get_palette());
    }
    else {
        throw std::invalid_argument("Unknown view '"+opt_view+"'");
    }

    save_image(view, *make_stdout_stream(), "stream:out:std", "png");
}
