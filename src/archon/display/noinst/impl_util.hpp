// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_IMPL_UTIL_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_IMPL_UTIL_HPP


#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/display/geometry.hpp>


namespace archon::display::impl {


// Maximum size of subbox produced by subdivide() (applies separately in each direction).
//
static constexpr display::Size subdivide_max_subbox_size = 64;


// This function divides `box` into smaller sub-boxes and calls `func` for each one. No
// sub-box will be larger than `subdivide_max_subbox_size`, and no sub-box will be empty
// (`display::Box::is_empty()`). It follows then, that `func` will never be called if the
// specified box (`box`) is empty. The specified box must be valid
// (`image::Box::is_valid()`).
//
template<class F> void subdivide(const display::Box& box, F&& func);








// Implementation


template<class F> void subdivide(const display::Box& box, F&& func)
{
    ARCHON_ASSERT(box.is_valid());
    display::Pos pos = box.pos;
    display::Size size = box.size;
    int y = 0;
    while (ARCHON_LIKELY(y < size.height)) {
        int h = std::min(subdivide_max_subbox_size.height, size.height - y);
        int x = 0;
        while (ARCHON_LIKELY(x < size.width)) {
            int w = std::min(subdivide_max_subbox_size.width, size.width - x);
            func(display::Box(pos + display::Size(x, y), { w, h })); // Throws
            x += w;
        }
        y += h;
    }
}


} // namespace archon::display::impl

#endif // ARCHON_X_DISPLAY_X_NOINST_X_IMPL_UTIL_HPP
