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


#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/format.hpp>
#include <archon/check.hpp>
#include <archon/util/pixel_size.hpp>
#include <archon/util/pixel_pos.hpp>
#include <archon/util/pixel_box.hpp>


using namespace archon;
namespace pixel = util::pixel;

namespace {


template<class F> void for_each_pos_in(const pixel::Box& box, F&& func)
{
    for (int y = 0; y <= box.size.height; ++y) {
        for (int x = 0; x <= box.size.width; ++x)
            func(pixel::Pos(box.pos.x + x, box.pos.y + y));
    }
}


template<class F> void for_each_box_in(const pixel::Box& box, F&& func)
{
    for_each_pos_in(box, [&](pixel::Pos pos) {
        pixel::Size diff = pos - box.pos;
        for (int h = 0; h <= box.size.height - diff.height; ++h) {
            for (int w = 0; w <= box.size.width - diff.width; ++w)
                func(pixel::Box(pos, { w, h }));
        }
    });
}


template<class F> void for_each_box_between(const pixel::Box& box_1, const pixel::Box& box_2, int min_size, F&& func)
{
    for_each_pos_in(box_1, [&](pixel::Pos pos_1) {
        for_each_pos_in(box_2, [&](pixel::Pos pos_2) {
            pixel::Size size = pos_2 - pos_1;
            if (size.width >= min_size && size.height >= min_size)
                func(pixel::Box(pos_1, size));
        });
    });
}


} // unnamed namespace



ARCHON_TEST(Util_Pixel_Box_ContainedIn)
{
    auto test_not_contained_in_fixed = [](check::TestContext& parent_test_context, int fixed_size,
                                          const pixel::Box& box) {
        ARCHON_TEST_TRAIL(parent_test_context, box);
        pixel::Box fixed = pixel::Box({2, 2}, fixed_size);
        ARCHON_CHECK_NOT(box.contained_in(fixed));
    };

    // Extend to the left of the fixed box, fixed box is 0x0
    for_each_box_between({{ 0, 0 }, { 1, 4 }}, {{ 0, 0 }, { 4, 4 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 0, box);
    });

    // Extend to the right of the fixed box, fixed box is 0x0
    for_each_box_between({{ 0, 0 }, { 4, 4 }}, {{ 3, 0 }, { 1, 4 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 0, box);
    });

    // Extend above fixed box, fixed box is 0x0
    for_each_box_between({{ 0, 0 }, { 4, 1 }}, {{ 0, 0 }, { 4, 4 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 0, box);
    });

    // Extend below fixed box, fixed box is 0x0
    for_each_box_between({{ 0, 0 }, { 4, 4 }}, {{ 0, 3 }, { 4, 1 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 0, box);
    });

    // Extend to the left of the fixed box, fixed box is 3x3
    for_each_box_between({{ 0, 0 }, { 1, 7 }}, {{ 0, 0 }, { 7, 7 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 3, box);
    });

    // Extend to the right of the fixed box, fixed box is 3x3
    for_each_box_between({{ 0, 0 }, { 7, 7 }}, {{ 6, 0 }, { 1, 7 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 3, box);
    });

    // Extend above fixed box, fixed box is 3x3
    for_each_box_between({{ 0, 0 }, { 7, 1 }}, {{ 0, 0 }, { 7, 7 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 3, box);
    });

    // Extend below fixed box, fixed box is 3x3
    for_each_box_between({{ 0, 0 }, { 7, 7 }}, {{ 0, 6 }, { 7, 1 }}, 0, [&](const pixel::Box& box) {
        test_not_contained_in_fixed(test_context, 3, box);
    });

    auto test_contained_in_fixed = [](check::TestContext& parent_test_context, int fixed_size,
                                      const pixel::Box& box) {
        ARCHON_TEST_TRAIL(parent_test_context, box);
        pixel::Box fixed = pixel::Box({2, 2}, fixed_size);
        ARCHON_CHECK(box.contained_in(fixed));
    };

    for_each_box_in(pixel::Box({ 2, 2 }, 0), [&](const pixel::Box& box) {
        test_contained_in_fixed(test_context, 0, box);
    });

    for_each_box_in(pixel::Box({ 2, 2 }, 3), [&](const pixel::Box& box) {
        test_contained_in_fixed(test_context, 3, box);
    });

    auto test_not_fixed_contained_in = [](check::TestContext& parent_test_context, int fixed_size,
                                          const pixel::Box& box) {
        ARCHON_TEST_TRAIL(parent_test_context, box);
        pixel::Box fixed = pixel::Box({ 2, 2 }, fixed_size);
        ARCHON_CHECK_NOT(fixed.contained_in(box));
    };

    // Everything to the left of fixed box with gap, fixed box is 0x0
    for_each_box_in({{ 0, 0 }, { 1, 4 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 0, box);
    });

    // Everything to the right of fixed box with gap, fixed box is 0x0
    for_each_box_in({{ 3, 0 }, { 1, 4 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 0, box);
    });

    // Everything above fixed box with gap, fixed box is 0x0
    for_each_box_in({{ 0, 0 }, { 4, 1 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 0, box);
    });

    // Everything below fixed box with gap, fixed box is 0x0
    for_each_box_in({{ 0, 3 }, { 4, 1 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 0, box);
    });

    // Everything to the left of rightmost pixel of fixed box, fixed box is 3x3
    for_each_box_in({{ 0, 0 }, { 4, 7 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 3, box);
    });

    // Everything to the right of leftmost pixel of fixed box, fixed box is 3x3
    for_each_box_in({{ 3, 0 }, { 4, 7 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 3, box);
    });

    // Everything above bottom-most pixel of fixed box, fixed box is 3x3
    for_each_box_in({{ 0, 0 }, { 7, 4 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 3, box);
    });

    // Everything below top-most pixel of fixed box, fixed box is 3x3
    for_each_box_in({{ 0, 3 }, { 7, 4 }}, [&](const pixel::Box& box) {
        test_not_fixed_contained_in(test_context, 3, box);
    });

    auto test_fixed_contained_in = [](check::TestContext& parent_test_context, int fixed_size,
                                      const pixel::Box& box) {
        ARCHON_TEST_TRAIL(parent_test_context, box);
        pixel::Box fixed = pixel::Box({ 2, 2 }, fixed_size);
        ARCHON_CHECK(fixed.contained_in(box));
    };

    for_each_box_between({{ 0, 0 }, { 2, 2 }}, {{ 2, 2 }, { 2, 2 }}, 0, [&](const pixel::Box& box) {
        test_fixed_contained_in(test_context, 0, box);
    });

    for_each_box_between({{ 0, 0 }, { 2, 2 }}, {{ 5, 5 }, { 2, 2 }}, 0, [&](const pixel::Box& box) {
        test_fixed_contained_in(test_context, 3, box);
    });
}


