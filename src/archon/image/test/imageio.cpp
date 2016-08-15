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

#include <limits>
#include <iostream>
#include <iomanip>

#include <archon/core/text.hpp>
#include <archon/core/file.hpp>
#include <archon/image/imageio.hpp>
#include <archon/image/writer.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::util;
using namespace archon::Imaging;

struct Tracker: FileFormat::ProgressTracker
{
  void defined(BufferedImage::ConstRefArg i) throw()
  {
    cerr << "=================================================== Defined ===================================================" << endl;
    image = i;
  }

  void progress(double fraction) throw()
  {
    ostringstream o1, o2;
    o1 << fixed<<setprecision(0)<<setw(3)<<setfill('0')<<fraction*100<<"%";
    string s = o1.str();
    cerr << "=================================================== Progress "<<s<<"% ===================================================" << endl;
    // o2 <<"/tmp/img-"<<setw(3)<<setfill('0')<<++counter<<"-"<<s<<".png";
    // string path = o2.str();
    // image->save(path);
    // cout << "Saved: "<<path<< endl;
  }

  Tracker(): counter(0) {}

  Image::ConstRef image;
  int counter;
};

int main(int argc, const char *argv[]) throw()
{
  string in_file = argc < 2 ? File::dir_of(argv[0])+"../alley_baggett.png" : argv[1];

  Tracker tracker;
  Image::Ref image1 = ImageIO::load(in_file, "", &Logger::get_default_logger(), &tracker);

  // Might be a good idea to have Libjpeg convert YCbCr to RGB by default, since it can do it before conversion to integer representation.
//  Image::Ref image1 = Image::load("/home/kristian/persist/images/KimPossible/Kim_Possible_03.jpg");
//  Image::Ref image1 = Image::load();
//  Image::Ref image2 = Image::load("/home/kristian/persist/devel/local/archon/src/archon/image/test/dille2alpha.png");

  ImageWriter(image1).set_clip(10, 10, 80, 80).set_foreground_color(PackedTRGB(0x80FFFF00)).fill();

  cout << image1->get_color_space()->get_mnemonic(image1->has_alpha_channel()) << endl;

//  image1->put_image(image2->get_sub_view(-50, -50, 100, 100, 2, 2), 100, 100, 0, 0, 300, 300);
  string const out_file = "/tmp/archon_image_imageio.png";
  image1->save(out_file);
  cout << "Image saved to: " << out_file << endl;
  cout << get_word_type_name(image1->get_word_type()) << endl;
  return 0;
}
