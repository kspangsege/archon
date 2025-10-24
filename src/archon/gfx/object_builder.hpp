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


#include <optional>
#include <vector>

#include <archon/core/features.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>


namespace archon::gfx {


/// \brief Facility for construction of vertex-based surfaces.
///
/// This is an abstract class that helps facilitate the construction of vertex-based 3-D
/// surfaces in a style similar to the fixed-function pipeline of OpenGL 2.1. Concrete
/// subclasses build up an object representation suitable for a particular type of rendering
/// API. For example, \ref render::object_builder targets OpenGL 4.1.
///
/// Having one shared base class that is able to target different rendering APIs avoid the
/// need for duplication of object building code. In particular, it allows for a library of
/// functions that build particular shapes. See \ref gfx::build_box(), \ref
/// gfx::build_cylinder(), \ref gfx::build_cone(), \ref gfx::build_sphere(), and \ref
/// gfx::build_torus().
///
/// A surface can be built from different types of primitives (\ref primitive). In general,
/// the object building process consists of a sequence of primitive definition blocks (\ref
/// begin(), \ref end()). Within each primitive definition block, geometric primitives are
/// generated in response to the addition of vertices (\ref add_vertex()). Each added vertex
/// gets associated with a normal (\ref set_normal()), a color (\ref set_color()), and a
/// texture coordinate (\ref set_tex_coord()).
///
/// The object builder maintains a current coordinate transformation in the form of a 4-by-4
/// homogeneous transformation matrix. This allows for translation as well as scaling and
/// rotation. When a vertex is added, the specified coordinate gets transformed using the
/// current coordinate transformation. At the same time, that is when the vertex is added,
/// the current normal, as it was last specified, is transformed by the inverse transpose of
/// the upper-left 3-by-3 submatrix of the current coordinate transformation. This
/// transformed normal is the normal that the vertex gets associated with.
///
/// The object builder also maintains a current texture coordinate transformation. This one
/// also takes the form of a 4-by-4 homogeneous transformation matrix. When a vertex is
/// added, the current texture coordinate, as it was last specified, is transformed by the
/// current texture coordinate transformation. This transformed texture coordinate is the
/// texture coordinate that the vertex gets associated with.
///
/// The coordinate transformation and the current texture coordinate transformation can be
/// modified using \ref translate(), \ref scale(), and \ref rotate(). The current matrix
/// mode (\ref set_matrix_mode()) determines which transformation gets modified.
///
/// For each type of transformation, the object builder also maintains a stack of previous
/// transformations that can be returned to. The current transformation is pushed onto the
/// stack by \ref push(). That transformation can then be restored later using \ref
/// pop(). The current matrix mode also determines which transformation is targeted by \re
/// push() and \ref pop().
///
/// Transformation modification is not allowed inside a primitive definition block. Vertex
/// addition is only allowed inside a primitive definition block. Vertex attributes (normal,
/// color, texture coordinate) may be modified both inside and outside of a primitive
/// definition block.
///
/// \sa \ref render::object_builder
///
class object_builder {
public:
    /// \brief Available matrix modes.
    ///
    /// These are the available matrix modes. There is one mode for each type of
    /// transformation that is maintained by the object builder. The selected matrix mode
    /// determines the transformation that is affected by transformation changing functions
    /// such as \ref translate(). The matrix mode is selected using \ref set_matrix_mode().
    ///
    /// The "coordinate" transformation, selected by `coord_transform` matrix mode, is used
    /// to transform the vertex coordinates passed to \ref add_vertex(). The transpose of
    /// the inverse of the upper-left 3-by-3 submatrix of this transformation it is also
    /// used to transform normals as passed to \ref set_normal().
    ///
    /// The "texture coordinate" transformation, selected by `tex_coord_transform` matrix
    /// mode, is used to transform texture coordinates as passed to \ref set_tex_coord().
    ///
    enum class matrix_mode {
        coord_transform,
        tex_coord_transform,
    };

    /// \brief Select "coordinate" matrix mode,
    ///
    /// This function is shorthand for passing `matrix_mode::coord_transform` to \ref
    /// set_matrix_mode(). See \ref matrix_mode for documentation of each mode.
    ///
    void matrix_mode_coord() noexcept;

