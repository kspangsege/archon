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

#include <algorithm>
#include <numeric>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stack>
#include <iostream>

#include <GL/gl.h>
#include <fenv.h>

#include <archon/core/assert.hpp>
#include <archon/core/generate.hpp>
#include <archon/core/random.hpp>
#include <archon/core/text.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/intersect.hpp>
#include <archon/util/conv_hull.hpp>
#include <archon/util/color.hpp>
#include <archon/font/util.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/billboard.hpp>
#include <archon/render/text_formatter.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;


namespace
{
  int opt_num_points = 32;


  /**
   * How close can an axis aligned rectangle get to a circle
   * (exclusion area) while avoiding overlap and requiring that the
   * vector from the center of the circle to the center of the
   * rectangle has the specified angle.
   *
   * \param angle Specified in radians. Zero indicates that the
   * rectangle is on the right side of the circle, while half pi
   * indicates that it is above the circle.
   */
  Vec2 calc_point_label_pos(double excl_radius, double angle, Vec2 size, double corner_radius = 0)
  {
    Vec2 const dir(cos(angle), sin(angle));
    double const slope = dir[1] / dir[0];
    double const abs_slope = abs(slope);
    Vec2 const half_size(0.5 * size);
    double const w_in  = half_size[0] - corner_radius;
    double const w_out = half_size[0] + excl_radius;
    double const h_in  = half_size[1] - corner_radius;
    double const h_out = half_size[1] + excl_radius;
    double const low_slope  = h_in / w_out;
    double const high_slope = h_out / w_in;

    if(abs_slope <= low_slope)
    {
      double const x = dir[0] < 0 ? -w_out : w_out;
      return Vec2(x, slope*x) - half_size;
    }

    if(high_slope <= abs_slope)
    {
      double const y = dir[1] < 0 ? -h_out : h_out;
      return Vec2(y/slope, y) - half_size;
    }

    double dist;
    Vec2 corner(dir[0] < 0 ? -w_in : w_in, dir[1] < 0 ? -h_in : h_in);
    intersect_sphere<true>(Line2(-corner, dir), dist, excl_radius + corner_radius);
    return dist * dir - half_size;
  }




  struct RimFrag
  {
    size_t first, last;
    Vec3 proj_z;
    RimFrag(size_t first, size_t last, Vec3 const &proj_z):
      first(first), last(last), proj_z(proj_z) {}
  };

  struct Face
  {
    Vec3F color;
    Vec3 normal;
    Vec3 center;
  };


  struct TrifanHandler: ConvHull::TrifanHandler
  {
    void add_vertex(size_t point_index)
    {
      vertices.push_back(point_index);
      ++num_vertices;
    }

    void close_trifan()
    {
      trifans.push_back(num_vertices-2);
      num_vertices = 2;
      ++num_trifans;
    }

    void close_trifan_set()
    {
      trifan_sets.push_back(num_trifans);
      num_vertices = num_trifans = 0;
    }

    TrifanHandler(vector<size_t> *v, vector<size_t> *t, vector<size_t> *s):
      vertices(*v), trifans(*t), trifan_sets(*s), num_vertices(0), num_trifans(0) {}

    vector<size_t> &vertices, &trifans, &trifan_sets;
    size_t num_vertices, num_trifans;
  };



