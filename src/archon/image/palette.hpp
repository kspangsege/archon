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

#ifndef ARCHON_IMAGE_PALETTE_HPP
#define ARCHON_IMAGE_PALETTE_HPP

#include <archon/image/buffered_image.hpp>


namespace Archon {
namespace Imaging {

Image::ConstRef get_direct_color_view(Image::ConstRefArg indexImage, Image::ConstRefArg palette);

/// This class represents a palette for use when working with indirect
/// colors. It is itself an image whose width is equal to the number of colors
/// in the palette, and whose height is 1. The left-most pixel corresponds with
/// the first color in the palette.
///
/// It also provides methods for converting between direct and indirect color,
/// which may involve color quantization, and dithering.
class Palette: public BufferedImage {
public:
    static Ref generate_palette_for(Image::ConstRefArg, int max_colors=256);

    get_direct_color_view()
};

} // namespace Imaging
} // namespace Archon

#endif // ARCHON_IMAGE_PALETTE_HPP