    /// \brief Select "texture coordinate" matrix mode,
    ///
    /// This function is shorthand for passing `matrix_mode::tex_coord_transform` to \ref
    /// set_matrix_mode(). See \ref matrix_mode for documentation of each mode.
    ///
    void matrix_mode_tex_coord() noexcept;

    /// \brief Set matrix mode for subsequent transformation changes.
    ///
    /// This function sets the current matrix mode. The matrix mode controls which one of
    /// the available transformations is affected by subsequent transformation changes (\ref
    /// push(), \ref pop(), \ref translate(), \ref scale(), \ref rotate()). See \ref
    /// matrix_mode for the options.
    ///
    void set_matrix_mode(matrix_mode) noexcept;

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
    void pop() noexcept;

    /// \brief Apply translation to vertex coordinate transformation.
    ///
    /// This function applies the specified translation to the current vertex coordinate
    /// transformation. The resulting transformation will apply to subsequently added
    /// vertices. The resulting transformation will first translate vertex coordinates as
    /// specified and then subject them to the original transformation, i.e., the
    /// transformation as it was before the invocation of `translate()`.
    ///
    /// This function corresponds directly to the legacy OpenGL function `glTranslatef()`.
    ///
    void translate(const math::Vector3F& vec) noexcept;

    /// \{
    ///
    /// \brief Apply scalation to vertex coordinate transformation.
    ///
    /// These functions apply the specified scalation to the current vertex coordinate
    /// transformation. The resulting transformation will apply to subsequently added
    /// vertices. The resulting transformation will first scale vertex coordinates as
    /// specified and then subject them to the original transformation, i.e., the
    /// transformation as it was before the invocation of `scale()`.
    ///
    /// These functions correspond directly to the legacy OpenGL function `glScalef()`.
    ///
    void scale(float factor) noexcept;
    void scale(float x_factor, float y_factor, float z_factor) noexcept;
    /// \}

    /// \brief Apply rotation to vertex coordinate transformation.
    ///
    /// This function applies the specified rotation to the current vertex coordinate
    /// transformation. The resulting transformation will apply to subsequently added
    /// vertices. The resulting transformation will first rotate vertex coordinates as
    /// specified and then subject them to the original transformation, i.e., the
    /// transformation as it was before the invocation of `rotate()`.
    ///
    /// This function corresponds directly to the legacy OpenGL function `glRotatef()`
    /// except that the angle here is expressed in radians and not degrees.
    ///
    void rotate(const math::Rotation& rot) noexcept;

    /// \brief Available generable primitives.
    ///
    /// These are the primitives that can be generated by the object builder. See \ref
    /// begin() for documentation on each of them.
    ///
    enum class primitive {
        trifan,
        quads,
        quad_strip,
        polygon,
    };

    /// \brief Begin generation of triangle-fan primitive.
    ///
    /// This function is shorthand for passing `primitive::polygon` to \ref begin(). See
    /// \ref begin() for documentation on generation of triangle-fan primitives.
    ///
    void begin_trifan();

    /// \brief Begin generation of separate quad primitives.
    ///
    /// This function is shorthand for passing `primitive::quads` to \ref begin(). See \ref
    /// begin() for documentation on generation of separate quad primitives.
    ///
    void begin_quads();

    /// \brief Begin generation of quad-strip primitive.
    ///
    /// This function is shorthand for passing `primitive::quad_strip` to \ref begin(). See
    /// \ref begin() for documentation on generation of quad-strip primitives.
    ///
    void begin_quad_strip();

    /// \brief Begin generation of polygon primitive.
    ///
    /// This function is shorthand for passing `primitive::polygon` to \ref begin(). See
    /// \ref begin() for documentation on generation of polygon primitives.
    ///
    void begin_polygon();

