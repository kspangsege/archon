// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cmath>

#include <archon/core/math.hpp>
#include <archon/core/vector.hpp>
#include <archon/gfx/object_builder.hpp>


using namespace archon;


namespace {


inline void add_axis_permuted_vertex(gfx::object_builder& builder, int orient,
                                     double a, double b, double c)
{
    switch (orient) {
        case 0: // front
            builder.add_vertex({ a, b, c }); // Throws
            break;
        case 1: // back
            builder.add_vertex({ b, a, c }); // Throws
            break;
        case 2: // right
            builder.add_vertex({ c, a, b }); // Throws
            break;
        case 3: // left
            builder.add_vertex({ c, b, a }); // Throws
            break;
        case 4: // top
            builder.add_vertex({ b, c, a }); // Throws
            break;
        case 5: // bottom
            builder.add_vertex({ a, c, b }); // Throws
            break;
    }
}


// Build a planar axis-aligned mesh that is perpendicular to one of the coordinate axes. It
// faces in one direction if the number of interval revarsals (to < from) is even, and in
// the opposite direction if it is odd.
//
//    orient   a   b   c    facing (even/odd)
//   ------------------------------------------
//      0      x   y   z    front  / back
//      1      y   x   z    back   / front
//      2      y   z   x    right  / left
//      3      z   y   x    left   / right
//      4      z   x   y    top    / bottom
//      5      x   z   y    bottom / top
//
// The specified a-range is always mapped to the interval [0;1] on the primary texture axis
// (S). Likewise, the b-range is mapped to the secondary texture axis (T).
//
void build_mesh(gfx::object_builder& builder, bool texture, int orient,
                double a_from, double a_to, double b_from, double b_to,
                double c, int a_steps, int b_steps)
{
    double a_diff = a_to - a_from, b_diff = b_to - b_from;
    double s_step = 1.0 / a_steps, t_step = 1.0 / b_steps;
    double r = ((a_diff < 0) == (b_diff < 0) ? 1 : -1);
    switch (orient) {
        case 0: // front
            builder.set_normal({  0,  0,  r }); // Throws
            break;
        case 1: // back
            builder.set_normal({  0,  0, -r }); // Throws
            break;
        case 2: // right
            builder.set_normal({  r,  0,  0 }); // Throws
            break;
        case 3: // left
            builder.set_normal({ -r,  0,  0 }); // Throws
            break;
        case 4: // top
            builder.set_normal({  0,  r,  0 }); // Throws
            break;
        case 5: // bottom
            builder.set_normal({  0, -r,  0 }); // Throws
            break;
    }
    for (int i = 0; i < a_steps; ++i) {
        double s_0 = s_step * (0 + i), s_1 = s_step * (1 + i);
        double a_0 =                            a_from + s_0 * a_diff;
        double a_1 = (1 + i == a_steps ? a_to : a_from + s_1 * a_diff);
        builder.begin_quad_strip(); // Throws
        for (int j = 0; j <= b_steps; ++j) {
            double t = t_step * j;
            double b = (j == b_steps ? b_to : b_from + t * b_diff);
            if (texture)
                builder.set_tex_coord({ s_0, t }); // Throws
            ::add_axis_permuted_vertex(builder, orient, a_0, b, c); // Throws
            if (texture)
                builder.set_tex_coord({ s_1, t }); // Throws
            ::add_axis_permuted_vertex(builder, orient, a_1, b, c); // Throws
        }
        builder.end(); // Throws
    }
}


} // unnamed namespace


void gfx::build_xyz_mesh(gfx::object_builder& builder, bool texture, bool reverse,
                         double x_0, double x_1, double y_0, double y_1, double z,
                         int x_steps, int y_steps)
{
    if (reverse) {
        ::build_mesh(builder, texture, 1, y_0, y_1, x_0, x_1, z, y_steps, x_steps); // Throws
    }
    else {
        ::build_mesh(builder, texture, 0, x_0, x_1, y_0, y_1, z, x_steps, y_steps); // Throws
    }
}