ARCHON_TEST(Util_Pixel_Box_Clip)
{
    auto test_empty_overlap = [](check::TestContext& parent_test_context, int fixed_size, const pixel::Box& box) {
        ARCHON_TEST_TRAIL(parent_test_context, box);
        pixel::Box fixed = pixel::Box({2, 2}, fixed_size);
        pixel::Box box_2 = box;
        if (ARCHON_LIKELY(ARCHON_CHECK_NOT(fixed.clip(box_2))))
            ARCHON_CHECK_EQUAL(box_2, box);
        pixel::Box box_3 = fixed;
        if (ARCHON_LIKELY(ARCHON_CHECK_NOT(box.clip(box_3))))
            ARCHON_CHECK_EQUAL(box_3, fixed);
    };

    for_each_box_in({{ 0, 0 }, { 4, 4 }}, [&](const pixel::Box& box) {
        test_empty_overlap(test_context, 0, box);
    });

    // Everything to the left
    for_each_box_in({{ 0, 0 }, { 2, 7 }}, [&](const pixel::Box& box) {
        test_empty_overlap(test_context, 3, box);
    });

    // Everything to the right
    for_each_box_in({{ 5, 0 }, { 2, 7 }}, [&](const pixel::Box& box) {
        test_empty_overlap(test_context, 3, box);
    });

    // Everything above
    for_each_box_in({{ 0, 0 }, { 7, 2 }}, [&](const pixel::Box& box) {
        test_empty_overlap(test_context, 3, box);
    });

    // Everything below
    for_each_box_in({{ 0, 5 }, { 7, 2 }}, [&](const pixel::Box& box) {
        test_empty_overlap(test_context, 3, box);
    });

    auto test_nonempty_overlap = [](check::TestContext& parent_test_context, int fixed_size, const pixel::Box& box,
                                    const pixel::Box& expect) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s/%s", box, expect));
        pixel::Box fixed = pixel::Box({2, 2}, fixed_size);
        ARCHON_ASSERT(expect.contained_in(fixed));
        ARCHON_ASSERT(expect.contained_in(box));
        pixel::Box box_2 = box;
        if (ARCHON_LIKELY(ARCHON_CHECK(fixed.clip(box_2))))
            ARCHON_CHECK_EQUAL(box_2, expect);
        pixel::Box box_3 = fixed;
        if (ARCHON_LIKELY(ARCHON_CHECK(box.clip(box_3))))
            ARCHON_CHECK_EQUAL(box_3, expect);
    };

    for_each_box_between({{ 0, 0 }, { 4, 4 }}, {{ 3, 3 }, { 4, 4 }}, 1, [&](const pixel::Box& box) {
        int x = std::max(box.pos.x, 2);
        int y = std::max(box.pos.y, 2);
        int w = std::min(box.pos.x + box.size.width,  5) - x;
        int h = std::min(box.pos.y + box.size.height, 5) - y;
        test_nonempty_overlap(test_context, 3, box, {{ x, y }, { w, h }});
    });
}
