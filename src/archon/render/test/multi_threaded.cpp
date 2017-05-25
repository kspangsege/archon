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

#include <cstdlib>
#include <string>
#include <iostream>

#include <GL/gl.h>

#include <archon/core/string.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/thread/thread.hpp>
#include <archon/display/implementation.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::thread;
using namespace archon::display;
using namespace archon::render;


namespace {

class Facet: public Thread, public Application {
public:
    Facet(const Application::Config& cfg, Connection::Arg c, int i, double m):
        Application("archon::render::MultiThreaded #"+format_int(i+1), cfg, std::locale::classic(), c)
    {
        set_scene_spin(Rotation3(Vec3(0,1,0), m));

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_COLOR_MATERIAL);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    }

private:
    void main()
    {
        Application::run();
    }

    void render() override
    {
        glScalef(4.0/9, 4.0/9, 4.0/9);

        glBegin(GL_QUADS);

        glColor3f(1, 0, 0);
        glVertex3f(-0.9, 0.6, -2);
        glVertex3f(-0.9, 0.1, -2);
        glVertex3f(-0.6, 0.1, -2);
        glVertex3f(-0.6, 0.6, -2);

        glColor3f(0, 1, 0);
        glVertex3f(-0.4, 0.6, -2);
        glVertex3f(-0.4, 0.1, -2);
        glVertex3f(-0.1, 0.1, -2);
        glVertex3f(-0.1, 0.6, -2);

        glColor3f(0, 0, 1);
        glVertex3f(-0.9, -0.1, -2);
        glVertex3f(-0.9, -0.6, -2);
        glVertex3f(-0.6, -0.6, -2);
        glVertex3f(-0.6, -0.1, -2);

        glColor3f(1, 1, 0);
        glVertex3f(-0.4, -0.1, -2);
        glVertex3f(-0.4, -0.6, -2);
        glVertex3f(-0.1, -0.6, -2);
        glVertex3f(-0.1, -0.1, -2);

        glEnd();
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);
    try_fix_preinstall_datadir(argv[0], "render/test/");

    Application::Config cfg;
    int opt_num_wins = 3;

    CommandlineOptions opts;
    opts.add_help("Test application for the multi-threaded capability "
                  "of the rendering application foundation");
    opts.check_num_args();
    opts.add_group(cfg);
    opts.add_param("n", "num-wins", opt_num_wins,
                   "Number of windows to open. Each window has its own rendering thread");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    {
        Implementation::Ptr impl = archon::display::get_default_implementation();
        Connection::Ptr conn = impl->new_connection();

        int n = opt_num_wins;
        for (int i = 0; i < n; ++i) {
            CntRef<Facet> f(new Facet(cfg, conn, i,
                                      1 < n ? 0.44 + 0.55*double(i)/(n-1) : 0.71));
            f->set_window_pos(cfg.win_pos[0]+i*(cfg.win_size[0]+10), cfg.win_pos[1]);
            Thread::start(f);
        }
    }

    std::cout << "Waiting for all threads to terminate\n" << std::flush;
    Thread::main_exit_wait();

    std::cout << "All threads terminated\n" << std::flush;
}
