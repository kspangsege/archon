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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_MATH_INTERSECT_HPP
#define ARCHON_MATH_INTERSECT_HPP

#include <cmath>
#include <algorithm>

#include <archon/math/geometry.hpp>


namespace Archon {
namespace Math {

/**
 * Test for collision bewteen ray and plane and determine the distance
 * along the ray from the ray origin to the intersection point.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always strictly positive.
 *
 * When there is no collision reported the \a dist parameter is left
 * unchanged.
 *
 * \param front_to_back_only If true then a requirement for
 * intersection is that the ray originates from a location in front of
 * the plane.
 *
 * \return True iff the ray and the plane intersect.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 */
bool intersect(const Line3&, const Hyperplane3&, bool front_to_back_only, double& dist);



/**
 * Test for collision between ray and an axis aligned box, and
 * determine the distance along the ray from the ray origin to the
 * surface of the box.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always strictly positive.
 *
 * By default, the distance is mesured to the point where the ray
 * enters into the interior of the box, and not to the point where it
 * exits.
 *
 * When there is no intersection, the \a dist parameter is left
 * unchanged.
 *
 * This function returns an indication of where the interesting
 * intersection happens:
 *
 * <pre>
 *
 *   0 -> nowhere
 *   1 -> left face   (-box_size.x)
 *   2 -> right face  (box_size.x)
 *   3 -> bottom face (-box_size.y)
 *   4 -> top face    (box_size.y)
 *   5 -> back face   (-box_size.z)
 *   6 -> front face  (box_size.z)
 *
 * </pre>
 *
 * \tparam want_exit Set to true if you want the point where the ray
 * exits the box, rather than the one where it enters. In any case, an
 * intersection will be reported only if it occurs at a positive
 * distance from the ray origin.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 */
template<bool want_exit, int N, class T>
int intersect_box(const BasicLine<N,T>& ray, T& dist,
                  const BasicBox<N,T>& box = BasicBox<N,T>(BasicVec<N,T>(2)));



/**
 * Test for collision between ray and an sphere centered at the
 * origin, and determine the distance along the ray from the origin of
 * the ray to the surface of the sphere.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always strictly positive.
 *
 * By default, the distance is mesured to the point where the ray
 * enters into the interior of the sphere, and not the point where it
 * exits.
 *
 * When there is no intersection, the \a dist parameter is left
 * unchanged.
 *
 * \tparam want_exit Set to true if you want the point where the ray
 * exits the sphere, rather than the one where it enters. In any case,
 * an intersection will be reported only if it occurs at a positive
 * distance from the ray origin.
 *
 * \return True if, and only if the ray and the sphere intersect.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 */
template<bool want_exit, int N, class T>
bool intersect_sphere(const BasicLine<N,T>&, T& dist, T radius = 1);



/**
 * Test for collision between ray and cone and determine the distance
 * along the ray from the ray origin to the surface of the cone.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always positive.
 *
 * The cone is origin centred with the axis of revolution coincident
 * with the Y-axis. The apex of the code always points upward (in the
 * direction of the Y-axis) and the code is positioned such that the
 * origin is at the midpoint between the apex and the center of the
 * base (bottom cap).
 *
 * By default only collision points where the ray enters into the
 * interior of the cone are considered, but if false is passed as the
 * enter_only parameter then also points where the ray leaves the
 * interior are considered.
 *
 * When there is no collision reported the \a dist parameter is left
 * unchanged.
 *
 * Solid/non-solid cones: To support X3D cones it is possible to
 * disable the various parts of the cone (side and bottom). Naturally,
 * removing parts of a cone is equivalent to considerering it as a
 * non-solid object. One should always pass false as the 'enter_only'
 * parameter if any part of the cone is disabled.
 *
 * This function returns an indication of which part of the cone was
 * intersected first:
 *
 * <pre>
 *
 *   0 -> nowhere
 *   1 -> side        (x^2 + z^2 = (1/2 - y/height)^2 bottom_radius^2,
 *                     -height/2 <= y <= height/2)
 *   2 -> bottom cap  (y = -height/2, x^2 + z^2 < bottom_radius^2)
 *
 * </pre>
 *
 * \param dist Set to the distance described above.
 *
 * \param height Distance from center of bottom cap to apex.
 *
 * \param bottom_radius Radius at the base (bottom) of the cone.
 *
 * \param side If false then intersections with the side are
 * ignored. (See the note on Solid/non-solid cones)
 *
 * \param bottom If false then intersections with the bottom cap are
 * ignored. (See the note on Solid/non-solid cones)
 *
 * \param enter_only By default only collisions where the ray enters
 * into the interior of the cone are considered, but if false is
 * passed here then also points where the ray leaves the interior are
 * considered. The default, of course, only makes sense if the cone is
 * solid, and no parts of the cone are disabled.
 *
 * \return An indication of where the intersection was. See the
 * general description for details.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 *
 * \note It is not meaningfull to request 'enter_only' and also
 * specify that one or more of the cone parts should be disabled. That
 * is, enter_only=true, is equivalent to saying that the cone is
 * solid, and thus cannot lack bottom or sides - of course.
 */
int intersect_cone(const Line3&, double& dist, double height = 2, double bottom_radius = 1,
                   bool side = true, bool bottom = true, bool enter_only = true);



/**
 * Test for collision between ray and cylinder and determine the
 * distance along the ray from the ray origin to the surface of the
 * cylinder.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always strictly positive.
 *
 * The cylinder is origin centred with the axis of revolution
 * coincident with the Y-axis.
 *
 * By default only collision points where the ray enters into the
 * interior of the cylinder are considered, but if false is passed as
 * the \a enter_only parameter, then also points where the ray leaves
 * the interior are considered.
 *
 * When there is no collision reported the \a dist parameter is left
 * unchanged.
 *
 * Solid/non-solid cylinders: To support X3D cylinders it is possible
 * to disable the various parts of the cylinder (side, top and
 * bottom). Naturally, removing parts of a cylinder is equivalent to
 * considerering it as a non-solid object. One should always pass
 * false as the 'enter_only' parameter if any part of the cylinder is
 * disabled.
 *
 * This function returns an indication of which part of the cylinder
 * was intersected first:
 *
 * <pre>
 *
 *   0 -> nowhere
 *   1 -> side        (x^2 + z^2 = radius^2, -height/2 <= y <= height/2)
 *   2 -> bottom cap  (y = -height/2, x^2 + z^2 < radius^2)
 *   3 -> top cap     (y =  height/2, x^2 + z^2 < radius^2)
 *
 * </pre>
 *
 * \param height Distance from bottom cap to top cap. Alternatively a
 * negative value indicates a cylinder of infinite length.
 *
 * \param radius Radius of cylinder.
 *
 * \param dist Set to the distance described above.
 *
 * \param side If false then intersections with the side are
 * ignored. (See the note on Solid/non-solid cylinders)
 *
 * \param top If false then intersections with the top cap are
 * ignored. (See the note on Solid/non-solid cylinders)
 *
 * \param bottom If false then intersections with the bottom cap are
 * ignored. (See the note on Solid/non-solid cylinders)
 *
 * \param enter_only By default only collisions where the ray enters
 * into the interior of the cylinder are considered, but if false is
 * passed here then also points where the ray leaves the interior are
 * considered. The default, of course, only makes sense if the
 * cylinder is solid, and no parts of the cylinder are disabled.
 *
 * \return An indication of where the intersection was. See general
 * description for details.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 *
 * \note It is not meaningfull to request 'enter_only' and also
 * specify that one or more of the cylinder parts should be
 * disabled. That is, enter_only=true, is equivalent to saying that
 * the cylinder is solid, and thus cannot lack top, bottom or sides -
 * of course.
 */
int intersect_cylinder(const Line3&, double& dist, double height = 2, double radius = 1,
                       bool side = true, bool top = true, bool bottom = true,
                       bool enter_only = true);



/**
 * Test for collision between ray and torus, and optionally determine
 * the distance along the ray from the ray origin to the torus
 * surface.
 *
 * The distance is mesured in units of the ray direction vector length
 * and is always strictly positive.
 *
 * By default only collision points where the ray enters into the
 * interior are considered, but if false is passed as the \c
 * ext_to_int_only parameter then also points where the ray leaves the
 * interior are considered.
 *
 * If true is passed as the \c surface_origin parameter then the
 * collision point closest to the ray origin (if any) is discarded as
 * a possible collision point regardless of whether it actually lies
 * inside or outside the torus interior. This feature may be used to
 * stabilize results in a raytracing context.
 *
 * When there is no collision reported the \a dist parameter is left
 * unchanged.
 *
 * \return True iff the ray and the torus intersect.
 *
 * \note The ray is understood as originating at the fixed point of
 * the specified line.
 */
bool intersect_torus(const Line3&, double& dist, double major_torus_radius = 2,
                     double minor_torus_radius = 1, bool surface_origin = false,
                     bool ext_to_int_only = true);




// Implementation

template<bool want_exit, int N, class T>
inline int intersect_box(const BasicLine<N,T>& ray, T& dist, const BasicBox<N,T>& box)
{
    using std::swap;

    double dist_1 = 0, dist_2 = 0;
    int where_1, where_2;
    for (int i=0; i<N; ++i) {
        if (ray.direction[i] == 0 && (ray.origin[i] <= box.lower[i] ||
                                      box.upper[i] <= ray.origin[i]))
            return 0;
        T d1 = (box.lower[i] - ray.origin[i]) / ray.direction[i];
        T d2 = (box.upper[i] - ray.origin[i]) / ray.direction[i];
        int w1 = 2 * i + 1;
        int w2 = w1 + 1;
        if (d2 < d1) {
            swap(d1, d2);
            swap(w1, w2);
        }
        if (i==0 || dist_1 < d1) {
            dist_1 = d1;
            where_1 = w1;
        }
        if (i==0 || d2 < dist_2) {
            if (d2 <= 0)
                return 0;
            dist_2 = d2;
            where_2 = w2;
        }
        if (0 < i && dist_2 < dist_1)
            return 0;
    }

    if (want_exit) {
        dist = dist_2;
        return where_2;
    }

    if (dist_1 <= 0)
        return 0;

    dist = dist_1;
    return where_1;
}


template<bool want_exit, int N, class T>
inline bool intersect_sphere(const BasicLine<N,T>& ray, T& dist, T radius)
{
    using std::sqrt;

    if (want_exit) {
        BasicVec<N,T> p = ray.origin;
        T b = -dot(ray.direction, p);
        T c = sq_sum(p) - square(radius);
        if (0 <= c && b <= 0)
            return false; // The ray originates outside the sphere and extends away from the center
        T a = sq_sum(ray.direction);
        T d = square(b) - a*c;
        if (d < 0)
            return false; // No solutions
        dist = (b + sqrt(d)) / a;
        return true;
    }

    BasicVec<N,T> p = ray.origin;
    T b = -dot(ray.direction, p);
    if (b <= 0)
        return false; // The ray extends away from the center
    T c = sq_sum(p) - square(radius);
    if (c <= 0)
        return false; // The ray originates inside the sphere
    T a = sq_sum(ray.direction);
    T d = square(b) - a*c;
    if (d < 0)
        return false; // No solutions
    dist = (b - sqrt(d)) / a;
    return true;
}

} // namespace Math
} // namespace Archon

#endif // ARCHON_MATH_INTERSECT_HPP
