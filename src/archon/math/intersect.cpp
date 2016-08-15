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

#include <limits>

#include <archon/math/quartic_solve.hpp>
#include <archon/math/intersect.hpp>

using namespace std;


namespace archon
{
  namespace Math
  {
    bool intersect(Line3 const &ray, Hyperplane3 const &plane, bool front_to_back_only, double *dist)
    {
      double k = dot(plane.normal, ray.direction);
      if(k == 0 || k > 0 && front_to_back_only) return false; // backface culling!!
      if(dist)
      {
	k = (dot(plane.normal, ray.origin) - plane.dist) / -k;
	if(k <= 0) return false; // The ray either originates on the
				 // plane or originates on the same
				 // side as it extends to.
	*dist = k;
      }
      return true;
    }



    /**
     * Ray/cone intersection.
     *
     * Theory:
     *
     * Equation of infinite double cone whos axis of revolution is
     * coincident with the Y-axis and whos apex is at y = h/2 and whos
     * radius in the plane y = -h/2 is r:
     *
     *   x² + z² = (½ - y/h)² r²                                      (1)
     *
     * Equation of ray:
     *
     *   v  =  P + t D
     *
     * where P is the origin and D is the direction of the
     * ray.
     *
     * or
     *
     *   x  =  px + t dx
     *   y  =  py + t dy
     *   z  =  pz + t dz
     *
     * To ease the computation we use the following 3 substitutions:
     *
     *   y'  = (½ -  y/h) r
     *   py' = (½ - py/h) r
     *   dy' =    - dy/h  r
     *
     * This gives us the equation of the standard infinite double cone:
     *
     *   x² + z² = y'²                                                (1)
     *
     * and because
     *
     *   py' + t dy' = (½ - py/h) r - t dy/h r =
     *     (½ - (py + t dy)/h) r = (½ - y/h) r = y'
     *
     * we have:
     *
     *   x  =  px  + t dx
     *   y' =  py' + t dy'                                            (2)
     *   z  =  pz  + t dz
     *
     * Inserting (2) into (1) we get:
     *
     *   (px + t dx)² + (pz + t dz)² = (py' + t dy')²   <=>
     *
     *   px² + t²dx² + 2 px t dx + pz² + t²dz² + 2 pz t dz =
     *     py'² + t²dy'² + 2 py' t dy'   <=>
     *
     *   (dx² + dz² - dy'²) t² + 2 (px dx + pz dz - py' dy') t +
     *     (px² + pz² - py'²) = 0                                     (3)
     *
     * Alas, we need to solve a quadratic equation in t:
     *
     *   a  =  dx² + dz² - dy'² 
     *   b  =  2 (px dx + pz dz - py' dy')
     *   c  =  px² + pz² - py'²
     *   d  =  b² - 4 a c
     *   t = (-b ± sqrt(d)) / 2a;
     *
     * Which is equivalent to this computationally more efficient form:
     *
     *   a' = 2 (dx² + dz² - dy'²) 
     *   b' = -2 (px dx + pz dz - py' dy')
     *   c  = px² + pz² - py'²
     *   d  = b'² - 2 a' c
     *   t  = (b' ± sqrt(d)) / a'   if d >= 0
     *
     * Note that if a' = 0 then equation (3) is in fact not a
     * quadratic equation. If b' != 0 We get:
     *
     *   2 (px dx + pz dz - py' dy') t + (px² + pz² - py'²) = 0
     *
     *           -(px² + pz² - py'²)        c
     *   t = --------------------------- = ---
     *       2 (px dx + pz dz - py' dy')    b'
     *
     * If both a' = 0 and b' = 0 then if c = 0 we have an infinity of
     * solutions and if c != 0 we have no solutions.
     *
     *
     * The discriminating plane:
     *
     * I will now define the discriminating plane for the intersection
     * between a line and an infinite double cone to be the plane that
     * contains both the line and the apex of the double cone. In the
     * special case where the apex of the cone is coincident with the
     * line, the discriminating plane is not defined.
     *
     * The important feature of the discriminating plane is that it
     * help us understand when the line intersects the cone and it
     * gives us a geometrical interpretation of the discriminant d as
     * defined above.
     *
     * Say that the angle between the central axis of the cone and its
     * sides is A. In this case, if the discriminating plane is
     * defined and if the smallest angle between the discriminating
     * plane and the central axis of the cone is strictly less than A,
     * then the line intersects the cone.
     *
     * If the discriminating plane is defined and the smallest angle
     * between the discriminating plane and the central axis of the
     * cone is strictly greater than A, then the line does not
     * intersect the cone.
     *
     * In the special case where the discriminating plane is defined
     * and the smallest angle between the discriminating plane and the
     * central axis of the cone is exactly equal to A, the line
     * generally intersects the cone in a tagential fashion since in
     * this case the discriminating plane is a tangential plane to the
     * cone. The only exception is when the line is parallel to the
     * line arising as the tangential intersection between the plane
     * and the cone.
     *
     * In every case where the discriminating plane is not defined the
     * line intersects with the apex of the cone by definition.
     *
     * If we say that B is the smallest angle between the line and the
     * central axis of the cone, we have the following relationship:
     *
     *   A <= B
     *
     * Now, as announced earlier, there is a simple correspondance
     * between the discriminating plane and the descriminant d of the
     * quadratic equation above. In the case of this method, the apex
     * of the cone is at the origin of the coordinate system, the
     * angle between its central axis and its sides is 45°, and its
     * central axis is coincident with the Y-axis. This means that the
     * normal N of the descriminating plane can be expessed as
     * follows:
     *
     *   N = P × D
     *
     * By expanding the expression of the discriminant and then
     * reducing we get:
     *
     *	 d/4 = (py*dz - pz*dy)² + (px*dy - py*dx)² - (pz*dx - px*dz)²
     *
     * From this we get the following:
     *
     *   d/4 = |Nxz|² - |Ny|²
     *
     * where Ny is the projection of N onto the Y-axis and Nxz is the
     * projection of N onto the X-Z-plane.
     *
     * Ergo, we have the following relationship between the
     * discriminating plane and the discriminant d:
     *
     *   d > 0 The smallest angle between the discriminating plane and
     *         the central axis of the cone is strictly less than A.
     *
     *   d < 0 The smallest angle between the discriminating plane and
     *         the central axis of the cone is strictly greater than A.
     *
     *   d = 0 The smallest angle between the discriminating plane and
     *         the central axis of the cone is exactly equal to A.
     *
     *
     *
     *
     * a' = 0 iff the smallest angle between the ray and the upwards
     * direction is exactly 45 degrees. Alternatively we could say
     * that a' = 0 iff a plane, that contains both the ray and the
     * apex of the cone, intersects the cone surface along a straight
     * line that is paralell with the ray.
     *
     * a > 0 iff the smallest angle between the ray and the upwards direction
     * is greater than 45 degrees. then either the ray passes by the infinite
     * double cone without intersecting it, or it intersects the
     * upper nappe twice or it intersects the lower nappe twice.
     *
     * If a < 0 then the ray has exactly one intersection with the
     * upper nappe of the infinite double cone. It also has exactly
     * one intersection with the lower nappe.
     *
     * b' = 0 iff the ray is tangential to the hyperboloid that
     * contains the ray origin and is asymptotically equal to the cone
     * surface.
     *
     * c = 0 restricts the ray to originate from the surface of the
     * standard infinite double cone with apex at the origin of the
     * coordinate system.
     *
     * If c > 0 then the ray originates outside the infinite double
     * cone.
     *
     * If c < 0 then the ray originates inside the infinite double
     * cone.
     *
     *
     *  a < 0 => d > 0
     *
     *
     * Whenever a' > 0 and d > 0 the ray intersects the infinite
     * double cone twice and either both intersections are with the
     * upper nappe or they are both with the lower one. In this case
     * it holds that:
     *
     *   (b' - sqrt(d)) / a'   <=   (b' + sqrt(d)) / a'
     *
     * Whenever a' < 0 and d > 0 the ray intersects the infinite
     * double cone twice. Once with the upper nappe and once with the
     * lower one. In this case it holds that:
     *
     *   (b' - sqrt(d)) / a'   >=   (b' + sqrt(d)) / a'
     *
     * 
     *
     *
     * Note that 'on geometry surface' is considdered to be 'inside
     * geometry interiour'.
     */
    int intersect_cone(Line3 const &ray, double &dist, double height, double bottom_radius,
                       bool side, bool bottom, bool enter_only)
    {
      Vec3 rd = ray.direction;
      Vec3 rp = ray.origin;

      // Transform the ray into the space where the cone becomes
      // coincident with the upper nappe of the canonical infinite
      // double cone revolving around the y-axis and with apex at the
      // origin.
      rp[1] = (0.5 - rp[1]/height) * bottom_radius;
      rd[1] =      - rd[1]/height  * bottom_radius;

      double a =  2 * (rd[0] * rd[0] + rd[2] * rd[2] - rd[1] * rd[1]);
      double b = -2 * (rd[0] * rp[0] + rd[2] * rp[2] - rd[1] * rp[1]);
      double c =       rp[0] * rp[0] + rp[2] * rp[2] - rp[1] * rp[1];

      if(a > 0)
      {
	double d = b * b - 2 * a * c;

	if(d < 0) return 0; // No real solutions

	double sqrt_d = sqrt(d);

	// Get the smallest of the two sollutions
	double t = (b - sqrt_d) / a;
	// Note that if b < 0 then t < 0 since a > 0

	double y = rp[1] + rd[1] * t;

	// First intersection with infinite extension of the double cone
	// is below the apex, but since a > 0 this means that both
	// intersections occur on the lower nappe. Since the lower nappe
	// is not part of the original cone, it means that there is no
	// intersection in this case.
	if(y < 0) return 0;

	// First intersection with the infinite extension of the double cone is
	// above the cap
	if(y > bottom_radius)
	{
	  double t2 = (b + sqrt_d) / a;
	  if(t2 <= 0) return 0; // No hit since no sollution is positive
	  double y2 = rp[1] + t2 * rd[1];
	  if(y2 > bottom_radius) return 0; // No hit since both intersections occure above the cap

	  // Ray is extending downwards
	  if(bottom)
	  {
	    if(rp[1] > bottom_radius)
	    {
	      // Intersection with cap from outside
	      dist = (bottom_radius - rp[1]) / rd[1];
	      return 2;
	    }
	  }

	  if(enter_only || !side) return 0;
	  // Intersection with side from inside via missing cap
	  dist = t2;
	  return 1;
	}

	// The first intersection with the infinite extension of the
	// cone is between the apex and the cap

	if(side)
	{
	  if(t>0)
	  {
	    // Intersection with side from outside
	    dist = t;
	    return 1;
	  }
	}
	if(enter_only) return 0;

	double t2 = (b + sqrt_d) / a;
	if(t2 <= 0) return 0; // No hit since no sollution is positive
	double y2 = rp[1] + t2 * rd[1];
	if(y2 > bottom_radius)
	{
	  // Ray is extending upwards
	  if(!bottom || rp[1] >= bottom_radius) return 0;
	  // Intersection with cap from inside via side
	  dist = (bottom_radius - rp[1]) / rd[1];
	  return 2;
	}

	if(!side) return 0;

	// Intersection with side from inside via side
	dist = t2;
	return 1;
      }


      // a <= 0

      if(rd[1] > 0)
      {
	// Ray extends upward

	double t;

	if(a == 0)
	{
	  if(b < 0) return 0; // Ray-line hits lower nappe only
	  if(b == 0)
	  {
	    if(c != 0) return 0;
	    t = -rp[1] / rd[1];
	  }
	  else t = c / b;
	}
	else t = (b - sqrt(b * b - 2 * a * c)) / a;

	if(t <= 0)
	{
	  // Ray originates inside the upper nappe
	  if(rp[1] >= bottom_radius) return 0;
	}
	else
	{
	  double y = rp[1] + rd[1] * t;
	  if(y > bottom_radius) return 0; // Intersection is above the cap
	  if(side)
	  {
	    dist = t;
	    return 1;
	  }
	}

	// Intersection with cap from inside via missing side
	if(enter_only || !bottom) return 0;
	dist = (bottom_radius - rp[1]) / rd[1];
	return 2;
      }

      // rd[1] < 0
      // Ray extends downwards

      double t;

      if(a == 0)
      {
	if(b > 0) return 0; // Ray-line hits lower nappe only
	if(b == 0)
	{
	  if(c != 0) return 0;
	  t = -rp[1] / rd[1];
	}
	else t = c / b;
      }
      else t = (b + sqrt(b * b - 2 * a * c)) / a;

      if(t <= 0) return 0; // Ray originates below upper upper nappe
      double y = rp[1] + rd[1] * t;
      if(y > bottom_radius) return 0; // Intersection is above the cap
      if(rp[1] > bottom_radius && bottom)
      {
	dist = (bottom_radius - rp[1]) / rd[1];
	return 2;
      }

      if(enter_only || !side) return 0;
      dist = t;
      return 1;
    }



