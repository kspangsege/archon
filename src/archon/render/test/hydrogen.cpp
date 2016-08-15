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

#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/math/functions.hpp>
#include <archon/math/interval.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/color.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/image/writer.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::render;


namespace {

struct Particle {
    Vec3   loc;     // Location in meters
    Vec3   veloc;   // Velocity in meters per second
    Vec3   accel;   // Acceleration in meters per second squared
    double charge;  //
    double mass;    // Mass in kilograms
    double radius;  // Radius of particle in meters
    Vec3   color;   // Color of particle

    Particle():
        loc(0, 0, 0),
        veloc(0, 0, 0),
        accel(0, 0, 0),
        mass(1),
        radius(1),
        color(1)
    {
    }
};

const double speed_of_light  = 299792458;          // meters per second
const double permeability    = 4*M_PI*1E-7;          // Vacuum permeability (magnetic constant) in henry per meter
const double permitivity     = 1/(permeability*square(speed_of_light)); // Vacuum permitivity (electric constant) in farads per meter
const double coul_const      = square(speed_of_light)*1E-7; // Coulomb's constant (newton meters squared per coulomb squared)
const double grav_const      = 6.67428E-11;        // Gravitational constant (metres cubed per kilogram per second squared)
const double elem_charge     = 1.602176487E-19;    // coulombs
const double elec_mass       = 9.10938215E-31;     // kilograms
const double prot_mass       = 1.672621637E-27;    // kilograms
const double radius_scale    = 0.1 / archon::math::cbrt(prot_mass);
const double avg_elec_dist   = 37E-12;
const double init_elec_speed = sqrt(2*(coul_const*square(elem_charge)/avg_elec_dist)/elec_mass) / 1.3;
const double elec_orbit_time = M_PI * 2 * avg_elec_dist / init_elec_speed;
const double scale           = 1/avg_elec_dist; // model distance times scale is real dist
const double time_scale      = 3/elec_orbit_time; // model time times time_scale is real time
const double neutral_dist    = avg_elec_dist/16;  // The distance where the repulsive force cancels Coulomb's force exactly
const double repul_fact      = neutral_dist * coul_const * square(elem_charge); // Such that repul_fact * neutral_dist^3 == coulomb(electron, electron, neutral_dist)
const double wall_elasticity = 1;

const int num_protons = 3;
const int num_electrons = 3;

const int num_particles = num_protons + num_electrons;

const Interval loc_range(-1, 1);

const Vec3 low_bound  = avg_elec_dist * Vec3(loc_range.begin-2, -2, -2);
const Vec3 high_bound = avg_elec_dist * Vec3(loc_range.end+2,    2,  2);

Vec3F floor_color(0.5, 0.5, 0.5);
Vec3F wall_color(0.3, 0.3, 0.3);
Vec3F ceil_color(0.1, 0.1, 0.1);

Vec3 magnetic_field(0, 0, 35E35); // teslas


class Hydrogen: public Application {
public:
    Hydrogen(const Application::Config& cfg):
        Application("archon::render::Hydrogen", cfg),
        m_image_writer(1000, 1000)
    {
        for (int i = 0; i < num_protons; ++i) {
            double loc = avg_elec_dist * lin_interp(i, 0, num_protons-1, loc_range.begin, loc_range.end);
            Particle& p = m_particles[i];
            p.loc    = Vec3(loc, 0, 0);
            p.veloc  = Vec3(0,0,0);
            p.charge = elem_charge;
            p.mass   = prot_mass;
            p.radius = radius_scale * archon::math::cbrt(p.mass);
            p.color  = color::cvt_HSV_to_RGB(Vec3(double(i)/num_protons, 0.5, 1));
        }
        for (int i = 0; i < num_electrons; ++i) {
            double loc = avg_elec_dist * lin_interp(i, 0, num_electrons-1, loc_range.begin, loc_range.end);
            Particle& p = m_particles[num_protons + i];
            p.loc    = Vec3(loc, avg_elec_dist, 0);
            p.veloc  = Vec3(init_elec_speed,0,0);
            p.charge = -elem_charge;
            p.mass   = elec_mass;
            p.radius = radius_scale * archon::math::cbrt(p.mass);
            p.color  = color::cvt_HSV_to_RGB(Vec3(double(i)/num_electrons, 0.5, 1));
        }

        m_particles[num_protons].loc[2] = avg_elec_dist/20;

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_NORMALIZE);
        glEnable(GL_CULL_FACE);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

