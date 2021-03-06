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
///
/// Testing the convex hull computer.

#include <cmath>
#include <cstdlib>
#include <vector>
#include <iostream>

#include <archon/core/random.hpp>
#include <archon/core/string.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/conv_hull.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;

/*
Grid: 32
Number of points: 128
Random seed: 11684281426618421174
Fail depth: 11
*/

namespace {

class TrifanHandler: public conv_hull::TrifanHandler {
public:
    void add_vertex(size_t)
    {
    }

    void close_trifan()
    {
    }

    void close_trifan_set()
    {
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    CommandlineOptions opts;
    opts.add_help("Test application for the convex hull computer", "NUM_POINTS MAX_DEPTH");
    opts.check_num_args();
    if (int stop = opts.process(argc, argv))
        return (stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE);

    int num_points = parse_value<int>(argv[1]);
    int max_depth = parse_value<int>(argv[2]);

    TrifanHandler trifan_handler;
    std::vector<Vec3> points;
    unsigned long seed;
    try {
        for (;;) {
            points.clear();
            seed = 11684281426618421174UL; // Random().get_uint<unsigned long>();
            Random random(seed);
            for (int i=0; i<num_points; ++i) {
                points.push_back(Vec3(random.get_uint<unsigned>(31)/31.0-0.5,
                                      random.get_uint<unsigned>(31)/31.0-0.5,
                                      random.get_uint<unsigned>(31)/31.0-0.5));
            }
            conv_hull::compute(points, trifan_handler, max_depth);
        }
    }
    catch (...) {
        std::cerr << "Number of points: " << num_points << "\n";
        std::cerr << "Maximum depth: " << max_depth << "\n";
        std::cerr << "Random seed: " << seed << "\n";

        for (int i = 0; i < max_depth; ++i) {
            try {
                conv_hull::compute(points, trifan_handler, i+1);
            }
            catch (...) {
                std::cerr << "Fail depth: " << (i+1) << "\n";
                break;
            }
        }

        throw;
    }
}
