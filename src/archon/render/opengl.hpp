// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <chrono>

#include <archon/core/features.h>
#include <archon/render/impl/config.h>


/// \def ARCHON_RENDER_HAVE_OPENGL
///
/// \brief Whether OpenGL is available.
///
/// This macro expands to `1` if OpenGL is availabe. Otherwise it expands to `0`.
///
/// For OpenGL manual pages, see https://registry.khronos.org/OpenGL-Refpages/ or https://docs.gl/.
///
/// For the OpenGL specification, see https://registry.khronos.org/OpenGL/specs/.


#if ARCHON_RENDER_HAVE_OPENGL
#  if ARCHON_APPLE
#    define GL_SILENCE_DEPRECATION
#    include <OpenGL/gl.h>
#  elif ARCHON_WINDOWS
#    if !defined NOMINMAX
#      define NOMINMAX
#    endif
#    include <windows.h>
#    include <gl/GL.h>
#  else
#    include <GL/gl.h>
#  endif
#endif // ARCHON_RENDER_HAVE_OPENGL


#if !ARCHON_RENDER_HAVE_OPENGL

// Define some OpenGL types allowing for certain functions to exist even when OpenGL is
// unavailable (e.g., `get_opengl_error_message()`).

using GLenum = unsigned;

#endif // !ARCHON_RENDER_HAVE_OPENGL


namespace archon::render {


/// \brief Get string for OpenGL error code.
///
/// This function returns a string that describes the specified OpenGL error code as
/// returned by `glGetError()`.
///
/// If OpenGL is not available (\ref ARCHON_RENDER_HAVE_OPENGL), this function returns the
/// empty string. Note that `GLenum` is an alias for `int` in this case.
///
auto get_opengl_error_message(GLenum error) noexcept -> std::string_view;


} // namespace archon::render


#endif // ARCHON_X_RENDER_X_OPENGL_HPP
