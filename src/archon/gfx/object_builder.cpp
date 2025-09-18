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


#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>
#include <archon/gfx/math.hpp>
#include <archon/gfx/object_builder.hpp>


using namespace archon;
using object_builder = gfx::object_builder;


void object_builder::push()
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    switch (m_matrix_mode) {
        case matrix_mode::coord_transform:
            m_coord_transform_stack.push_back(m_coord_transform); // Throws
            return;
        case matrix_mode::tex_coord_transform:
            m_tex_coord_transform_stack.push_back(m_tex_coord_transform); // Throws
            return;
    }
    ARCHON_ASSERT_UNREACHABLE();
}


void object_builder::pop() noexcept
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    switch (m_matrix_mode) {
        case matrix_mode::coord_transform:
            m_coord_transform = m_coord_transform_stack.back();
            m_coord_transform_stack.pop_back();
            m_have_normal_transform = false;
            return;
        case matrix_mode::tex_coord_transform:
            m_tex_coord_transform = m_tex_coord_transform_stack.back();
            m_tex_coord_transform_stack.pop_back();
            return;
    }
    ARCHON_ASSERT_UNREACHABLE();
}


void object_builder::translate(const math::Vector3F& vec) noexcept
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    switch (m_matrix_mode) {
        case matrix_mode::coord_transform:
            gfx::translate(m_coord_transform.transform, vec);
            m_have_normal_transform = false;
            return;
        case matrix_mode::tex_coord_transform:
            gfx::translate(m_tex_coord_transform, vec);
            return;
    }
    ARCHON_ASSERT_UNREACHABLE();
}


void object_builder::scale(float x_factor, float y_factor, float z_factor) noexcept
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    switch (m_matrix_mode) {
        case matrix_mode::coord_transform: {
            gfx::scale(m_coord_transform.transform, x_factor, y_factor, z_factor);
            bool nonuniform_scalation = (y_factor != x_factor || z_factor != x_factor);
            if (nonuniform_scalation)
                m_coord_transform.maybe_nonorthogonal = true;
            m_have_normal_transform = false;
            return;
        }
        case matrix_mode::tex_coord_transform:
            gfx::scale(m_tex_coord_transform, x_factor, y_factor, z_factor);
            return;
    }
    ARCHON_ASSERT_UNREACHABLE();
}


void object_builder::rotate(const math::Rotation& rot) noexcept
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    switch (m_matrix_mode) {
        case matrix_mode::coord_transform:
            gfx::rotate(m_coord_transform.transform, rot.axis, rot.angle);
            m_have_normal_transform = false;
            return;
        case matrix_mode::tex_coord_transform:
            gfx::rotate(m_tex_coord_transform, rot.axis, rot.angle);
            return;
    }
    ARCHON_ASSERT_UNREACHABLE();
}


void object_builder::begin(primitive prim)
{
    ARCHON_ASSERT(!m_in_primitive_definition);
    m_primitive = prim;
    m_primitive_definition_started = true;
    m_in_primitive_definition = true;
    do_begin(prim); // Throws
}


void object_builder::end()
{
    ARCHON_ASSERT(m_in_primitive_definition);
    do_end(m_primitive); // Throws
    m_in_primitive_definition = false;
}


void object_builder::set_normal(const math::Vector3F& normal) noexcept
{
    m_normal = normal;
    m_have_transformed_normal = false;
}


void object_builder::set_color(const math::Vector4F& color) noexcept
{
    m_color = color;
}


void object_builder::set_tex_coord(const math::Vector3F& tex_coord) noexcept
{
    m_tex_coord = tex_coord;
    m_have_transformed_tex_coord = false;
}


void object_builder::add_vertex(const math::Vector3F& coord)
{
    ARCHON_ASSERT(m_in_primitive_definition);
    math::Vector3F coord_2 = coord;
    gfx::transform(coord_2, m_coord_transform.transform);
    do_add_vertex(coord_2); // Throws
}


void object_builder::reset() noexcept
{
    m_matrix_mode = matrix_mode::coord_transform;
    m_coord_transform_stack.clear();
    m_tex_coord_transform_stack.clear();
    m_coord_transform = { math::Matrix4F::identity(), false };
    m_tex_coord_transform = math::Matrix4F::identity();
    m_normal = { 0, 0, 1 };
    m_color = { 1, 1, 1, 1 };
    m_tex_coord = { 0, 0, 0 };
    m_primitive_definition_started = false;
    m_in_primitive_definition = false;
    m_have_normal_transform = false;
    m_have_transformed_normal = false;
    m_have_transformed_tex_coord = false;
}


auto object_builder::transform_normal() noexcept -> math::Vector3F
{
    ARCHON_ASSERT(!m_have_transformed_normal);
    if (ARCHON_UNLIKELY(!m_have_normal_transform))
        update_normal_transform();
    math::Vector3F normal = m_normal_transform * m_normal;
    m_transformed_normal = normal;
    m_have_transformed_normal = true;
    return normal;
}


auto object_builder::transform_tex_coord() noexcept -> math::Vector3F
{
    ARCHON_ASSERT(!m_have_transformed_tex_coord);
    math::Vector3F tex_coord = m_tex_coord;
    gfx::transform(tex_coord, m_tex_coord_transform);
    m_transformed_tex_coord = tex_coord;
    m_have_transformed_tex_coord = true;
    return tex_coord;
}


void object_builder::update_normal_transform() noexcept
{
    ARCHON_ASSERT(!m_have_normal_transform);
    m_normal_transform = math::Matrix3F(m_coord_transform.transform);
    if (m_coord_transform.maybe_nonorthogonal)
        m_normal_transform = transpose(inv(m_normal_transform));
    m_have_normal_transform = true;
}
