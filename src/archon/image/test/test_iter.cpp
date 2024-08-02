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
#include <memory>
#include <algorithm>

#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/check.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/test/box_utils.hpp>


using namespace archon;
namespace test = image::test;


ARCHON_TEST(Image_Iter_RepeatLeft)
{
    image::Size inner_size = 9;
    image::Size margin = 1;
    image::Size outer_size = inner_size + 2 * margin;
    image::Box inner_box = { image::Pos() + margin, inner_size };
    using comp_type = std::size_t;
    int num_channels = 2;
    std::size_t buffer_size = 1;
    core::int_mul(buffer_size, num_channels);
    core::int_mul(buffer_size, outer_size.width);
    core::int_mul(buffer_size, outer_size.height);
    std::unique_ptr<comp_type[]> buffer_1 = std::make_unique<comp_type[]>(buffer_size);
    std::unique_ptr<comp_type[]> buffer_2 = std::make_unique<comp_type[]>(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i)
        buffer_1[i] = i;
    std::ptrdiff_t horz_stride = num_channels;
    std::ptrdiff_t vert_stride = outer_size.width * num_channels;
    image::Iter iter_1 = { buffer_1.get(), horz_stride, vert_stride };
    image::Iter iter_2 = { buffer_2.get(), horz_stride, vert_stride };
    auto test = [&](check::TestContext& parent_test_context, const image::Box& pattern) {
        ARCHON_TEST_TRAIL(parent_test_context, pattern);
        std::copy_n(buffer_1.get(), buffer_size, buffer_2.get());
        int size = pattern.pos.x - inner_box.pos.x;
        iter_2.repeat_left(pattern, size, num_channels);
        image::Box affected_area = { splice(inner_box.pos, pattern.pos), image::Size(size, pattern.size.height) };
        for (int y = 0; y < outer_size.height; ++y) {
            for (int x = 0; x < outer_size.width; ++x) {
                image::Pos pos_1 = { x, y };
                image::Pos pos_2 = pos_1;
                if (affected_area.contains_pixel_at(pos_1))
                    pos_1.x = pattern.pos.x + core::int_periodic_mod(pos_1.x - pattern.pos.x, pattern.size.width);
                const comp_type* pixel_1 = iter_1(pos_1);
                const comp_type* pixel_2 = iter_2(pos_2);
                ARCHON_CHECK_EQUAL_SEQ(core::Span(pixel_1, pixel_1 + num_channels),
                                       core::Span(pixel_2, pixel_2 + num_channels));
            }
        }
    };
    test::for_each_box_in(inner_box, [&](const image::Box& pattern) {
        if (pattern.size.width != 0)
            test(test_context, pattern);
    });
}


ARCHON_TEST(Image_Iter_RepeatRight)
{
    image::Size inner_size = 9;
    image::Size margin = 1;
    image::Size outer_size = inner_size + 2 * margin;
    image::Box inner_box = { image::Pos() + margin, inner_size };
    using comp_type = std::size_t;
    int num_channels = 2;
    std::size_t buffer_size = 1;
    core::int_mul(buffer_size, num_channels);
    core::int_mul(buffer_size, outer_size.width);
    core::int_mul(buffer_size, outer_size.height);
    std::unique_ptr<comp_type[]> buffer_1 = std::make_unique<comp_type[]>(buffer_size);
    std::unique_ptr<comp_type[]> buffer_2 = std::make_unique<comp_type[]>(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i)
        buffer_1[i] = i;
    std::ptrdiff_t horz_stride = num_channels;
    std::ptrdiff_t vert_stride = outer_size.width * num_channels;
    image::Iter iter_1 = { buffer_1.get(), horz_stride, vert_stride };
    image::Iter iter_2 = { buffer_2.get(), horz_stride, vert_stride };
    auto test = [&](check::TestContext& parent_test_context, const image::Box& pattern) {
        ARCHON_TEST_TRAIL(parent_test_context, pattern);
        std::copy_n(buffer_1.get(), buffer_size, buffer_2.get());
        int size = (inner_box.pos.x + inner_box.size.width) - (pattern.pos.x + pattern.size.width);
        iter_2.repeat_right(pattern, size, num_channels);
        image::Box affected_area = { pattern.pos + pattern.size.proj_x(), image::Size(size, pattern.size.height) };
        for (int y = 0; y < outer_size.height; ++y) {
            for (int x = 0; x < outer_size.width; ++x) {
                image::Pos pos_1 = { x, y };
                image::Pos pos_2 = pos_1;
                if (affected_area.contains_pixel_at(pos_1))
                    pos_1.x = pattern.pos.x + core::int_periodic_mod(pos_1.x - pattern.pos.x, pattern.size.width);
                const comp_type* pixel_1 = iter_1(pos_1);
                const comp_type* pixel_2 = iter_2(pos_2);
                ARCHON_CHECK_EQUAL_SEQ(core::Span(pixel_1, pixel_1 + num_channels),
                                       core::Span(pixel_2, pixel_2 + num_channels));
            }
        }
    };
    test::for_each_box_in(inner_box, [&](const image::Box& pattern) {
        if (pattern.size.width != 0)
            test(test_context, pattern);
    });
}


