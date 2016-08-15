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
/// Testing the rectangle packing utility

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <vector>
#include <iomanip>
#include <iostream>

#include <archon/core/generate.hpp>
#include <archon/core/random.hpp>
#include <archon/core/file.hpp>
#include <archon/core/options.hpp>
#include <archon/util/rect_packer.hpp>
#include <archon/image/writer.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Util;
using namespace archon::Imaging;


namespace {

struct Box {
    Box(int w, int h):
        width(w),
        height(h)
    {
    }
    int width, height;
    int x,y; // Position in image
};

struct BoxHeightOrderCmp {
    bool operator()(int a, int b) const
    {
        return boxes[b].height < boxes[a].height;
    }
    BoxHeightOrderCmp(const vector<Box>& g):
        boxes(g)
    {
    }
    const vector<Box>& boxes;
};

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    int opt_num     = 400;
    int opt_width   =  40;
    int opt_height  =  40;
    int opt_spacing =   0;

    CommandlineOptions opts;
    opts.add_help("Test the rectangle packing utility");
    opts.check_num_args();
    opts.add_param("n", "num", opt_num, "Number of boxes to generate");
    opts.add_param("W", "width", opt_width, "Maximum width of a box");
    opts.add_param("H", "height", opt_height, "Maximum height of a box");
    opts.add_param("S", "spacing", opt_spacing, "Minimum spacing between boxes");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    int n = opt_num;

    typedef vector<Box> Boxes;
    Boxes boxes;
    {
        int w1 = 3, h1 = 3;
        int w2 = opt_width - (w1-1), h2 = opt_height - (h1-1);
        Random r;
        for (int i = 0; i < n; ++i)
            boxes.push_back(Box(w1 + floor(w2*r.get_uniform()), h1 + floor(h2*r.get_uniform())));
    }

    // Sort according to height
    vector<int> box_order(n);
    generate(box_order.begin(), box_order.end(), make_inc_generator<int>());

    sort(box_order.begin(), box_order.end(), BoxHeightOrderCmp(boxes));

    long area = 0;
    int max_width = 0;
    for (Boxes::const_iterator g = boxes.begin(); g != boxes.end(); ++g) {
        if (max_width < g->width)
            max_width = g->width;
        area += (g->height + opt_spacing) * long(g->width + opt_spacing);
    }

    int width = max<int>(sqrt(double(area)), max_width) + opt_spacing;
    RectanglePacker packer(width, -1, opt_spacing);
    for (int i=0; i<n; ++i) {
        int box_index = box_order[i];
        Box& g = boxes[box_index];
        if (!packer.insert(g.width, g.height, g.x, g.y))
            throw runtime_error("Out of space in image");
    }

    int height = packer.get_height();

    cerr << "Size: " << width << " x " << height << endl;
    cerr << "Coverage: " << packer.get_coverage() << endl;

    ImageWriter img(width, height);
    Random random;
    for (Boxes::const_iterator g = boxes.begin(); g != boxes.end(); ++g) {
        img.set_clip(g->x, g->y, g->width, g->height);
        img.set_foreground_color(PackedTRGB(0x1000000UL*random.get_uniform()));
        img.fill();
    }
    string out_file = "/tmp/archon_image_rect_packer.png";
    img.save(out_file);
    cout << "Result saved to: " << out_file << endl;
}
