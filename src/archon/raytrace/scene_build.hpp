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

#ifndef ARCHON_RAYTRACE_SCENE_BUILD_HPP
#define ARCHON_RAYTRACE_SCENE_BUILD_HPP

#include <cmath>
#include <memory>
#include <string>

#include <archon/math/rotation.hpp>
#include <archon/graphics/scene_builder.hpp>
#include <archon/raytrace/raytracer.hpp>


namespace archon {
namespace raytrace {

class SceneBuilder;

std::unique_ptr<SceneBuilder>
make_scene_builder(Raytracer&, graphics::SpatialSceneBuilder* aux_builder = nullptr);


/// A tool to help build new scenes for raytracing.
///
/// The methods of this class are not thread-safe.
class SceneBuilder {
public:
    virtual void translate(math::Vec3) = 0;

    void scale(double f);

    virtual void scale(math::Vec3) = 0;

    /// \param axis Need not be of unit length.
    ///
    /// \param angle Specified in radians.
    void rotate(math::Vec3 axis, double angle);

    virtual void rotate(math::Rotation3) = 0;

    virtual void push() = 0;

    virtual void pop() = 0;

    virtual void add_box() = 0;

    virtual void add_cone() = 0;

    virtual void add_cylinder() = 0;

    virtual void add_sphere() = 0;

    virtual void add_torus(double minor_radius = 0.5) = 0;

    virtual void add_directional_light() = 0;

    virtual void add_point_light() = 0;

    virtual void add_spot_light(double cutoff_angle = M_PI/4, double hotspot_angle = M_PI/2) = 0;

    virtual void set_material_diffuse_color(math::Vec3 color) = 0;

    virtual void set_material_transparency(double transparency) = 0;

    /// Passing the empty string will disable texturing.
    virtual void set_texture(std::string image_path = "",
                             bool repeat_s = true, bool repeat_t = true) = 0;

    virtual void tex_translate(math::Vec2 v) = 0;

    void tex_scale(double);

    virtual void tex_scale(math::Vec2 s) = 0;

    /// Specified in radians.
    virtual void tex_rotate(double angle) = 0;

    virtual void reset_tex_transform() = 0;

    /// Default is 'white'.
    virtual void set_light_color(math::Vec3) = 0;

    /// Default is 0.
    virtual void set_light_ambience(double) = 0;

    /// Default is 1.
    virtual void set_light_intencity(double) = 0;

    /// The scaling factor is 1/(constant + linear*d + quadratic*d^2) where \c d
    /// is the distance between the light source and the shaded surface point
    /// measured with respect to the global coordinate system.
    ///
    /// The default is 1,0,0, e.i. no attenuation.
    virtual void set_light_attenuation(double constant, double linear,
                                       double quadratic) = 0;

    virtual ~SceneBuilder() {}
};




// Implementation

inline void SceneBuilder::scale(double f)
{
    scale(math::Vec3(f));
}

inline void SceneBuilder::rotate(math::Vec3 axis, double angle)
{
    rotate(math::Rotation3(unit(axis), angle));
}

inline void SceneBuilder::tex_scale(double f)
{
    tex_scale(math::Vec2(f));
}

} // namespace raytrace
} // namespace archon

#endif // ARCHON_RAYTRACE_SCENE_BUILD_HPP
