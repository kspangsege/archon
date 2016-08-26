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

#ifndef ARCHON_RENDER_SCENE_BUILDER_HPP
#define ARCHON_RENDER_SCENE_BUILDER_HPP

#include <string>
#include <vector>

#include <GL/gl.h>

#include <archon/graphics/scene_builder.hpp>
#include <archon/render/texture_cache.hpp>


namespace archon {
namespace render {

class OpenGlSceneBuilder: public graphics::SpatialSceneBuilder {
public:
    OpenGlSceneBuilder(GLuint list, TextureCache&, std::vector<TextureUse>* = nullptr);

    ~OpenGlSceneBuilder() noexcept;

    using TextureFilterMode = TextureCache::FilterMode;

    /// Is TextureFilterMode::mipmap by default.
    void set_texture_filter_mode(TextureFilterMode);

protected:
    void do_begin_quad_strip() override;
    void do_begin_polygon() override;
    void do_end() override;

    void do_set_color(util::PackedTRGB) override;
    void do_set_normal(math::Vec3) override;
    void do_set_tex_coord(math::Vec2) override;
    void do_add_vertex(math::Vec3) override;

    void do_push_matrix() override;
    void do_pop_matrix() override;

    void do_translate(math::Vec3) override;
    void do_scale(math::Vec3) override;
    void do_rotate(math::Rotation3) override;

    int do_make_texture(std::string, bool, bool) override;
    int do_make_texture(image::Image::ConstRefArg, std::string, bool, bool) override;

    void do_bind_texture(int) override;

    void do_tex_translate(math::Vec2) override;
    void do_tex_scale(math::Vec2) override;
    void do_tex_rotate(double) override;

    void do_reset_tex_transform() override;

private:
    int make_texture_helper(std::unique_ptr<TextureSource>, bool, bool);

    void set_matrix_mode(GLint);
    void provide_tex_transform();

    const GLuint m_list;
    std::vector<TextureUse> m_textures;
    std::vector<TextureUse>* const m_textures_ext;
    TextureFilterMode m_texture_filter_mode = TextureFilterMode::mipmap;
    TextureCache& m_texture_cache;
    GLint m_matrix_mode;
    bool m_has_tex_transform = false;
};




// Implementation

inline void OpenGlSceneBuilder::set_texture_filter_mode(TextureFilterMode mode)
{
    m_texture_filter_mode = mode;
}

} // namespace render
} // namespace archon

#endif // ARCHON_RENDER_SCENE_BUILDER_HPP
