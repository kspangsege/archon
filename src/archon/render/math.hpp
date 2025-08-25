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

#ifndef ARCHON_X_RENDER_X_MATH_HPP
#define ARCHON_X_RENDER_X_MATH_HPP

/// \file


#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/quaternion.hpp>


namespace archon::render {


/// \brief Apply translation to 4-by-4 homogeneous transformation.
///
/// This function multiplies the specified homogeneous matrix (\p mat) by the translation
/// matrix corresponding to the specified translation vector (\p vec). The translation
/// matrix is constructed as if by \ref render::make_translation(). This function
/// corresponds to `glTranslatef()`.
///
void translate(math::Matrix4F& mat, const math::Vector3F& vec) noexcept;


/// \brief Apply rotation to 4-by-4 homogeneous transformation.
///
/// This function multiplies the specified homogeneous matrix (\p mat) by the rotation
/// matrix corresponding to the specified axis and angle (\p axis, \p angle). The rotation
/// matrix is constructed as if by \ref render::make_rotation(). This function corresponds
/// to `glRotatef()`.
///
void rotate(math::Matrix4F& mat, const math::Vector3& axis, double angle) noexcept;


/// \brief Extend 3-by-3 matrix to 4-by-4 homogeneous matrix.
///
/// This function extends a 3-by-3 matrix to the corresponding 4-by-4 matrix for use with
/// homogeneous coordinates.
///
auto extend_matrix(const math::Matrix3F& mat) noexcept -> math::Matrix4F;


/// \brief Construct homogeneous perspective matrix.
///
/// This function constructs the 4-by-4 homogeneous perspective matrix that is also
/// constructed by glFrustum().
///
auto make_perspective(double left, double right, double bottom, double top,
                      double near_, double far_) noexcept -> math::Matrix4F;


/// \brief Construct homogeneous translation matrix.
///
/// This function constructs the 4-by-4 homogeneous translation matrix corresponding to the
/// specified translation vector.
///
auto make_translation(const math::Vector3F& vec) noexcept -> math::Matrix4F;


/// \brief Construct homogeneous rotation matrix.
///
/// This function constructs the 4-by-4 homogeneous rotation matrix corresponding to the
/// specified axis and angle. The axis must be a unit vector, and the angle is expressed in
/// radians.
///
auto make_rotation(const math::Vector3& axis, double angle) noexcept -> math::Matrix4F;








// Implementation


inline void translate(math::Matrix4F& mat, const math::Vector3F& vec) noexcept
{
    mat *= make_translation(vec);
}


inline void rotate(math::Matrix4F& mat, const math::Vector3& axis, double angle) noexcept
{
    mat *= make_rotation(axis, angle);
}


inline auto extend_matrix(const math::Matrix3F& mat) noexcept -> math::Matrix4F
{
    math::Matrix4F mat_2 = math::Matrix4F::identity();
    mat_2.set_submatrix(0, 0, mat);
    return mat_2;
}


inline auto make_translation(const math::Vector3F& vec) noexcept -> math::Matrix4F
{
    math::Matrix4F mat = math::Matrix4F::identity();
    mat.set_subcol(0, 3, vec);
    return mat;
}


inline auto make_rotation(const math::Vector3& axis, double angle) noexcept -> math::Matrix4F
{
    math::Quaternion quat = math::Quaternion::from_axis_angle(axis, angle);
    return extend_matrix(math::Matrix3F(quat.to_rotation_matrix()));
}


} // namespace archon::render

#endif // ARCHON_X_RENDER_X_MATH_HPP
