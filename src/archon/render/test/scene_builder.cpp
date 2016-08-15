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
 */

#include <cmath>
#include <cstdlib>
#include <string>

#include <GL/gl.h>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/scene_builder.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::display;
using namespace archon::render;


namespace {

class SceneBuilderApp: public Application {
public:
    SceneBuilderApp(const Application::Config& cfg):
        Application("archon::render::SceneBuilder", cfg),
        m_resource_dir(cfg.archon_datadir+"render/test/")
    {
        register_key_handler(KeySym_Right, &SceneBuilderApp::next,
                             "Go to next object.");
        register_key_handler(KeySym_Left, &SceneBuilderApp::prev,
                             "Go to previous object.");

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glEnable(GL_NORMALIZE);

        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

        const TextureUse seamless  = load_texture(m_resource_dir+"seamless.png");
        const TextureUse spherical = load_texture(m_resource_dir+"spherical.png");

        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            seamless.bind();
            build_box(builder);
            m_lists.push_back(list);
        }
        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            seamless.bind();
            build_cone(builder);
            m_lists.push_back(list);
        }
        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            seamless.bind();
            build_cylinder(builder);
            m_lists.push_back(list);
        }
        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            spherical.bind();
            build_sphere(builder);
            m_lists.push_back(list);
        }
        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            seamless.bind();
            build_torus(builder);
            m_lists.push_back(list);
        }
    }

private:
    void render_scene()
    {
        glCallList(m_lists[m_list_idx]);
    }

    bool next(bool key_down)
    {
        if (key_down) {
            if (++m_list_idx == m_lists.size())
                m_list_idx = 0;
            return true;
        }
        return false;
    }


    bool prev(bool key_down)
    {
        if (key_down) {
            if (m_list_idx == 0)
                m_list_idx = m_lists.size();
            --m_list_idx;
            return true;
        }
        return false;
    }

    const std::string m_resource_dir;
    std::vector<TextureUse> m_texture_use;
    std::vector<GLuint> m_lists;
    size_t m_list_idx = 0;
};

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    std::set_terminate(&cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "render/test/");
    Application::Config cfg;
    CommandlineOptions opts;
    opts.add_help("Test application for the scene builder feature");
    opts.check_num_args();
    opts.add_group(cfg);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    SceneBuilderApp(cfg).run();
}
