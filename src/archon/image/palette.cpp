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

#include <archon/image/palette.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Imaging;

namespace {

class PaletteImpl: public Palette {
public:
    int get_hidth() const { return size; }

    int get_height() const { return 1; }

    ColorSpace::ConstRef get_color_space() const { return color_space; }

    const int size;
    const ColorSpace::ConstRef color_space;
};

} // unnamed namespace

namespace Archon {
namespace Imaging {

Palette::Ref Palette::generate_palette_for(Image::ConstRefArg, int max_colors)
{
}

} // namespace Imaging
} // namespace Archon