    /**
     * Ray/cylinder intersection.
     *
     * Theory:
     *
     * Equation of infinite cylinder whos axis of revolution is
     * coincident with the Y-axis:
     *
     *   x² + z² = r²                                                 (1)
     *
     * Equation of ray:
     *
     *   v  =  p + t d
     *
     * or
     *
     *   x  =  px + t dx
     *   y  =  py + t dy                                              (2)
     *   z  =  pz + t dz
     *
     * Inserting (2) into (1) we get:
     *
     *   (px + t dx)² + (pz + t dz)² = r²   <=>
     *
     *   px² + t²dx² + 2 px t dx + pz² + t²dz² + 2 pz t dz = r²   <=>
     *
     *   (dx² + dz²) t² + 2 (px dx + pz dz) t + (px² + pz² - r²) = 0   (3)
     *
     * If p' is the projection of p onto the X-Z-plane and d' is the
     * projection of d then (3) corresponds to
     *
     *   |d'|² t² + 2 p'·d' t + |p'|²-r² = 0
     *
     * Anyway we need to solve a quadratic equation in t:
     *
     *   a  = dx² + dz² 
     *   b  = 2 (px dx + pz dz)
     *   c  = px² + pz² - r²
     *   d  = b² - 4 a c
     *   t  = (-b ± sqrt(d)) / 2a;
     *
     * Which is equivalent to this computationally more efficient form:
     *
     *   a' =  2 (dx² + dz²) 
     *   b' =  -2 (px dx + pz dz)
     *   c  =  px² + pz² - r²
     *   d  =  b'² - 2 a' c
     *   t  = (b' ± sqrt(d)) / a'   if d >= 0
     *
     * Where it always holds that:
     *
     *   (b' - sqrt(d)) / a'   <=   (b' + sqrt(d)) / a'   if d >= 0
     *
     * Note that a' = 0 implies b' = 0. This means that the ray is
     * strictly vertical. In this case (3) degenerates to:
     *
     *   px² + pz² = r²
     *
     * Which means that if the ray originates on the surface of the
     * cylinder then there is an infinity of solutions for
     * t. Otherwise there are no sollutions.
     *
     * Note that 'on geometry surface' is considdered to be 'inside
     * geometry interiour'.
     */
    int intersect_cylinder(Line3 const &ray, double &dist, double height, double radius,
                           bool side, bool top, bool bottom, bool enter_only)
    {
      Vec3 rd = ray.direction;
      Vec3 rp = ray.origin;
      double a =  2 * (rd[0] * rd[0] + rd[2] * rd[2]);
      double b = -2 * (rd[0] * rp[0] + rd[2] * rp[2]);
      double c =      (rp[0] * rp[0] + rp[2] * rp[2]) - radius * radius;
      double d = b * b - 2 * a * c;

      if(d < 0) return 0; // No real solutions

      double sqrt_d = sqrt(d);

      // Get the smallest of the two sollutions
      double t = a==0 ? -numeric_limits<double>::max() : (b - sqrt_d) / a;
      // Note that if b < 0 then t < 0 since a >= 0

      double h = height/2;

      if(h>=0)
      {
	double y = rp[1] + rd[1] * t;

	// First intersection with infinite extension of cylinder is
	// below the bottom cap
	if(y < -h)
	{
	  double t2 = a==0 ? numeric_limits<double>::max() : (b + sqrt_d) / a;
	  if(t2 <= 0) return 0; // No hit since no sollution is positive
	  double y2 = rp[1] + t2 * rd[1];
	  if(y2 < -h) return 0; // No hit since both intersections occure below the bottom cap
	  if(bottom)
	  {
	    if(rp[1] < -h)
	    {
	      // Intersection with bottom cap from outside
	      dist = (-h - rp[1]) / rd[1];
	      return 2;
	    }
	  }
	  if(enter_only) return 0;
	  // Assume non-solid
	  if(y2 <= h)
	  {
	    if(!side) return 0;
	    // Intersection with side from inside via missing bottom cap
	    dist = t2;
	    return 1;
	  }
	  if(!top || rp[1] >= h) return 0;
	  // Intersection with top cap from inside via missing bottom cap
	  dist = (h - rp[1]) / rd[1];
	  return 3;
	}

	// First intersection with infinite extension of cylinder is
	// above the top cap
	if(y > h)
	{
	  double t2 = a==0 ? numeric_limits<double>::max() : (b + sqrt_d) / a;
	  if(t2 <= 0) return 0; // No hit since no sollution is positive
	  double y2 = rp[1] + t2 * rd[1];
	  if(y2 > h) return 0; // No hit since both intersections occure above the top cap
	  if(top)
	  {
	    if(rp[1] > h)
	    {
	      // Intersection with top cap from outside
	      dist = (h - rp[1]) / rd[1];
	      return 3;
	    }
	  }
	  if(enter_only) return 0;
	  // Assume non-solid
	  if(y2 >= -h)
	  {
	    if(!side) return 0;
	    // Intersection with side from inside via missing top cap
	    dist = t2;
	    return 1;
	  }
	  if(!bottom || rp[1] <= -h) return 0;
	  // Intersection with bottom cap from inside via missing top cap
	  dist = (-h - rp[1]) / rd[1];
	  return 2;
	}
      }

      // First intersection with infinite extension of cylinder is
      // between the two caps

      if(side && t>0)
      {
	// Intersection with side from outside
	dist = t;
	return 1;
      }

      if(enter_only) return 0;

      double t2 = a==0 ? numeric_limits<double>::max() : (b + sqrt_d) / a;
      if(t2 <= 0) return 0; // No hit since no sollution is positive
      if(h>=0)
      {
	double y2 = rp[1] + t2 * rd[1];
	if(y2 < -h)
	{
	  if(!bottom || rp[1] <= -h) return 0;
	  // Intersection with bottom cap from inside via side
	  dist = (-h - rp[1]) / rd[1];	  
	  return 2;
	}
	if(y2 > h)
	{
	  if(!top || rp[1] >= h) return 0;
	  // Intersection with top cap from inside via side
	  dist = (h - rp[1]) / rd[1];	  
	  return 3;
	}
      }

      if(!side) return 0;

      // Intersection with side from inside via side
      dist = t2;
      return 1;
    }



