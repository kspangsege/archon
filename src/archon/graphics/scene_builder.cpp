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
#include <vector>

#include <archon/math/vector.hpp>
#include <archon/graphics/scene_builder.hpp>


using namespace std;
using namespace Archon::Math;
using namespace Archon::Graphics;


namespace {

inline void add_axis_permuted_vertex(SpatialObjectBuilder &builder, int orient,
                                     double a, double b, double c)
{
    switch (orient) {
        case 0: // front
            builder.add_vertex(a,b,c);
            break;
        case 1: // back
            builder.add_vertex(b,a,c);
            break;
        case 2: // right
            builder.add_vertex(c,a,b);
            break;
        case 3: // left
            builder.add_vertex(c,b,a);
            break;
        case 4: // top
            builder.add_vertex(b,c,a);
            break;
        case 5: // bottom
            builder.add_vertex(a,c,b);
            break;
    }
}


/// Build a planar axis-aligned mesh that is perpendicular to one of the
/// coordinate axes. It faces in one direction if the number of interval
/// revarsals (to < from) is even, and in the opposite direction if it is odd.
///
/// <pre>
///
///    orient   a   b   c    facing (even/odd)
///   ------------------------------------------
///      0      x   y   z    front  / back
///      1      y   x   z    back   / front
///      2      y   z   x    right  / left
///      3      z   y   x    left   / right
///      4      z   x   y    top    / bottom
///      5      x   z   y    bottom / top
///
/// </pre>
///
/// The specified a-range is always mapped to the interval [0;1] on the primary
/// texture axis (S). Likewise, the b-range is mapped to the secondary texture
/// axis (T).
void build_mesh(SpatialObjectBuilder& builder, bool texture, int orient,
                double a_from, double a_to, double b_from, double b_to,
                double c, int a_steps, int b_steps)
{
    double a_diff = a_to - a_from, b_diff = b_to - b_from;
    double s_step = 1.0 / a_steps, t_step = 1.0 / b_steps;
    double r = a_diff<0 == b_diff<0 ? 1 : -1;
    switch (orient) {
        case 0: // front
            builder.set_normal(  0,  0,  r );
            break;
        case 1: // back
            builder.set_normal(  0,  0, -r );
            break;
        case 2: // right
            builder.set_normal(  r,  0,  0 );
            break;
        case 3: // left
            builder.set_normal( -r,  0,  0 );
            break;
        case 4: // top
            builder.set_normal(  0,  r,  0 );
            break;
        case 5: // bottom
            builder.set_normal(  0, -r,  0 );
            break;
    }
    for (int i = 0; i < a_steps; ++i) {
        double s0 = s_step * (0+i), s1 = s_step * (1+i);
        double a0 =                       a_from + s0*a_diff;
        double a1 = 1+i==a_steps ? a_to : a_from + s1*a_diff;
        builder.begin_quad_strip();
        for (int j = 0; j <= b_steps; ++j) {
            double t = t_step * j;
            double b = j==b_steps ? b_to : b_from + t*b_diff;
            if (texture)
                builder.set_tex_coord(s0, t);
            add_axis_permuted_vertex(builder, orient, a0, b, c);
            if (texture)
                builder.set_tex_coord(s1, t);
            add_axis_permuted_vertex(builder, orient, a1, b, c);
        }
        builder.end();
    }
}

} // unnamed namespace


