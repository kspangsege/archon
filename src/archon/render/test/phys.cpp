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

#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/color.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::Imaging;
using namespace archon::Render;


namespace
{
  // Fluid viscosities
  double const viscos_water_0      = 1.79E-3; // Water at 0 deg. Celcius
  double const viscos_water_20     = 1.01E-3; // Water at 20 deg. Celcius
  double const viscos_water_40     = 0.66E-3; // Water at 40 deg. Celcius
  double const viscos_glycerine_20 = 1.49E+1; // Glycerine at 20 deg. Celcius
  double const viscos_oil          = 3.16E-1; // Oil
  double const viscos_air_0        = 1.71E-5; // Water at 0 deg. Celcius
  double const viscos_air_20       = 1.82E-5; // Water at 20 deg. Celcius
  double const viscos_air_40       = 1.90E-5; // Water at 40 deg. Celcius
  double const viscos_hydrogen_20  = 0.88E-5; // Hydrogen at 20 deg. Celcius
  double const viscos_argon_20     = 2.23E-5; // Argon at 20 deg. Celcius


  // Physical quantities
  int    const num_particles     = 5;
  Vec3   const acceleration      = Vec3(0, -0.03, 0); // Universal acceleration -0.0015;
  double const gravity_constant  = 6.668E-11; // Constant of gravity
  double const viscosity         = viscos_air_20;
  double const low_mass          = 10E9;
  double const high_mass         = 10E9;
  double const low_radius        = 1;
  double const high_radius       = 1;
  double const time_resolution   = 100; // ticks per second
  Vec3   const low_bound         = Vec3(-10, -10, -10);
  Vec3   const high_bound        = Vec3( 10,  10,  10);



  struct Particle
  {
    Vec3   location;       // Location / m
    Vec3   velocity;       // Velosity / m/s
    Vec3   acceleration;   // Acceleration / m/s^2
    double mass;           // Mass / kg
    double radius;         // Radius / m
    double viscos_effect;  // Effect of viscosity (6*pi*radius*viscosity)
    Vec3   color;          // Color of particle

    Particle(): location(0, 0, 0), velocity(0, 0, 0),
                acceleration(0, 0, 0), mass(1), radius(1), viscos_effect(0), color(1) {}
  };



  /**
   * \param d The direction from the center of ball 1 towards the
   * center of ball 2 (length must be non-zero).
   *
   * \param m1 Mass of ball 1.
   *
   * \param m2 Mass of ball 2.
   *
   * \param v1 Velocity of ball 1.
   *
   * \param v2 Velocity of ball 2.
   */
/*
  void elastic_ball_collision(const Vec3 &d, double m1, double m2, Vec3 &v1, Vec3 &v2)
  {
    // Copy for performance
    Vec3 u1 = v1, u2 = v2;

    // Determine velocity of center-of-mass
    // v1 = p1'-p1
    // v2 = p2'-p2
    // pc  = m1*p1  + m2*p2  / (m1+m2)
    // pc' = m1*p1' + m2*p2' / (m1+m2)
    // vc = pc' - pc = (m1*p1' + m2*p2') / (m1+m2) - (m1*p1 + m2*p2) / (m1+m2) = (m1*p1' - m1*p1 + m2*p2' - m2*p2) / (m1+m2) = (m1*(p1'-p1) + m2*(p2'-p2)) / (m1+m2) = (m1*v1 + m2*v2) / (m1+m2)
    Vec3 v = u1;
    v *= m1;
    Vec3 vc = u2;
    vc *= m2;
    vc += v;
    vc /= m1+m2;

    // Tranform to center-of-mass system
    u1 -= vc;
    u2 -= vc;

    // Do the reflections in the center-of-mass system
    double const f = -2 / sq_sum(d);
    v = d;
    v  *= dot(u1, v) * f;
    u1 += v;
    v   = d;
    v  *= dot(u2, v) * f;
    u2 += v;

    // Transform back to original system
    u1 += vc;
    u2 += vc;

    // Push result
    v1 = u1;
    v2 = u2;
  }
*/


  struct Phys: Application
  {
    Phys(Application::Config const &cfg):
      Application("archon::Render::Phys", cfg)
    {
      for(int i=0;i<num_particles;++i)
      {
        Particle &p = particles[i];
        double angle = i*(2*M_PI/num_particles);
        p.location = Vec3(cos(angle), sin(angle), num_particles == 1 ? 0 : 2.0*i/(num_particles-1)-1) * 7.0;
        p.color    = Vec3(square(cos(angle)), square(sin(angle)), 1.0*i/num_particles);
        // Distribute masses 0,1,...,i-1 lineary between low_mass and high_mass
        p.mass     = (high_mass   - low_mass)   / (num_particles-1.0) * i + low_mass;
        p.radius   = (high_radius - low_radius) / (num_particles-1.0) * i + low_radius;
        p.viscos_effect = 6*M_PI*p.radius*viscosity;
      }
      //particles[0].velocity[0]=0.5;
      //particles[1].velocity[0]=-0.5;

      glEnable(GL_LIGHTING);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_COLOR_MATERIAL);

      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

