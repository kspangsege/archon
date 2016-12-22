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

#include <cmath>

#include <archon/math/matrix_adapt.hpp>
#include <archon/raytrace/material.hpp>


using namespace archon::core;
using namespace archon::math;

/*

A texture with translucency applied to a solid object must be thought of as painting on the surface only, there is no effect on the interior of the object which is to be considered completely transparent.

Only alpha component is specified by a material, which directly determines the alpha of the result of the lighting calculation.

See http://glprogramming.com/red/chapter05.html

And see http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/Part01/components/lighting.html#Lightsourcesemantics

Want to emulate OpenGL modulate mode

*/

namespace archon {
namespace raytrace {

std::shared_ptr<Material> Material::get_default()
{
    static std::shared_ptr<Material> m = std::make_shared<PhongMaterial>();
    return m;
}


/// Modeled after X3D.
///
/// \sa http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/Part01/components/lighting.html#Lightingequations
void PhongMaterialBase::shade(Vec2 texture_point, Vec3 normal, Vec3 view_dir,
                              const std::vector<LightInfo>& lights, double global_ambience,
                              Vec4& rgba) const
{
    Vec4 diffuse_color;
    get_diffuse_color(texture_point, diffuse_color);
    double exponent = m_shininess * 128;
    Vec3 color = m_emissive_color + global_ambience * m_ambience * diffuse_color.slice<3>();
    for (const LightInfo& light: lights) {
        double diffuse_factor = dot(normal, light.direction);
        double specular_factor = std::pow(dot(normal, unit(view_dir + light.direction)), exponent);
        for (int i = 0; i < 3; ++i) {
            double ambient  = light.ambience  *  diffuse_color[i] * m_ambience;
            double diffuse  = light.intencity *  diffuse_color[i] * diffuse_factor;
            double specular = light.intencity * m_specular_color[i] * specular_factor;;
            color[i] += light.color[i] * (ambient + diffuse + specular);
        }
    }

    rgba.set(color[0], color[1], color[2], diffuse_color[3]);
}

} // namespace raytrace
} // namespace archon