namespace Archon {
namespace Graphics {

void build_xyz_mesh(SpatialObjectBuilder& builder, bool texture, bool reverse,
                    double x0, double x1, double y0, double y1, double z,
                    int x_steps, int y_steps)
{
    if (reverse) {
        build_mesh(builder, texture, 1, y0, y1, x0, x1, z, y_steps, x_steps);
    }
    else {
        build_mesh(builder, texture, 0, x0, x1, y0, y1, z, x_steps, y_steps);
    }
}


void build_yzx_mesh(SpatialObjectBuilder& builder, bool texture, bool reverse,
                    double y0, double y1, double z0, double z1, double x,
                    int y_steps, int z_steps)
{
    if (reverse) {
        build_mesh(builder, texture, 3, z0, z1, y0, y1, x, z_steps, y_steps);
    }
    else {
        build_mesh(builder, texture, 2, y0, y1, z0, z1, x, y_steps, z_steps);
    }
}


void build_zxy_mesh(SpatialObjectBuilder& builder, bool texture, bool reverse,
                    double z0, double z1, double x0, double x1, double y,
                    int z_steps, int x_steps)
{
    if (reverse) {
        build_mesh(builder, texture, 5, x0, x1, z0, z1, y, x_steps, z_steps);
    }
    else {
        build_mesh(builder, texture, 4, z0, z1, x0, x1, y, z_steps, x_steps);
    }
}


void build_box(SpatialObjectBuilder& builder, bool texture,
               bool front, bool back, bool right, bool left, bool top, bool bottom,
               int x_steps, int y_steps, int z_steps)
{
    if (front)  //  X,  Y
        build_xyz_mesh(builder, texture, false, -1,  1, -1,  1,  1, x_steps, y_steps);
    if (back)   // -X,  Y
        build_xyz_mesh(builder, texture, false,  1, -1, -1,  1, -1, x_steps, y_steps);
    if (right)  // -Z,  Y
        build_yzx_mesh(builder, texture, true,  -1,  1,  1, -1,  1, y_steps, z_steps);
    if (left)   //  Z,  Y
        build_yzx_mesh(builder, texture, true,  -1,  1, -1,  1, -1, y_steps, z_steps);
    if (top)    //  X, -Z
        build_zxy_mesh(builder, texture, true,   1, -1, -1,  1,  1, z_steps, x_steps);
    if (bottom) //  X,  Z
        build_zxy_mesh(builder, texture, true,  -1,  1, -1,  1, -1, z_steps, x_steps);
}


void build_cone(SpatialObjectBuilder& builder, bool texture, bool bottom,
                int azimuth_steps, int height_steps, int radial_steps)
{
    vector<Vec2> roots(azimuth_steps);
    {
        double f = 2*M_PI / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i].set(-cos(a), -sin(a)); // z,x
        }
    }

    // FIXME: If azimuth_steps < height_steps, it would be better to run the
    // quad-strips in the axial direction rather than the angular direction. The
    // actual break-even point can be determined counting total number of calls
    // to set_normal(), set_tex_coord(), and add_vertex(). A similar situation
    // exists for the caps.

    // Side
    {
        double f = 1.0 / height_steps;
        double g = 1.0 / sqrt(5);
        for (int i = 0; i < height_steps; ++i) {
            double t1 = f * (0+i), t2 = f * (1+i);
            double y1 = 2*t1 - 1, y2 = 2*t2 - 1;
            double f1 = 1 - t1, f2 = 1 - t2;

            builder.begin_quad_strip();

            for (int j = 0; j < azimuth_steps; ++j) {
                Vec2 r = roots[j];
                double s = j / double(azimuth_steps), x = r[1], z = r[0];
                builder.set_normal(x*2*g, g, z*2*g);
                if (texture)
                    builder.set_tex_coord(s, t2);
                builder.add_vertex(f2 * x, y2, f2 * z);
                if (texture)
                    builder.set_tex_coord(s, t1);
                builder.add_vertex(f1 * x, y1, f1 * z);
            }

            builder.set_normal(0, g, -2*g);
            if (texture)
                builder.set_tex_coord(1, t2);
            builder.add_vertex(0, y2, -f2);
            if (texture)
                builder.set_tex_coord(1, t1);
            builder.add_vertex(0, y1, -f1);

            builder.end();
        }
    }