        GLUquadric* quadric = gluNewQuadric();
        m_sphere = glGenLists(1);
        glNewList(m_sphere, GL_COMPILE);
        gluSphere(quadric, 1, adjust_detail(20, 3), adjust_detail(20, 3));
        glEndList();
        gluDeleteQuadric(quadric);
    }

private:
    void render_scene()
    {
        int n = 1000; // Precision
        double time = 1.0/60 / time_scale / n;
        for (int i = 0; i < n; ++i) {
            update_accel();
            integrate(time);
        }

        {
            glCullFace(GL_FRONT);
            Vec3 h = scale * high_bound;
            Vec3 l = scale * low_bound;
            glBegin(GL_QUADS);
            glColor3f(wall_color[0], wall_color[1], wall_color[2]);
            glNormal3f(  1,  0,  0 );
            glVertex3d(h[0], l[1], h[2]); // 1
            glVertex3d(h[0], l[1], l[2]); // 2
            glVertex3d(h[0], h[1], l[2]); // 4
            glVertex3d(h[0], h[1], h[2]); // 3
            glColor3f(ceil_color[0], ceil_color[1], ceil_color[2]);
            glNormal3f(  0,  1,  0 );
            glVertex3d(h[0], h[1], h[2]); // 3
            glVertex3d(h[0], h[1], l[2]); // 4
            glVertex3d(l[0], h[1], l[2]); // 6
            glVertex3d(l[0], h[1], h[2]); // 5
            glColor3f(wall_color[0], wall_color[1], wall_color[2]);
            glNormal3f( -1,  0,  0 );
            glVertex3d(l[0], h[1], h[2]); // 5
            glVertex3d(l[0], h[1], l[2]); // 6
            glVertex3d(l[0], l[1], l[2]); // 8
            glVertex3d(l[0], l[1], h[2]); // 7
            glColor3f(floor_color[0], floor_color[1], floor_color[2]);
            glNormal3f(  0, -1,  0 );
            glVertex3d(l[0], l[1], h[2]); // 7
            glVertex3d(l[0], l[1], l[2]); // 8
            glVertex3d(h[0], l[1], l[2]); // 2
            glVertex3d(h[0], l[1], h[2]); // 1
            glColor3f(wall_color[0], wall_color[1], wall_color[2]);
            glNormal3f(  0,  0,  1 );
            glVertex3d(l[0], l[1], h[2]); // 7
            glVertex3d(h[0], l[1], h[2]); // 1
            glVertex3d(h[0], h[1], h[2]); // 3
            glVertex3d(l[0], h[1], h[2]); // 5
            glColor3f(wall_color[0], wall_color[1], wall_color[2]);
            glNormal3f(  0,  0, -1 );
            glVertex3d(l[0], h[1], l[2]); // 6
            glVertex3d(h[0], h[1], l[2]); // 4
            glVertex3d(h[0], l[1], l[2]); // 2
            glVertex3d(l[0], l[1], l[2]); // 8
            glEnd();
        }

        glCullFace(GL_BACK);
        for (int i = 0; i < num_particles; ++i) {
            Particle& p = m_particles[i];
            glPushMatrix();
            Vec3 loc = scale * p.loc;
            glTranslated(loc[0], loc[1], loc[2]);
            glScaled(p.radius, p.radius, p.radius);
            glColor3d(p.color[0], p.color[1], p.color[2]);
            glCallList(m_sphere);
            glPopMatrix();
        }

        if (++m_num_iters % 600 == 0) {
            std::ostringstream o;
            o << "/tmp/hallgeir_" << (m_num_iters / 600) << ".png";
            m_image_writer.save(o.str());
        }
    }


