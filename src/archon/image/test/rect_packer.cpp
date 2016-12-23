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


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace {

struct Box {
    Box(int w, int h):
        width{w},
        height{h}
    {
    }
    int width, height;
    int x,y; // Position in image
};

// Order boxes according to decreasing height first, then by decreasing width.
struct BoxHeightOrderCmp {
    bool operator()(int a, int b) const
    {
        if (ARCHON_LIKELY(boxes[a].height > boxes[b].height))
            return true;
        if (ARCHON_LIKELY(boxes[a].height < boxes[b].height))
            return false;
        return boxes[a].width > boxes[b].width;
    }
    BoxHeightOrderCmp(const std::vector<Box>& b):
        boxes{b}
    {
    }
    const std::vector<Box>& boxes;
};

} // unnamed namespace


int main(int argc, const char* argv[])
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

    std::vector<Box> boxes;
    {
        int w_1 = 3, h_1 = 3;
        int w_2 = opt_width - (w_1-1), h_2 = opt_height - (h_1-1);
        Random r;
        for (int i = 0; i < n; ++i) {
            boxes.push_back(Box(w_1 + std::floor(w_2*r.get_uniform()),
                                h_1 + std::floor(h_2*r.get_uniform())));
        }
    }

    // Sort according to decreasing height, then by decreasing width. The
    // secondary criteron might not have any significant effect.
    std::vector<int> box_order(n);
    generate(box_order.begin(), box_order.end(), make_inc_generator<int>());

    sort(box_order.begin(), box_order.end(), BoxHeightOrderCmp{boxes});

    long area = 0;
    int max_width = 0;
    for (const Box& box: boxes) {
        if (max_width < box.width)
            max_width = box.width;
        area += (box.height + opt_spacing) * long(box.width + opt_spacing);
    }

    int width = std::max<int>(std::sqrt(double(area)), max_width) + opt_spacing;
    RectanglePacker packer{width, -1, opt_spacing};
    for (int i = 0; i < n; ++i) {
        int box_index = box_order[i];
        Box& box = boxes[box_index];
        if (!packer.insert(box.width, box.height, box.x, box.y))
            throw std::runtime_error("Out of space in image");
    }

    int height = packer.get_height();

    std::cout << "Size: " << width << " x " << height << std::endl;
    std::cout << "Coverage: " << packer.get_coverage() << std::endl;

    ImageWriter img{width, height};
    Random random;
    for (const Box& box: boxes) {
        img.set_clip(box.x, box.y, box.width, box.height);
        img.set_foreground_color(PackedTRGB(0x1000000UL*random.get_uniform()));
        img.fill();
    }
    std::string out_file = "/tmp/archon_image_rect_packer.png";
    img.save(out_file);
    std::cout << "Result saved to: " << out_file << std::endl;
}
