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
#include <string>

#include <GL/gl.h>
#include <GL/glu.h>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/thread/thread.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::Thread;
using namespace archon::Render;


namespace
{
  struct Render: Application
  {
    Render(Application::Config const &cfg):
      Application("archon::Render::Render", cfg)
    {
/*
      set_scene_orientation(Rotation3(Vec3(1,0,0), M_PI/8));
      set_scene_spin(Rotation3(Vec3(0,1,0), M_PI/16));
*/

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_CULL_FACE);
      glEnable(GL_LIGHTING);

      {
        GLfloat params[] = { 1, 0, 0, 1 };
        glLightfv(GL_LIGHT1, GL_DIFFUSE, params);
      }
      {
        GLfloat params[] = { 1, 1, 1, 1 };
        glLightfv(GL_LIGHT1, GL_SPECULAR, params);
      }

/*
      glEnable(GL_LIGHTING);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_COLOR_MATERIAL);

      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
*/

      quadric = gluNewQuadric();
    }

    virtual ~Render()
    {
      gluDeleteQuadric(quadric);
    }

  private:
    void render_scene()
    {
      {
        glPushMatrix();
        glTranslated(-1, 0, 0);
        glDisable(GL_LIGHT1);
        gluSphere(quadric, 0.01, adjust_detail(32, 3), adjust_detail(32, 3));
        glPushMatrix();
        {
          GLfloat params[] = { 0, 0, 0, 1 };
          glLightfv(GL_LIGHT1, GL_POSITION, params);
        }
        {
          GLfloat params[] = { 0.5 };
          glLightfv(GL_LIGHT1, GL_LINEAR_ATTENUATION, params);
        }
        glPopMatrix();
        glNormal3d(0, 0, 1);
        glEnable(GL_LIGHT1);
        int const m = 20;
        for(int i=0; i<m; ++i)
        {
          double const r = (i+1) / double(m);
          glBegin(GL_POLYGON);
          int const n = adjust_detail(32, 3);
          for(int j=0; j<n; ++j)
          {
            double const a = j * (2*M_PI / n);
            glVertex3d(r*cos(a), r*sin(a), -(1+i));
          }
          glEnd();
        }
        glPopMatrix();
      }
      {
        glPushMatrix();
        glTranslated(1, 0, 0);
        glDisable(GL_LIGHT1);
        gluSphere(quadric, 0.01, adjust_detail(32, 3), adjust_detail(32, 3));
        glPushMatrix();
        glScaled(0.1, 0.1, 0.1);
        {
          GLfloat params[] = { 0, 0, 0, 1 };
          glLightfv(GL_LIGHT1, GL_POSITION, params);
        }
        {
          GLfloat params[] = { 0.5 };
          glLightfv(GL_LIGHT1, GL_LINEAR_ATTENUATION, params);
        }
        glPopMatrix();
        glNormal3d(0, 0, 1);
        glEnable(GL_LIGHT1);
        int const m = 20;
        for(int i=0; i<m; ++i)
        {
          double const r = (i+1) / double(m);
          glBegin(GL_POLYGON);
          int const n = adjust_detail(32, 3);
          for(int j=0; j<n; ++j)
          {
            double const a = j * (2*M_PI / n);
            glVertex3d(r*cos(a), r*sin(a), -(1+i));
          }
          glEnd();
        }
        glPopMatrix();
      }


/*
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glColor3f(0.3, 0.8, 0.3);
      gluSphere(quadric, 1, 50, 50);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      glPushMatrix();
      glColor3f(0.8, 0.3, 0.3);
      glTranslated(-0.07, 0, -60.5);
      gluCylinder(quadric, 0.02, 0.02, 64,
                  adjust_detail(25, 3), adjust_detail(200, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.8, 0.3, 0.3);
      glTranslated(+0.07, 0, -60.5);
      gluCylinder(quadric, 0.02, 0.02, 64,
                  adjust_detail(25, 3), adjust_detail(200, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.1, 0.9, 0.9);
      glTranslated(0, 0, -16);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, -12.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.9, 0.1, 0.9);
      glTranslated(0, 0, -9);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, -5.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.9, 0.9, 0.1);
      glTranslated(0, 0, -2);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();

      glPushMatrix();
      glColor3f(0.2, 0.2, 0.8);
      glTranslated(0, 0, 1.5);
      gluCylinder(quadric, 0.2, 0.2, 1.6,
                  adjust_detail(50, 3), adjust_detail(25, 1));
      glPopMatrix();
*/
    }

    GLUquadric *quadric;
  };
}

int main(int argc, char const *argv[]) throw()
{
  set_terminate(&Cxx::terminate_handler);
    
  try_fix_preinstall_datadir(argv[0], "render/test/");
  Application::Config cfg;
  CommandlineOptions opts;
  opts.add_help("Test application for the rendering application foundation");
  opts.check_num_args();
  opts.add_group(cfg);
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;
  Render(cfg).run();
  Thread::main_exit_wait();
  return 0;
}
