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


#include <cstddef>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/viewport.hpp>


using namespace archon;


auto display::find_viewport(core::Span<const display::Viewport> viewports, display::Pos window_pos,
                            display::Size window_size) noexcept -> std::size_t
{
    display::Box box = { window_pos, window_size };
    display::Pos center = box.pos + box.size / 2;
    std::size_t i = std::size_t(-1);
    std::size_t n = viewports.size();
    for (std::size_t j = 0; j < n; ++j) {
        const display::Viewport& viewport = viewports[j];
        if (ARCHON_UNLIKELY(viewport.bounds.contains_pixel_at(center))) {
            i = j;
            break;
        }
        if (ARCHON_UNLIKELY(i == std::size_t(-1) && viewport.bounds.intersects(box)))
            i = j;
    }
    return i;
}