    // Bottom
    if(bottom) {
	builder.set_normal(0, -1, 0);
	builder.begin_polygon();
        double f = 1.0 / radial_steps;
	for (int i = azimuth_steps-1; 0 <= i; --i) {
            Vec2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord((1+x)/2, (1+z)/2);
            builder.add_vertex(x, -1, z);
	}
	builder.end();
        for (int i = 1; i < radial_steps; ++i) {
            double f1 = (0+i) * f, f2 = (1+i) * f;
            builder.begin_quad_strip();
            if (texture)
                builder.set_tex_coord(0.5, (1-f1)/2);
            builder.add_vertex(0, -1, -f1);
            if (texture)
                builder.set_tex_coord(0.5, (1-f2)/2);
            builder.add_vertex(0, -1, -f2);
            for (int j = azimuth_steps-1; 0 <= j; --j) {
                Vec2 r = roots[j];
                double x1 = f1 * r[1], z1 = f1 * r[0];
                double x2 = f2 * r[1], z2 = f2 * r[0];
                if (texture)
                    builder.set_tex_coord((1+x1)/2, (1+z1)/2);
                builder.add_vertex(x1, -1, z1);
                if (texture)
                    builder.set_tex_coord((1+x2)/2, (1+z2)/2);
                builder.add_vertex(x2, -1, z2);
            }
            builder.end();
        }
    }
}


void build_cylinder(SpatialObjectBuilder& builder, bool texture, bool top, bool bottom,
                    int azimuth_steps, int height_steps, int radial_steps)
{
    vector<Vec2> roots(azimuth_steps);
    {
        double f = 2*M_PI / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i].set(-cos(a), -sin(a)); // z,x
        }
    }

    // FIXME: If azimuth_steps < height_steps, it would be better to run the
    // quad-strips in the axial direction rather than the angular direction. The
    // actual break-even point can be determined counting total number of calls
    // to set_normal(), set_tex_coord(), and add_vertex(). A similar situation
    // exists for the caps.

    // Side
    {
        double f = 1.0 / height_steps;
        for (int i = 0; i < height_steps; ++i) {
            double t1 = f * (0+i), t2 = f * (1+i);
            double y1 = 2*t1 - 1, y2 = 2*t2 - 1;

            builder.begin_quad_strip();

            for (int j = 0; j < azimuth_steps; ++j) {
                Vec2 r = roots[j];
                double s = j / double(azimuth_steps), x = r[1], z = r[0];
                builder.set_normal(x, 0, z);
                if (texture)
                    builder.set_tex_coord(s, t2);
                builder.add_vertex(x, y2, z);
                if (texture)
                    builder.set_tex_coord(s, t1);
                builder.add_vertex(x, y1, z);
            }

            builder.set_normal(0, 0, -1);
            if (texture)
                builder.set_tex_coord(1, t2);
            builder.add_vertex(0, y2, -1);
            if (texture)
                builder.set_tex_coord(1, t1);
            builder.add_vertex(0, y1, -1);

            builder.end();
        }
    }

    // Top
    if (top) {
	builder.set_normal(0, 1, 0);
	builder.begin_polygon();
        double f = 1.0 / radial_steps;
	for (int i = 0; i < azimuth_steps; ++i) {
            Vec2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord((1+x)/2, (1-z)/2);
            builder.add_vertex(x, 1, z);
	}
	builder.end();
        for (int i = 1; i < radial_steps; ++i) {
            double f1 = (0+i) * f, f2 = (1+i) * f;
            builder.begin_quad_strip();
            for (int j = 0; j < azimuth_steps; ++j) {
                Vec2 r = roots[j];
                double x1 = f1 * r[1], z1 = f1 * r[0];
                double x2 = f2 * r[1], z2 = f2 * r[0];
                if (texture)
                    builder.set_tex_coord((1+x1)/2, (1-z1)/2);
                builder.add_vertex(x1, 1, z1);
                if (texture)
                    builder.set_tex_coord((1+x2)/2, (1-z2)/2);
                builder.add_vertex(x2, 1, z2);
            }
            if (texture)
                builder.set_tex_coord(0.5, (1+f1)/2);
            builder.add_vertex(0, 1, -f1);
            if (texture)
                builder.set_tex_coord(0.5, (1+f2)/2);
            builder.add_vertex(0, 1, -f2);
            builder.end();
        }
    }

    // Bottom
    if (bottom) {
	builder.set_normal(0, -1, 0);
	builder.begin_polygon();
        double f = 1.0 / radial_steps;
	for (int i = azimuth_steps-1; 0 <= i; --i) {
            Vec2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord((1+x)/2, (1+z)/2);
            builder.add_vertex(x, -1, z);
        }
        builder.end();
        for (int i = 1; i < radial_steps; ++i) {
            double f1 = (0+i) * f, f2 = (1+i) * f;
            builder.begin_quad_strip();
            if (texture)
                builder.set_tex_coord(0.5, (1-f1)/2);
            builder.add_vertex(0, -1, -f1);
            if (texture)
                builder.set_tex_coord(0.5, (1-f2)/2);
            builder.add_vertex(0, -1, -f2);
            for (int j = azimuth_steps-1; 0 <= j; --j) {
                Vec2 r = roots[j];
                double x1 = f1 * r[1], z1 = f1 * r[0];
                double x2 = f2 * r[1], z2 = f2 * r[0];
                if (texture)
                    builder.set_tex_coord((1+x1)/2, (1+z1)/2);
                builder.add_vertex(x1, -1, z1);
                if (texture)
                    builder.set_tex_coord((1+x2)/2, (1+z2)/2);
                builder.add_vertex(x2, -1, z2);
            }
            builder.end();
        }
    }
}


