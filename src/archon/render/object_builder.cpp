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

#include <archon/core/integer.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/object_builder.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

using namespace archon;
using render::object_builder;


void object_builder::begin(primitive prim)
{
    ARCHON_ASSERT(!m_primitive.has_value());
    m_primitive = prim;
    m_vertex_offset = m_num_vertices;
}


void object_builder::end()
{
    primitive prim = m_primitive.value(); // Throws
    switch (prim) { // Throws
        case primitive::quads:
            generate_quads(); // Throws
            break;
    }
    m_primitive.reset();
}


auto object_builder::create() -> descriptor
{
    GLsizei num_indices = {};
    core::int_cast(m_indices.size(), num_indices); // Throws

    GLsizeiptr size = {};
    core::int_cast(m_components.size(), size); // Throws
    core::int_mul(size, sizeof (GLfloat)); // Throws
    glBufferData(GL_ARRAY_BUFFER, size, m_components.data(), GL_STATIC_DRAW);
    core::int_cast(num_indices, size); // Throws
    core::int_mul(size, sizeof (GLuint)); // Throws
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, m_indices.data(), GL_STATIC_DRAW);

    return descriptor(num_indices);
}


void object_builder::generate_quads()
{
    promoted_vertex_index_type offset = m_vertex_offset;
    promoted_vertex_index_type n =  promoted_vertex_index_type((m_num_vertices - offset) / 4);
    for (promoted_vertex_index_type i = 0; i < n; ++i) {
        for (int j : { 0, 1, 2, 2, 3, 0 })
            m_indices.push_back(GLuint(offset + j)); // Throws
        offset += 4;
    }
    ARCHON_ASSERT(offset == m_num_vertices);
}


void object_builder::descriptor::configure(GLuint positions_index, GLuint normals_index)
{
    {
        GLint size = 3;
        GLenum type = GL_FLOAT;
        GLboolean normalized = GL_FALSE;
        GLsizei stride = 6 * sizeof (GLfloat);
        std::size_t offset = 0;
        glVertexAttribPointer(positions_index, size, type, normalized, stride, reinterpret_cast<void*>(offset));
    }

    {
        GLint size = 3;
        GLenum type = GL_FLOAT;
        GLboolean normalized = GL_FALSE;
        GLsizei stride = 6 * sizeof (GLfloat);
        std::size_t offset = 3 * sizeof (GLfloat);
        glVertexAttribPointer(normals_index, size, type, normalized, stride, reinterpret_cast<void*>(offset));
    }
}


void object_builder::descriptor::draw()
{
    GLsizei count = m_num_indices;
    GLenum type = GL_UNSIGNED_INT;
    std::size_t offset = 0;
    glDrawElements(GL_TRIANGLES, count, type, reinterpret_cast<void*>(offset));
}




#endif // ARCHON_DISPLAY_HAVE_OPENGL