    void update_accel()
    {
        for (int i = 0; i < num_particles; ++i) {
            Particle& p = m_particles[i];
            p.accel.set(0);

            for (int j = 0; j < i; ++j) {
                Particle& q = m_particles[j];
                Vec3 diff = q.loc - p.loc;
                double dist_sq = sq_sum(diff);
                double dist = sqrt(dist_sq);
                double dist_cb = dist_sq * dist;
                double force = coulombs_force(p, q, dist_sq) - repul_fact / dist_cb;
                double force_over_dist = force / dist;
                p.accel += (force_over_dist / p.mass) * diff;
                q.accel -= (force_over_dist / q.mass) * diff;
            }

            p.accel += p.charge * (p.veloc * magnetic_field);
        }
    }


    // Newton's law of universal gravitation
    double newtonian_gravity(const Particle& p, const Particle& q, double dist_sq)
    {
        return grav_const * ((p.mass * q.mass) / dist_sq);
    }


    // Coulomb's law
    double coulombs_force(const Particle& p, const Particle& q, double dist_sq)
    {
        return -coul_const * ((p.charge * q.charge) / dist_sq);
    }


    // Integrate velosity and position
    void integrate(double time)
    {
        Particle& track_prot = m_particles[0];
        Particle& track_elec = m_particles[num_protons];
        Vec3 track_elec_loc_before = track_elec.loc;

        double half_time = 0.5 * time;

        for (int i = 0; i < num_particles; ++i) {
            Particle& p = m_particles[i];

            Vec3 prev_veloc = p.veloc;

            // Integrate velosity
            p.veloc += p.accel * time;

            // Integrate position
            p.loc += (p.veloc+prev_veloc) * half_time;

            // Reflect on boundary
            double d;
            if (0<(d=p.loc[0]-high_bound[0]) || (d=p.loc[0]-low_bound[0])<0) {
                p.loc[0] -= d*(1+wall_elasticity);
                p.veloc[0]*=-wall_elasticity;
            }
            if (0<(d=p.loc[1]-high_bound[1]) || (d=p.loc[1]-low_bound[1])<0) {
                p.loc[1] -= d*(1+wall_elasticity);
                p.veloc[1]*=-wall_elasticity;
            }
            if (0<(d=p.loc[2]-high_bound[2]) || (d=p.loc[2]-low_bound[2])<0) {
                p.loc[2] -= d*(1+wall_elasticity);
                p.veloc[2]*=-wall_elasticity;
            }
        }

        bool behind_before = track_elec_loc_before[2] < track_prot.loc[2];
        bool behind_after  = track_elec.loc[2]        < track_prot.loc[2];
        if (behind_before != behind_after) {
            double scale = 1000 / (6*avg_elec_dist);
            Vec3 v = track_elec_loc_before + (track_prot.loc[2] - track_elec_loc_before[2]) / (track_elec.loc[2] - track_elec_loc_before[2]) * (track_elec.loc - track_elec_loc_before);
            m_image_writer.set_pos(500+scale*v[0], 500+scale*v[1]).put_pixel(color::white);
        }
    }


    Particle m_particles[num_particles];
    GLuint m_sphere;
    ImageWriter m_image_writer;
    int m_num_iters = 0;
};

} // unnamed namespace


int main(int argc, char const *argv[]) throw()
{
    std::set_terminate(&cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "render/test/");
    Application::Config cfg;
    CommandlineOptions opts;
    opts.add_help("Hydrogen atom simulator");
    opts.check_num_args();
    opts.add_group(cfg);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    Hydrogen(cfg).run();
}