void build_sphere(SpatialObjectBuilder& builder, bool texture,
                  int azimuth_steps, int elevation_steps)
{
    vector<Vec2> roots(azimuth_steps);
    {
        double f = 2*M_PI / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i].set(-cos(a), -sin(a)); // z,x
        }
    }

    {
        double f = 1.0 / elevation_steps;
        for (int i = 0; i < elevation_steps; ++i) {
            double t1 = f * (0+i), t2 = f * (1+i);
            double a1 = M_PI * t1, a2 = M_PI * t2;
            double y1 = -cos(a1), y2 = -cos(a2);
            double f1 =  sin(a1), f2 =  sin(a2);

            builder.begin_quad_strip();

            for (int j = 0; j <= azimuth_steps; ++j) {
                Vec2 r = roots[j==azimuth_steps ? 0 : j];
                double s = j / double(azimuth_steps);
                double x1 = f1 * r[1], z1 = f1 * r[0];
                double x2 = f2 * r[1], z2 = f2 * r[0];
                builder.set_normal(x2, y2, z2);
                if (texture)
                    builder.set_tex_coord(s, t2);
                builder.add_vertex(x2, y2, z2);
                builder.set_normal(x1, y1, z1);
                if (texture)
                    builder.set_tex_coord(s, t1);
                builder.add_vertex(x1, y1, z1);
            }

            builder.end();
        }
    }
}



void build_torus(SpatialObjectBuilder& builder, bool texture,
                 double major_radius, int major_steps, int minor_steps)
{
    double minor_radius = 1;

    vector<Vec2> roots(major_steps);
    {
        double f = 2*M_PI / major_steps;
        for (int i = 0; i < major_steps; ++i) {
            double a = i * f;
            roots[i].set(-cos(a), -sin(a)); // z,x
        }
    }

    {
        double f = 1.0 / minor_steps;
        for (int i = 0; i < minor_steps; ++i) {
            double t1 = f * (0+i), t2 = f * (1+i);
            double a1 =                        2*M_PI * t1;
            double a2 = 1+i==minor_steps ? 0 : 2*M_PI * t2;
            Vec2 q1 = Vec2(-cos(a1), -sin(a1)), q2 = Vec2(-cos(a2), -sin(a2));
            double y1 = minor_radius * q1[1], y2 =  minor_radius * q2[1];
            double d1 = major_radius + minor_radius * q1[0];
            double d2 = major_radius + minor_radius * q2[0];

            builder.begin_quad_strip();

            for (int j = 0; j <= major_steps; ++j) {
                Vec2 r = roots[j==major_steps ? 0 : j];
                double s = j / double(major_steps);
                if (texture)
                    builder.set_tex_coord(s, t2);
                builder.set_normal(q2[0] * r[1], q2[1], q2[0] * r[0]);
                builder.add_vertex(d2    * r[1], y2,    d2    * r[0]);
                if (texture)
                    builder.set_tex_coord(s, t1);
                builder.set_normal(q1[0] * r[1], q1[1], q1[0] * r[0]);
                builder.add_vertex(d1    * r[1], y1,    d1    * r[0]);
            }

            builder.end();
        }
    }
}

} // namespace Graphics
} // namespace Archon
