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

#ifndef ARCHON_X_GFX_X_SCENE_BUILDER_HPP
#define ARCHON_X_GFX_X_SCENE_BUILDER_HPP

/// \file


                                   
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/image/image.hpp>
#include <archon/gfx/object_builder.hpp>


namespace archon::gfx {


class scene_builder
    : public gfx::object_builder {
public:
    /// \brief Push vertex coordinate transformation.
    ///
    /// This function pushes the current vertex coordinate transformation onto the vertex
    /// coordinate transformation stack.
    ///
    /// \sa \ref pop()
    ///
    void push();

    /// \brief Pop vertex coordinate transformation.
    ///
    /// This function pop the last pushed vertex coordinate transformation from the vertex
    /// coordinate transformation stack and makes it current.
    ///
    /// \sa \ref push()
    ///
    void pop();

    void translate(const math::Vector3& vec);

    void scale(double factor);
    void scale(double x_factor, double y_factor, double z_factor);

    /// \brief Apply rotation to modelview matrix.
    ///
    /// This function applies the specified rotation to the current modelview matrix. This
    /// causes subsequently added vertices to be rotated as specified realtive to where they
    /// would appear without the rotation. The axis must be a unit vector.
    ///
    void rotate(const math::Rotation& rot);

    class Texture;

    auto register_texture(const image::Image&, bool repeat) -> Texture;
    auto register_texture(const image::Image&, bool repeat_s, bool vert_repeat_t) -> Texture;
/*
    int make_texture(std::string_view image_path, bool repeat_s, bool repeat_t);
    int make_texture(std::string_view image_path, bool repeat = true);
    int make_texture(const image::Image&, std::string name, bool repeat);
    int make_texture(const image::Image&, std::string name, bool repeat_s, bool vert_repeat_t);
*/

    void bind_texture(Texture tex);

    void tex_translate(const math::Vector2& vec);

    void tex_scale(double factor);
    void tex_scale(double s_factor, double t_factor);

    /// \brief    
    ///
    /// The angle is specified in radians.
    ///
    void tex_rotate(double angle);

    /// \brief Reset texture coordinate transformation.
    ///
    /// This function resets the current texture coordinate transformation to an identity
    /// transformation.
    ///
    void reset_tex_transform();

protected:
    virtual void do_push() = 0;
    virtual void do_pop() = 0;

    virtual void do_translate(const math::Vector3&) = 0;
    virtual void do_scale(double x_factor, double y_factor, double z_factor) = 0;
    virtual void do_rotate(const math::Rotation&) = 0;

    virtual auto do_register_texture(const image::Image&, bool repeat_s, bool repeat_t) -> Texture = 0;

    virtual void do_bind_texture(int texture_index) = 0;

    virtual void do_tex_translate(math::Vec2 v) = 0;
    virtual void do_tex_scale(double s_factor, double t_factor) = 0;
    virtual void do_tex_rotate(double radians) = 0;

    virtual void do_reset_tex_transform() = 0;

/*
    virtual int add_directional_light(math::Vec3 dir) = 0;  
    virtual int add_point_light(math::Vec3 pos) = 0;
    virtual int add_spot_light_light() = 0;
    virtual void remove_light() = 0;
*/
};








// Implementation


inline void scene_builder::push()
{
    do_push(); // Throws
}


inline void scene_builder::pop()
{
    do_pop(); // Throws
}


inline void scene_builder::translate(const math::Vector3& vec)
{
    do_translate(vec); // Throws
}


inline void scene_builder::scale(double factor)
{
    scale(factor, factor, factor); // Throws
}


inline void scene_builder::scale(double x_factor, double y_factor, double z_factor)
{
    do_scale(s); // Throws
}


inline void scene_builder::rotate(const math::Rotation& rot)
{
    do_rotate(r); // Throws
}


inline int scene_builder::register_texture(const image::Image& image, bool repeat)
{
    return register_texture(image, repeat, repeat); // Throws
}


inline int scene_builder::register_texture(const image::Image& image, bool s_repeat, bool t_repeat)
{
    return do_register_texture(image, s_repeat, t_repeat); // Throws
}


     


inline void scene_builder::bind_texture(int texture_index)
{
    do_bind_texture(texture_index);
}


inline void scene_builder::tex_translate(math::Vec2 v)
{
    do_tex_translate(v);
}


inline void scene_builder::tex_scale(math::Vec2 s)
{
    do_tex_scale(s);
}


inline void scene_builder::tex_translate(double s, double t)
{
    do_tex_translate(math::Vec2(s,t));
}


inline void scene_builder::tex_scale(double f)
{
    tex_scale(f,f);
}


inline void scene_builder::tex_scale(double s, double t)
{
    do_tex_scale(math::Vec2(s,t));
}


inline void scene_builder::tex_rotate(double angle)
{
    do_tex_rotate(angle);
}


inline void scene_builder::reset_tex_transform()
{
    do_reset_tex_transform();
}


inline int scene_builder::do_make_texture(std::string image_path, bool rep_s, bool rep_t)
{
    return do_make_texture(image::Image::load(image_path), image_path, rep_s, rep_t);
}


} // namespace archon::gfx

#endif // ARCHON_X_GFX_X_SCENE_BUILDER_HPP
