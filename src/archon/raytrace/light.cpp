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
#include <algorithm>

#include <archon/core/functions.hpp>
#include <archon/raytrace/light.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Math;
using namespace archon::Raytrace;


namespace archon
{
  namespace Raytrace
  {
    void StandardLight::get_specs(Vec3 &col, double &ambi, double &inten) const
    {
      col   = color;
      ambi  = ambience;
      inten = intencity;
    }



    Vec3 DirectionalLight::get_direction(Vec3) const
    {
      return -direction;
    }

    void DirectionalLight::get_distance_and_attenuation(Vec3, double &dist, double &atten) const
    {
      dist  = -1;
      atten = 1;
    }



    Vec3 PointLight::get_direction(Vec3 point) const
    {
      return unit(position - point);
    }

    void PointLight::get_distance_and_attenuation(Vec3 p, double &dist, double &atten) const
    {
      double const d = len(position - p);
      dist  = d;
      atten = 1 / max(attenuation[0] + attenuation[1]*d + attenuation[2]*square(d), 1.0);
    }



    Vec3 SpotLight::get_direction(Vec3 point) const
    {
      return unit(position - point);
    }

    void SpotLight::get_distance_and_attenuation(Vec3 p, double &dist, double &atten) const
    {
      Vec3   const diff = p - position;
      double const d = len(diff);
      double const a = acos(dot(diff, direction)/d);
      double const a0 = clamp(cutoff_angle,  0.0, M_PI/2);
      double const a1 = clamp(hotspot_angle, 0.0, M_PI/2);
      double const f = a0<a ? 0 : a<a1 ? 1 : (a-a0) / (a1-a0);
      dist  = d;
      atten = f / max(attenuation[0] + attenuation[1]*d + attenuation[2]*square(d), 1.0);
    }
  }
}
