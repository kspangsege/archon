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


#include <archon/image/geom.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/block.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/face.hpp>


using namespace archon;


void font::face::render_glyph_mask(image::Writer& writer, buffer_type& buffer)
{
    image::Box box = get_target_glyph_box(); // Throws
    image::PixelBlock_Alpha_8 block(box.size, buffer); // Throws
    tray_type tray = block.tray();
    render_glyph_mask_a(box.pos, tray); // Throws
    writer.put_block_mask(box.pos, tray); // Throws
}


void font::face::render_glyph_rgba(image::Writer& writer, buffer_type& buffer)
{
    image::Box box = get_target_glyph_box(); // Throws
    image::PixelBlock_RGBA_8 block(box.size, buffer); // Throws
    tray_type tray = block.tray();
    render_glyph_rgba_a(box.pos, tray); // Throws
    writer.put_block_rgba(box.pos, tray); // Throws
}