    /// \brief Initiate primitive definition block.
    ///
    /// This function initiates a primitive definition block for primitives of the specified
    /// type (\p prim).
    ///
    /// A primitive definition block defines one or more geometric primitives of a
    /// particular type. For some types of primitives, e.g. `primitive::quads`, multiple
    /// independent primitives are generated as new vertices are added using \ref
    /// add_vertex(), and this continues until \ref end() is called. For other types, such
    /// as `primitive::quad_strip`, a single more complex primitive is generated. See below
    /// for full documentation on each type of primitive.
    ///
    /// See the class-level documentation (\ref object_builder) for information on what
    /// functions are, and are not allowed inside a primitive definition block.
    ///
    /// In general, if an invalid number of vertices have been added when \ref end() is
    /// called, an exception will be thrown.
    ///
    /// **Triangle fan:** When `begin()` is called with an argument of `primitive::trifan`,
    /// a single triangle fan will be defined. A triangle fan is a sequence of connected
    /// triangles that all share the first vertex. After the first two vertices have been
    /// added, one new triangle is defined for each subsequent vertex, so if N vertices are
    /// added in total, the triangle fan will contain N-2 triangles. The i'th triangle,
    /// counting from zero, will be formed from vertices 0, i+1, and i+2 in that order. For
    /// example, if 4 vertices are added, two triangles will be defined; the first one out
    /// of vertices 0, 1, and 2; and the second one out of vertices 0, 2, and 3. A valid
    /// number of vertices for a triangle fan is n+2 where n >= 0.
    ///
    /// **Separate quadrilaterals:** When `begin()` is called with an argument of
    /// `primitive::quads`, a sequence of individual, unconnected quadrilaterals will be
    /// defined. One new quadrilateral is defined for every group of four vertices added, so
    /// if N vertices are added, N/4 quadrilaterals will be defined. The i'th quadrilateral,
    /// counting from zero, is formed from vertices 4i, 4i+1, 4i+2, 4i+3 in that order. For
    /// example, if 8 vertices are added, two quadrilaterals will be defined; the first one
    /// out of vertices 0, 1, 2, and 3; and the second one out of vertices 4, 5, 6, and 7. A
    /// valid number of vertices for separate quadrilaterals is 4n where n >= 0.
    ///
    /// **Quadrilateral strip:** When `begin()` is called with an argument of
    /// `primitive::quad_strip`, a single sequence of connected quadrilaterals will be
    /// defined. After the first two vertices have been added, one quadrilateral is defined
    /// for every subsequent pair of vertices, so if N vertices are added in total, N/2 - 1
    /// quadrilaterals will be defined. The i'th quadrilateral, counting from zero, is
    /// formed from vertices 2i, 2i+1, 2i+3, and 2i+2 in that order. For example, if 6
    /// vertices are added, two quadrilaterals will be defined; the first one out of
    /// vertices 0, 1, 3, and 2; and the second one out of vertices 2, 3, 5, and 4. Note the
    /// "zigzag" order. A valid number of vertices for a quadrilateral strip is 2n+2 where n
    /// >= 0.
    ///
    /// **Polygon:** Currently, when `begin()` is called with an argument of
    /// `primitive::polygon`, a single triangle fan will be defined exactly as if `begin()`
    /// was called with an argument of `primitive::trifan`. In the future, it is expected
    /// that there will be a difference between a polygon and a triangle fan, especially
    /// with respect to the "edge flag", that is, the absence of visible internal edges in a
    /// polygon from a wireframe point of view. In general, a polygon will be flat, but this
    /// is not expected to ever become a requirement.
    ///
    /// \sa \ref begin_trifan(), \ref begin_quads(), \ref begin_quad_strip(), \ref begin_polygon()
    ///
    void begin(primitive prim);

    /// \brief Finalize primitive definition block.
    ///
    /// See \ref begin().
    ///
    void end();

