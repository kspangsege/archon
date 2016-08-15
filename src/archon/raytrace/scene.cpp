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

#include <vector>
#include <archon/core/memory.hpp>
#include <archon/math/coord_system.hpp>
//#include <archon/image/color.hpp>
#include <archon/raytrace/scene.hpp>


//#include <iostream>


using namespace std;
using namespace archon::core;
using namespace archon::math;
//using namespace archon::image;
using namespace archon::raytrace;


namespace
{
  struct Object
  {
    virtual UniquePtr<Object> clone() const = 0;
  };



  struct SceneImpl: Scene
  {
    void translate(Vec3 const &v)
    {
      coord_system.translate(v);
    }

    void add_sphere(double radius)
    {
      UniquePtr<Object> obj(new_sphere(radius).release());
      add_object(obj);
    }

    void add_point_light(Vec3 const &/*color*/)
    {
    }

    UniquePtr<Sphere> new_sphere(double radius)
    {
      UniquePtr<Sphere> obj(new Sphere(radius));
      return obj;
    }

    void add_object(UniquePtr<Object> obj)
    {
      objects.push_back(obj);
    }

    CoordSystem3 coord_system;
    DeletingVector<Object> objects;
  };


/*
    Vec4 trace(Line3 const &ray, Object const *originObject) const
    {
      // Find the closest geometry intersection
      double dist;
      Object::Part const *part;
      Object const *object = 0;
      for(vector<Object::ConstRef>::const_iterator i = objects.begin(),
            j = objects.end(); i != j; ++i)
      {
        double d;
        Object::Part const *p;
        if(!(*i)->intersect(ray, originObject, d, &p) || object && d <= dist) continue;
        dist   = d;
        object = i->get();
        part   = p;
      }

cerr << "dist=" << dist << endl;

      // If the ray hits nothing, we think of the light as coming from
      // the "background" and therefore default to a fixed background
      // color.
      if(!object) return backgroundColor;

      // Determine the intersection point
      Vec3 intersection = ray.direction;
      intersection *= dist;
      intersection += ray.origin;

cerr << "intersection=" << intersection << endl;

      // Fetch surface properties from the intersected object-part
      Surface const *surface = part->getSurface();
      Vec3 normal;
      Vec2 texturePoint;
      part->map(intersection, normal);
      Vec4 surfaceColor = surface->map(texturePoint);

cerr << "normal=" << normal << endl;
cerr << "surfaceColor=" << surfaceColor << endl;

      // Add up color contributions from visible light sources.
      Vec3 color = Vec3::zero();
      for(vector<Light::ConstRef>::const_iterator i = lights.begin(),
            j = lights.end(); i != j; ++i)
      {
        // When testing for eclipsed light sources we generate a ray
        // from the intersection point towards the light source, and
        // test for intersection between that line and all the
        // geometry in the world. We can terminate the check
        // imediately, though, as soon as we encounter an
        // intersection, because this guarantees eclipse.
	Vec3 lightDirection = (*i)->getPosition() - intersection;
	double lightDist = len(lightDirection);
	lightDirection /= lightDist;
	Line3 rayToLight(intersection, lightDirection);

cerr << "lightPosition=" << (*i)->getPosition() << endl;
cerr << "lightDirection=" << lightDirection << endl;

        // Since in practice it always is so, that the intersected
	// object shadows at least half of the world space from the
	// point of view of the intersection point, it generally pays
	// to check for eclipse by this object before checking other
	// objects.
	//
	// Note that this check will be particularly simple for convex
	// objects since in this case, if the angle between the surface
	// normal and the light direction is greater than 90 degrees,
	// then the light source is eclipsed.
	double d;
	if(dot(normal, lightDirection) < 0) continue;

cerr << "*CLICK*" << endl;

	// Check for eclipse by other objects.
	bool eclipsed = false;
	for(vector<Object::ConstRef>::const_iterator k = objects.begin(),
              l = objects.end(); k != l; ++k)
	{
	  if((*k)->intersect(rayToLight, object, d) && d < lightDist)
          {
            eclipsed = true;
            break;
          }
	}
	if(eclipsed) continue;

	// Add color contribution to pixel
	Vec3 c = surface->shade(i->get(), normal,
                                -ray.direction,
                                lightDirection, surfaceColor);
	color[0] += c[0];
	color[1] += c[1];
	color[2] += c[2];
      }

      return Vec4(color, 1);
    }

    Vec4 trace(Line3 const &ray) const
    {
      return trace(ray, 0);
    }

    SceneImpl(): backgroundColor(Vec4(0, 0, 0, 1))
    {
    }

    vector<Object::ConstRef> objects;
    vector<Light::ConstRef>  lights;
    Vec4 backgroundColor;
  };

  struct SceneBuilderImpl: SceneBuilder
  {
    void translate(Vec3 const &translation)
    {
    }

    void rotate(Rotation3 const &rotation)
    {
    }

    void scale(Vec3 const &scaling)
    {
    }

    void addObject(Object::ConstRefArg object)
    {
      scene->objects.push_back(object);
    }

    void addLight(Light::ConstRefArg light)
    {
      scene->lights.push_back(light);
    }

    void setBackgroundColor(Vec4 const &color)
    {
      scene->backgroundColor = color;
    }

    Scene::ConstRef getScene() const
    {
      return scene;
    }

    SceneBuilderImpl(): scene(new SceneImpl())
    {
    }

    CntRef<SceneImpl> scene;
*/

}


namespace archon
{
  namespace raytrace
  {
    UniquePtr<Scene> new_scene()
    {
      UniquePtr<Scene> s(new SceneImpl());
      return s;
    }
  }
}
