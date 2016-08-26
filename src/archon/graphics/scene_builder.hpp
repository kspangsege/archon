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

#ifndef ARCHON_GRAPHICS_SCENE_BUILDER_HPP
#define ARCHON_GRAPHICS_SCENE_BUILDER_HPP

#include <string>

#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/image/image.hpp>


namespace archon {
namespace graphics {

class SpatialObjectBuilder {
public:
    void begin_quad_strip();
    void begin_polygon();
    void end();

    /// Default color is fully opaque white
    void set_color(util::PackedTRGB);

    void set_normal(math::Vec3); // Must be a unit vector
    void set_tex_coord(math::Vec2);
    void add_vertex(math::Vec3);

    void set_normal(double x, double y, double z); // Must be a unit vector
    void set_tex_coord(double s, double t);
    void add_vertex(double x, double y, double z);

    virtual ~SpatialObjectBuilder() {}

protected:
    virtual void do_begin_quad_strip() = 0;
    virtual void do_begin_polygon() = 0;
    virtual void do_end() = 0;

    virtual void do_set_color(util::PackedTRGB) = 0;
    virtual void do_set_normal(math::Vec3) = 0;
    virtual void do_set_tex_coord(math::Vec2) = 0;
    virtual void do_add_vertex(math::Vec3) = 0;
};



class SpatialSceneBuilder: public SpatialObjectBuilder {
public:
    void push_matrix();
    void pop_matrix();

    void translate(math::Vec3);
    void scale(math::Vec3);
    void rotate(math::Rotation3);

    void translate(double x, double y, double z);
    void scale(double f);
    void scale(double x, double y, double z);
    void rotate(double radians, double x, double y, double z); // Axis must be a unit vector.

    int make_texture(std::string image_path, bool repeat_s, bool repeat_t);
    int make_texture(std::string image_path, bool repeat = true);
    int make_texture(image::Image::ConstRefArg, std::string name, bool repeat);
    int make_texture(image::Image::ConstRefArg, std::string name,
                     bool repeat_s, bool vert_repeat_t);

    void bind_texture(int texture_index);

    void tex_translate(math::Vec2 v);
    void tex_scale(math::Vec2 s);

    void tex_translate(double s, double t);
    void tex_scale(double s, double t);
    void tex_scale(double f);
    void tex_rotate(double radians);

    void reset_tex_transform();

protected:
    virtual void do_push_matrix() = 0;
    virtual void do_pop_matrix() = 0;

    virtual void do_translate(math::Vec3) = 0;
    virtual void do_scale(math::Vec3) = 0;
    virtual void do_rotate(math::Rotation3) = 0;

    virtual int do_make_texture(std::string image_path, bool repeat_s, bool repeat_t);
    virtual int do_make_texture(image::Image::ConstRefArg, std::string name,
                                bool repeat_s, bool repeat_t) = 0;

    virtual void do_bind_texture(int texture_index) = 0;

    virtual void do_tex_translate(math::Vec2 v) = 0;
    virtual void do_tex_scale(math::Vec2 s) = 0;
    virtual void do_tex_rotate(double radians) = 0;