    /// \{
    ///
    /// \brief Set vertex normal.
    ///
    /// These functions set the normal that is applied to subsequently added vertices. It
    /// must be, or be very close to a unit vector. The default normal is parallel to the Z
    /// axis and points in the direction of positive Z.
    ///
    void set_normal(float x, float y, float z) noexcept;
    void set_normal(const math::Vector3F&) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Set vertex color.
    ///
    /// These functions set the color that is applied to subsequently added vertices. The
    /// default color is fully opaque white.
    ///
    void set_color(util::Color) noexcept;
    void set_color(float r, float g, float b, float a = 1) noexcept;
    void set_color(const math::Vector3F&) noexcept;
    void set_color(const math::Vector4F&) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Set vertex texture coordinate.
    ///
    /// This function sets the texture coordinate that is applied to subsequently added
    /// vertices. The default coordinate is the origin, i.e., 0,0,0.
    ///
    void set_tex_coord(float s, float t, float r = 0) noexcept;
    void set_tex_coord(const math::Vector2F&) noexcept;
    void set_tex_coord(const math::Vector3F&) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Add vertex.
    ///
    /// These functions add a new vertex. The vertex will be associated with the current
    /// color (\ref set_color()), the current normal (\ref set_normal()), and the current
    /// texture coordinate (\ref set_tex_coord()). See \ref begin() for the various kinds of
    /// geometric primitives that can be generated as vertices are added.
    ///
    void add_vertex(float x, float y, float z);
    void add_vertex(const math::Vector3F&);
    /// \}

    /// \brief Reset object builder to its initial state.
    ///
    /// This function resets the object builder and allows it to be used to build a new
    /// object from scratch. After a reset, the object builder will be in the same logical
    /// state as it was initially. The main advantage of reusing an object builder is that
    /// it allows for memory, that was dynamically allocated internally in the object
    /// builder during one session, to be reused in the next session.
    ///
    virtual void reset() noexcept;

    virtual ~object_builder() noexcept = default;

protected:
    /// \brief Subclass's hook for initiation of primitive definition block.
    ///
    /// This function is called when a primitive definition block is initiated (\ref
    /// begin()). The implementing subclass must override this function.
    ///
    virtual void do_begin(primitive prim) = 0;

    /// \brief Subclass's hook for finalization of primitive definition block.
    ///
    /// This function is called when a primitive definition block is finalized (\ref
    /// end()). The implementing subclass must override this function.
    ///
    /// The implementing subclass is required to throw an exception if the number of added
    /// vertices is invalid with respect to the type of primitive definition block being
    /// finalized (\p prim).
    ///
    virtual void do_end(primitive prim) = 0;

    /// \brief Subclass's hook for addition of vertex.
    ///
    /// This function is called for each added vertex (\ref add_vertex()). The implementing
    /// subclass must override this function.
    ///
    /// The implementing subclass is supposed to call \ref get_transformed_normal(), \ref
    /// get_color(), and \ref get_transformed_tex_coord() to get vertex attributes as
    /// needed.
    ///
    virtual void do_add_vertex(const math::Vector3F& coord) = 0;

    /// \brief Transformed normal for currently added vertex.
    ///
    /// This function computes the transformed normal for a currently added vertex. It is
    /// supposed to be called from the implementation of \ref do_add_vertex().
    ///
    /// The returned normal is not necessarily of unit length, because it has not be
    /// renormalized after transformation.
    ///
    auto get_transformed_normal() noexcept -> math::Vector3F;

    /// \brief Vertex color for currently added vertex.
    ///
    /// This function returns the vertex color for the currently added vertex. It is
    /// supposed to be called from the implementation of \ref do_add_vertex().
    ///
    auto get_color() noexcept -> math::Vector4F;

    /// \brief Transformed texture coordinate for currently added vertex.
    ///
    /// This function computes the transformed texture coordinate for a currently added
    /// vertex. It is supposed to be called from the implementation of \ref do_add_vertex().
    ///
    auto get_transformed_tex_coord() noexcept -> math::Vector3F;

    /// \brief Whether primitive definition has begun.
    ///
    /// This function returns `false` initially and after a reset (\ref reset()). It
    /// switches to return `true` after the first initiation of a primitive definition block
    /// (\ref begin()).
    ///
    /// When this function returns `false`, no vertices can have been added yet.
    ///
    bool primitive_definition_started() const noexcept;

    /// \brief Whether primitive definition block is in progress.
    ///
    /// This function returns `true` when, and only when a primitive definition block is in
    /// progress (\ref begin(), \ref end()).
    ///
    bool in_primitive_definition() const noexcept;

private:
    struct coord_transform {
        math::Matrix4F transform;
        bool maybe_nonorthogonal;
    };

