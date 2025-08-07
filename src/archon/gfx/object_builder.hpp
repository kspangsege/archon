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

#ifndef ARCHON_X_GFX_X_OBJECT_BUILDER_HPP
#define ARCHON_X_GFX_X_OBJECT_BUILDER_HPP

/// \file


#include <archon/math/vector.hpp>
#include <archon/util/color.hpp>


namespace archon::gfx {


class object_builder {
public:
    void begin_quad_strip();
    void begin_polygon();
    void end();

    /// \brief Set vertex color.
    ///
    /// This function sets the color that is applied to subsequently added vertices. The
    /// default color is fully opaque white.
    ///
    void set_color(util::Color);

    /// \brief Set vertex normal.
    ///
    /// This function sets the normal that is applied to subsequently added vertices. It
    /// must be a unit vector.
    ///
    void set_normal(const math::Vector3&);

    void set_tex_coord(const math::Vector2&);

    void add_vertex(const math::Vector3&);

    virtual ~object_builder() noexcept = default;

protected:
    virtual void do_begin_quad_strip() = 0;
    virtual void do_begin_polygon() = 0;
    virtual void do_end() = 0;

    virtual void do_set_color(util::Color) = 0;
    virtual void do_set_normal(const math::Vector3&) = 0;
    virtual void do_set_tex_coord(const math::Vector2&) = 0;
    virtual void do_add_vertex(const math::Vector3&) = 0;
};



/// \brief Build XYZ mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local Z-axis. It faces in
/// the direction of the positive Z-axis (towards the viewer) if the number reversals is
/// even, and in the opposite direction (away from the viewer) if it is odd. Both axis order
/// and range reversals count. For example, the X-range is reversed when `x_1 < x_0`.
///
/// When texture coordinates are generated, the specified X-range (Y-range, if reversed axis
/// order) will be mapped to [0;1] on the primary texture coordinate axis, and likewise the
/// specified Y-range (X-range, if reversed axis order) is mapped to the secondary texture
/// coordinate axis.
///
void build_xyz_mesh(gfx::object_builder& builder,
                    bool gen_texture_coords = true, bool reverse_xy_order = false,
                    double x_0 = -1, double x_1 = 1, double y_0 = -1, double y_1 = 1,
                    double z = 0, int x_steps = 12, int y_steps = 12);



/// \brief Build YZX mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local Y-axis. It faces in
/// the direction of the positive Y-axis (upwards) if the number reversals is even, and in
/// the opposite direction (downwards) if it is odd. Both axis order and range reversals
/// count. For example, the Y-range is reversed when `y_1 < y_0`.
///
/// When texture coordinates are generated, the specified Y-range (Z-range, if reversed axis
/// order) will be mapped to [0;1] on the primary texture coordinate axis, and likewise the
/// specified Z-range (Y-range, if reversed axis order) is mapped to the secondary texture
/// coordinate axis.
///
void build_yzx_mesh(gfx::object_builder& builder,
                    bool gen_texture_coords = true, bool reverse_yz_order = false,
                    double y_0 = -1, double y_1 = 1, double z_0 = -1, double z_1 = 1,
                    double x = 0, int y_steps = 12, int z_steps = 12);



/// \brief Build ZXY mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local X-axis. It faces in
/// the direction of the positive X-axis (towards the right) if the number reversals is
/// even, and in the opposite direction (towards the left) if it is odd. Both axis order and
/// range reversals count. For example, the Z-range is reversed when `z_1 < z_0`.
///
/// When texture coordinates are generated, the specified Z-range (X-range, if reversed axis
/// order) will be mapped to [0;1] on the primary texture coordinate axis, and likewise the
/// specified X-range (Z-range, if reversed axis order) is mapped to the secondary texture
/// coordinate axis.
///
void build_zxy_mesh(gfx::object_builder& builder,
                    bool gen_texture_coords = true, bool reverse_zx_order = false,
                    double z_0 = -1, double z_1 = 1, double x_0 = -1, double x_1 = 1,
                    double y = 0, int z_steps = 12, int x_steps = 12);



/// \brief Build centered 2x2x2 box.
///
/// Build a 2 by 2 by 2 axis-aligned rectangular box centered at the origin.
///
/// When texture coordinates are generated, the unit texture square is mapped on to each of
/// the six faces. On the front face it is mapped such that the primary and secondary
/// texture coordinate axes are codirectional with the spatial X and Y axes respectively. On
/// the left, right, and back faces, the mapping is done such that it corresponds to
/// rotations of the front face around the spatial Y axis. On the top and bottom faces, it
/// is done such that it corresponds to rotations of the front face around the spatial X
/// axis.
///
void build_centered_box(gfx::object_builder&, bool gen_texture_coords = true,
                        bool has_front = true, bool has_back = true, bool has_right = true,
                        bool has_left = true, bool has_top = true, bool has_bottom = true,
                        int x_steps = 12, int y_steps = 12, int z_steps = 12);

/// \brief Build unit box.
///
/// Build a 1 by 1 by 1 axis-aligned rectangular box with one corner in (0,0,0), and the
/// opposite corner in (1,1,1).
///
/// Texture coordinates a generated in the same was as is done by \ref
/// gfx::build_centered_box().
///
void build_unit_box(gfx::object_builder&, bool gen_texture_coords = true,
                    bool has_front = true, bool has_back = true, bool has_right = true,
                    bool has_left = true, bool has_top = true, bool has_bottom = true,
                    int x_steps = 12, int y_steps = 12, int z_steps = 12);



void build_cone(gfx::object_builder&, bool gen_texture_coords = true,
                bool has_bottom = true, int azimuth_steps = 36,
                int height_steps = 12, int radial_steps = 6);



/// \brief Build cylinder.
///
/// An optionally capped cylinder centered at the origin of the local coordinate system and
/// with a central axis oriented along the local Y-axis. The radius of the cylinder is 1,
/// and the height of the cylinder along the central axis is 2.
///
/// When a texture is applied to a cylinder, it is applied differently to the sides, top,
/// and bottom. On the sides, the texture wraps counterclockwise when viewed from above
/// (positive Y), starting at the back of the cylinder (negative Z). The texture has a
/// vertical "seam" at the back, intersecting the X=0 plane.  For the top and bottom caps, a
/// circle is cut out of the unit texture squares centered at (0, +/-1, 0) with dimensions 2
/// by 2. When the cylinder is rotated 90 degrees around the X-axis such that the bottom is
/// in the direction of the negative Z-axis, the primary and secondary texture coordinate
/// axes of the bottom texture will coincide with the local spatial X and Y-axes
/// respectively. Likewise, when the cylinder is rotated 90 degrees in the opposite
/// direction, the primary and secondary texture coordinate axes of the top texture will
/// coincide with the local spatial X and Y-axis respectively.
///
void build_cylinder(gfx::object_builder&, bool gen_texture_coords = true,
                    bool has_top = true, bool has_bottom = true,
                    int azimuth_steps = 36, int height_steps = 12, int radial_steps = 6);



void build_sphere(gfx::object_builder&, bool gen_texture_coords = true,
                  int azimuth_steps = 36, int elevation_steps = 18);



/// \brief Build torus.
///
/// Build a torus with major radius of 1.
///
void build_torus(gfx::object_builder&, bool gen_texture_coords = true,
                 double minor_radius = 0.5, int major_azimuth_steps = 36,
                 int minor_azimuth_steps = 18);








// Implementation


inline void object_builder::begin_quad_strip()
{
    do_begin_quad_strip(); // Throws
}


inline void object_builder::begin_polygon()
{
    do_begin_polygon(); // Throws
}


inline void object_builder::end()
{
    do_end(); // Throws
}


inline void object_builder::set_color(util::Color color)
{
    do_set_color(color); // Throws
}


inline void object_builder::set_normal(const math::Vector3& n)
{
    do_set_normal(n); // Throws
}


inline void object_builder::set_tex_coord(const math::Vector2& c)
{
    do_set_tex_coord(c); // Throws
}


inline void object_builder::add_vertex(const math::Vector3& v)
{
    do_add_vertex(v); // Throws
}


} // namespace archon::gfx

#endif // ARCHON_X_GFX_X_OBJECT_BUILDER_HPP
