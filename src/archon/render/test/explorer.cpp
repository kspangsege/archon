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

#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <GL/gl.h>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/image/image.hpp>
#include <archon/render/load_texture.hpp>
#include <archon/render/app.hpp>
#include <archon/render/object.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::image;
using namespace archon::render;


namespace
{
  struct Explorer: Application
  {
    Explorer(Application::Config const &cfg, Object const &obj):
      Application("archon::render::Explorer", cfg), object(obj)
    {
      glEnable(GL_LIGHTING);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_NORMALIZE);
      glEnable(GL_CULL_FACE);

      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    }

  private:
    void render_scene()
    {
      object.render();
    }

    Object const &object;
  };
}

int main(int argc, char const *argv[]) throw()
{
  set_terminate(&cxx::terminate_handler);
    
  try_fix_preinstall_datadir(argv[0], "render/test/");

  Application::Config cfg;
  CommandlineOptions opts;
  opts.add_help("Test application for the archon::Display library", "OBJECT-FILE");
  opts.check_num_args(0,1);
  opts.add_group(cfg);
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  string obj_file  = argc < 2 ? cfg.archon_datadir+"render/test/test.obj" : argv[1];

  Object obj;
  {
    ifstream in(obj_file.c_str());
    obj.load(in);
  }

  Explorer(cfg, obj).run();
  return 0;
}
