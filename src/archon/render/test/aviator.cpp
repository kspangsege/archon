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
#include <archon/math/quaternion.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/scene_builder.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::display;
using namespace archon::render;


namespace {

class AviatorApp: public Application {
public:
    AviatorApp(const Application::Config& cfg):
        Application("archon::render::Aviator", cfg),
        m_resource_dir(cfg.archon_datadir+"render/test/")
    {
        constexpr double angle_step = M_PI / 128;
        auto adjust_alpha = [this](bool key_down) {
            if (key_down) {
                m_angle_choice = AngleChoice::alpha;
                set_status(L"Adjust alpha");
            }
            return false;
        };
        auto adjust_beta = [this](bool key_down) {
            if (key_down) {
                m_angle_choice = AngleChoice::beta;
                set_status(L"Adjust beta");
            }
            return false;
        };
        auto adjust_gamma = [this](bool key_down) {
            if (key_down) {
                m_angle_choice = AngleChoice::gamma;
                set_status(L"Adjust gamma");
            }
            return false;
        };
        auto inc_angle = [this](bool key_down) {
            if (key_down) {
                switch (m_angle_choice) {
                    case AngleChoice::alpha:
                        m_alpha += angle_step;
                        break;
                    case AngleChoice::beta:
                        m_beta  += angle_step;
                        break;
                    case AngleChoice::gamma:
                        m_gamma += angle_step;
                        break;
                }
                update_orientation();
                return true;
            }
            return false;
        };
        auto dec_angle = [this](bool key_down) {
            if (key_down) {
                switch (m_angle_choice) {
                    case AngleChoice::alpha:
                        m_alpha -= angle_step;
                        break;
                    case AngleChoice::beta:
                        m_beta  -= angle_step;
                        break;
                    case AngleChoice::gamma:
                        m_gamma -= angle_step;
                        break;
                }
                update_orientation();
                return true;
            }
            return false;
        };
        bind_key(KeySym_1, std::move(adjust_alpha), "Adjust alpha angle (Euler angles)."); // Throws
        bind_key(KeySym_2, std::move(adjust_beta),  "Adjust beta angle (Euler angles)."); // Throws
        bind_key(KeySym_3, std::move(adjust_gamma), "Adjust gamma angle (Euler angles)."); // Throws
        bind_key(KeySym_Up,   std::move(inc_angle), "Increase selected angle."); // Throws
        bind_key(KeySym_Down, std::move(dec_angle), "Decrease selected angle."); // Throws

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_NORMALIZE);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

        const TextureUse plane_mid = load_texture(m_resource_dir+"plane_mid.png");
        const TextureUse dirt_seamless = load_texture(m_resource_dir+"dirt_seamless.png");

        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            dirt_seamless.bind();
            builder.tex_scale(128);
            bool gen_texture_coords = true;
            bool reverse_zx_order = false;
            double z0 = -256, z1 = 256;
            double x0 = -256, x1 = 256;
            double y = -10;
            int z_steps = 512, x_steps = 512;
            build_zxy_mesh(builder, gen_texture_coords, reverse_zx_order, z0, z1, x0, x1, y,
                           z_steps, x_steps);
            m_ground_list = list;
        }

        {
            GLuint list = glGenLists(1);
            OpenGlSceneBuilder builder(list, get_texture_cache(), &m_texture_use);
            builder.rotate(Rotation3{Vec3{1, 0, 0}, -M_PI/2});
            builder.scale(0.18);
            plane_mid.bind();
            builder.tex_rotate(M_PI/2);
            {
                builder.push_matrix();
                builder.scale(Vec3{1, 3, 1});
                bool gen_texture_coords = true;
                bool has_top = false, has_bottom = false;
                build_cylinder(builder, gen_texture_coords, has_top, has_bottom);
                builder.pop_matrix();
            }
            {
                builder.push_matrix();
                builder.translate(Vec3{0, 4, 0});
                builder.set_tex_coord(Vec2{0, 0});
                bool gen_texture_coords = false;
                bool has_bottom = false;
                build_cone(builder, gen_texture_coords, has_bottom);
                builder.pop_matrix();
            }
            {
                builder.push_matrix();
                builder.translate(Vec3{0, -3.5, 0});
                builder.rotate(Rotation3{Vec3{0, 0, 1}, M_PI});
                builder.scale(Vec3{1, 0.5, 1});
                builder.set_tex_coord(Vec2{0, 0});
                bool gen_texture_coords = false;
                bool has_bottom = false;
                build_cone(builder, gen_texture_coords, has_bottom);
                builder.pop_matrix();
            }
            {
                builder.push_matrix();
                builder.translate(Vec3{0, -2, 0});
                builder.rotate(Rotation3{Vec3{1, 0, 0}, -M_PI/3});
                builder.translate(Vec3{0, -1.5, 0});
                builder.scale(Vec3{0.05, 1, 0.5});
                bool gen_texture_coords = false;
                build_centered_box(builder, gen_texture_coords);
                builder.pop_matrix();
            }
            {
                builder.push_matrix();
                builder.translate(Vec3{0, 0.8, -0.55});
                builder.rotate(Rotation3{Vec3{0, 1, 0}, +M_PI/32});
                builder.rotate(Rotation3{Vec3{0, 0, 1}, +M_PI/10});
                builder.rotate(Rotation3{Vec3{1, 0, 0}, +M_PI/64});
                builder.scale(Vec3{2.2, 0.8, 0.05});
                builder.translate(Vec3{-1.25, 0, 0});
                bool gen_texture_coords = false;
                build_centered_box(builder, gen_texture_coords);
                builder.pop_matrix();
            }
            {
                builder.push_matrix();
                builder.translate(Vec3{0, 0.8, -0.55});
                builder.rotate(Rotation3{Vec3{0, 1, 0}, -M_PI/32});
                builder.rotate(Rotation3{Vec3{0, 0, 1}, -M_PI/10});
                builder.rotate(Rotation3{Vec3{1, 0, 0}, +M_PI/64});
                builder.scale(Vec3{2.2, 0.8, 0.05});
                builder.translate(Vec3{+1.25, 0, 0});
                bool gen_texture_coords = false;
                build_centered_box(builder, gen_texture_coords);
                builder.pop_matrix();
            }
            m_object_list = list;
        }
    }

private:
    enum class AngleChoice { alpha, beta, gamma };

    const std::string m_resource_dir;
    std::vector<TextureUse> m_texture_use;
    GLuint m_ground_list = 0, m_object_list = 0;
    double m_alpha = 0, m_beta = 0, m_gamma = 0; // Euler angles
    AngleChoice m_angle_choice = AngleChoice::alpha;
    Quaternion m_orientation; // Versor

    void render() override
    {
        glCallList(m_ground_list);

        glPushMatrix();
        Rotation3 r{m_orientation};
        glRotated(r.angle*(180/M_PI), r.axis[0], r.axis[1], r.axis[2]);
        glCallList(m_object_list);
        glPopMatrix();
    }

    void update_orientation()
    {
        Quaternion q = versor_from_proper_euler_angles(m_alpha, m_beta, m_gamma);
        // Interpret Euler angles with respect to a horizontal coordinate system
        // (Z-axis upwards, X-axis to the right).
        m_orientation = horiz_to_vert(q);
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);
    try_fix_preinstall_datadir(argv[0], "render/test/");

    Application::Config cfg;
    CommandlineOptions opts;
    opts.add_help("Experimentation with rotation");
    opts.check_num_args();
    opts.add_group(cfg);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    AviatorApp(cfg).run();
}
