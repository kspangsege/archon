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


#include <cstddef>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/math/vector.hpp>
#include <archon/gfx/object_builder.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/object_builder.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

using namespace archon;
using render::object_builder;


void object_builder::set_attrib_layout(core::Span<const vertex_attrib> layout)
{
    if (primitive_definition_started())
        throw std::runtime_error("Attribute layout change after primitive definition");
    m_attrib_layout.assign(layout.begin(), layout.end()); // Throws
}


auto object_builder::create() -> GLsizei
{
    if (ARCHON_UNLIKELY(in_primitive_definition()))
        throw std::runtime_error("Create while in primitive definition block");

    GLsizei num_indices = {};
    core::int_cast(m_indices.size(), num_indices); // Throws

    GLsizeiptr size = {};
    core::int_cast(m_components.size(), size); // Throws
    core::int_mul(size, sizeof (GLfloat)); // Throws
    glBufferData(GL_ARRAY_BUFFER, size, m_components.data(), GL_STATIC_DRAW);
    core::int_cast(num_indices, size); // Throws
    core::int_mul(size, sizeof (GLuint)); // Throws
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, m_indices.data(), GL_STATIC_DRAW);

    return num_indices;
}


void object_builder::reset() noexcept
{
    gfx::object_builder::reset();
    m_components.clear();
    m_indices.clear();
    m_num_vertices = 0;
}


void object_builder::configure_attribs(core::Span<const vertex_attrib> layout,
                                       core::Span<const std::pair<vertex_attrib, GLuint>> layout_map)
{
    core::FlatMap<vertex_attrib, std::pair<int, std::size_t>, 12> map;
    GLsizei stride = {};
    {
        std::size_t offset = 0;
        for (vertex_attrib attrib : layout) {
            int size = get_attrib_size(attrib);
            map.emplace(attrib, std::pair(size, offset)); // Throws
            core::int_add(offset, size * sizeof (GLfloat)); // Throws
        }
        core::int_cast(offset, stride); // Throws
    }
    for (auto entry : layout_map) {
        vertex_attrib attrib = entry.first;
        auto p = map.at(attrib); // Throws
        GLuint location = entry.second;
        GLint size = GLint(p.first);
        GLenum type = GL_FLOAT;
        GLboolean normalized = GL_FALSE;
        std::size_t offset = p.second;
        glVertexAttribPointer(location, size, type, normalized, stride, reinterpret_cast<void*>(offset));
    }
}


void object_builder::configure_and_enable_attribs(core::Span<const vertex_attrib> layout,
                                                  core::Span<const std::pair<vertex_attrib, GLuint>> layout_map)
{
    configure_attribs(layout, layout_map); // Throws
    for (auto entry : layout_map) {
        GLuint location = entry.second;
        glEnableVertexAttribArray(location);
    }
}


void object_builder::draw(GLsizei num_indices)
{
    GLsizei count = num_indices;
    GLenum type = GL_UNSIGNED_INT;
    std::size_t offset = 0;
    glDrawElements(GL_TRIANGLES, count, type, reinterpret_cast<void*>(offset));
}


