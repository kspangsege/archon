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

#include <archon/math/functions.hpp>
#include <archon/raytrace/surface.hpp>


using namespace std;
using namespace Archon::Math;
using namespace Archon::Raytrace;


namespace
{
  struct FeaturelessSurface: Surface
  {
    Vec4 map(Vec2 const &) const
    {
      return color;
    }

    Vec3 shade(Light const *light,
               Vec3 const &normal,
               Vec3 const &viewDirection,
               Vec3 const &lightDirection,
               Vec4 const &surfaceColor) const
    {
      /*
       * Note: Phongs transmission term is not regarded.
       */
      Vec3 h = lightDirection;
      h += viewDirection;
      h /= 2;

      double f = diffuseReflection * dot(lightDirection, normal);
      double s = specularReflection * pow(dot(normal, h), reflectiveExponent);

      Vec3 lightColor = light->getColor();
      return Vec3(lightColor[0] * surfaceColor[0] * f + s,
                  lightColor[1] * surfaceColor[1] * f + s,
                  lightColor[2] * surfaceColor[2] * f + s);
    }

    FeaturelessSurface(Vec4 const &color,
                       double ambientReflection,
                       double diffuseReflection,
                       double specularReflection,
                       double specularRefraction,
                       double reflectiveExponent,
                       double refractiveExponent):
      color(color),
      ambientReflection(ambientReflection),
      diffuseReflection(diffuseReflection),
      specularReflection(specularReflection),
      specularRefraction(specularRefraction),
      reflectiveExponent(reflectiveExponent),
      refractiveExponent(refractiveExponent)
    {
    }

    Vec4 color;
    double ambientReflection;
    double diffuseReflection;
    double specularReflection;
    double specularRefraction;
    double reflectiveExponent;
    double refractiveExponent;
  };
}


namespace Archon
{
  namespace Raytrace
  {
    Surface::ConstRef Surface::newFeaturelessSurface(Vec4 const &color,
                                                     double ambientReflection,
                                                     double diffuseReflection,
                                                     double specularReflection,
                                                     double specularRefraction,
                                                     double reflectiveExponent,
                                                     double refractiveExponent)
    {
      return Surface::ConstRef(new FeaturelessSurface(color,
                                                      ambientReflection,
                                                      diffuseReflection,
                                                      specularReflection,
                                                      specularRefraction,
                                                      reflectiveExponent,
                                                      refractiveExponent));
    }
  }
}
