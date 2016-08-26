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

#ifndef ARCHON_RAYTRACE_OBJECT_HPP
#define ARCHON_RAYTRACE_OBJECT_HPP

#include <memory>

#include <archon/core/unique_ptr.hpp>
#include <archon/math/geometry.hpp>
#include <archon/raytrace/material.hpp>
#include <archon/raytrace/surface.hpp>


namespace archon {
namespace raytrace {

class Object {
public:
    /// A cube with edge length 2, centred at the origin of the local coordinate
    /// system, and with edges aligned with the local coordinate axes.
    ///
    /// Textures are applied individually to each face of the square.  On the
    /// front, back, right, and left faces of the box, when viewed from the
    /// outside with the positive Y-axis pointing upwards, the texture is mapped
    /// onto each face with the same orientation as if the image were displayed
    /// normally in 2-D.  On the top face of the box, when viewed from above and
    /// looking down the Y-axis toward the origin with the -Z-axis as the view
    /// up direction, the texture is mapped onto the face with the same
    /// orientation as if the image were displayed normally in 2D.  On the
    /// bottom face of the box, when viewed from below looking up the Y-axis
    /// toward the origin with the +Z-axis as the view up direction, the texture
    /// is mapped onto the face with the same orientation as if the image were
    /// displayed normally in 2-D.
    static std::unique_ptr<Object> make_box(core::SharedPtr<Material>);

    /// A cone which is centered in the local coordinate system and whose
    /// central axis is aligned with the local Y-axis, and the apex pointing
    /// upwards (in the direction of the positive Y-axis.) The radius of the
    /// cone's base is 1, and the height of the cone from the centre of the base
    /// to the apex is 2.
    ///
    /// When a texture is applied to the sides of the cone, the texture wraps
    /// counterclockwise when viewed from above, starting at the back of the
    /// cone. The texture has a vertical "seam" at the back in the X=0 plane,
    /// from the apex (0, 1, 0) to the point (0, -1, -1). For the bottom cap, a
    /// circle is cut out of the texture square centered at (0, -1, 0) with
    /// dimensions 2 by 2. When the cone is rotated 90 degrees around the X-axis
    /// such that the apex points in the direction of the negative Z-axis, the
    /// primary and secondary texture coordinate axes coincide with the local
    /// spatial X and Y-axis respectively.
    static std::unique_ptr<Object> make_cone(core::SharedPtr<Material>);

    /// A capped cylinder centered at the origin of the local coordinate system
    /// and with a central axis oriented along the local Y-axis. The radius of
    /// the cylinder is 1, and the height of the cylinder along the central axis
    /// is 2.
    ///
    /// When a texture is applied to a cylinder, it is applied differently to
    /// the sides, top, and bottom. On the sides, the texture wraps
    /// counterclockwise when viewed from above (positive Y), starting at the
    /// back of the cylinder (negative Z). The texture has a vertical "seam" at
    /// the back, intersecting the X=0 plane.  For the top and bottom caps, a
    /// circle is cut out of the unit texture squares centred at (0, +/-1, 0)
    /// with dimensions 2 by 2. When the cylinder is rotated 90 degrees around
    /// the X-axis such that the bottom is in the direction of the negative
    /// Z-axis, the primary and secondary texture coordinate axes of the bottom
    /// texture will coincide with the local spatial X and Y-axes
    /// respectively. Likewise, when the cylinder is rotated 90 degrees in the
    /// opposite direction, the primary and secondary texture coordinate axes of
    /// the top texture will coincide with the local spatial X and Y-axis
    /// respectively.
    static std::unique_ptr<Object> make_cylinder(core::SharedPtr<Material>);

    /// A unit sphere, centered in the origin of the local coordinate
    /// system. The north pole of the sphere is in the direction of the positive
    /// Y-axis (upwards).
    ///
    /// A texture is applied to the sphere by taking the polar coordinates from
    /// the surface of the sphere as the texture coordinates (scaled
    /// apropriately). The latitude or elevation angle plays the role of the
    /// texture Y-coordinate such that the north pole corresponds to the top of
    /// the texture and the south pole corresponds to the bottom. The logitude
    /// or azimuth angle plays the role of the texture X-coordinate. The zero
    /// meridian is the meridian that intersects the negative z-axis (ahead in
    /// the canonical view). This corresponds to the left and right edge of the
    /// texture. The texture is wraped counter-clockwise around the sphere when
    /// seen from above, and has a "seam" at the back of the sphere.
    static std::unique_ptr<Object> make_sphere(core::SharedPtr<Material>);