    // `m_primitive` has meaning only when `m_in_primitive_definition` is true.
    primitive m_primitive;
    matrix_mode m_matrix_mode = matrix_mode::coord_transform;

    std::vector<coord_transform> m_coord_transform_stack;
    std::vector<math::Matrix4F> m_tex_coord_transform_stack;

    coord_transform m_coord_transform = { math::Matrix4F::identity(), false };
    math::Matrix4F m_tex_coord_transform = math::Matrix4F::identity();

    math::Vector3F m_normal = { 0, 0, 1 };
    math::Vector4F m_color = { 1, 1, 1, 1 };
    math::Vector3F m_tex_coord = { 0, 0, 0 };

    // `m_normal_transform` has meaning only when `m_have_normal_transform` is true.
    math::Matrix3F m_normal_transform;
    math::Vector3F m_transformed_normal;
    math::Vector3F m_transformed_tex_coord;

    bool m_primitive_definition_started = false;
    bool m_in_primitive_definition = false;
    bool m_have_normal_transform = false;
    bool m_have_transformed_normal = false;
    bool m_have_transformed_tex_coord = false;

    auto transform_normal() noexcept -> math::Vector3F;
    auto transform_tex_coord() noexcept -> math::Vector3F;

    void update_normal_transform() noexcept;
};








// Implementation


inline void object_builder::matrix_mode_coord() noexcept
{
    set_matrix_mode(matrix_mode::coord_transform);
}


inline void object_builder::matrix_mode_tex_coord() noexcept
{
    set_matrix_mode(matrix_mode::tex_coord_transform);
}


inline void object_builder::set_matrix_mode(matrix_mode mode) noexcept
{
    m_matrix_mode = mode;
}


inline void object_builder::scale(float factor) noexcept
{
    scale(factor, factor, factor);
}


inline void object_builder::begin_trifan()
{
    begin(primitive::trifan); // Throws
}


inline void object_builder::begin_quads()
{
    begin(primitive::quads); // Throws
}


inline void object_builder::begin_quad_strip()
{
    begin(primitive::quad_strip); // Throws
}


inline void object_builder::begin_polygon()
{
    begin(primitive::polygon); // Throws
}


inline void object_builder::set_normal(float x, float y, float z) noexcept
{
    set_normal(math::Vector3F(x, y, z));
}


inline void object_builder::set_color(util::Color color) noexcept
{
    math::Vector4F color_2;
    color.to_lin_vec(color_2);
    set_color(color_2);
}


inline void object_builder::set_color(float r, float g, float b, float a) noexcept
{
    set_color(math::Vector4F(r, g, b, a));
}


inline void object_builder::set_color(const math::Vector3F& color) noexcept
{
    set_color(math::Vector4F(color, 1));
}


inline void object_builder::set_tex_coord(float s, float t, float r) noexcept
{
    set_tex_coord(math::Vector3F(s, t, r));
}


inline void object_builder::set_tex_coord(const math::Vector2F& tex_coord) noexcept
{
    set_tex_coord(math::Vector3F(tex_coord));
}


inline void object_builder::add_vertex(float x, float y, float z)
{
    add_vertex({ x, y, z }); // Throws
}


inline auto object_builder::get_transformed_normal() noexcept -> math::Vector3F
{
    if (ARCHON_LIKELY(m_have_transformed_normal))
        return m_transformed_normal;
    return transform_normal();
}


inline auto object_builder::get_color() noexcept -> math::Vector4F
{
    return m_color;
}


inline auto object_builder::get_transformed_tex_coord() noexcept -> math::Vector3F
{
    if (ARCHON_LIKELY(m_have_transformed_tex_coord))
        return m_transformed_tex_coord;
    return transform_tex_coord();
}


inline bool object_builder::primitive_definition_started() const noexcept
{
    return m_primitive_definition_started;
}


inline bool object_builder::in_primitive_definition() const noexcept
{
    return m_in_primitive_definition;
}


} // namespace archon::gfx

#endif // ARCHON_X_GFX_X_OBJECT_BUILDER_HPP
