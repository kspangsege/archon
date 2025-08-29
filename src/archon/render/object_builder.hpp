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

#ifndef ARCHON_X_RENDER_X_OBJECT_BUILDER_HPP
#define ARCHON_X_RENDER_X_OBJECT_BUILDER_HPP

/// \file


#include <optional>
#include <vector>

#include <archon/core/integer.hpp>
#include <archon/math/vector.hpp>
#include <archon/display/opengl.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

namespace archon::render {


class object_builder {
public:
    enum class primitive {
        quads,
    };

    void begin(primitive);
    void end();

    void normal(const math::Vector3F&);

    void vertex(const math::Vector3F&);

    class descriptor;

    auto create() -> descriptor;

private:
    using promoted_vertex_index_type = decltype(GLuint() + unsigned());

    math::Vector3F m_normal = { 0, 0, 1 };
    std::vector<GLfloat> m_components;
    std::vector<GLuint> m_indices;

    std::optional<primitive> m_primitive;
    promoted_vertex_index_type m_num_vertices = 0;
    promoted_vertex_index_type m_vertex_offset = 0;

    template<class V> void push_components(const V& vec);
    void generate_quads();
};



class object_builder::descriptor {
public:
    descriptor() noexcept = default;
    void configure(GLuint positions_index, GLuint normals_index);
    void draw();


private:
    GLsizei m_num_indices = 0;

    descriptor(GLsizei num_indices) noexcept;

    friend class object_builder;
};








// Implementation


inline void object_builder::normal(const math::Vector3F& normal)
{
    m_normal = normal;
}


inline void object_builder::vertex(const math::Vector3F& vertex)
{
    push_components(vertex); // Throws
    push_components(m_normal); // Throws
    core::int_add(m_num_vertices, 1); // Throws
}


template<class V> inline void object_builder::push_components(const V& vec)
{
    constexpr int n = V::size;
    for (int i = 0; i < n; ++i)
        m_components.push_back(vec[i]); // Throws
}


inline object_builder::descriptor::descriptor(GLsizei num_indices) noexcept
    : m_num_indices(num_indices)
{
}


} // namespace archon::render

#endif // ARCHON_DISPLAY_HAVE_OPENGL

#endif // ARCHON_X_RENDER_X_OBJECT_BUILDER_HPP