void gfx::build_yzx_mesh(gfx::object_builder& builder, bool texture, bool reverse,
                         double y_0, double y_1, double z_0, double z_1, double x,
                         int y_steps, int z_steps)
{
    if (reverse) {
        ::build_mesh(builder, texture, 3, z_0, z_1, y_0, y_1, x, z_steps, y_steps); // Throws
    }
    else {
        ::build_mesh(builder, texture, 2, y_0, y_1, z_0, z_1, x, y_steps, z_steps); // Throws
    }
}


void gfx::build_zxy_mesh(gfx::object_builder& builder, bool texture, bool reverse,
                         double z_0, double z_1, double x_0, double x_1, double y,
                         int z_steps, int x_steps)
{
    if (reverse) {
        ::build_mesh(builder, texture, 5, x_0, x_1, z_0, z_1, y, x_steps, z_steps); // Throws
    }
    else {
        ::build_mesh(builder, texture, 4, z_0, z_1, x_0, x_1, y, z_steps, x_steps); // Throws
    }
}


void gfx::build_centered_box(gfx::object_builder& builder, bool texture,
                             bool front, bool back, bool right, bool left, bool top, bool bottom,
                             int x_steps, int y_steps, int z_steps)
{
    if (front)  //  X,  Y
        gfx::build_xyz_mesh(builder, texture, false, -1,  1, -1,  1,  1, x_steps, y_steps); // Throws
    if (back)   // -X,  Y
        gfx::build_xyz_mesh(builder, texture, false,  1, -1, -1,  1, -1, x_steps, y_steps); // Throws
    if (right)  // -Z,  Y
        gfx::build_yzx_mesh(builder, texture, true,  -1,  1,  1, -1,  1, y_steps, z_steps); // Throws
    if (left)   //  Z,  Y
        gfx::build_yzx_mesh(builder, texture, true,  -1,  1, -1,  1, -1, y_steps, z_steps); // Throws
    if (top)    //  X, -Z
        gfx::build_zxy_mesh(builder, texture, true,   1, -1, -1,  1,  1, z_steps, x_steps); // Throws
    if (bottom) //  X,  Z
        gfx::build_zxy_mesh(builder, texture, true,  -1,  1, -1,  1, -1, z_steps, x_steps); // Throws
}


void gfx::build_unit_box(gfx::object_builder& builder, bool texture,
                         bool front, bool back, bool right, bool left, bool top, bool bottom,
                         int x_steps, int y_steps, int z_steps)
{
    if (front)  //  X,  Y
        gfx::build_xyz_mesh(builder, texture, false, 0, 1, 0, 1, 1, x_steps, y_steps); // Throws
    if (back)   // -X,  Y
        gfx::build_xyz_mesh(builder, texture, false, 1, 0, 0, 1, 0, x_steps, y_steps); // Throws
    if (right)  // -Z,  Y
        gfx::build_yzx_mesh(builder, texture, true,  0, 1, 1, 0, 1, y_steps, z_steps); // Throws
    if (left)   //  Z,  Y
        gfx::build_yzx_mesh(builder, texture, true,  0, 1, 0, 1, 0, y_steps, z_steps); // Throws
    if (top)    //  X, -Z
        gfx::build_zxy_mesh(builder, texture, true,  1, 0, 0, 1, 1, z_steps, x_steps); // Throws
    if (bottom) //  X,  Z
        gfx::build_zxy_mesh(builder, texture, true,  0, 1, 0, 1, 0, z_steps, x_steps); // Throws
}


