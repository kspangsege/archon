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

#ifndef ARCHON_RAYTRACE_SCENE_HPP
#define ARCHON_RAYTRACE_SCENE_HPP

#include <archon/core/unique_ptr.hpp>
#include <archon/math/vector.hpp>
//#include <archon/math/geometry.hpp>
//#include <archon/math/rotation.hpp>
//#include <archon/raytrace/object.hpp>
//#include <archon/raytrace/light.hpp>


namespace archon
{
  namespace raytrace
  {
    struct Scene;

    /**
     * Get a fresh empty scene object.
     */
    core::UniquePtr<Scene> new_scene();



    /**
     * Represents a complete scene in the form needed by the
     * raytracer. The methods of this class need not be thread-safe.
     */
    struct Scene
    {
      virtual void translate(math::Vec3 const &v) = 0;
      virtual void add_sphere(double radius) = 0;
      virtual void add_point_light(math::Vec3 const &color) = 0;

      virtual ~Scene() {}
    };



      /**
       * Trace the specified ray into the scene and determine its
       * color and opacity.
       *
       * \return The color and opacity of the ray represented as RGBA.
       */
//      virtual math::Vec4 trace(math::Line3 const &ray) const = 0;


    /**
     * A helper class for building scenes. The methods of this class
     * should not be considered thread safe.
     */
      /**
       * Get a new scene builder. There is no initial coordinate
       * transformation.
       */
/*
      static Ref newSceneBuilder();

      virtual void translate(math::Vec3 const &translation) = 0;

      virtual void rotate(math::Rotation3 const &rotation) = 0;

      virtual void scale(math::Vec3 const &scaling) = 0;
*/
      /**
       * Add the specified object whose geometry is interpreted
       * relative to the currently effective coordinate
       * transformation.
       */
//      virtual void addObject(Object::ConstRefArg object) = 0;

      /**
       * Add the specified light source whose position is interpreted
       * relative to the currently effective coordinate
       * transformation.
       */
//      virtual void addLight(Light::ConstRefArg light) = 0;

      /**
       * Set the background color for the scene. This color is used
       * for rays that does not intersect any geometry.
       */
//      virtual void setBackgroundColor(math::Vec4 const &color) = 0;

      /**
       * Get the scene built so far.
       */
//      virtual SceneX::ConstRef getScene() const = 0;

  }
}

#endif // ARCHON_RAYTRACE_SCENE_HPP
