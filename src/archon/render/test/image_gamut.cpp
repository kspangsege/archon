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
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>

#include <archon/core/build_config.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/conv_hull.hpp>
#include <archon/image/reader.hpp>
#include <archon/render/object.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::render;


namespace {

class TriangleSaver: public conv_hull::TriangleHandler {
public:
    void add_triangle(std::size_t a, std::size_t b, std::size_t c)
    {
        object.add_triangle(map(a), map(b), map(c));
    }

    std::size_t map(std::size_t i)
    {
        std::size_t& j = point_map[i];
        if (j == 0) {
            const Vec3& v = points[i];
            j = object.add_vertex(v[0], v[1], v[2]) + 1;
        }
        return j - 1;
    }

    TriangleSaver(const std::vector<Vec3>& p, Object& o):
        points(p),
        object(o),
        point_map(points.size())
    {
    }

private:
    const std::vector<Vec3>& points;
    Object& object;
    std::vector<std::size_t> point_map;
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);
    try_fix_preinstall_datadir(argv[0], "render/test/");

    CommandlineOptions opts;
    opts.add_help("Test application for the convex hull computation", "IMAGE");
    opts.check_num_args(0,1);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::string assets_dir = get_value_of(build_config_param_DataDir) + "render/test/";
    std::string in_file  = argc < 2 ? assets_dir+"alley_baggett.png" : argv[1];
    Image::ConstRef image = Image::load(in_file);

    std::vector<Vec3> points;

    {
        ImageReader reader(image);
        int width = reader.get_width(), height = reader.get_height();
        points.resize(height * std::size_t(width));
        std::unique_ptr<double[]> buf = std::make_unique<double[]>(width * 3);
        for (int i = 0; i < height; ++i) {
            reader.set_pos(0,i).get_block_rgb(buf.get(), width, 1);
            std::size_t offset = i * width;
            for(int j = 0; j < width; ++j) {
                double* b = buf.get() + j*3;
                points[offset + j].set(b[0], b[1], b[2]);
            }
        }
    }

    {
        sort(points.begin(), points.end());
        auto i = unique(points.begin(), points.end());
        points.erase(i, points.end());
    }

    std::cerr << "num_unique_points: " << points.size() << std::endl;

/*
    for (std::size_t i = 0; i < points.size(); ++i)
        std::cerr << "p" << i << ": " << points[i] << "\n";
*/

    Object object;
    {
        TriangleSaver triangles(points, object);
        conv_hull::compute(points, triangles);
    }

    std::ofstream out("/tmp/out.obj");
    object.save(out);
}