void gfx::build_cone(gfx::object_builder& builder, bool texture, bool bottom,
                     int azimuth_steps, int height_steps, int radial_steps)
{
    core::Vector<math::Vector2, 128> roots(azimuth_steps); // Throws
    {
        double f = 2 * core::pi<double> / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i] = { -std::cos(a), -std::sin(a) }; // z,x
        }
    }

    // FIXME: If azimuth_steps < height_steps, it would be better to run the quad-strips in
    // the axial direction rather than the angular direction. The actual break-even point
    // can be determined counting total number of calls to set_normal(), set_tex_coord(),
    // and add_vertex(). A similar situation exists for the caps.

    // Side
    {
        double f = 1.0 / height_steps;
        double g = 1.0 / std::sqrt(5);

        for (int i = 0; i < height_steps; ++i) {
            double t_1 = f * (0 + i), t_2 = f * (1 + i);
            double y_1 = 2 * t_1 - 1, y_2 = 2 * t_2 - 1;
            double f_1 = 1 - t_1, f_2 = 1 - t_2;

            builder.begin_quad_strip(); // Throws

            for (int j = 0; j < azimuth_steps; ++j) {
                math::Vector2 r = roots[j];
                double s = j / double(azimuth_steps), x = r[1], z = r[0];
                builder.set_normal({ x * 2 * g, g, z * 2 * g }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_2 }); // Throws
                builder.add_vertex({ f_2 * x, y_2, f_2 * z }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_1 }); // Throws
                builder.add_vertex({ f_1 * x, y_1, f_1 * z }); // Throws
            }

            builder.set_normal({ 0, g, -2 * g }); // Throws
            if (texture)
                builder.set_tex_coord({ 1, t_2 }); // Throws
            builder.add_vertex({ 0, y_2, -f_2 }); // Throws
            if (texture)
                builder.set_tex_coord({ 1, t_1 }); // Throws
            builder.add_vertex({ 0, y_1, -f_1 }); // Throws

            builder.end(); // Throws
        }
    }

    // Bottom
    if(bottom) {
        builder.set_normal({ 0, -1, 0 }); // Throws
        builder.begin_polygon(); // Throws
        double f = 1.0 / radial_steps;
        for (int i = azimuth_steps-1; 0 <= i; --i) {
            math::Vector2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord({ (1 + x) / 2, (1 + z) / 2 }); // Throws
            builder.add_vertex({ x, -1, z }); // Throws
        }
        builder.end(); // Throws
        for (int i = 1; i < radial_steps; ++i) {
            double f_1 = (0 + i) * f, f_2 = (1 + i) * f;
            builder.begin_quad_strip(); // Throws
            if (texture)
                builder.set_tex_coord({ 0.5, (1 - f_1) / 2 }); // Throws
            builder.add_vertex({ 0, -1, -f_1 }); // Throws
            if (texture)
                builder.set_tex_coord({ 0.5, (1 - f_2) / 2 }); // Throws
            builder.add_vertex({ 0, -1, -f_2 }); // Throws
            for (int j = azimuth_steps-1; 0 <= j; --j) {
                math::Vector2 r = roots[j];
                double x_1 = f_1 * r[1], z_1 = f_1 * r[0];
                double x_2 = f_2 * r[1], z_2 = f_2 * r[0];
                if (texture)
                    builder.set_tex_coord({ (1 + x_1) / 2, (1 + z_1) / 2 }); // Throws
                builder.add_vertex({ x_1, -1, z_1 }); // Throws
                if (texture)
                    builder.set_tex_coord({ (1 + x_2) / 2, (1 + z_2) / 2 }); // Throws
                builder.add_vertex({ x_2, -1, z_2 }); // Throws
            }
            builder.end(); // Throws
        }
    }
}


