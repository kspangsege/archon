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

#include <limits>
#include <iostream>
#include <iomanip>

#include <archon/core/cxx.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/text.hpp>
#include <archon/core/file.hpp>
#include <archon/image/imageio.hpp>
#include <archon/image/writer.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;

class Tracker: public FileFormat::ProgressTracker {
public:
    void defined(BufferedImage::ConstRefArg i) throw()
    {
        std::cerr << "=================================================== Defined ===================================================\n";
        image = i;
    }

    void progress(double fraction) throw()
    {
        std::ostringstream o1, o2;
        o1 << std::fixed<<std::setprecision(0)<<std::setw(3)<<std::setfill('0')<<fraction*100<<"%";
        std::string s = o1.str();
        std::cerr << "=================================================== Progress "<<s<<"% ===================================================\n";
        // o2 <<"/tmp/img-"<<std::setw(3)<<std::setfill('0')<<++counter<<"-"<<s<<".png";
        // std::string path = o2.str();
        // image->save(path);
        // std::cout << "Saved: "<<path<< std::endl;
    }

    Image::ConstRef image;
    int counter = 0;
};


int main(int argc, const char *argv[]) throw()
{
    std::set_terminate(&cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "image/test/");

    std::string assets_dir = get_value_of(build_config_param_DataDir) + "image/test/";

    std::string in_file = (argc < 2 ? assets_dir+"alley_baggett.png" : argv[1]);

    Tracker tracker;
    Image::Ref image_1 = load_image(in_file, "", &Logger::get_default_logger(), &tracker);

    // Might be a good idea to have Libjpeg convert YCbCr to RGB by default, since it can do it before conversion to integer representation.
//    Image::Ref image_1 = Image::load(assets_dir+"kim_possible_3.jpg");
//    Image::Ref image_1 = Image::load();
//    Image::Ref image_2 = Image::load(assets_dir+"dille2alpha.png");

    ImageWriter(image_1).set_clip(10, 10, 80, 80).set_foreground_color(PackedTRGB(0x80FFFF00)).fill();

    std::cout << image_1->get_color_space()->get_mnemonic(image_1->has_alpha_channel()) << std::endl;

//    image_1->put_image(image2->get_sub_view(-50, -50, 100, 100, 2, 2), 100, 100, 0, 0, 300, 300);
    std::string out_file = "/tmp/archon_image_imageio.png";
    image_1->save(out_file);
    std::cout << "Image saved to: " << out_file << std::endl;
    std::cout << get_word_type_name(image_1->get_word_type()) << std::endl;
}
