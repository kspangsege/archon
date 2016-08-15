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

#include <utility>
#include <vector>
#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/math/coord_system.hpp>
#include <archon/util/perspect_proj.hpp>
#include <archon/util/ticker.hpp>
#include <archon/image/writer.hpp>
#include <archon/raytrace/raytracer.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::Raytrace;


/*

Box and Sphere intersection computation is in half changed state over in math/geometry.[HC]!!!!!!!!!!!!!!!

Transmission and overlapping solid objects:

At all times, we need to know exactly which objects the
next section of the ray is propagating through. This is in
general some subset of all solid objects.

Assume we start at a point that is outside all solid objects.

For each object, find intersection at lowest strictly positive position on ray. (intersect_enter)

Choose one of the objects with lowest position on ray.

Generate new refracted transmitted ray.

First, find intersection with self. (intersect_exit)

If none, then then we assume an immediate exit (for example, an infinitely thin object).

Else we mark the chosen object as one that we are inside. We then need to check if the transmitted ray enters other overlapping solid objects before it leaves this one.

We could precompute for each object, the list of possibly overlapping objects. The test could be based on some sort of bounding volume intersection test.

Types of intersections:
none
enter - for solid object
exit - for solid objects
through - for infinitely thin objects

If we assume that there is only solid objects:

Provide for each object an intersect method that will determine not only whether there is an intersection and the distance to it, but also, when there is, whether it is an entry into, or an exit from the interiour of the object.

Provide another method that tests for intersection, given that the ray originates on the surface of the object itself, and propagates away on the outside. (intersect_enter) Convex objects would always return false.

Provide another method that tests for intersection, given that the ray originates on the surface of the object itself, and propagates away on the inside. (intersect_exit)

All three intersect methods, must return strictly positive distances, which are intepreted relative to the ray direction vector.

General step with ray origin on object surface:

If we entered into the object, call intersect_exit for this object. Otherwise call intersect_enter for this object.

Call the general intersect for all other objects.

If for any other object, the event is 'exit', and that object was marked as one that we were outside, then mark the object as one that we entered at the origin point of the current ray.

If for any other object, the event is 'enter', and that object was marked as one that we were inside, then mark the object as one that we exited at the origin point of the current ray.

Among all the solid objects, it is the one with the highest rank (last added) whose refractive index BAD BAD BAD, we cannot have the refractive index decided after the ray has propagated.

Entry case: YES, maybe like this: Generate a new refracted ray based on chosen/assumed closest intersection. Then if an object with higher rank turns out to also have been entered, restart with new ray refraction. The problem is that the new refraction angle may cause the ray to not enter the higher rank object after all. It is an astable/bistable situation. Also, this scheme does not offer a solution for simultaneous crossing multiple infinitely thing objects - some objects may get missed, which is bad, if the missed object had higher rank than the selected one.

This scheme also allows for some level of CSG (constructive solid geometry) - one can for example make holes in a slab using a transparent cylinder with higher rank than the slab.

*/


namespace {

class Transform {
public:
    Transform(const CoordSystem3& s):
        inv(s)
    {
        inv.inv();
    }

    CoordSystem3 inv; // Describes the reference coordinate system in local coordinates.
    std::vector<const Object*> objects;
};


class RaytracerImpl: public Raytracer {
public:
    int make_transform(const CoordSystem3& s) override
    {
        int i = m_transforms.size();
        m_transforms.push_back(Transform(s));
        return i;
    }

    void add_object(std::unique_ptr<Object> obj, int transform) override
    {
        m_object_owner.reserve(m_object_owner.size() + 1);
        m_transforms.at(transform).objects.push_back(obj.get());
        m_object_owner.push_back(std::move(obj));
    }

    void add_light(std::unique_ptr<Light> light) override
    {
        m_lights.push_back(std::move(light));
    }

    void set_background_color(Vec4 rgba) override
    {
        m_background_color = rgba;
    }

    void set_global_ambience(double intencity) override
    {
        m_global_ambience = intencity;
    }

    void render(Image::RefArg img, Vec3 eye, CoordSystem3x2 screen,
                ProgressTracker *tracker, int supersampling_level) const override;

    /// Must be thread-safe.
    void trace(const Line3& ray, const Object* origin_obj, Vec4& color,
               std::vector<Material::LightInfo>& light_info) const;

    /// Must be thread-safe.
    bool eclipsed(const Line3& ray, double dist, const Object* object) const;

    Vec4 m_background_color{0};
    double m_global_ambience = 0.2;

    std::vector<std::unique_ptr<Object>> m_object_owner;

    std::vector<Transform> m_transforms;