    /**
     * Ray/torus intersection:
     *
     * Equation of torus where torus is constrained to be origin
     * centered and lie in the x-z plane:
     *
     *   (sqrt(x² + z²)-R)² + y²  =  r²
     *
     *   x² + z² + R² - 2*R*sqrt(x² + z²) + y²  =  r²
     *
     *   x² + y² + z² + R² - r²  =  2*R*sqrt(x² + z²)
     *
     *   (x² + y² + z² + R² - r²)²  = 4 * R² * (x² + z²)  [1]
     *
     * Equation of ray:
     *
     *   x  =  p[0] + t * d[0]
     *   y  =  p[1] + t * d[1]
     *   z  =  p[2] + t * d[2]
     *
     *   x²  =  p[0]² + t² * d[0]² + 2*t*p[0]*d[0]
     *   y²  =  p[1]² + t² * d[1]² + 2*t*p[1]*d[1]
     *   z²  =  p[2]² + t² * d[2]² + 2*t*p[2]*d[2]
     *
     *   x² + y² + z²  =  p[0]² + p[1]² + p[2]²
     *                 +  t² * (d[0]² + d[1]² + d[2]²)
     *                 +  2*t*(p[0]*d[0] + p[1]*d[1] + p[2]*d[2])
     *
     *                 =  |p|² + t² * |d|² + 2 * t * dot(p, d)
     *
     * Our mission is to obtain the distance 't' along the ray from its
     * origin 'p' to the first intersection point with the torus
     * measured in units of |d|. We cannot assume that the direction
     * vector 'd' has unit length because the local coordinate system
     * that we work in may be scaled and the length we need is the
     * length as observed in the global coordinate system.
     *
     * Left side of [1]:
     *
     *   (x² + y² + z² + R² - r²)²  =
     *
     *   (|p|² + t² * |d|² + 2 * t * dot(p, d) + R² - r²)²  =
     *
     *   (|d|² * t²  +  2 * dot(p, d) * t  +  |p|² + R² - r²)²  =
     *
     *   |d|^4 * t^4  +
     *   4 * dot(p, d)² * t²  +
     *   (|p|² + R² - r²)²  + 
     *   4 * |d|² * dot(p, d) * t³  +
     *   2 * |d|² * (|p|² + R² - r²) * t²  +
     *   4 * dot(p, d) * (|p|² + R² - r²) * t            [2]
     *
     * Right side of [1]:
     *
     *   4 * R² * (x² + z²)  =
     *
     *   4 * R² * (x² + y² + z² - y²)  =
     *
     *   4 * R² * (|p|² + t² * |d|² + 2 * t * dot(p, d) - p[1]² - t² * d[1]² - 2*t*p[1]*d[1])  =
     *
     *   4 * R² * ((|d|² - d[1]²) * t²  +  2 * (dot(p, d) - p[1]*d[1]) * t  +  |p|² - p[1]²)  =
     *
     *   4 * R² * (|d|² - d[1]²) * t²  +
     *   8 * R² * (dot(p, d) - p[1]*d[1]) * t  +
     *   4 * R² * (|p|² - p[1]²)                          [3]
     *
     * Subtracting [3] from [2] we get:
     *
     *   |d|^4 * t^4  +
     *   4 * |d|² * dot(p, d) * t³  +
     *   2 * (2 * dot(p, d)² + |d|² * (|p|² - r²) - R² * (|d|² - 2*d[1]²)) * t²  +
     *   4 * (dot(p, d) * (|p|² - r²) - R² * (dot(p, d) - 2*p[1]*d[1])) * t  +
     *   (|p|² + R² - r²)² - 4 * R² * (|p|² - p[1]²)                    [4]
     *
     * The following substitutions makes it all simpler:
     *
     *   d  =  e*|d|
     *   t  =  u/|d|
     *
     * Then dot(p, d) = |d|*dot(p, e), d[2] = |d|*e[2] and [4] becomes:
     *
     *   u^4  +
     *   4 * dot(p, e) * u³  +
     *   2 * (2 * dot(p, e)² + |p|² - r² - R² * (1 - 2*e[1]²)) * u²  +
     *   4 * (dot(p, e) * (|p|² - r²) - R² * (dot(p, e) - 2*p[1]*e[1])) * u  +
     *   (|p|² + R² - r²)² - 4 * R² * (|p|² - p[1]²)
     *
     * We now have a normalized 4th order polynomial in u whos roots may
     * be found by a standard quartic solver.
     *
     * Primary reference:
     * http://www.cl.cam.ac.uk/Teaching/1999/AGraphHCI/SMAG/node2.html
     */
    bool intersect_torus(Line3 const &ray, double &dist, double major_radius,
                         double minor_radius, bool surface_origin, bool ext_to_int_only)
    {
      double l = len(ray.direction);
      Vec3 d = ray.direction;
      d /= l;
      Vec3 const &p = ray.origin;

      double spx = p[0]*p[0];
      double spy = p[1]*p[1];
      double spz = p[2]*p[2];

      double dpx = d[0]*p[0];
      double dpy = d[1]*p[1];
      double dpz = d[2]*p[2];

      double dp  = dpx + dpy + dpz;

      double sma = major_radius*major_radius;

      double a = spx + spy + spz - minor_radius*minor_radius;
      double b = a + sma;

      double k3 = 4*dp;
      double k2 = 2*(2*dp*dp + a - sma*(1 - 2*d[1]*d[1]));
      double k1 = 4*(dp*a - sma*(dp - 2*dpy));
      double k0 = b*b - 4*sma*(spx + spz);

      // Solve: u^4 + k3 u^3 + k2 u^2 + k1 u + k0 = 0

      double roots[4];
      int num_real_roots =
	Math::quartic_solve(k3, k2, k1, k0, roots);

      if(num_real_roots == 0) return false;

      // If we know that the ray originates from the surface then we
      // know that the root closest to zero should be regarded as zero
      if(surface_origin)
      {
	int min_abs = -1;
	for(int i=0; i<num_real_roots; ++i)
	  if(min_abs == -1 || fabs(roots[i]) < fabs(roots[min_abs])) min_abs = i;
	roots[min_abs] = 0;
      }

      // Get rid of non-positive roots
      int num_positive_roots = 0;
      for(int i=0; i<num_real_roots; ++i)
	if(roots[i] > 0) roots[num_positive_roots++] = roots[i];

      if(num_positive_roots == 0) return false;

      if(ext_to_int_only)
      {
	switch(num_positive_roots)
	{
	case 1:
	  return false;
	case 2:
	  dist = std::min(roots[0], roots[1]) / l;
	  return true;
	case 3:
          // Find the middle root
          if(roots[0] > roots[1]) std::swap(roots[0], roots[1]);
          dist = (roots[2] < roots[0] ? roots[0] :
                  roots[2] > roots[1] ? roots[1] : roots[2]) / l;
	  return true;
	}
      }
      else
      {
	switch(num_positive_roots)
	{
	case 1:
          dist = roots[0] / l;
          return true;
	case 2:
          dist = std::min(roots[0], roots[1]) / l;
          return true;
	case 3:
          dist = std::min(std::min(roots[0], roots[1]), roots[2]) / l;
          return true;
	}
      }

      // num_positive_roots == 4
      dist = std::min(std::min(roots[0], roots[1]), std::min(roots[2], roots[3])) / l;
      return true;
    }
  }
}