  struct ConvHullApp: Application
  {
    ConvHullApp(Application::Config const &cfg):
      Application("archon::render::ConvHull", cfg),
      text_formatter(get_font_provider()),
      points_display_on(true), point_labels_display_on(false),
      colorize_on(true), normals_display_on(false),
      point_winding(150), max_depth(1)
    {
      register_key_handler(KeySym_p, &ConvHullApp::toggle_points_display,
                           "Toggle display of points generating the convex hull.");
      register_key_handler(KeySym_d, &ConvHullApp::toggle_point_labels_display,
                           "Toggle display of point labels.");
      register_key_handler(KeySym_c, &ConvHullApp::toggle_colorize,
                           "Toggle colorization of the convex hull.");
      register_key_handler(KeySym_n, &ConvHullApp::toggle_normals_display,
                           "Toggle display of face normals.");
      register_key_handler(KeySym_Insert, &ConvHullApp::inc_num_points,
                           "Increment number of points.");
      register_key_handler(KeySym_Delete, &ConvHullApp::dec_num_points,
                           "Decrement number of points.");
      register_key_handler(KeySym_Right, &ConvHullApp::inc_point_winding,
                           "Increase point winding.");
      register_key_handler(KeySym_Left, &ConvHullApp::dec_point_winding,
                           "Decrease point winding.");
      register_key_handler(KeySym_Page_Up, &ConvHullApp::inc_max_depth,
                           "Increase maximum depth of algorithm.");
      register_key_handler(KeySym_Page_Down, &ConvHullApp::dec_max_depth,
                           "Decrease maximum depth of algorithm.");

      text_formatter.set_text_color(Vec4F(1,1,0,1));

      init();

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_NORMALIZE);
      glEnable(GL_POINT_SMOOTH);
      glEnable(GL_COLOR_MATERIAL);
      glEnable(GL_CULL_FACE);
      glShadeModel(GL_FLAT);
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
      glPointSize(7);
    }



    void init_points()
    {
      points.clear();

/*
      // Minimal
      {
        points.push_back(Vec3(0,0,0));
        points.push_back(Vec3(1,0,0));
        points.push_back(Vec3(0,1,0));
        points.push_back(Vec3(0,0,-1));
      }
*/



/*
      // Produce a set of points for testing purposes
      {
        int const n = 3;
        for(int i=0; i<n; ++i)
        {
          double f = 2*M_PI*i/double(n);
          points.push_back(Vec3(0.5*sin(f), -1, 0.5*cos(f)));
        }
        for(int i=0; i<n; ++i)
        {
          double f = 2*M_PI*i/double(n);
          points.push_back(Vec3(sin(f), 0, cos(f)));
        }
        for(int i=0; i<n; ++i)
        {
          double f = 2*M_PI*i/double(n);
          points.push_back(Vec3(0.5*sin(f), 1, 0.5*cos(f)));
        }
      }
*/


/*
      {
        points.push_back(Vec3(-1, 0, 0));
        points.push_back(Vec3(+1, 0, 0));
        int const n = 19;
        for(int i=0; i<n; ++i)
        {
          double f = 2*M_PI*i/double(n);
          points.push_back(Vec3(0, sin(f), cos(f)));
        }
      }
*/


/*
      // Axis elongated spiral
      {
        int const n = 128;
        for(int i=0; i<n; ++i)
        {
          double f = i/double(n);
          double a = sqrt(1-(f*2-1)*(f*2-1))/2/2;
          points.push_back(Vec3(a*sin(point_winding*f), f*2-1, a*cos(point_winding*f)));
        }
      }
*/


      // A set of random points
      {
        unsigned long const seed = 11684281426618421174UL;
//        unsigned long const seed = Random().get_uint<unsigned long>();
        cerr << "Random seed = " << seed << endl;
        Random random(seed);

/*
        for(int i=0; i<opt_num_points; ++i) points.push_back(Vec3(random.get_uniform()-0.5,
                                                                  random.get_uniform()-0.5,
                                                                  random.get_uniform()-0.5));
*/

        for(int i=0; i<117; ++i) points.push_back(Vec3(random.get_uint<unsigned>(31)/31.0-0.5,
                                                       random.get_uint<unsigned>(31)/31.0-0.5,
                                                       random.get_uint<unsigned>(31)/31.0-0.5));
      }


/*
      // A cube
      {
        points.push_back(Vec3(-1,-1,-1));
        points.push_back(Vec3(+1,-1,-1));
        points.push_back(Vec3(-1,+1,-1));
        points.push_back(Vec3(+1,+1,-1));
        points.push_back(Vec3(-1,-1,+1));
        points.push_back(Vec3(+1,-1,+1));
        points.push_back(Vec3(-1,+1,+1));
        points.push_back(Vec3(+1,+1,+1));
      }
*/


/*
      // A tetrahedron
      {
        points.push_back(Vec3(-1,+0,+1));
        points.push_back(Vec3(+1,+0,+1));
        points.push_back(Vec3(+0,-1,-1));
        points.push_back(Vec3(+0,+1,-1));
      }
*/


/*
      // Color gamut from image
      {
        Image::ConstRef image = Image::load("alley_baggett.png");
        ImageReader reader(image);
        int const width = reader.get_width(), height = reader.get_height();
        points.resize(height * size_t(width));
        Array<double> const buf(width * 3);
        for(int i=0; i<height; ++i)
        {
          reader.set_pos(0,i).get_block_rgb(buf.get(), width, 1);
          size_t const offset = i * width;
          for(int j=0; j<width; ++j)
          {
            double *const b = buf.get() + j*3;
            points[offset + j].set(b[0], b[1], b[2]);
          }
        }
        sort(points.begin(), points.end());
        vector<Vec3>::iterator const i = unique(points.begin(), points.end());
        points.erase(i, points.end());
      }
*/
    }



