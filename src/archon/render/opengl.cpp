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
#include <iterator>
#include <string_view>
#include <initializer_list>

#include <archon/log/logger.hpp>
#include <archon/math/matrix.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/opengl.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

using namespace archon;


bool render::compile_shader(GLenum type, std::string_view label, std::string_view source, log::Logger& logger,
                            GLuint& shader)
{
    GLuint shader_2 = glCreateShader(type);

    GLsizei count = 1;
    const GLchar* string = source.data();
    GLint length = {};
    core::int_cast(source.size(), length); // Throws
    glShaderSource(shader_2, count, &string, &length);

    glCompileShader(shader_2);

    int success = {};
    glGetShaderiv(shader_2, GL_COMPILE_STATUS, &success);
    if (ARCHON_LIKELY(success)) {
        shader = shader_2;
        return true;
    }

    // FIXME: What if message is longer than 512?    
    char info_log[512] = {};
    GLsizei max_length = std::size(info_log);
    GLsizei length_2 = {};
    glGetShaderInfoLog(shader_2, max_length, &length_2, info_log);
    std::string_view str = { info_log, std::size_t(length_2) };
    logger.error("Compilation of %s failed:\n%s", label, core::chomp(str)); // Throws
    return false;
}


bool render::link_shader(std::string_view label, std::initializer_list<GLuint> shaders, log::Logger& logger,
                         GLuint& program)
{
    GLuint program_2 = glCreateProgram();

    for (GLuint shader : shaders)
        glAttachShader(program_2, shader);

    glLinkProgram(program_2);

    int success = {};
    glGetProgramiv(program_2, GL_LINK_STATUS, &success);
    if (ARCHON_LIKELY(success)) {
        program = program_2;
        return true;
    }

    // FIXME: What if message is longer than 512?    
    char info_log[512] = {};
    GLsizei max_length = std::size(info_log);
    GLsizei length = {};
    glGetShaderInfoLog(program_2, max_length, &length, info_log);
    std::string_view str = { info_log, std::size_t(length) };
    logger.error("Linking of %s failed:\n%s", label, core::chomp(str)); // Throws
    return false;
}


void render::set_uniform_bool(GLint location, bool value)
{
    glUniform1i(location, (value ? 1 : 0));
}


void render::set_uniform_matrix(GLint location, const math::Matrix3F& value)
{
    GLsizei count = 1;
    GLboolean transpose = GL_TRUE;
    GLfloat components[9] = {};
    value.to_array(components);
    glUniformMatrix3fv(location, count, transpose, components);
}


void render::set_uniform_matrix(GLint location, const math::Matrix4F& value)
{
    GLsizei count = 1;
    GLboolean transpose = GL_TRUE;
    GLfloat components[16] = {};
    value.to_array(components);
    glUniformMatrix4fv(location, count, transpose, components);
}


#endif // ARCHON_DISPLAY_HAVE_OPENGL