void build_cylinder(gfx::object_builder& builder, bool texture, bool top, bool bottom,
                    int azimuth_steps, int height_steps, int radial_steps)
{
    core::Vector<math::Vector2, 128> roots(azimuth_steps); // Throws
    {
        double f = 2 * core::pi<double> / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i] = { -std::cos(a), -std::sin(a) }; // z,x
        }
    }

    // FIXME: If azimuth_steps < height_steps, it would be better to run the quad-strips in
    // the axial direction rather than the angular direction. The actual break-even point
    // can be determined counting total number of calls to set_normal(), set_tex_coord(),
    // and add_vertex(). A similar situation exists for the caps.

    // Side
    {
        double f = 1.0 / height_steps;
        for (int i = 0; i < height_steps; ++i) {
            double t_1 = f * (0 + i), t_2 = f * (1 + i);
            double y_1 = 2 * t_1 - 1, y_2 = 2 * t_2 - 1;

            builder.begin_quad_strip(); // Throws

            for (int j = 0; j < azimuth_steps; ++j) {
                math::Vector2 r = roots[j];
                double s = j / double(azimuth_steps), x = r[1], z = r[0];
                builder.set_normal({ x, 0, z }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_2 }); // Throws
                builder.add_vertex({ x, y_2, z }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_1 }); // Throws
                builder.add_vertex({ x, y_1, z }); // Throws
            }

            builder.set_normal({ 0, 0, -1 }); // Throws
            if (texture)
                builder.set_tex_coord({ 1, t_2 }); // Throws
            builder.add_vertex({ 0, y_2, -1 }); // Throws
            if (texture)
                builder.set_tex_coord({ 1, t_1 }); // Throws
            builder.add_vertex({ 0, y_1, -1 }); // Throws

            builder.end(); // Throws
        }
    }

    // Top
    if (top) {
        builder.set_normal({ 0, 1, 0 }); // Throws
        builder.begin_polygon(); // Throws
        double f = 1.0 / radial_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            math::Vector2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord({ (1 + x) / 2, (1 - z) / 2 }); // Throws
            builder.add_vertex({ x, 1, z }); // Throws
        }
        builder.end(); // Throws
        for (int i = 1; i < radial_steps; ++i) {
            double f_1 = (0 + i) * f, f_2 = (1 + i) * f;
            builder.begin_quad_strip(); // Throws
            for (int j = 0; j < azimuth_steps; ++j) {
                math::Vector2 r = roots[j];
                double x_1 = f_1 * r[1], z_1 = f_1 * r[0];
                double x_2 = f_2 * r[1], z_2 = f_2 * r[0];
                if (texture)
                    builder.set_tex_coord({ (1 + x_1) / 2, (1 - z_1) / 2 }); // Throws
                builder.add_vertex({ x_1, 1, z_1 }); // Throws
                if (texture)
                    builder.set_tex_coord({ (1 + x_2) / 2, (1 - z_2) / 2 }); // Throws
                builder.add_vertex({ x_2, 1, z_2 }); // Throws
            }
            if (texture)
                builder.set_tex_coord({ 0.5, (1 + f_1) / 2 }); // Throws
            builder.add_vertex({ 0, 1, -f_1 }); // Throws
            if (texture)
                builder.set_tex_coord({ 0.5, (1 + f_2) / 2 }); // Throws
            builder.add_vertex({ 0, 1, -f_2 }); // Throws
            builder.end(); // Throws
        }
    }

    // Bottom
    if (bottom) {
        builder.set_normal({ 0, -1, 0 }); // Throws
        builder.begin_polygon(); // Throws
        double f = 1.0 / radial_steps;
        for (int i = azimuth_steps-1; 0 <= i; --i) {
            math::Vector2 r = roots[i];
            double x = f * r[1], z = f * r[0];
            if (texture)
                builder.set_tex_coord({ (1 + x) / 2, (1 + z) / 2 }); // Throws
            builder.add_vertex({ x, -1, z }); // Throws
        }
        builder.end(); // Throws
        for (int i = 1; i < radial_steps; ++i) {
            double f_1 = (0 + i) * f, f_2 = (1 + i) * f;
            builder.begin_quad_strip(); // Throws
            if (texture)
                builder.set_tex_coord({ 0.5, (1 - f_1) / 2 }); // Throws
            builder.add_vertex({ 0, -1, -f_1 }); // Throws
            if (texture)
                builder.set_tex_coord({ 0.5, (1 - f_2) / 2 }); // Throws
            builder.add_vertex({ 0, -1, -f_2 }); // Throws
            for (int j = azimuth_steps-1; 0 <= j; --j) {
                math::Vector2 r = roots[j];
                double x_1 = f_1 * r[1], z_1 = f_1 * r[0];
                double x_2 = f_2 * r[1], z_2 = f_2 * r[0];
                if (texture)
                    builder.set_tex_coord({ (1 + x_1) / 2, (1 + z_1) / 2 }); // Throws
                builder.add_vertex({ x_1, -1, z_1 }); // Throws
                if (texture)
                    builder.set_tex_coord({ (1 + x_2) / 2, (1 + z_2) / 2 }); // Throws
                builder.add_vertex({ x_2, -1, z_2 }); // Throws
            }
            builder.end(); // Throws
        }
    }
}


