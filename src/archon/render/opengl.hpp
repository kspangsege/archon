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

#ifndef ARCHON_X_RENDER_X_OPENGL_HPP
#define ARCHON_X_RENDER_X_OPENGL_HPP

/// \file


#include <string_view>
#include <initializer_list>

#include <archon/log/logger.hpp>
#include <archon/math/matrix.hpp>
#include <archon/display/opengl.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

namespace archon::render {


/// \brief Compile shader object.
///
/// This function compiles the specified shader source code (\p source). If compilation
/// succeeds, it returns `true` after assigning the resulting shader object to \p shader. If
/// compilation fails, it returns `false` and leaves \p shader unchanged.
///
/// The \p type argument must be one of the valid type arguments for `glCreateShader()`.
///
/// The \p label argument is used for generating error messages, and could for example be
/// `"fragment shader"`.
///
bool compile_shader(GLenum type, std::string_view label, std::string_view source, log::Logger& logger, GLuint& shader);


/// \brief Link shader program.
///
/// This function links the specified shader objects (\p shaders) into a shader program. If
/// linking succeeds, it returns `true` after assigning the resulting program object to \p
/// program. If linking fails, it returns `false` and leaves \p program unchanged.
///
/// The \p label argument is used for generating error messages, and could for example be
/// `"shader program"`.
///
bool link_shader(std::string_view label, std::initializer_list<GLuint> shaders, log::Logger& logger, GLuint& program);


/// \{
///
/// \brief Assign new value to uniform variable.
///
/// These functions assign a new value to the specified uniform variable in the currently
/// bound OpenGL shader program. The location must be one that was obtained by
/// `glGetUniformLocation()`.
///
/// In the case of `set_uniform_bool()`, the type of the uniform variable must be `bool`.
///
/// In the case of `set_uniform_matrix()`, the type of the uniform variable must be `mat3`
/// or `mat4` depending on the type of the value argument.
///
void set_uniform_bool(GLint location, bool value);
void set_uniform_matrix(GLint location, const math::Matrix3F& value);
void set_uniform_matrix(GLint location, const math::Matrix4F& value);
/// \}


} // namespace archon::render

#endif // ARCHON_DISPLAY_HAVE_OPENGL

#endif // ARCHON_X_RENDER_X_OPENGL_HPP
