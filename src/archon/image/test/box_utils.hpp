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

#ifndef ARCHON_X_IMAGE_X_TEST_X_BOX_UTILS_HPP
#define ARCHON_X_IMAGE_X_TEST_X_BOX_UTILS_HPP

/// \file


#include <archon/image/geom.hpp>


namespace archon::image::test {


template<class F> void for_each_pos_in(const image::Box& box, F&& func);
template<class F> void for_each_box_in(const image::Box& box, F&& func);
template<class F> void for_each_box_between(const image::Box& box_1, const image::Box& box_2, int min_size, F&& func);








// Implementation


template<class F> void for_each_pos_in(const image::Box& box, F&& func)
{
    for (int y = 0; y <= box.size.height; ++y) {
        for (int x = 0; x <= box.size.width; ++x)
            func(image::Pos(box.pos.x + x, box.pos.y + y));
    }
}


template<class F> void for_each_box_in(const image::Box& box, F&& func)
{
    test::for_each_pos_in(box, [&](image::Pos pos) {
        image::Size diff = pos - box.pos;
        for (int h = 0; h <= box.size.height - diff.height; ++h) {
            for (int w = 0; w <= box.size.width - diff.width; ++w)
                func(image::Box(pos, { w, h }));
        }
    });
}


template<class F> void for_each_box_between(const image::Box& box_1, const image::Box& box_2, int min_size, F&& func)
{
    test::for_each_pos_in(box_1, [&](image::Pos pos_1) {
        test::for_each_pos_in(box_2, [&](image::Pos pos_2) {
            image::Size size = pos_2 - pos_1;
            if (size.width >= min_size && size.height >= min_size)
                func(image::Box(pos_1, size));
        });
    });
}


} // namespace archon::image::test

#endif // ARCHON_X_IMAGE_X_TEST_X_BOX_UTILS_HPP
