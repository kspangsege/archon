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

#ifndef ARCHON_RAYTRACE_RAYTRACER_HPP
#define ARCHON_RAYTRACE_RAYTRACER_HPP

#include <memory>

#include <archon/math/coord_system.hpp>
#include <archon/util/progress.hpp>
#include <archon/image/image.hpp>
#include <archon/raytrace/object.hpp>
#include <archon/raytrace/light.hpp>


namespace archon {
namespace Raytrace {

class Raytracer;

/// Get a new ray tracer object.
std::unique_ptr<Raytracer> make_raytracer();

/// This class implements a ray tracer.
///
/// The methods of this class need not be tread safe.
class Raytracer {
public:
    virtual int make_transform(const math::CoordSystem3&) = 0;

    virtual void add_object(std::unique_ptr<Object> obj, int transform) = 0;

    virtual void add_light(std::unique_ptr<Light> light) = 0;

    virtual void set_background_color(math::Vec4 rgba) = 0;

    virtual void set_global_ambience(double intencity) = 0;

    /// Render the currently loaded scene to the specified image.
    virtual void render(Imaging::Image::RefArg img, math::Vec3 eye, math::CoordSystem3x2 screen,
                        util::ProgressTracker *tracker, int supersampling_level = 0) const = 0;

    virtual ~Raytracer() {}
};

} // namespace Raytrace
} // namespace archon

#endif // ARCHON_RAYTRACE_RAYTRACER_HPP