ARCHON_TEST(Image_Iter_RepeatUp)
{
    image::Size inner_size = 9;
    image::Size margin = 1;
    image::Size outer_size = inner_size + 2 * margin;
    image::Box inner_box = { image::Pos() + margin, inner_size };
    using comp_type = std::size_t;
    int num_channels = 2;
    std::size_t buffer_size = 1;
    core::int_mul(buffer_size, num_channels);
    core::int_mul(buffer_size, outer_size.width);
    core::int_mul(buffer_size, outer_size.height);
    std::unique_ptr<comp_type[]> buffer_1 = std::make_unique<comp_type[]>(buffer_size);
    std::unique_ptr<comp_type[]> buffer_2 = std::make_unique<comp_type[]>(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i)
        buffer_1[i] = i;
    std::ptrdiff_t horz_stride = num_channels;
    std::ptrdiff_t vert_stride = outer_size.width * num_channels;
    image::Iter iter_1 = { buffer_1.get(), horz_stride, vert_stride };
    image::Iter iter_2 = { buffer_2.get(), horz_stride, vert_stride };
    auto test = [&](check::TestContext& parent_test_context, const image::Box& pattern) {
        ARCHON_TEST_TRAIL(parent_test_context, pattern);
        std::copy_n(buffer_1.get(), buffer_size, buffer_2.get());
        int size = pattern.pos.y - inner_box.pos.y;
        iter_2.repeat_up(pattern, size, num_channels);
        image::Box affected_area = { splice(pattern.pos, inner_box.pos), image::Size(pattern.size.width, size) };
        for (int y = 0; y < outer_size.height; ++y) {
            for (int x = 0; x < outer_size.width; ++x) {
                image::Pos pos_1 = { x, y };
                image::Pos pos_2 = pos_1;
                if (affected_area.contains_pixel_at(pos_1))
                    pos_1.y = pattern.pos.y + core::int_periodic_mod(pos_1.y - pattern.pos.y, pattern.size.height);
                const comp_type* pixel_1 = iter_1(pos_1);
                const comp_type* pixel_2 = iter_2(pos_2);
                ARCHON_CHECK_EQUAL_SEQ(core::Span(pixel_1, pixel_1 + num_channels),
                                       core::Span(pixel_2, pixel_2 + num_channels));
            }
        }
    };
    test::for_each_box_in(inner_box, [&](const image::Box& pattern) {
        if (pattern.size.height != 0)
            test(test_context, pattern);
    });
}


ARCHON_TEST(Image_Iter_RepeatDown)
{
    image::Size inner_size = 9;
    image::Size margin = 1;
    image::Size outer_size = inner_size + 2 * margin;
    image::Box inner_box = { image::Pos() + margin, inner_size };
    using comp_type = std::size_t;
    int num_channels = 2;
    std::size_t buffer_size = 1;
    core::int_mul(buffer_size, num_channels);
    core::int_mul(buffer_size, outer_size.width);
    core::int_mul(buffer_size, outer_size.height);
    std::unique_ptr<comp_type[]> buffer_1 = std::make_unique<comp_type[]>(buffer_size);
    std::unique_ptr<comp_type[]> buffer_2 = std::make_unique<comp_type[]>(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i)
        buffer_1[i] = i;
    std::ptrdiff_t horz_stride = num_channels;
    std::ptrdiff_t vert_stride = outer_size.width * num_channels;
    image::Iter iter_1 = { buffer_1.get(), horz_stride, vert_stride };
    image::Iter iter_2 = { buffer_2.get(), horz_stride, vert_stride };
    auto test = [&](check::TestContext& parent_test_context, const image::Box& pattern) {
        ARCHON_TEST_TRAIL(parent_test_context, pattern);
        std::copy_n(buffer_1.get(), buffer_size, buffer_2.get());
        int size = (inner_box.pos.y + inner_box.size.height) - (pattern.pos.y + pattern.size.height);
        iter_2.repeat_down(pattern, size, num_channels);
        image::Box affected_area = { pattern.pos + pattern.size.proj_y(), image::Size(pattern.size.width, size) };
        for (int y = 0; y < outer_size.height; ++y) {
            for (int x = 0; x < outer_size.width; ++x) {
                image::Pos pos_1 = { x, y };
                image::Pos pos_2 = pos_1;
                if (affected_area.contains_pixel_at(pos_1))
                    pos_1.y = pattern.pos.y + core::int_periodic_mod(pos_1.y - pattern.pos.y, pattern.size.height);
                const comp_type* pixel_1 = iter_1(pos_1);
                const comp_type* pixel_2 = iter_2(pos_2);
                ARCHON_CHECK_EQUAL_SEQ(core::Span(pixel_1, pixel_1 + num_channels),
                                       core::Span(pixel_2, pixel_2 + num_channels));
            }
        }
    };
    test::for_each_box_in(inner_box, [&](const image::Box& pattern) {
        if (pattern.size.height != 0)
            test(test_context, pattern);
    });
}
