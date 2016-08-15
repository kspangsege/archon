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

#include <archon/util/color.hpp>

using namespace archon::math;


namespace archon {
namespace util {
namespace color {

/// Linear interpolation of RGB colors in the HSV space.
Vec3 interp(double x, double x1, double x2, const Vec3& y1, const Vec3& y2)
{
    Vec3 z1 = cvt_RGB_to_HSV(y1);
    Vec3 z2 = cvt_RGB_to_HSV(y2);

    // Pick a sensible value for hue when it is arbitrary/undefined
    if (z1[1] == 0) {
        z1[0] = z2[0];
    }
    else if (z2[1] == 0) {
        z2[0] = z1[0];
    }

    // Pick a sensible value for saturation when it is arbitrary/undefined
    if (z1[2] == 0) {
        z1[1] = z2[1];
    }
    else if (z2[2] == 0) {
        z2[1] = z1[1];
    }

    // Pick the shortest hue arc
    double d = z2[0] - z1[0];
    if (d < -0.5) {
        z2[0] += 1;
    }
    else if (d > 0.5) {
        z1[0] += 1;
    }

    return cvt_HSV_to_RGB(lin_interp(x, x1, x2, z1, z2));
}

} // namespace color
} // namespace util
} // namespace archon
