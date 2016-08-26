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

#ifndef ARCHON_RENDER_BILLBOARD_HPP
#define ARCHON_RENDER_BILLBOARD_HPP

#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>


namespace archon {
namespace render {
namespace billboard {

/// Rotate the local OpenGL coordinate system such that the z-axis is directed
/// towards the viewer.
///
/// There are two modes of operation. The simplest one is when you specify a
/// rotation axis (non zero vector). In this case the z-axis of the local
/// coordinate system is rotated around the specified axis by the angle than
/// minimizes the final angle between it and the direction from the local origin
/// to the viewer. The rotation axis must be specified in local coordinates.
///
/// The other mode is when there is no rotation angle (zero vector). In this
/// case the local coordinate system is first rotated around an arbitrary axis
/// such that its z-axis ends up pointing towards the viewer. Next, the local
/// coordinate system is rotated such that its y-axis lies in the plain spanned
/// by the viewers y-axis (up direction) and the local origin. This aligns the
/// upwards direction of the billborad with the upwards direction of the viewer
/// as much as possible.
///
/// \param rot_axis If set to zero (the default) the actual axis will be chosen
/// to optimize the alignment of the local y-axiz with the viewers notion of
/// 'upwards'.
///
/// \return Angle of projection onto the screen of vector from center of screen
/// to origin of local OpenGL coordinate system.
double rotate(const math::Vec3& rot_axis = math::Vec3::zero());


/// Both \a rot_axis and \a rot are specified in local coordinates.
void calculate_rotation(const math::Mat3& subframe_basis,
                        const math::Vec3& subframe_origin,
                        const math::Vec3& rot_axis,
                        math::Rotation3& rot);

} // namespace billboard
} // namespace render
} // namespace archon

#endif // ARCHON_RENDER_BILLBOARD_HPP