    virtual void do_reset_tex_transform() = 0;

/*
    virtual int add_directional_light(math::Vec3 dir) = 0;
    virtual int add_point_light(math::Vec3 pos) = 0;
    virtual int add_spot_light_light() = 0;
    virtual void remove_light() = 0;
*/
};








/// Build a planar axis-aligned mesh that is perpendicular to the local
/// Z-axis. It faces in the direction of the positive Z-axis (towards the
/// viewer) if the number revarsals is even, and in the opposite direction (away
/// from the viewer) if it is odd. Both axis order and range reversals
/// count. For example, the X-range is reversed when 'x1 < x0'.
///
/// When texture coordinates are generated, the specified X-range (Y-range, if
/// reversed axis order) will be mapped to [0;1] on the primary texture
/// coordinate axis, and likewise the specified Y-range (X-range, if reversed
/// axis order) is mapped to the secondary texture coordinate axis.
void build_xyz_mesh(SpatialObjectBuilder& builder,
                    bool gen_texture_coords = true, bool reverse_xy_order = false,
                    double x0 = -1, double x1 = 1, double y0 = -1, double y1 = 1,
                    double z = 0, int x_steps = 12, int y_steps = 12);



/// Build a planar axis-aligned mesh that is perpendicular to the local
/// Y-axis. It faces in the direction of the positive Y-axis (upwards) if the
/// number revarsals is even, and in the opposite direction (downwards) if it is
/// odd. Both axis order and range reversals count. For example, the Y-range is
/// reversed when 'y1 < y0'.
///
/// When texture coordinates are generated, the specified Y-range (Z-range, if
/// reversed axis order) will be mapped to [0;1] on the primary texture
/// coordinate axis, and likewise the specified Z-range (Y-range, if reversed
/// axis order) is mapped to the secondary texture coordinate axis.
void build_yzx_mesh(SpatialObjectBuilder& builder,
                    bool gen_texture_coords = true, bool reverse_yz_order = false,
                    double y0 = -1, double y1 = 1, double z0 = -1, double z1 = 1,
                    double x = 0, int y_steps = 12, int z_steps = 12);



/// Build a planar axis-aligned mesh that is perpendicular to the local
/// X-axis. It faces in the direction of the positive X-axis (towards the right)
/// if the number revarsals is even, and in the opposite direction (towards the
/// left) if it is odd. Both axis order and range reversals count. For example,
/// the Z-range is reversed when 'z1 < z0'.
///
/// When texture coordinates are generated, the specified Z-range (X-range, if
/// reversed axis order) will be mapped to [0;1] on the primary texture
/// coordinate axis, and likewise the specified X-range (Z-range, if reversed
/// axis order) is mapped to the secondary texture coordinate axis.
void build_zxy_mesh(SpatialObjectBuilder& builder,
                    bool gen_texture_coords = true, bool reverse_zx_order = false,
                    double z0 = -1, double z1 = 1, double x0 = -1, double x1 = 1,
                    double y = 0, int z_steps = 12, int x_steps = 12);



/// Build a 2 by 2 by 2 axis-aligned rectangular box centered at the origin.
///
/// When texture coordinates are generated, the unit texture square is mapped on
/// to each of the six faces. On the front face it is mappped such that the
/// primary and secondary texture coordinate axes are codirectional with the
/// spatial X and Y axes respectively. On the left, right, and back faces, the
/// mapping is done such that it corresponds to rotations of the front face
/// around the spatial Y axis. On the top and bottom faces, it is done such that
/// it corresponds to rotations of the front face around the spatial X axis.
void build_centered_box(SpatialObjectBuilder&, bool gen_texture_coords = true,
                        bool has_front = true, bool has_back = true, bool has_right = true,
                        bool has_left = true, bool has_top = true, bool has_bottom = true,
                        int x_steps = 12, int y_steps = 12, int z_steps = 12);

/// Build a 1 by 1 by 1 axis-aligned rectangular box with one corner in (0,0,0),
/// and the opposite corner in (1,1,1).
///
/// Texture coordinates a generated in the same was as is done by
/// build_centered_box().
void build_unit_box(SpatialObjectBuilder&, bool gen_texture_coords = true,
                    bool has_front = true, bool has_back = true, bool has_right = true,
                    bool has_left = true, bool has_top = true, bool has_bottom = true,
                    int x_steps = 12, int y_steps = 12, int z_steps = 12);



void build_cone(SpatialObjectBuilder&, bool gen_texture_coords = true,
                bool has_bottom = true, int azimuth_steps = 36,
                int height_steps = 12, int radial_steps = 6);



/// An optionally capped cylinder centered at the origin of the local coordinate
/// system and with a central axis oriented along the local Y-axis. The radius
/// of the cylinder is 1, and the height of the cylinder along the central axis
/// is 2.
///
/// When a texture is applied to a cylinder, it is applied differently to the
/// sides, top, and bottom. On the sides, the texture wraps counterclockwise
/// when viewed from above (positive Y), starting at the back of the cylinder
/// (negative Z). The texture has a vertical "seam" at the back, intersecting
/// the X=0 plane.  For the top and bottom caps, a circle is cut out of the unit
/// texture squares centred at (0, +/-1, 0) with dimensions 2 by 2. When the
/// cylinder is rotated 90 degrees around the X-axis such that the bottom is in
/// the direction of the negative Z-axis, the primary and secondary texture
/// coordinate axes of the bottom texture will coincide with the local spatial X
/// and Y-axes respectively. Likewise, when the cylinder is rotated 90 degrees
/// in the opposite direction, the primary and secondary texture coordinate axes
/// of the top texture will coincide with the local spatial X and Y-axis
/// respectively.
void build_cylinder(SpatialObjectBuilder&, bool gen_texture_coords = true,
                    bool has_top = true, bool has_bottom = true,
                    int azimuth_steps = 36, int height_steps = 12, int radial_steps = 6);



void build_sphere(SpatialObjectBuilder&, bool gen_texture_coords = true,
                  int azimuth_steps = 36, int elevation_steps = 18);



/// Build a torus with major radius of 1.
void build_torus(SpatialObjectBuilder&, bool gen_texture_coords = true,
                 double minor_radius = 0.5, int major_azimuth_steps = 36,
                 int minor_azimuth_steps = 18);




// Implementation

inline void SpatialObjectBuilder::begin_quad_strip()
{
    do_begin_quad_strip();
}

inline void SpatialObjectBuilder::begin_polygon()
{
    do_begin_polygon();
}

inline void SpatialObjectBuilder::end()
{
    do_end();
}

inline void SpatialObjectBuilder::set_color(util::PackedTRGB color)
{
    do_set_color(color);
}

inline void SpatialObjectBuilder::set_normal(math::Vec3 n)
{
    do_set_normal(n);
}

inline void SpatialObjectBuilder::set_tex_coord(math::Vec2 c)
{
    do_set_tex_coord(c);
}

inline void SpatialObjectBuilder::add_vertex(math::Vec3 v)
{
    do_add_vertex(v);
}

inline void SpatialObjectBuilder::set_normal(double x, double y, double z)
{
    do_set_normal(math::Vec3(x,y,z));
}

inline void SpatialObjectBuilder::set_tex_coord(double s, double t)
{
    do_set_tex_coord(math::Vec2(s,t));
}

inline void SpatialObjectBuilder::add_vertex(double x, double y, double z)
{
    do_add_vertex(math::Vec3(x,y,z));
}

inline void SpatialSceneBuilder::push_matrix()
{
    do_push_matrix();
}

inline void SpatialSceneBuilder::pop_matrix()
{
    do_pop_matrix();
}

inline void SpatialSceneBuilder::translate(math::Vec3 v)
{
    do_translate(v);
}

inline void SpatialSceneBuilder::scale(math::Vec3 s)
{
    do_scale(s);
}

inline void SpatialSceneBuilder::rotate(math::Rotation3 r)
{
    do_rotate(r);
}

inline void SpatialSceneBuilder::translate(double x, double y, double z)
{
    do_translate(math::Vec3(x,y,z));
}

inline void SpatialSceneBuilder::scale(double f)
{
    scale(f,f,f);
}

inline void SpatialSceneBuilder::scale(double x, double y, double z)
{
    do_scale(math::Vec3(x,y,z));
}

inline void SpatialSceneBuilder::rotate(double angle, double x, double y, double z)
{
    do_rotate(math::Rotation3(math::Vec3(x,y,z), angle));
}

inline int SpatialSceneBuilder::make_texture(std::string img_path, bool rep)
{
    return make_texture(img_path, rep, rep);
}

inline int SpatialSceneBuilder::make_texture(std::string img_path, bool rep_s, bool rep_t)
{
    return do_make_texture(img_path, rep_s, rep_t);
}

inline int SpatialSceneBuilder::make_texture(image::Image::ConstRefArg img,
                                             std::string name, bool rep)
{
    return make_texture(img, name, rep, rep);
}

inline int SpatialSceneBuilder::make_texture(image::Image::ConstRefArg img,
                                             std::string name, bool rep_s, bool rep_t)
{
    return do_make_texture(img, name, rep_s, rep_t);
}

inline void SpatialSceneBuilder::bind_texture(int texture_index)
{
    do_bind_texture(texture_index);
}

inline void SpatialSceneBuilder::tex_translate(math::Vec2 v)
{
    do_tex_translate(v);
}

inline void SpatialSceneBuilder::tex_scale(math::Vec2 s)
{
    do_tex_scale(s);
}

inline void SpatialSceneBuilder::tex_translate(double s, double t)
{
    do_tex_translate(math::Vec2(s,t));
}

inline void SpatialSceneBuilder::tex_scale(double f)
{
    tex_scale(f,f);
}

inline void SpatialSceneBuilder::tex_scale(double s, double t)
{
    do_tex_scale(math::Vec2(s,t));
}

inline void SpatialSceneBuilder::tex_rotate(double angle)
{
    do_tex_rotate(angle);
}

inline void SpatialSceneBuilder::reset_tex_transform()
{
    do_reset_tex_transform();
}

inline int SpatialSceneBuilder::do_make_texture(std::string image_path, bool rep_s, bool rep_t)
{
    return do_make_texture(image::Image::load(image_path), image_path, rep_s, rep_t);
}

} // namespace graphics
} // namespace archon

#endif // ARCHON_GRAPHICS_SCENE_BUILDER_HPP
