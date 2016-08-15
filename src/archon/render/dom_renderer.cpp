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

#include <GL/gl.h>

#include <archon/math/vector.hpp>
#include <archon/render/dom_renderer.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Math;
using namespace archon::Util;
using namespace archon::Render;
using namespace archon::Imaging;


namespace {

TextureDecl declare_texture(TextureCache& cache, Image::ConstRefArg img, const string& name,
                            bool repeat, TextureCache::FilterMode f)
{
    UniquePtr<TextureSource> src(new TextureImageSource(img, name));
    GLenum wrap = repeat ? GL_REPEAT : GL_CLAMP;
    return cache.declare(src, wrap, wrap, f);
}

TextureDecl get_dashed_texture_decl(TextureCache& cache)
{
    unsigned char buffer[4] = {
        numeric_limits<unsigned char>::max(), numeric_limits<unsigned char>::max(),
        0, 0
    };
    Image::Ref img = Image::copy_image_from(buffer, 2, 1, ColorSpace::get_Lum(), true);
    return declare_texture(cache, img, "Dashed pattern", true, TextureCache::filter_mode_Nearest);
}

TextureDecl get_dotted_texture_decl(TextureCache& cache, const string& resource_dir)
{
    Image::Ref const img = Image::load(resource_dir + "render/dotted.png");
    return declare_texture(cache, img, "Dotted pattern", true, TextureCache::filter_mode_Mipmap);
}

} // anonymous namespace


namespace archon {
namespace Render {


DomRenderer::DomRenderer(TextureCache& cache, const string& resource_dir):
    m_dashed_texture_decl(get_dashed_texture_decl(cache)),
    m_dotted_texture_decl(get_dotted_texture_decl(cache, resource_dir))
{
}


void DomRenderer::filled_box(int x, int y, int width, int height, PackedTRGB color)
{
    cerr << "rect(" << x << ", " << y << ", " << width << ", " << height << ")\n";
    Vec4F rgba;
    color.unpack_rgba(rgba);
    glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);
    int x1 = x;
    int x2 = x1 + width;
    int y2 = m_viewport_height - y;
    int y1 = y2 - height;
    glBegin(GL_QUADS);
    glVertex2i(x1, y1);
    glVertex2i(x2, y1);
    glVertex2i(x2, y2);
    glVertex2i(x1, y2);
    glEnd();
}


void DomRenderer::border_box(int x, int y, int width, int height, const Border* sides)
{
    int ox1 = x;
    int ox2 = ox1 + width;
    int oy2 = m_viewport_height - y;
    int oy1 = oy2 - height;

    const Border& top    = sides[0];
    const Border& right  = sides[1];
    const Border& bottom = sides[2];
    const Border& left   = sides[3];

    int ix1 = ox1 + left.width;
    int ix2 = ox2 - right.width;
    int iy1 = oy1 + bottom.width;
    int iy2 = oy2 - top.width;

    render_border<0>(top, ox1, ix1, ix2, ox2, oy2, iy2);
    render_border<1>(right, oy2, iy2, iy1, oy1, ox2, ix2);
    render_border<2>(bottom, ox2, ix2, ix1, ox1, oy1, iy1);
    render_border<3>(left, oy1, iy1, iy2, oy2, ox1, ix1);
}


template<int side_idx>
void DomRenderer::render_border(const Border& side, int s0, int s1, int s2, int s3, int t0, int t1)
{
    if (!side.width || side.style == DomImpl::borderStyle_None)
        return;

    Vec4F rgba;
    side.color.unpack_rgba(rgba);
    glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

    if (side.style == DomImpl::borderStyle_Solid) {
        glBegin(GL_QUADS);
        if (side_idx == 0 || side_idx == 2) {
            glVertex2i(s0, t0);
            glVertex2i(s1, t1);
            glVertex2i(s2, t1);
            glVertex2i(s3, t0);
        }
        else {
            glVertex2i(t0, s0);
            glVertex2i(t1, s1);
            glVertex2i(t1, s2);
            glVertex2i(t0, s3);
        }
        glEnd();
        return;
    }

    double len;
    if (side.style == DomImpl::borderStyle_Dashed) {
        if (!m_dashed_texture)
            m_dashed_texture = m_dashed_texture_decl.acquire();
        m_dashed_texture.bind();
        len = 2*3*side.width;
    }
    else {
        if (!m_dotted_texture)
            m_dotted_texture = m_dotted_texture_decl.acquire();
        m_dotted_texture.bind();
        len = 2*side.width;
    }
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    switch (side_idx) {
        case 0: // Top
            glTexCoord2f(0, 1);
            glVertex2i(s0, t0);
            glTexCoord2f((s1-s0)/len, 0);
            glVertex2i(s1, t1);
            glTexCoord2f((s2-s0)/len, 0);
            glVertex2i(s2, t1);
            glTexCoord2f((s3-s0)/len, 1);
            glVertex2i(s3, t0);
            break;
        case 1: // Right
            glTexCoord2f(0, 1);
            glVertex2i(t0, s0);
            glTexCoord2f((s0-s1)/len, 0);
            glVertex2i(t1, s1);
            glTexCoord2f((s0-s2)/len, 0);
            glVertex2i(t1, s2);
            glTexCoord2f((s0-s3)/len, 1);
            glVertex2i(t0, s3);
            break;
        case 2: // Bottom
            glTexCoord2f(0, 1);
            glVertex2i(s0, t0);
            glTexCoord2f((s0-s1)/len, 0);
            glVertex2i(s1, t1);
            glTexCoord2f((s0-s2)/len, 0);
            glVertex2i(s2, t1);
            glTexCoord2f((s0-s3)/len, 1);
            glVertex2i(s3, t0);
            break;
        case 3: // Left
            glTexCoord2f(0, 1);
            glVertex2i(t0, s0);
            glTexCoord2f((s1-s0)/len, 0);
            glVertex2i(t1, s1);
            glTexCoord2f((s2-s0)/len, 0);
            glVertex2i(t1, s2);
            glTexCoord2f((s3-s0)/len, 1);
            glVertex2i(t0, s3);
            break;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}


} // namespace Render
} // namespace archon
