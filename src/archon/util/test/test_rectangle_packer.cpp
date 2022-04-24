// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cstddef>
#include <vector>
#include <random>

#include <archon/core/features.h>
#include <archon/core/random.hpp>
#include <archon/check.hpp>
#include <archon/util/rectangle_packer.hpp>


using namespace archon;


ARCHON_TEST(Util_RectanglePacker_Randomize)
{
    std::mt19937_64 random(test_context.seed_seq());

    struct Rect {
        int x, y;
        int width, height;
    };
    std::vector<Rect> rects;

    // Generate set of randomly sized rectangles
    std::size_t num_rects = 512;
    for (std::size_t i = 0; i < num_rects; ++i) {
        Rect rect = {};
        rect.width  = core::rand_int(random, 1, 10);
        rect.height = core::rand_int(random, 1, 10);
        rects.push_back(rect);
    }

    // Pack into bin of suggested width
    util::RectanglePacker packer;
    for (const Rect& rect : rects)
        packer.add_rect(rect.width, rect.height);
    int max_width = packer.suggest_bin_width();
    if (!ARCHON_CHECK(packer.pack(max_width)))
        return;
    int bin_width  = packer.get_utilized_width();
    ARCHON_CHECK_LESS_EQUAL(bin_width, max_width);
    int bin_height = packer.get_utilized_height();
    for (std::size_t i = 0; i < num_rects; ++i) {
        Rect& rect = rects[i];
        packer.get_rect_pos(i, rect.x, rect.y);
    }

    // Verify that all rectangles are confined to the bin
    for (const Rect& rect : rects) {
        bool horz_confined = (ARCHON_CHECK_GREATER_EQUAL(rect.x, 0) &&
                              ARCHON_CHECK_LESS_EQUAL(rect.x, bin_width) &&
                              ARCHON_CHECK_LESS_EQUAL(rect.width, bin_width - rect.x));
        bool vert_confined = (ARCHON_CHECK_GREATER_EQUAL(rect.y, 0) &&
                              ARCHON_CHECK_LESS_EQUAL(rect.y, bin_height) &&
                              ARCHON_CHECK_LESS_EQUAL(rect.height, bin_height - rect.y));
        bool good = (horz_confined && vert_confined);
        if (!good)
            break;
    }

    // Verify that no two rectangles overlap
    for (std::size_t i = 0; i < num_rects; ++i) {
        for (std::size_t j = 0; j < num_rects; ++j) {
            if (ARCHON_UNLIKELY(j == i))
                continue;
            const Rect& a = rects[i];
            const Rect& b = rects[j];
            bool horz_overlap = (a.x <= b.x ? a.width  > b.x - a.x : b.width  > a.x - b.x);
            bool vert_overlap = (a.y <= b.y ? a.height > b.y - a.y : b.height > a.y - b.y);
            bool overlap = (horz_overlap && vert_overlap);
            if (!ARCHON_CHECK(!overlap))
                return;
        }
    }
}
