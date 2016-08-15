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

#ifndef ARCHON_RAYTRACE_LIGHT_HPP
#define ARCHON_RAYTRACE_LIGHT_HPP

#include <archon/math/vector.hpp>


namespace archon
{
  namespace Raytrace
  {
    /**
     * An abstract light source.
     */
    struct Light
    {
      /**
       * Determine the direction towards this light source from the
       * specified point.
       *
       * The raytracer will pass a point that is expressed in global
       * coordinates, and it expects that the returned direction is
       * also expressed in global coordinates, and is a unit vector.
       *
       * This method must be thread-safe.
       */
      virtual math::Vec3 get_direction(math::Vec3 point) const = 0;

      /**
       * Determine the distance between this light source and the
       * specified point, and also the attenuation at the specified
       * point.
       *
       * The raytracer will pass a point that is expressed in global
       * coordinates, and it expects that the returned distance is
       * expressed relative to the global coordinate system too. If
       * the returned distance is negative, the raytracer will
       * interpret it as a light source that is infinitely far away.
       *
       * The returned attenuation shall be interpreted as a scaling
       * factor for the intencity if this light source.
       *
       * This method must be thread-safe.
       */
      virtual void get_distance_and_attenuation(math::Vec3 point, double &distance,
                                                double &attenuation) const = 0;

      virtual void get_specs(math::Vec3 &color, double &ambience, double &intencity) const = 0;

      virtual ~Light() {}
    };




    struct StandardLight: Light
    {
      /**
       * The position must be specified in global coordinates.
       */
      StandardLight(math::Vec3 col, double ambi, double inten):
        color(col), ambience(ambi), intencity(inten) {}

      void get_specs(math::Vec3 &color, double &ambience, double &intencity) const;

    protected:
      math::Vec3 const color;
      double const ambience, intencity;
    };




    struct DirectionalLight: StandardLight
    {
      /**
       * The direction must be specified in global coordinates, and it
       * must be a unit vector.
       */
      DirectionalLight(math::Vec3 dir, math::Vec3 col = math::Vec3(1),
                       double ambi = 0, double inten = 1):
        StandardLight(col, ambi, inten), direction(dir) {}

      math::Vec3 get_direction(math::Vec3 point) const;

      void get_distance_and_attenuation(math::Vec3 point, double &distance,
                                        double &attenuation) const;

    private:
      math::Vec3 const direction;
    };




    struct PointLight: StandardLight
    {
      /**
       * The position must be specified in global coordinates.
       */
      PointLight(math::Vec3 pos, math::Vec3 col = math::Vec3(1), double ambi = 0, double inten = 1,
                 math::Vec3 atten = math::Vec3(1,0,0)):
        StandardLight(col, ambi, inten), position(pos), attenuation(atten) {}

      math::Vec3 get_direction(math::Vec3 point) const;

      void get_distance_and_attenuation(math::Vec3 point, double &distance,
                                        double &attenuation) const;

    private:
      math::Vec3 const position;
      math::Vec3 const attenuation;
    };




    struct SpotLight: StandardLight
    {
      /**
       * Both the position and the direction must be specified in
       * global coordinates. The direction must be a unit vector.
       */
      SpotLight(math::Vec3 pos, math::Vec3 dir, double cutoff = M_PI/4, double hotspot = M_PI/2,
                math::Vec3 col = math::Vec3(1), double ambi = 0, double inten = 1,
                math::Vec3 atten = math::Vec3(1,0,0)):
        StandardLight(col, ambi, inten), position(pos), direction(dir),
        cutoff_angle(cutoff), hotspot_angle(hotspot), attenuation(atten) {}

      math::Vec3 get_direction(math::Vec3 point) const;

      void get_distance_and_attenuation(math::Vec3 point, double &distance,
                                        double &attenuation) const;

    private:
      math::Vec3 const position;
      math::Vec3 const direction;
      double const cutoff_angle, hotspot_angle;
      math::Vec3 const attenuation;
    };
  }
}

#endif // ARCHON_RAYTRACE_LIGHT_HPP
