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

#include <string>
#include <iostream>

#include <archon/core/random.hpp>
#include <archon/core/file.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/writer.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;
using namespace Archon::Imaging;

int main(int argc, const char *argv[]) throw()
{
  string in_file  = argc < 2 ? File::dir_of(argv[0])+"../alley_baggett.png" : argv[1];
  string out_file = argc < 3 ? "/tmp/archon_image_writer.png" : argv[2];

  ColorSpace::ConstRef color_space = ColorSpace::get_YCbCr();
  bool has_alpha = true;

  double buffer[] =
  {
    0.0, 0.0, 1.0, 0.0,   0.0, 0.0, 1.0, 1.0,
    0.1, 1.0, 0.9, 0.1,   0.1, 1.0, 0.9, 0.9,
    0.2, 0.0, 0.8, 0.2,   0.2, 0.0, 0.8, 0.8,
    0.3, 1.0, 0.7, 0.3,   0.3, 1.0, 0.7, 0.7,
    0.4, 0.0, 0.6, 0.4,   0.4, 0.0, 0.6, 0.6,
    0.5, 1.0, 0.5, 0.5,   0.5, 1.0, 0.5, 0.5,
    0.6, 0.0, 0.4, 0.6,   0.6, 0.0, 0.4, 0.4,
    0.7, 1.0, 0.3, 0.7,   0.7, 1.0, 0.3, 0.3,
    0.8, 0.0, 0.2, 0.8,   0.8, 0.0, 0.2, 0.2,
    0.9, 1.0, 0.1, 0.9,   0.9, 1.0, 0.1, 0.1,
    1.0, 0.0, 0.0, 1.0,   1.0, 0.0, 0.0, 0.0
  };


  // Write blocks of pixels
  ImageWriter w(in_file);
  w.set_background_color(PackedTRGB(0x8F8080)).set_clip(1,1,24,28).set_pos(10,10);
  w.put_block(buffer, 2, 11, color_space, has_alpha);
  w.set_pos(14,12).enable_blending();
  w.put_block(buffer, 2, 11, color_space, has_alpha);

  Random random;

  // Write single pixels
  {
    w.set_clip(50, 50, 380, 500).enable_blending(true);
    int x0 = 280, y0 = 280;
    for(int i=0;i<100000; ++i)
    {
      double a = M_PI*13/8 * random.get_uniform(), r = 220 * random.get_uniform();
      w.set_pos(x0 + r*cos(a), y0 + r*sin(a)).put_pixel(PackedTRGB(frac_float_to_n_bit_int<double, unsigned long>(random.get_uniform(), 32)));
    }
  }

  w.set_pos_align(0.5, 0.5);

  // Write entire images
  {
    vector<string> letters;
    letters.push_back(File::dir_of(argv[0])+"../Q-small.png");
    letters.push_back(File::dir_of(argv[0])+"../R-small.png");
    letters.push_back(File::dir_of(argv[0])+"../S-small.png");
    w.set_clip().enable_blending();
    double x = 0.5, y = 0.5, r = 0.4;
    for(int i=0;i<24; ++i)
    {
      double a = 2*M_PI * random.get_uniform();
      w.set_rel_pos(x + r*cos(a), y + r*sin(a)).put_image(letters[letters.size()*random.get_uniform()]);
    }
  }

  w.set_rel_pos(0.5, 0.5).put_image(File::dir_of(argv[0])+"../16bit_gray_alpha.png");

  w.save(out_file);
  cout << "Image saved to: " << out_file << endl;

  return 0;
}
