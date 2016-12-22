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

#include <memory>

#include <archon/core/functions.hpp>
#include <archon/core/memory.hpp>
#include <archon/math/matrix_adapt.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/raytrace/texture.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::raytrace;


namespace {

/// \todo FIXME: Allow image to be stored with other word types including at
/// least one floating point type such that significant information is not
/// discarded from "exotic" source images.
///
/// \todo FIXME: Add support for mipmapping or similar filtering feature.
class ImageTexture: public Texture {
public:
    ImageTexture(Image::ConstRefArg img, bool rep_s, bool rep_t):
        width{img->get_width()},
        height{img->get_height()},
        has_alpha{img->has_alpha_channel()},
        num_channels{has_alpha ? 4 : 3},
        buffer{std::make_unique<unsigned char[]>(std::size_t(height) * width * num_channels)},
        repeat_s{rep_s},
        repeat_t{rep_t}
    {
        Image::Ref img2 =
            BufferedImage::new_image(buffer.get(), width, height, ColorSpace::get_RGB(), has_alpha,
                                     BufferFormat::get_simple_format<unsigned char>(num_channels));
        img2->put_image(img, 0, 0, false);
    }


    void map(Vec2 point, Vec4& rgba) const
    {
        double x = width * point[0] - 0.5, y = height * point[1] - 0.5;

        // Determine indices of upper left pixel of relevant 2x2 pixel block.
        int xi = floor(x), yi = floor(y);

        double block[2*2*4]; // 2x2 pixels with 4 channels each (RGBA)
        get_block(xi, yi, block);

        double xf = x - xi, yf = y - yi;
        rgba = Vec4((1-xf) * (1-yf), xf * (1-yf), (1-xf) * yf, xf * yf) * mat4x4_adapt(block);
    }


    void get_block(int x, int y, double* block) const
    {
        int edge_left  = 0;
        int edge_right = width - 1;
        int x0 = x;
        int x1 = x + 1;
        if (x0 < edge_left) {
            if (repeat_s) {
                x0 = modulo(x0, width);
                x1 = x0 == edge_right ? edge_left : x0 + 1;
            }
            else {
                x0 = x1 = edge_left;
            }
        }
        else if (edge_right < x1) {
            if (repeat_s) {
                x1 = modulo(x1, width);
                x0 = x1 == edge_left ? edge_right : x1 - 1;
            }
            else {
                x0 = x1 = edge_right;
            }
        }

        int edge_bottom  = 0;
        int edge_top     = height - 1;
        int y0 = y;
        int y1 = y + 1;
        if (y0 < edge_bottom) {
            if (repeat_t) {
                y0 = modulo(y0, height);
                y1 = y0 == edge_top ? edge_bottom : y0 + 1;
            }
            else {
                y0 = y1 = edge_bottom;
            }
        }
        else if (edge_top < y1) {
            if (repeat_t) {
                y1 = modulo(y1, height);
                y0 = y1 == edge_bottom ? edge_top : y1 - 1;
            }
            else {
                y0 = y1 = edge_top;
            }
        }

        get_pixel(x0, y0, block + 0*4);
        get_pixel(x1, y0, block + 1*4);
        get_pixel(x0, y1, block + 2*4);
        get_pixel(x1, y1, block + 3*4);
    }


    void get_pixel(int x, int y, double* pixel) const
    {
        unsigned char* p = buffer.get() + (size_t(y) * width + x) * num_channels;
        frac_any_to_any(p, pixel, num_channels);
        if (!has_alpha)
            pixel[3] = 1;
    }


    const int width, height;
    const bool has_alpha;
    const int num_channels;
    const std::unique_ptr<unsigned char[]> buffer;
    const bool repeat_s, repeat_t;
};

} // unnamed namespace


namespace archon {
namespace raytrace {

std::shared_ptr<Texture> Texture::get_image_texture(Image::ConstRefArg img, bool rep_s, bool rep_t)
{
    return std::make_shared<ImageTexture>(img, rep_s, rep_t); // Throws
}


void TexturedPhongMaterial::get_diffuse_color(Vec2 tex_point, Vec4& rgba) const
{
    m_texture->map(m_transform(tex_point), rgba);
}

} // namespace raytrace
} // namespace archon
