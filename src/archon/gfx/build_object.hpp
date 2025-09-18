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

#ifndef ARCHON_X_GFX_X_BUILD_OBJECT_HPP
#define ARCHON_X_GFX_X_BUILD_OBJECT_HPP

/// \file


#include <archon/gfx/object_builder.hpp>


namespace archon::gfx {


/// \brief Build XYZ mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local Z-axis. It faces in
/// the direction of the positive Z-axis (towards the viewer) if the number reversals is
/// even, and in the opposite direction (away from the viewer) if it is odd. Both axis order
/// and range reversals count. For example, the X-range is reversed when `x_1 < x_0`.
///
/// The specified X-range (Y-range, if reversed axis order) will be mapped to [0;1] on the
/// primary texture coordinate axis, and likewise the specified Y-range (X-range, if
/// reversed axis order) is mapped to the secondary texture coordinate axis.
///
void build_xyz_mesh(gfx::object_builder& builder, bool reverse_xy_order = false,
                    float x_0 = -1, float x_1 = 1, float y_0 = -1, float y_1 = 1,
                    float z = 0, int x_steps = 12, int y_steps = 12);



/// \brief Build YZX mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local Y-axis. It faces in
/// the direction of the positive Y-axis (upwards) if the number reversals is even, and in
/// the opposite direction (downwards) if it is odd. Both axis order and range reversals
/// count. For example, the Y-range is reversed when `y_1 < y_0`.
///
/// The specified Y-range (Z-range, if reversed axis order) will be mapped to [0;1] on the
/// primary texture coordinate axis, and likewise the specified Z-range (Y-range, if
/// reversed axis order) is mapped to the secondary texture coordinate axis.
///
void build_yzx_mesh(gfx::object_builder& builder, bool reverse_yz_order = false,
                    float y_0 = -1, float y_1 = 1, float z_0 = -1, float z_1 = 1,
                    float x = 0, int y_steps = 12, int z_steps = 12);



/// \brief Build ZXY mesh.
///
/// Build a planar axis-aligned mesh that is perpendicular to the local X-axis. It faces in
/// the direction of the positive X-axis (towards the right) if the number reversals is
/// even, and in the opposite direction (towards the left) if it is odd. Both axis order and
/// range reversals count. For example, the Z-range is reversed when `z_1 < z_0`.
///
/// The specified Z-range (X-range, if reversed axis order) will be mapped to [0;1] on the
/// primary texture coordinate axis, and likewise the specified X-range (Z-range, if
/// reversed axis order) is mapped to the secondary texture coordinate axis.
///
void build_zxy_mesh(gfx::object_builder& builder, bool reverse_zx_order = false,
                    float z_0 = -1, float z_1 = 1, float x_0 = -1, float x_1 = 1,
                    float y = 0, int z_steps = 12, int x_steps = 12);



/// \{
///
/// \brief Build unit box.
///
/// These functions build a 1-by-1-by-1 axis-aligned rectangular box with one corner in
/// (0,0,0), and the opposite corner in (1,1,1).
///
/// The unit texture square is mapped on to each of the six faces. On the front face it is
/// mapped such that the primary and secondary texture coordinate axes are codirectional
/// with the spatial X and Y axes respectively. On the left, right, and back faces, the
/// mapping is done such that the result corresponds to rotations of the front face around
/// the spatial Y axis. On the top and bottom faces, it is done such that the result
/// corresponds to rotations of the front face around the spatial X axis.
///
void build_box(gfx::object_builder&, int steps = 12);
void build_box(gfx::object_builder&, bool has_front, bool has_back, bool has_right, bool has_left,
                    bool has_top, bool has_bottom, int x_steps, int y_steps, int z_steps);
/// \}



/// \{
///
/// \brief Build cylinder.
///
/// These functions build an optionally capped cylinder centered at the origin and with
/// symmetry axis coincident with the Y-axis. The radius of the cylinder is 1, and the
/// height of the cylinder along the symmetry axis is 2.
///
/// On the side of the cylinder, that is, on the curved part of its surface, texture is
/// mapped around the spatial Y axis in counterclockwise direction when viewed from above
/// (positive Y) and with the "seam" at the back of the cone (zero X and negative Z). The
/// primary texture coordinate is codirectional with the spatial Y axis and the secondary
/// texture coordinate maps to the radial direction.
///
/// For the top and bottom caps, a circular cutout of the texture is applied. The cutout is
/// as big as possible while fitting inside the primary texture square, i.e., it has a
/// diameter of one. On the top cap, the primary texture coordinate axis is codirectional
/// with the spatial X axis, and the secondary texture axis is antidirectional to the
/// spatial Z axis. On the bottom cap, it is the same, except that the secondary texture
/// coordinate axis is flipped so that it is now codirectional with the spatial Z axis.
///
void build_cylinder(gfx::object_builder&, int azimuth_steps = 36, int height_steps = 12, int radial_steps = 6);
void build_cylinder(gfx::object_builder&, bool has_side, bool has_top, bool has_bottom,
                    int azimuth_steps, int height_steps, int radial_steps);
/// \}



/// \{
///
/// \brief Build cone.
///
/// These functions build an optionally capped cone centered at the origin (origin is half
/// way between apex and base) and with symmetry axis coincident with the Y-axis. The base
/// radius of the cylinder is 1, and the height of the cylinder along the symmetry axis
/// is 2.
///
/// On the side of the cone, that is, on the curved part of its surface, texture is mapped
/// around the spatial Y axis in counterclockwise direction when viewed from above (positive
/// Y) and with the "seam" at the back of the cone (zero X and negative Z). The primary
/// texture coordinate is codirectional with the spatial Y axis and the secondary texture
/// coordinate maps to the radial direction.
///
/// For the bottom cap, a circular cutout of the texture is applied. The cutout is as big as
/// possible while fitting inside the primary texture square, i.e., it has a diameter of
/// one. The primary texture coordinate axis is codirectional with the spatial X axis, and
/// the secondary texture axis is codirectional with the spatial Z axis.
///
void build_cone(gfx::object_builder&, int azimuth_steps = 36, int height_steps = 12, int radial_steps = 6);
void build_cone(gfx::object_builder&, bool has_side, bool has_bottom,
                int azimuth_steps, int height_steps, int radial_steps);
/// \}



/// \brief Build sphere.
///
/// This function builds a unit sphere centered at the origin.
///
/// Texture is mapped onto the sphere around the spatial Y axis in counterclockwise
/// direction when viewed from above (positive Y) and with the "seam" at the back of the
/// sphere (zero X and negative Z). The primary texture coordinate is codirectional with the
/// spatial Y axis and the secondary texture coordinate maps to the radial direction.
///
void build_sphere(gfx::object_builder&, int azimuth_steps = 36, int elevation_steps = 18);



/// \brief Build torus.
///
/// This function builds a torus with major radius of 1 and centered at the origin. Its
/// symmetry axis coincides with the Y-axis.
///
/// The primary texture coordinate is mapped to the major radial direction around the
/// symmetry axis of the torus, and with the "seam" at the back (zero X and negative Z). The
/// direction is counterclockwise when viewed from above (positive Y).
///
/// The secondary texture coordinate is mapped to the minor radial direction, which is the
/// direction to a torus surface point from the nearest point on the major generating circle
/// (the directrix). The secondary texture coordinate runs from the "seam" at the inside of
/// the torus (zero Y) towards the top (positive Y), then via the outside of the torus
/// towards its bottom (negative Y), and finally back to the "seam".
///
void build_torus(gfx::object_builder&, float minor_radius = 0.5, int major_azimuth_steps = 36,
                 int minor_azimuth_steps = 18);








// Implementation


inline void build_box(gfx::object_builder& builder, int steps)
{
    bool has_front  = true;
    bool has_back   = true;
    bool has_right  = true;
    bool has_left   = true;
    bool has_top    = true;
    bool has_bottom = true;
    int x_steps = steps;
    int y_steps = steps;
    int z_steps = steps;
    gfx::build_box(builder, has_front, has_back, has_right, has_left, has_top, has_bottom,
                   x_steps, y_steps, z_steps); // Throws
}


inline void build_cylinder(gfx::object_builder& builder, int azimuth_steps, int height_steps, int radial_steps)
{
    bool has_side   = true;
    bool has_top    = true;
    bool has_bottom = true;
    gfx::build_cylinder(builder, has_side, has_top, has_bottom, azimuth_steps, height_steps, radial_steps); // Throws
}


inline void build_cone(gfx::object_builder& builder, int azimuth_steps, int height_steps, int radial_steps)
{
    bool has_side   = true;
    bool has_bottom = true;
    gfx::build_cone(builder, has_side, has_bottom, azimuth_steps, height_steps, radial_steps); // Throws
}


} // namespace archon::gfx

#endif // ARCHON_X_GFX_X_BUILD_OBJECT_HPP
