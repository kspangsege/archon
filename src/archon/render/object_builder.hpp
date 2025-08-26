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


#include <cstddef>
#include <utility>
#include <vector>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/math/vector.hpp>
#include <archon/gfx/object_builder.hpp>
#include <archon/display/opengl.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL || defined DOXYGEN

namespace archon::render {


/// \brief Object builder implementation targeting OpenGL 4.1.
///
/// This class is a full implementation of the object builder (\ref gfx::object_builder)
/// that targets OpenGL 4.1.
///
/// The application must first specify an attribute layout in the vertex buffer that will
/// ultimately be created. The layout is specified by calling \ref set_attrib_layout().
///
class object_builder
    : public gfx::object_builder {
public:
    /// \brief Available per-vertex attributes to be included in vertex buffer.
    ///
    /// These are the available per-vertex attributes that can be included in the created
    /// OpenGL vertex buffer (\ref create()).
    ///
    /// \sa \ref set_attrib_layout()
    ///
    enum class vertex_attrib {
        coord_3,     ///< 3-component vertex coordinate (X, Y, Z).
        normal_3,    ///< 3-component vertex normal (X, Y, Z).
        color_3,     ///< 3-component color (R, G, B).
        color_4,     ///< 4-component color (R, G, B, A).
        tex_coord_2, ///< 2-component texture coordinate (S, T).
        tex_coord_3, ///< 3-component texture coordinate (S, T, R).
    };

    /// \brief Set layout of attribute in vertex buffer.
    ///
    /// This function sets the layout of vertex attributes as it will be in the vertex
    /// buffer that will ultimately be created (\ref create()). It is an error to call this
    /// function after initiation of a primitive definition block without a reset (\ref
    /// gfx::object_builder::reset()).
    ///
    /// The default layout is empty.
    ///
    void set_attrib_layout(core::Span<const vertex_attrib> layout);

    /// \brief Produce VBO + EBO contents for generated triangles.
    ///
    /// This function updates the currently bound vertex buffer object (VBO) and element
    /// buffer object (EBO) with new contents. The contents of the vertex buffer becomes as
    /// specified by the current attribute layout (\ref set_attrib_layout). I.e., there will
    /// be a block of components for every added vertex, and the components in each block
    /// will be as specified by the layout.
    ///
    /// The contents of the element buffer becomes the vertex indices corresponding to the
    /// generated triangles. This assumes that `glDrawElements()` will be called with an
    /// argument of `GL_TRIANGLES`.
    ///
    /// This function returns the number of generated vertex indices, which is the number
    /// that should be passed to `glDrawElements()`, but see \ref draw() for a helper
    /// function that does that.
    ///
    /// Before the generated triangles can be rendered, an OpenGL vertex array object (VAO)
    /// must have be created and configured. See \ref configure_attribs() and \ref
    /// configure_and_enable_attribs() for functions that help do that.
    ///
    /// It is an error to call this function while in a primitive definition block.
    ///
    auto create() -> GLsizei;

    /// \brief Reset object builder to its initial state.
    ///
    /// See \ref gfx::object_builder::reset().
    ///
    void reset() noexcept override;

    /// \brief Helper function for OpenGL attribute configuration.
    ///
    /// This helper function configures OpenGL vertex attributes of the currently bound
    /// vertex array object (VAO) in accordance with the specified layout (\p layout), which
    /// should be the same layout as was set for the object builder (\ref
    /// set_attrib_layout()). It does that using `glVertexAttribPointer()`.
    ///
    /// The layout map (\p layout_map) specifies the mapping of attributes to attribute
    /// locations. The same attribute is allowed to map to more than one location. If a
    /// location is specified more than once, the last one wins. If the layout contains a
    /// particular attribute more than once, the one that will be mapped to a location is
    /// the one that occurs first in the layout.
    ///
    /// \sa \ref configure_and_enable_attribs()
    ///
    static void configure_attribs(core::Span<const vertex_attrib> layout,
                                  core::Span<const std::pair<vertex_attrib, GLuint>> layout_map);

    /// \brief Helper function for OpenGL attribute configuration and enablement.
    ///
    /// This function does what \ref configure_attribs() does. Additionally, it enables each
    /// of the attribute locations mentioned in \p layout_map using
    /// `glEnableVertexAttribArray()`.
    ///
    static void configure_and_enable_attribs(core::Span<const vertex_attrib> layout,
                                             core::Span<const std::pair<vertex_attrib, GLuint>> layout_map);

    /// \brief Draw triangles
    ///
    /// This function draws the triangles referenced by the currently bound OpenGL vertex
    /// array object (VAO). It does that using `glDrawElements()` with an argument of
    /// `GL_TRIANGLES`. The specified number of vertex indices (\p num_indices) must be the
    /// number that was returned by \ref create().
    ///
    static void draw(GLsizei num_indices);

    /// \brief Get number of components in particular vertex attribute.
    ///
    /// This function returns the number of components per added vertex that will be placed
    /// in the vertex buffer on behalf of the specified attribute.
    ///
    static int get_attrib_size(vertex_attrib) noexcept;

private:
    std::vector<vertex_attrib> m_attrib_layout;
    std::vector<GLfloat> m_components;
    std::vector<GLuint> m_indices;

    std::size_t m_num_vertices = 0;
    std::size_t m_vertex_offset = {};

    void do_begin(primitive) override;
    void do_end(primitive) override;
    void do_add_vertex(const math::Vector3F&) override;

    template<class V> void push_components(const V& vec);
    void generate_trifan();
    void generate_quads();
    void generate_quad_strip();
    void add_quad(std::size_t offset, std::size_t a, std::size_t b, std::size_t c, std::size_t d);
    void add_tri(std::size_t offset, std::size_t a, std::size_t b, std::size_t c);
    void add_index(std::size_t offset, std::size_t i);
};


} // namespace archon::render

#endif // ARCHON_DISPLAY_HAVE_OPENGL || defined DOXYGEN

#endif // ARCHON_X_RENDER_X_OBJECT_BUILDER_HPP