    /// A torus centered at the local origin with axis of revolution coincident
    /// with the Y-axis, and with a major radius of 1, and a variable minor
    /// radius.
    ///
    /// The major radius describes a circle in the Z-X-plane centered at the
    /// origin of the local coordinate system. Any point on the surface of the
    /// torus can then be describes by a vector drawn from some point on this
    /// circle. Each such vector must be perpendicular to the circle at that
    /// point and be as long as the minor radius. Thus, the torus is centered
    /// at the origin of the local coordinate system and has the Y-axis as the
    /// axis of revolution.  When textures are applied to a torus the texture
    /// X-coordinate is taken as the angle of the projection of the surface
    /// point onto the Z-X-plane taking zero to be the direction of the negative
    /// Z-axis and increasing counter-clockwise when seen from the positive
    /// Y-axis. The texture Y-coordinate is taken as the angle between the
    /// surface point and the center of the torus when seen from the point on
    /// the circle of revolution that is closest to the surface point. When seen
    /// from a point on the circle of revolution in the direction of revolution,
    /// the texture Y-coordinate increses clockwise.
    static std::unique_ptr<Object> make_torus(core::SharedPtr<Material>,
                                              double minor_radius = 0.5);

    /// Must be thread-safe.
    ///
    /// Check whether the specified ray intersects the surface of this object,
    /// and if it does, report the distance from the ray origin to the closest
    /// point of intersection (in case there are multiple intersection points.)
    /// If there is no intersection \a dist and \a surface must be left
    /// "untouched" and \c false must be returned. Otherwise \a dist must be set
    /// to the distance relative to the length of the ray direction vector, and
    /// a pointer to the intersected surface must be stored in \a surface, and
    /// \c true must be returned.
    ///
    /// It is allowed for this method to assume that the ray does not originate
    /// from the surface of this object.
    ///
    /// \param ray The incident ray expressed in local coordinates.
    ///
    /// \param dist This method must set it to the distance from the ray origin
    /// to the first point of intersection, expressed relative to the length of
    /// the ray direction vector, that is, if the distance is <tt>d</tt>, then
    /// the intersection point is <tt>ray.point + d * ray.direction</tt>
    ///
    /// \param origin_obj The object from whose surface the spcified ray
    /// originates, or \c null if it originates from the eye. For some types of
    /// geometry, the intersection method can be optimized and/or stabilized if
    /// it is known that the ray does or does not originate on the surface of
    /// itself.
    ///
    /// \param surface When intersection occurs, it is set to the intersected
    /// surface of this object, unless you pass \c null.
    ///
    /// \return True if, and only if there is at least one intersection point
    /// between this object and the incident ray.
    ///
    /// \note This method will not be called by the raytracer for a convex
    /// object if the ray originates on the surface of that object.
    virtual bool intersect(const math::Line3& ray, double& dist,
                           const Object* origin_obj = nullptr,
                           const Surface** surface = nullptr) const = 0;

    virtual ~Object() {}
};


/*
/// Describes the geometry and surface texture of a spatial object. The
/// description is relative to a local coordinate system which in general will
/// be different from the global coordinate system. Any implementations must
/// document how the object will be orinted and positioned relative to the local
/// coordinate system.
///
/// Solid vs. thin objects: Objects may be either solid or thin. A solid object
/// is one that has a closed surface and a well defined interior, such that
/// after an odd number of surface cossings, a ray must be in the interior of
/// that object.
///
/// All methods must be thread safe including those of Surface.
class Object: public core::CntRefObjectBase, core::CntRefDefs<Object> {
public:
//    virtual double get_refractive_index() const = 0;

    /// Tells whether or not this object is convex. An object is convex if and
    /// only if for any two points on the surface, all points on the connecting
    /// line segment are on the surface or in the interior. This is important
    /// because a ray cannot intersect the surface of a convex object if it also
    /// originates on the surface of that object (disregarding the case of
    /// transmission and disregarding the case where the ray is parallel to the
    /// surface.)
    ///
    /// Note that a solid object that consists of two disconnected parts can
    /// never be convex.
    virtual bool is_convex() const = 0;

    class Surface {
    public:
        virtual const Surface* get_surface() const = 0;

        /// Map the specified surface point to the corresponding surface normal
        /// and optionally to the corresponding texture coordinates. The mapping
        /// to texture coordinates must reflect the canonical mapping, and must
        /// not include the effect of any texture coordinate transformation.
        virtual void map(const math::Vec3& point,
                         math::Vec3& normal,
                         math::Vec2* texture_point = 0) const = 0;

        virtual ~Surface() {}
    };
*/

} // namespace raytrace
} // namespace archon

#endif // ARCHON_RAYTRACE_OBJECT_HPP
