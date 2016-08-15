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

#ifndef ARCHON_RAYTRACE_SURFACE_HPP
#define ARCHON_RAYTRACE_SURFACE_HPP

#include <archon/math/vector.hpp>
#include <archon/raytrace/material.hpp>


namespace archon {
namespace Raytrace {

class Surface {
public:
    /// Must be thread-safe.
    virtual Material* get_material() const = 0;

    /// Must be thread-safe.
    ///
    /// Map the specified surface point to the corresponding surface normal and
    /// optionally to the corresponding texture coordinates.
    ///
    /// The implementation may assume that the specified point is on the
    /// surface. The raytracer must gauarantee this.
    ///
    /// The implementation does not need to produce a normal of unit length, the
    /// raytracer will normalize it anyway.
    ///
    /// The mapping to texture coordinates must reflect the canonical mapping,
    /// and must not include the effect of any texture coordinate
    /// transformation.
    virtual void map(const Math::Vec3& point, Math::Vec3& normal,
                     Math::Vec2* tex_point = nullptr) const = 0;

    virtual ~Surface() {}
};


/*
/// An abstract surface with the ability to map texture coordinates to surface
/// properties and compute the color of an incident ray based on a specific
/// light source.
class Surface {
public:
    static ConstRef new_featureless_surface(const Math::Vec4& color,
                                            double ambientReflection = 0.1,
                                            double diffuseReflection = 0.5,
                                            double specularReflection = 0.5,
                                            double specularRefraction = 0.5,
                                            double reflectiveExponent = 20,
                                            double refractiveExponent = 20);

    virtual Math::Vec4 map(const Math::Vec2& tex_point) const = 0;

    /// \param light The light source.
    ///
    /// \param normal The surface normal at the surface point. It must be a
    /// vector of unit length.
    ///
    /// \param view_direction The direction from the surface point towards the
    /// view point. It must be a vector of unit length.
    ///
    /// \param light_direction The direction from the surface point towards the
    /// light source. It must be a vector of unit length.
    ///
    /// \param surface_color The color of the surface at the surface point.
    virtual Math::Vec3 shade(const Light* light,
                             const Math::Vec3& normal,
                             const Math::Vec3& view_direction,
                             const Math::Vec3& light_direction,
                             const Math::Vec4& surface_color) const = 0;

    virtual ~Surface() {}
};
*/

} // namespace Raytrace
} // namespace archon

#endif // ARCHON_RAYTRACE_SURFACE_HPP
