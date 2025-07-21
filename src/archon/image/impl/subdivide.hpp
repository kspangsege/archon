// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_IMAGE_X_IMPL_X_SUBDIVIDE_HPP
#define ARCHON_X_IMAGE_X_IMPL_X_SUBDIVIDE_HPP


#include <type_traits>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/image/geom.hpp>


namespace archon::image::impl {


// Divide operation into sequence of operations on smaller boxes. Specified box must be
// valid (`image::Box::is_valid()`).
template<class F> bool subdivide(const image::Box&, F&& func);








// Implementation


template<class F> bool subdivide(const image::Box& box, F&& func)
{
    auto call = [&](const image::Box& subbox) {
        using return_type = decltype(func(image::Box()));
        constexpr bool is_void_return = std::is_same_v<return_type, void>;
        if constexpr (is_void_return) {
            func(subbox); // Throws
            return true;
        }
        else {
            return func(subbox); // Throws
        }
    };

    ARCHON_ASSERT(box.is_valid());
    constexpr int preferred_block_width  = 64;
    constexpr int preferred_block_height = 64;
    constexpr int preferred_block_area = preferred_block_width * preferred_block_height;
    if (ARCHON_LIKELY(box.size.width >= preferred_block_width)) {
        int y = 0;
        for (;;) {
            int h, max_w;
            {
                int remaining_h = box.size.height - y;
                if (ARCHON_LIKELY(remaining_h >= preferred_block_height)) {
                    h = preferred_block_height;
                    max_w = preferred_block_width;
                }
                else {
                    if (remaining_h == 0)
                        break;
                    h = remaining_h;
                    max_w = preferred_block_area / h;
                }
            }
            int x = 0;
            for (;;) {
                int remaining_w = box.size.width - x;
                int w = std::min(remaining_w, max_w);
                bool proceed = call(image::Box(box.pos + image::Size(x, y), { w, h })); // Throws
                if (ARCHON_UNLIKELY(!proceed))
                    return false;
                x += w;
                if (ARCHON_LIKELY(x < box.size.width))
                    continue;
                break;
            }
            y += h;
        }
    }
    else if (box.size.width > 0) {
        int w = box.size.width;
        int max_h = preferred_block_area / w;
        int x = 0;
        int y = 0;
        for (;;) {
            int remaining_h = box.size.height - y;
            int h = std::min(remaining_h, max_h);
            bool proceed = call(image::Box(box.pos + image::Size(x, y), { w, h })); // Throws
            if (ARCHON_UNLIKELY(!proceed))
                return false;
            y += h;
            if (ARCHON_LIKELY(y < box.size.height))
                continue;
            break;
        }
    }
    return true;
}


} // namespace archon::image::impl

#endif // ARCHON_X_IMAGE_X_IMPL_X_SUBDIVIDE_HPP