void gfx::build_sphere(gfx::object_builder& builder, bool texture,
                       int azimuth_steps, int elevation_steps)
{
    core::Vector<math::Vector2, 128> roots(azimuth_steps); // Throws
    {
        double f = 2 * core::pi<double> / azimuth_steps;
        for (int i = 0; i < azimuth_steps; ++i) {
            double a = i * f;
            roots[i] = { -std::cos(a), -std::sin(a) }; // z,x
        }
    }

    {
        double f = 1.0 / elevation_steps;
        for (int i = 0; i < elevation_steps; ++i) {
            double t_1 = f * (0 + i), t_2 = f * (1 + i);
            double a_1 = core::pi<double> * t_1, a_2 = core::pi<double> * t_2;
            double y_1 = -std::cos(a_1), y_2 = -std::cos(a_2);
            double f_1 =  std::sin(a_1), f_2 =  std::sin(a_2);

            builder.begin_quad_strip(); // Throws

            for (int j = 0; j <= azimuth_steps; ++j) {
                math::Vector2 r = roots[j == azimuth_steps ? 0 : j];
                double s = j / double(azimuth_steps);
                double x_1 = f_1 * r[1], z_1 = f_1 * r[0];
                double x_2 = f_2 * r[1], z_2 = f_2 * r[0];
                builder.set_normal({ x_2, y_2, z_2 }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_2 }); // Throws
                builder.add_vertex({ x_2, y_2, z_2 }); // Throws
                builder.set_normal({ x_1, y_1, z_1 }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_1 }); // Throws
                builder.add_vertex({ x_1, y_1, z_1 }); // Throws
            }

            builder.end(); // Throws
        }
    }
}



void gfx::build_torus(gfx::object_builder& builder, bool texture,
                      double minor_radius, int major_steps, int minor_steps)
{
    double major_radius = 1;

    core::Vector<math::Vector2, 128> roots(major_steps); // Throws
    {
        double f = 2 * core::pi<double> / major_steps;
        for (int i = 0; i < major_steps; ++i) {
            double a = i * f;
            roots[i] = { -std::cos(a), -std::sin(a) }; // z,x
        }
    }

    {
        double f = 1.0 / minor_steps;
        for (int i = 0; i < minor_steps; ++i) {
            double t_1 = f * (0 + i), t_2 = f * (1 + i);
            double a_1 =                             2 * core::pi<double> * t_1;
            double a_2 = (1 + i == minor_steps ? 0 : 2 * core::pi<double> * t_2);
            math::Vector2 q_1 = { -std::cos(a_1), -std::sin(a_1) }, q_2 = { -std::cos(a_2), -std::sin(a_2) };
            double y_1 = minor_radius * q_1[1], y_2 =  minor_radius * q_2[1];
            double d_1 = major_radius + minor_radius * q_1[0];
            double d_2 = major_radius + minor_radius * q_2[0];

            builder.begin_quad_strip(); // Throws

            for (int j = 0; j <= major_steps; ++j) {
                math::Vector2 r = roots[j == major_steps ? 0 : j];
                double s = j / double(major_steps);
                if (texture)
                    builder.set_tex_coord({ s, t_2 }); // Throws
                builder.set_normal({ q_2[0] * r[1], q_2[1], q_2[0] * r[0] }); // Throws
                builder.add_vertex({ d_2    * r[1], y_2,    d_2    * r[0] }); // Throws
                if (texture)
                    builder.set_tex_coord({ s, t_1 }); // Throws
                builder.set_normal({ q_1[0] * r[1], q_1[1], q_1[0] * r[0] }); // Throws
                builder.add_vertex({ d_1    * r[1], y_1,    d_1    * r[0] }); // Throws
            }

            builder.end(); // Throws
        }
    }
}