int object_builder::get_attrib_size(vertex_attrib attrib) noexcept
{
    switch (attrib) {
        case vertex_attrib::coord_3:
            return 3;
        case vertex_attrib::normal_3:
            return 3;
        case vertex_attrib::color_3:
            return 3;
        case vertex_attrib::color_4:
            return 4;
        case vertex_attrib::tex_coord_2:
            return 2;
        case vertex_attrib::tex_coord_3:
            return 3;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


void object_builder::do_add_vertex(const math::Vector3F& coord)
{
    for (vertex_attrib attrib : m_attrib_layout) {
        switch (attrib) {
            case vertex_attrib::coord_3: {
                push_components(coord); // Throws
                break;
            }
            case vertex_attrib::normal_3: {
                math::Vector3F normal = get_transformed_normal();
                push_components(normal); // Throws
                break;
            }
            case vertex_attrib::color_3: {
                math::Vector4F color = get_color();
                push_components(math::Vector3F(color)); // Throws
                break;
            }
            case vertex_attrib::color_4: {
                math::Vector4F color = get_color();
                push_components(color); // Throws
                break;
            }
            case vertex_attrib::tex_coord_2: {
                math::Vector3F tex_coord = get_transformed_tex_coord();
                push_components(math::Vector2F(tex_coord)); // Throws
                break;
            }
            case vertex_attrib::tex_coord_3: {
                math::Vector3F tex_coord = get_transformed_tex_coord();
                push_components(tex_coord); // Throws
                break;
            }
        }
    }
    core::int_add(m_num_vertices, 1); // Throws
}


void object_builder::do_begin(primitive)
{
    m_vertex_offset = m_num_vertices;
}


void object_builder::do_end(primitive prim)
{
    switch (prim) { // Throws
        case primitive::trifan:
            generate_trifan(); // Throws
            break;
        case primitive::quads:
            generate_quads(); // Throws
            break;
        case primitive::quad_strip:
            generate_quad_strip(); // Throws
            break;
        case primitive::polygon:
            // FIXME: Not yet any support for an edge flag, so a polygon is effectively the
            // same as a triangle fan.
            generate_trifan(); // Throws
            break;
    }
}


template<class V> inline void object_builder::push_components(const V& vec)
{
    constexpr int n = V::size;
    for (int i = 0; i < n; ++i)
        m_components.push_back(vec[i]); // Throws
}


void object_builder::generate_trifan()
{
    std::size_t offset = m_vertex_offset;
    std::size_t size = std::size_t(m_num_vertices - offset);
    if (ARCHON_UNLIKELY(size < 2))
        throw std::runtime_error("Invalid number of vertices for trifan");
    std::size_t n =  std::size_t(size - 2);
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t a = std::size_t(i + 1);
        std::size_t b = std::size_t(i + 2);
        add_tri(offset, 0, a, b); // Throws
    }
}


void object_builder::generate_quads()
{
    std::size_t offset = m_vertex_offset;
    std::size_t size = std::size_t(m_num_vertices - offset);
    if (ARCHON_UNLIKELY(size % 4 != 0))
        throw std::runtime_error("Invalid number of vertices for separate quads");
    std::size_t n =  std::size_t(size / 4);
    for (std::size_t i = 0; i < n; ++i) {
        add_quad(offset, 0, 1, 2, 3); // Throws
        offset += 4;
    }
    ARCHON_ASSERT(offset == m_num_vertices);
}


void object_builder::generate_quad_strip()
{
    std::size_t offset = m_vertex_offset;
    std::size_t size = std::size_t(m_num_vertices - offset);
    if (ARCHON_UNLIKELY(size < 2 || (size - 2) % 2 != 0))
        throw std::runtime_error("Invalid number of vertices for quad strip");
    std::size_t n =  std::size_t((size - 2) / 2);
    for (std::size_t i = 0; i < n; ++i) {
        add_quad(offset, 0, 1, 3, 2); // Throws
        offset += 2;
    }
    offset += 2;
    ARCHON_ASSERT(offset == m_num_vertices);
}


void object_builder::add_quad(std::size_t offset, std::size_t a, std::size_t b, std::size_t c, std::size_t d)
{
    add_tri(offset, a, b, c); // Throws
    add_tri(offset, c, d, a); // Throws
}


void object_builder::add_tri(std::size_t offset, std::size_t a, std::size_t b, std::size_t c)
{
    add_index(offset, a); // Throws
    add_index(offset, b); // Throws
    add_index(offset, c); // Throws
}


void object_builder::add_index(std::size_t offset, std::size_t i)
{
    GLuint i_2 = {};
    core::int_cast(offset, i_2); // Throws
    core::int_add(i_2, i); // Throws
    m_indices.push_back(i_2); // Throws
}


#endif // ARCHON_DISPLAY_HAVE_OPENGL