    void init_hull()
    {
      vertices.clear();
      trifans.clear();
      trifan_sets.clear();
      error = false;
      TrifanHandler trifan_handler(&vertices, &trifans, &trifan_sets);
      ConvHull::compute(points, trifan_handler, max_depth);

      size_t const num_faces = accumulate(trifans.begin(), trifans.end(), 0);
      faces.resize(num_faces);

      size_t const num_points = points.size();

      {
        size_t num_colors;
        vector<size_t> face_colors(num_faces);
        {
          list<size_t> colors;
          vector<vector<size_t> > used(num_points);

          size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
          for(size_t i=0; i<trifan_sets.size(); ++i)
          {
            size_t n = trifan_sets[i], m = trifans[fan_idx++];
            size_t const i0 = vertices[vtx_idx++];
            size_t const i1 = vertices[vtx_idx++];
            Vec3 p0 = points[i0];
            Vec3 p1 = points[i1];
            Vec3 v1 = p1 - p0;
            vector<size_t> *u0 = &used[i0];
            vector<size_t> *u1 = &used[i1];
            for(;;)
            {
              size_t const i2 = vertices[vtx_idx++];
              Vec3 const p2 = points[i2];
              Vec3 const v2 = p2 - p0;
              Face &face = faces[tri_idx];
              face.normal = unit(v1 * v2);
              face.center = (p0 + p1 + p2) / 3.0;

              vector<size_t> *const u2 = &used[i2];
              set<size_t> u;
              u.insert(u0->begin(), u0->end());
              u.insert(u1->begin(), u1->end());
              u.insert(u2->begin(), u2->end());
              size_t color;
              list<size_t>::iterator j = colors.begin();
              for(;;)
              {
                if(j == colors.end())
                {
                  color = colors.size();
                  break;
                }
                color = *j;
                if(u.find(color) == u.end())
                {
                  colors.erase(j);
                  break;
                }
                ++j;
              }
              colors.push_back(color);
              u0->push_back(color);
              u1->push_back(color);
              u2->push_back(color);
              face_colors[tri_idx] = color;

              ++tri_idx;
              if(--m == 0)
              {
                if(--n == 0) break;
                m = trifans[fan_idx++];
                p0 = p2;
                v1 = p1 - p0;
                u0 = u2;
              }
              else
              {
                p1 = p2;
                v1 = v2;
                u1 = u2;
              }
            }
          }

          num_colors = colors.size();
        }

        vector<size_t> permutation(num_colors);
        generate(permutation.begin(), permutation.end(), make_inc_generator<size_t>());
        random_shuffle(permutation.begin(), permutation.end());

        vector<Vec3F> colors(num_colors);
        for(size_t i=0; i<num_colors; ++i)
          colors[i] = color::cvt_HSV_to_RGB(Vec3F(double(permutation[i])/num_colors, 0.3, 0.5));

        for(size_t i=0; i<faces.size(); ++i) faces[i].color = colors[face_colors[i]];
      }

/*
      cerr << "Status:\n";
      for(size_t i=0; i<faces.size(); ++i) cerr << "Face "<<i<<": normal = "<<faces[i].normal<<", color = "<<faces[i].color << endl;
*/
    }



