/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 *
 * \todo FIXME: This test must be compiled only when OpenGL is available.
 */

#include <iostream>

#include <GL/gl.h>

#include <archon/image/image.hpp>
#include <archon/display/implementation.hpp>


using namespace std;
using namespace archon::image;
using namespace archon::Display;


int main() throw()
{
    Implementation::Ptr impl = get_default_implementation();
    Connection::Ptr conn = impl->new_connection();
    int vis = conn->choose_gl_visual(-1, false);
    PixelBuffer::Ptr buf = conn->new_pixel_buffer(512, 512, -1, vis);
    Context::Ptr ctx_1 = conn->new_gl_context(-1, vis, true);
    Context::Ptr ctx_2 = conn->new_gl_context(-1, vis, true, ctx_1);

    GLuint texture;
    GLint level_1 = 0;
    GLint level_2 = 0;
    {
        Bind b(ctx_1, buf);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 444);
    }
    {
        Bind b(ctx_2, buf);
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &level_1);
        glDeleteTextures(1, &texture);
    }
    {
        Bind b(ctx_1, buf);
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &level_2);
    }

    cerr << "Level 1: " << level_1 << "\n";
    cerr << "Level 2: " << level_2 << "\n";
}
