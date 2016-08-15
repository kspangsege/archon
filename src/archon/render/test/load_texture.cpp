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

#include <string>
#include <vector>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/scene_builder.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::Core;
using namespace archon::Display;
using namespace archon::Render;


namespace {

class LoadTexture: public Application {
public:
    struct Config: Application::Config {
        bool mipmap;

        Config():
            mipmap(true)
        {
        }

        void populate(ConfigBuilder& cfg)
        {
            cfg.add_group(static_cast<Application::Config&>(*this));
            cfg.add_param("m", "mipmap", mipmap, "Enable mipmapping");
        }
    };

    LoadTexture(const Config& cfg, const std::vector<std::string>& textures):
        Application("archon::Render::LoadTexture", cfg)
    {
        for (const auto& path: textures) {
            TextureDecl decl = declare_texture(path, false, cfg.mipmap);
            m_texture_decls.push_back(decl);
        }

        register_key_handler(KeySym_Right, &LoadTexture::next_texture,
                             "Go to next texture.");
        register_key_handler(KeySym_Left, &LoadTexture::prev_texture,
                             "Go to previous texture.");


        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glEnable(GL_NORMALIZE);

        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64);

        m_list = glGenLists(1);
        {
            OpenGlSceneBuilder builder(m_list, get_texture_cache());
            builder.translate(-1.0, -1.0, 0.0);
            builder.scale(2.0);
            build_xyz_mesh(builder, true, false, 0, 1, 0, 1, 0,
                           adjust_detail(10, 1), adjust_detail(10, 1));
        }

        update_texture();
    }

private:
    void render_scene()
    {
        glCallList(m_list);
    }


    bool next_texture(bool key_down)
    {
        if (key_down) {
            if (++m_texture_index == m_texture_decls.size())
                m_texture_index = 0;
            update_texture();
            return true;
        }
        return false;
    }


    bool prev_texture(bool key_down)
    {
        if (key_down) {
            if (m_texture_index-- == 0)
                m_texture_index += m_texture_decls.size();
            update_texture();
            return true;
        }
        return false;
    }


    void update_texture()
    {
        m_texture = m_texture_decls[m_texture_index].acquire();
        m_texture.bind();
    }

    std::vector<TextureDecl> m_texture_decls;
    TextureUse m_texture;
    std::size_t m_texture_index = 0;
    GLuint m_list;
};

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    std::set_terminate(&Cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "render/test/");

    LoadTexture::Config cfg;
    CommandlineOptions opts;
    opts.add_help("Test application for the texture loading facility "
                  "of archon::Render::Application.", "TEXTURE-FILE");
    opts.check_num_args(0,-1);
    opts.add_group(cfg);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::vector<std::string> textures;
    if (1 < argc) {
        for (int i = 1; i < argc; ++i)
            textures.push_back(argv[i]);
    }
    else {
        textures.push_back(cfg.archon_datadir + "render/test/alpha_test.png");
    }

    LoadTexture(cfg, textures).run();
}
