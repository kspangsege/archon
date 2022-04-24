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
#include <algorithm>

#include <archon/core/integer.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/face.hpp>


using namespace archon;
using font::Face;


void Face::render_glyph_mask(image::Writer& writer)
{
    image::Box box = get_target_glyph_box(); // Throws
    std::ptrdiff_t horz_stride = 1; // 1 channel (alpha)
    std::ptrdiff_t vert_stride = horz_stride;
    core::int_mul(vert_stride, box.size.width); // Throws
    std::size_t buffer_size = std::size_t(vert_stride);
    core::int_mul(buffer_size, box.size.height); // Throws
    m_render_buffer.reserve(buffer_size); // Throws
    std::fill_n(m_render_buffer.data(), buffer_size, comp_type(0));
    iter_type iter { m_render_buffer.data(), horz_stride, vert_stride };
    image::Pos pos = image::Pos() - (box.pos - m_target_pos);
    do_render_glyph_mask(pos, iter, box.size); // Throws
    writer.put_block_mask(box.pos, { iter, box.size }); // Throws
}


void Face::render_glyph_rgba(image::Writer& writer)
{
    image::Box box = get_target_glyph_box(); // Throws
    std::ptrdiff_t horz_stride = 4; // 4 channels (RGBA)
    std::ptrdiff_t vert_stride = horz_stride;
    core::int_mul(vert_stride, box.size.width); // Throws
    std::size_t buffer_size = std::size_t(vert_stride);
    core::int_mul(buffer_size, box.size.height); // Throws
    m_render_buffer.reserve(buffer_size); // Throws
    iter_type iter { m_render_buffer.data(), horz_stride, vert_stride };
    image::Pos pos = image::Pos() - (box.pos - m_target_pos);
    do_render_glyph_rgba(pos, iter, box.size); // Throws
    writer.put_block_rgba(box.pos, { iter, box.size }); // Throws
}