      sphere = glGenLists(1);
      GLUquadric *const quadric = gluNewQuadric();
      glNewList(sphere, GL_COMPILE);
      gluSphere(quadric, 1, adjust_detail(20, 3), adjust_detail(20, 3));
      glEndList();
      gluDeleteQuadric(quadric);
    }

  private:
    void render_scene()
    {
      // Clear accelerations
      for(int i=0; i<num_particles; ++i) particles[i].acceleration = Vec3(0, 0, 0);

      // Add up acceleration contributions
      for(int i=0; i<num_particles; ++i)
      {
        Particle &p = particles[i];
        // Inter-particle forces

        for(int j=i+1; j<num_particles; ++j)
        {
          Particle &q = particles[j];

          // Determine the vector from object-i to object-j
          //Vec3 d = q.location - p.location;
          Vec3 d(d*1.0);

          // Determine the absolute distance
          double const sq_dist = sq_sum(d);
          double const dist = sqrt(sq_dist);

          // Add up scalar forces that act parralel to the line connecting the two objects.

          double force = 0; // Force on object-i caused by object-j (attraction is positive)
          
          // Gravity
          force += gravity_constant * p.mass*q.mass/sq_dist;

          if(dist<p.radius+q.radius)
          {
            
          }

          // Elastical bounce
          //if(dist<Radius[i]+Radius[j])
          //force -= 1E13/pow(dist,3); // +1/pow(Radius[i]+Radius[j],3);

          // Spring force
          //if(SpringMatrix[i][j][0]>0) F += (dist-SpringMatrix[i][j][0])*SpringMatrix[i][j][1];

          // Count in this force in the acceleration of both object-i and object-j

          double const accel_p =  force/p.mass; // Acceleration caused by object-j on object-i
          double const accel_q = -force/q.mass; // Acceleration caused by object-i on object-j

          // Determine unit-vector for rectagular projection
          Vec3 const proj = unit(d);

          p.acceleration += proj*accel_p;
          q.acceleration += proj*accel_q;


/*
          if(i==1 && j==4)
          {
            Accel[i].x -= dy/4/square_dist;
            Accel[i].y += dx/4/square_dist;
            Accel[j].x += dy/4/square_dist;
            Accel[j].y -= dx/4/square_dist;
          }
*/
/*
          if(i==18 && j==19)
          {
            Accel[i].x -= dy/10/square_dist;
            Accel[i].y += dx/10/square_dist;
            Accel[j].x += dy/10/square_dist;
            Accel[j].y -= dx/10/square_dist;
          }
*/
        }

        // Viscosity (low speed)
        p.acceleration -= p.velocity*p.viscos_effect;

        // Apply gravity
        p.acceleration += acceleration;
      }

      // Integrate velosity and position
      for(int i=0; i<num_particles; ++i)
      {
        Particle &p = particles[i];

        Vec3 prev_velocity = p.velocity;

        // Integrate velosity
        p.velocity += p.acceleration / time_resolution;

        // Integrate position
        p.location += (p.velocity+prev_velocity)/2.0/time_resolution;

        // Reflect on boundary
        double d;
        if(0<(d=p.location[0]-high_bound[0]) || (d=p.location[0]-low_bound[0])<0) { p.location[0] -= d*2; p.velocity[0]*=-1; }
        if(0<(d=p.location[1]-high_bound[1]) || (d=p.location[1]-low_bound[1])<0) { p.location[1] -= d*2; p.velocity[1]*=-1; }
        if(0<(d=p.location[2]-high_bound[2]) || (d=p.location[2]-low_bound[2])<0) { p.location[2] -= d*2; p.velocity[2]*=-1; }
      }

      for(int i=0; i<num_particles; ++i)
      {
        Particle &p = particles[i];

        // Draw spring connections

/*
        for(int j=0; j<i; ++j)
          if(SpringMatrix[j][i][0]>0)
          {
            update.drawLine(prevScreenPos[i].x, prevScreenPos[i].y, prevScreenPos[j].x, prevScreenPos[j].y, archon::Utilities::Window::sub);
            update.drawLine(screenPos[i].x, screenPos[i].y, screenPos[j].x, screenPos[j].y, archon::Utilities::Window::add);
          }
*/


        glPushMatrix();
        glTranslated(p.location[0], p.location[1], p.location[2]);
        glScaled(p.radius, p.radius, p.radius);
        glColor3d(p.color[0], p.color[1], p.color[2]);
        glCallList(sphere);
        glPopMatrix();
      }
    }

    Particle particles[num_particles];
    GLuint sphere;
  };
}

int main(int argc, char const *argv[]) throw()
{
  set_terminate(&Cxx::terminate_handler);
    
  try_fix_preinstall_datadir(argv[0], "render/test/");
  Application::Config cfg;
  CommandlineOptions opts;
  opts.add_help("Particle kinematics simulator");
  opts.check_num_args();
  opts.add_group(cfg);
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;
  Phys(cfg).run();
  return 0;
}