    std::vector<std::unique_ptr<Light>> m_lights;
};


/*
void render(Image::RefArg img)
{
    int w = img->get_width(), h = img->get_height();

    int s = 32;

    int n = w / s, m = h / s;

    // Divide image into 32x32 blocks. Each block is a work unit.

    // Create a number of worker threads and the calling thread will be a
    // master. The master will submit a work unit to an idle thread along with a
    // buffer. When the worker is done, it passes the filled buffer back to the
    // master.
}
*/


void RaytracerImpl::render(Image::RefArg img, Vec3 eye, CoordSystem3x2 screen,
                           ProgressTracker* tracker, int supersampling_level) const
{
    int w = img->get_width(), h = img->get_height();
    int n = 1 << supersampling_level;

    ImageWriter writer(img);
    ProgressTicker ticker(tracker, long(w)*h);
    std::vector<Material::LightInfo> light_info;

    screen.origin -= eye;

    // Make the unit square correspond to the lower left pixel rather than the
    // whoe screen.
    screen.basis.scale(Vec2(1.0/w, 1.0/h) / double(n));

    // Move it half a pixel up and to the right such that (0,0) is the center of
    // the lower left pixel. This compensates for the fact that we are referring
    // to the lower left corder of each pixel while tracing below.
    screen.translate(Vec2(0.5));

    double f = 1.0 / n / n;

    for (int y = 0; y < h; ++y) {
        long y2 = y * long(n);
        for (int x = 0; x < w; ++x) {
            long x2 = x * long(n);
            Vec4 accum(0);
            for (int i = 0; i < n; ++i) {
                double y3 = y2 + i;
                for (int j = 0; j < n; ++j) {
                    double x3 = x2 + j;
                    Vec4 color;
                    trace(Line3(eye, unit(screen(Vec2(x3, y3)))), 0, color, light_info);
                    accum += color;
                }
            }

            accum *= f;

            writer.set_pos(x,y);
            writer.put_pixel_rgb(accum);

            ticker.tick();
        }
    }
}


// The ray must always be expressed relative to the global coordinate system,
// and the ray direction vector must be of unit length.
void RaytracerImpl::trace(const Line3& ray, const Object* origin_obj, Vec4& color,
                          std::vector<Material::LightInfo>& light_info) const
{
    // Find the closest geometry intersection
    double dist = std::numeric_limits<double>::infinity();
    const Surface* surface = nullptr;
    const Object* object = nullptr;
    decltype(m_transforms)::const_iterator transform;
    auto transforms_end = m_transforms.end();
    for (auto i = m_transforms.begin(); i != transforms_end; ++i) {
        Line3 r = ray;
        r.pre_mult(i->inv);
        for (const Object* obj: i->objects) {
            double d;
            const Surface* s;
            if (!obj->intersect(r, d, origin_obj, &s))
                continue;
            if (d < dist) {
                dist      = d;
                object    = obj;
                surface   = s;
                transform = i;
            }
        }
    }

    // If the ray hits nothing, we think of the light as coming from the
    // "background" and therefore default to a fixed background color.
    if (!object) {
        color = m_background_color;
        return;
    }

    // Determine the intersection point in global coordinates.
    Vec3 point = ray.origin + dist * ray.direction;

    // Fetch surface properties from the intersected object-part
    Vec3 normal;
    Vec2 tex_point;
    // FIXME: Pass tex_point to map() when, but only when texture mapping is
    // effectively enabled for this particular object.
    surface->map(transform->inv(point), normal, &tex_point);

    // So far the normal is expressed in local object coordinates. To get a
    // description in global coordinates, it must be mapped through the
    // transpose of the inverse of the basis of the description of the object
    // coordinate system.
    normal = unit(normal * transform->inv.basis);

    // Gather information about all non-eclipsed light sources
    light_info.clear();
    for (const std::unique_ptr<Light>& light: m_lights) {
        // When testing for eclipsed light sources we generate a ray from the
        // intersection point towards the light source, and test for
        // intersection between that line and all the geometry in the world. We
        // can terminate the check imediately, though, as soon as we encounter
        // an intersection, because this guarantees eclipse.
        Vec3 light_dir = light->get_direction(point);

        // Since in practice it always is so, that the intersected object
        // shadows at least half of the world space from the point of view of
        // the intersection point, therefore it generally pays to check for
        // eclipse by this object before checking other objects.
        if (dot(normal, light_dir) < 0)
            continue;

        double light_dist, attenuation;
        light->get_distance_and_attenuation(point, light_dist, attenuation);
        if (attenuation <= 0)
            continue;

        // Check for eclipse by other objects.
        if (eclipsed(Line3(point, light_dir), light_dist, object))
            continue;

        Vec3 light_color;
        double ambience, intencity;
        light->get_specs(light_color, ambience, intencity);
        light_info.push_back(Material::LightInfo(light_dir, light_color,
                                                 ambience  * attenuation,
                                                 intencity * attenuation));
    }

    Material* material = surface->get_material();
    material->shade(tex_point, normal, -ray.direction, light_info, m_global_ambience, color);
}


// The ray must always be expressed relative to the global coordinate
// system. Unless it is negative, the distance must be expressed relative to the
// ray direction vector. A negative distance indicates a light source at
// infinite distance.
bool RaytracerImpl::eclipsed(const Line3& ray, double dist, const Object* object) const
{
    for (const Transform& transform: m_transforms) {
        Line3 r = ray;
        r.pre_mult(transform.inv);
        for (const Object* obj: transform.objects) {
            double d;
            if (obj->intersect(r, d, object) && (dist < 0 || d < dist))
                return true;
        }
    }
    return false;
}

} // unnamed namespace


namespace archon {
namespace Raytrace {

std::unique_ptr<Raytracer> make_raytracer()
{
    return std::make_unique<RaytracerImpl>();
}

} // namespace Raytrace
} // namespace archon
