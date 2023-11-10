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


#include <string_view>

#include <archon/render/opengl.hpp>


using namespace archon;


auto render::get_opengl_error_message(GLenum error) noexcept -> std::string_view
{
#if ARCHON_RENDER_HAVE_OPENGL

    switch (error) {
        case GL_INVALID_ENUM:
            return "An unacceptable value is specified for an enumerated argument";
        case GL_INVALID_VALUE:
            return "A numeric argument is out of range";
        case GL_INVALID_OPERATION:
            return "A specified operation is not allowed in the current state";
#  ifdef GL_INVALID_FRAMEBUFFER_OPERATION
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "The framebuffer object is not complete";
#  endif
        case GL_OUT_OF_MEMORY:
            return "Out of memory";
        case GL_STACK_UNDERFLOW:
            return "Stack underflow";
        case GL_STACK_OVERFLOW:
            return "Stack overflow";
    }

#else // !ARCHON_RENDER_HAVE_OPENGL

    static_cast<void>(error);

#endif // !ARCHON_RENDER_HAVE_OPENGL

    return "Unknown error";
}
