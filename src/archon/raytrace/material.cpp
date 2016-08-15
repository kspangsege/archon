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

#include <cmath>

#include <archon/math/matrix_adapt.hpp>
#include <archon/raytrace/material.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Math;

/*

A texture with translucency applied to a solid object must be thought of as painting on the surface only, there is no effect on the interior of the object which is to be considered completely transparent.

Only alpha component is specified by a material, which directly determines the alpha of the result of the lighting calculation.

See http://glprogramming.com/red/chapter05.html

And see http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/Part01/components/lighting.html#Lightsourcesemantics

Want to emulate OpenGL modulate mode

*/

namespace archon
{
  namespace Raytrace
  {
    SharedPtr<Material> Material::get_default()
    {
      static SharedPtr<Material> m(new PhongMaterial());
      return m;
    }


    /**
     * Modeled after X3D.
     *
     * \sa http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/Part01/components/lighting.html#Lightingequations
     */
    void PhongMaterialBase::shade(Vec2 texture_point, Vec3 normal, Vec3 view_dir,
                                  vector<LightInfo> const &lights, double global_ambience,
                                  Vec4 &rgba) const
    {
      Vec4 diffuse_color;
      get_diffuse_color(texture_point, diffuse_color);
      double const exponent = shininess * 128;
      Vec3 color = emissive_color + global_ambience * ambience * diffuse_color.slice<3>();
      typedef vector<LightInfo>::const_iterator light_iter;
      light_iter const lights_end = lights.end();
      for(light_iter i = lights.begin(); i!=lights_end; ++i)
      {
        double const diffuse_factor = dot(normal, i->direction);
        double const specular_factor = pow(dot(normal, unit(view_dir + i->direction)), exponent);
        for(int j=0; j<3; ++j)
        {
          double const ambient  = i->ambience  *  diffuse_color[j] * ambience;
          double const diffuse  = i->intencity *  diffuse_color[j] * diffuse_factor;
          double const specular = i->intencity * specular_color[j] * specular_factor;;
          color[j] += i->color[j] * (ambient + diffuse + specular);
        }
      }

      rgba.set(color[0], color[1], color[2], diffuse_color[3]);
    }
  }
}