    void init()
    {
      init_points();
      init_hull();
    }


  private:
    void  render_scene()
    {
      size_t const num_points = points.size();

      if(points_display_on)
      {
        glDisable(GL_LIGHTING);
        glColor3f(1,1,1);
        glBegin(GL_POINTS);
        for(size_t i=0; i<num_points; ++i)
        {
          if(i == vertices[0])      glColor3f(1.0, 0.3, 0.3);
          else if(i == vertices[1]) glColor3f(0.3, 1.0, 0.3);
          else if(i == vertices[2]) glColor3f(0.3, 0.3, 1.0);
          else                      glColor3f(1.0, 1.0, 1.0);
          Vec3 const &p = points[i];
          glVertex3d(p[0], p[1], p[2]);
        }
        glEnd();
      }

      if(error)
      {
        glDisable(GL_LIGHTING);
        glBegin(GL_TRIANGLES);
        Vec3 const &p1 = points[error_1];
        Vec3 const &p2 = points[error_2];
        Vec3 const &p3 = points[error_3];
        glColor3f(1,0,0);
        glVertex3d(p1[0], p1[1], p1[2]);
        glVertex3d(p2[0], p2[1], p2[2]);
        glVertex3d(p3[0], p3[1], p3[2]);
        glEnd();
      }

      glEnable(GL_LIGHTING);

      glColor3f(0.5, 0.5, 0.5);
      {
        size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
        for(size_t i=0; i<trifan_sets.size(); ++i)
        {
          size_t n = trifan_sets[i], m = trifans[fan_idx++];
          size_t vtx0 = vertices[vtx_idx++];
          size_t vtx1 = vertices[vtx_idx++];
          glBegin(GL_TRIANGLE_FAN);
          {
            Vec3 const &p0 = points[vtx0];
            Vec3 const &p1 = points[vtx1];
            glVertex3d(p0[0], p0[1], p0[2]);
            glVertex3d(p1[0], p1[1], p1[2]);
          }
          for(;;)
          {
            size_t const vtx2 = vertices[vtx_idx++];
            Face const &face = faces[tri_idx++];
            if(colorize_on)
            {
              Vec3F const &color = face.color;
              glColor3f(color[0], color[1], color[2]);
            }
            Vec3 const &normal = face.normal;
            glNormal3d(normal[0], normal[1], normal[2]);
            Vec3 const &point = points[vtx2];
            glVertex3d(point[0], point[1], point[2]);

            if(--m == 0)
            {
              glEnd();
              if(--n == 0) break;
              m = trifans[fan_idx++];
              vtx0 = vtx2;
              glBegin(GL_TRIANGLE_FAN);
              {
                Vec3 const &p0 = points[vtx0];
                Vec3 const &p1 = points[vtx1];
                glVertex3d(p0[0], p0[1], p0[2]);
                glVertex3d(p1[0], p1[1], p1[2]);
              }
            }
            else vtx1 = vtx2;
          }
        }
      }

      if(normals_display_on)
      {
        glDisable(GL_LIGHTING);
        glColor3f(1, 0, 0);
        glBegin(GL_LINES);
        size_t const num_faces = faces.size();
        for(size_t i=0; i<num_faces; ++i)
        {
          Face const &face = faces[i];
          Vec3 const &p0 = face.center;
          Vec3 p1 = p0 + face.normal;
          glVertex3d(p0[0], p0[1], p0[2]);
          glVertex3d(p1[0], p1[1], p1[2]);
        }
        glEnd();
      }

      if(point_labels_display_on)
      {
        GLint params[2];
        glGetIntegerv(GL_POLYGON_MODE, params);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_LIGHTING);
        glColor3f(1,1,1);
        for(size_t i=0; i<num_points; ++i)
        {
          Vec3 const &p = points[i];
          glPushMatrix();
          glTranslatef(p[0], p[1], p[2]);
          double const angle = billboard::rotate();
          glScalef(0.05, 0.05, 0.05);
          text_layout.set(&text_formatter, value_printer.print(i));
          Vec2 const size(text_layout.get_width(), text_layout.get_height());
          double const excl_radius = 0.75;
          double const corner_radius = 0.25;
          Vec2 const q = calc_point_label_pos(excl_radius, angle, size, corner_radius);
          glTranslatef(q[0], q[1], 0);
          text_layout.render();
          glPopMatrix();
        }
        glPolygonMode(GL_FRONT, params[0]);
        glPolygonMode(GL_BACK, params[1]);
      }
    }


    bool toggle_points_display(bool key_down)
    {
      if(key_down)
      {
        set_on_off_status(L"POINTS", points_display_on ^= true);
        return true;
      }
      return false;
    }


    bool toggle_point_labels_display(bool key_down)
    {
      if(key_down)
      {
        set_on_off_status(L"POINT LABELS", point_labels_display_on ^= true);
        return true;
      }
      return false;
    }


    bool toggle_colorize(bool key_down)
    {
      if(key_down)
      {
        set_on_off_status(L"COLORIZE", colorize_on ^= true);
        return true;
      }
      return false;
    }


    bool toggle_normals_display(bool key_down)
    {
      if(key_down)
      {
        set_on_off_status(L"NORMALS", normals_display_on ^= true);
        return true;
      }
      return false;
    }


    bool inc_num_points(bool key_down)
    {
      if(key_down)
      {
        set_int_status(L"", ++opt_num_points, L" POINTS");
        init();
        return true;
      }
      return false;
    }

    bool dec_num_points(bool key_down)
    {
      if(key_down)
      {
        set_int_status(L"", --opt_num_points, L" POINTS");
        init();
        return true;
      }
      return false;
    }


    bool inc_point_winding(bool key_down)
    {
      if(key_down)
      {
        set_float_status(L"POINT WINDING = ", point_winding += 0.5);
        init();
        return true;
      }
      return false;
    }

    bool dec_point_winding(bool key_down)
    {
      if(key_down)
      {
        set_float_status(L"POINT WINDING = ", point_winding -= 0.5);
        init();
        return true;
      }
      return false;
    }


    bool inc_max_depth(bool key_down)
    {
      if(key_down)
      {
        set_int_status(L"MAX DEPTH = ", ++max_depth);
        init_hull();
        return true;
      }
      return false;
    }

    bool dec_max_depth(bool key_down)
    {
      if(key_down)
      {
        set_int_status(L"MAX DEPTH = ", --max_depth);
        init_hull();
        return true;
      }
      return false;
    }



    // Text rendering
    archon::render::TextFormatter text_formatter;
    TextLayout text_layout;
    Text::WideValuePrinter value_printer;


    // Convex hull input
    vector<Vec3> points;

    // Convex hull output
    vector<size_t> vertices;
    vector<size_t> trifans; // There is one triangle fan per entry in this vector. Each entry states how many triangles are in the fan.
    vector<size_t> trifan_sets; // There is one set of triangle fans per entry in this vector. Each entry states the number of fans in the set.

    /*
      The triangles are generated as follows:

      For each entry in trifan_sets:
        Consume the next two vertices from 'vertices' and use them as the first two vertices of the first triangle fan in this set in the order they are extracted.
        For each triangle fan in the set:
          Consume as many vertices from 'vertices' as there are triangles in this fan.
          Use the last two vertices of this fan as the first two vertices of the next fan in reverse order.
    */


    // Render friendly representation of convex hull
    vector<Face> faces;

    bool points_display_on, point_labels_display_on, colorize_on, normals_display_on;

    double point_winding;

    size_t max_depth;

    bool error;
    size_t error_1, error_2, error_3;
  };
}



int main(int argc, char const *argv[]) throw()
{
  set_terminate(&cxx::terminate_handler);

  try_fix_preinstall_datadir(argv[0], "render/test/");

  Application::Config app_cfg;
  CommandlineOptions opts;
  opts.add_help("Test application for the rendering application foundation");
  opts.check_num_args();
  opts.add_group(app_cfg);
  opts.add_param("n", "num-points", opt_num_points, "Set the number of random points to use");
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  ConvHullApp(app_cfg).run();
  return 0;
}
