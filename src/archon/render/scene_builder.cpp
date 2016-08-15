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

/// \file
///
/// \author Kristian Spangsege

#include <archon/render/scene_builder.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::math;


namespace archon {
namespace Render {

OpenGlSceneBuilder::OpenGlSceneBuilder(GLuint list, TextureCache& texture_cache,
                                       vector<TextureUse>* textures_ext, bool mipmapping):
    m_list(list),
    m_mipmapping(mipmapping),
    m_textures_ext(textures_ext),
    m_texture_cache(texture_cache)
{
    glNewList(list, GL_COMPILE);
    glGetIntegerv(GL_MATRIX_MODE, &m_matrix_mode);
}

OpenGlSceneBuilder::~OpenGlSceneBuilder()
{
    glEndList();
}

void OpenGlSceneBuilder::do_begin_quad_strip()
{
    glBegin(GL_QUAD_STRIP);
}

void OpenGlSceneBuilder::do_begin_polygon()
{
    glBegin(GL_POLYGON);
}

void OpenGlSceneBuilder::do_end()
{
    glEnd();
}

void OpenGlSceneBuilder::do_set_normal(Vec3 n)
{
    glNormal3d(n[0], n[1], n[2]);
}

void OpenGlSceneBuilder::do_set_tex_coord(Vec2 p)
{
    glTexCoord2d(p[0], p[1]);
}

void OpenGlSceneBuilder::do_add_vertex(Vec3 v)
{
    glVertex3d(v[0], v[1], v[2]);
}

void OpenGlSceneBuilder::do_push_matrix()
{
    set_matrix_mode(GL_MODELVIEW);
    glPushMatrix();
}

void OpenGlSceneBuilder::do_pop_matrix()
{
    set_matrix_mode(GL_MODELVIEW);
    glPopMatrix();
}

void OpenGlSceneBuilder::do_translate(Vec3 v)
{
    set_matrix_mode(GL_MODELVIEW);
    glTranslated(v[0], v[1], v[2]);
}

void OpenGlSceneBuilder::do_scale(Vec3 s)
{
    set_matrix_mode(GL_MODELVIEW);
    glScaled(s[0], s[1], s[2]);
}

void OpenGlSceneBuilder::do_rotate(Rotation3 r)
{
    set_matrix_mode(GL_MODELVIEW);
    glRotated(r.angle * (180/M_PI), r.axis[0], r.axis[1], r.axis[2]);
}

int OpenGlSceneBuilder::do_make_texture(std::string image_path, bool h_rep, bool v_rep)
{
    UniquePtr<TextureSource> src(new TextureFileSource(image_path));
    return make_texture(src, h_rep, v_rep);
}

int OpenGlSceneBuilder::do_make_texture(Imaging::Image::ConstRefArg img, std::string name,
                                        bool h_rep, bool v_rep)
{
    UniquePtr<TextureSource> src(new TextureImageSource(img, name));
    return make_texture(src, h_rep, v_rep);
}

void OpenGlSceneBuilder::do_bind_texture(int index)
{
    m_textures.at(index).bind();
}

void OpenGlSceneBuilder::do_tex_translate(math::Vec2 v)
{
    provide_tex_transform();
    glTranslated(v[0], v[1], 0);
}

void OpenGlSceneBuilder::do_tex_scale(math::Vec2 s)
{
    provide_tex_transform();
    glScaled(s[0], s[1], 1);
}

void OpenGlSceneBuilder::do_tex_rotate(double radians)
{
    provide_tex_transform();
    glRotated(radians*(180/M_PI), 0, 0, 1);
}

void OpenGlSceneBuilder::do_reset_tex_transform()
{
    if (!m_has_tex_transform)
        return;
    set_matrix_mode(GL_TEXTURE);
    glPopMatrix();
}

int OpenGlSceneBuilder::make_texture(UniquePtr<TextureSource> src, bool rep_s, bool rep_t)
{
    GLenum wrap_s = rep_s ? GL_REPEAT : GL_CLAMP, wrap_t = rep_t ? GL_REPEAT : GL_CLAMP;
    TextureCache::FilterMode filter_mode =
        m_mipmapping ? TextureCache::filter_mode_Mipmap : TextureCache::filter_mode_Interp;
    TextureUse use = m_texture_cache.declare(src, wrap_s, wrap_t, filter_mode).acquire();
    int id = m_textures.size();
    m_textures.push_back(use);
    if (m_textures_ext)
        m_textures_ext->push_back(use);
    return id;
}

void OpenGlSceneBuilder::set_matrix_mode(GLint mode)
{
    if (mode == m_matrix_mode)
        return;
    glMatrixMode(mode);
    m_matrix_mode = mode;
}

void OpenGlSceneBuilder::provide_tex_transform()
{
    set_matrix_mode(GL_TEXTURE);
    if (m_has_tex_transform)
        return;
    glPushMatrix();
    m_has_tex_transform = true;
}

} // namespace Render
} // namespace archon
