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

#include <algorithm>
#include <vector>
#include <fstream>
#include                                   <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/file.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/conv_hull.hpp>
#include <archon/image/reader.hpp>
#include <archon/render/object.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::render;


namespace
{
  struct TriangleSaver: conv_hull::TriangleHandler
  {
    void add_triangle(std::size_t a, std::size_t b, std::size_t c)
    {
      object.add_triangle(map(a), map(b), map(c));
    }

    size_t map(size_t i)
    {
      size_t &j = point_map[i];
      if(j == 0)
      {
        Vec3 const &v = points[i];
        j = object.add_vertex(v[0], v[1], v[2]) + 1;
      }
      return j - 1;
    }

    TriangleSaver(vector<Vec3> const &p, Object &o):
      points(p), object(o), point_map(points.size()) {}

  private:
    vector<Vec3> const &points;
    Object &object;
    vector<size_t> point_map;
  };
}


int main(int argc, char const *argv[]) throw()
{
  set_terminate(&cxx::terminate_handler);
    
  CommandlineOptions opts;
  opts.add_help("Test application for the convex hull computation", "IMAGE");
  opts.check_num_args(0,1);
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  string in_file  = argc < 2 ? File::dir_of(argv[0])+"../alley_baggett.png" : argv[1];
  Image::ConstRef image = Image::load(in_file);

  vector<Vec3> points;

  {
    ImageReader reader(image);
    int const width = reader.get_width(), height = reader.get_height();
    points.resize(height * size_t(width));
    Array<double> const buf(width * 3);
    for(int i=0; i<height; ++i)
    {
      reader.set_pos(0,i).get_block_rgb(buf.get(), width, 1);
      size_t const offset = i * width;
      for(int j=0; j<width; ++j)
      {
        double *const b = buf.get() + j*3;
        points[offset + j].set(b[0], b[1], b[2]);
      }
    }
  }

  {
    sort(points.begin(), points.end());
    vector<Vec3>::iterator const i = unique(points.begin(), points.end());
    points.erase(i, points.end());
  }

  cerr << "num_unique_points: " << points.size() << endl;

/*
  for(size_t i=0; i<points.size(); ++i)
    cerr << "p" << i << ": " << points[i] << "\n";
*/


  Object object;
  {
    TriangleSaver triangles(points, object);
    conv_hull::compute(points, triangles);
  }

  ofstream out("/tmp/out.obj");
  object.save(out);

  return 0;
}
