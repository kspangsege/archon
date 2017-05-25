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

#include <cstddef>
#include <cstdlib>
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
#include <archon/util/named_colors.hpp>
#include <archon/font/util.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/billboard.hpp>
#include <archon/render/text_formatter.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;


namespace {

int opt_num_points = 32;


/// How close can an axis aligned rectangle get to a circle (exclusion area)
/// while avoiding overlap and requiring that the vector from the center of the
/// circle to the center of the rectangle has the specified angle.
///
/// \param angle Specified in radians. Zero indicates that the rectangle is on
/// the right side of the circle, while half pi indicates that it is above the
/// circle.
Vec2 calc_point_label_pos(double excl_radius, double angle, Vec2 size, double corner_radius = 0)
{
    Vec2 dir{std::cos(angle), std::sin(angle)};
    double slope = dir[1] / dir[0];
    double abs_slope = std::abs(slope);
    Vec2 half_size{0.5 * size};
    double cr = corner_radius;
    if (cr > half_size[0])
        cr = half_size[0];
    if (cr > half_size[1])
        cr = half_size[1];
    double w_in  = half_size[0] - cr;
    double w_out = half_size[0] + excl_radius;
    double h_in  = half_size[1] - cr;
    double h_out = half_size[1] + excl_radius;
    double low_slope  = h_in / w_out;
    double high_slope = h_out / w_in;

    if (abs_slope <= low_slope) {
        double x = dir[0] < 0 ? -w_out : w_out;
        return Vec2{x, slope*x} - half_size;
    }

    if (high_slope <= abs_slope) {
        double y = dir[1] < 0 ? -h_out : h_out;
        return Vec2{y/slope, y} - half_size;
    }

    double dist;
    Vec2 corner{(dir[0] < 0 ? -w_in : w_in), (dir[1] < 0 ? -h_in : h_in)};
    intersect_sphere<true>(Line2{-corner, dir}, dist, excl_radius + cr);
    return dist * dir - half_size;
}


struct RimFrag {
    std::size_t first, last;
    Vec3 proj_z;
    RimFrag(std::size_t first, std::size_t last, const Vec3& proj_z):
        first{first},
        last{last},
        proj_z{proj_z}
    {
    }
};

struct Face {
    Vec3F color;
    Vec3 normal;
    Vec3 center;
};


class TrifanHandler: public conv_hull::TrifanHandler {
public:
    std::vector<std::size_t>& vertices;
    std::vector<std::size_t>& trifans;
    std::vector<std::size_t>& trifan_sets;
    std::size_t num_vertices = 0, num_trifans = 0;

    void add_vertex(std::size_t point_index)
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

    TrifanHandler(std::vector<std::size_t>* v, std::vector<std::size_t>* t,
                  std::vector<std::size_t>* s):
        vertices(*v),
        trifans(*t),
        trifan_sets(*s)
    {
    }
};


class ConvHullApp: public Application {
public:
    ConvHullApp(const Application::Config& cfg):
        Application{"archon::render::ConvHull", cfg},
        m_label_formatter{get_font_provider()}
    {
        auto toggle_points_display = [this](bool key_down) {
            if (key_down) {
                set_on_off_status(L"POINTS", m_points_display_on ^= true);
                return true;
            }
            return false;
        };
        auto toggle_point_labels_display = [this](bool key_down) {
            if (key_down) {
                set_on_off_status(L"POINT LABELS", m_point_labels_display_on ^= true);
                return true;
            }
            return false;
        };
        auto toggle_colorize = [this](bool key_down) {
            if (key_down) {
                set_on_off_status(L"COLORIZE", m_colorize_on ^= true);
                return true;
            }
            return false;
        };
        auto toggle_normals_display = [this](bool key_down) {
            if (key_down) {
                set_on_off_status(L"NORMALS", m_normals_display_on ^= true);
                return true;
            }
            return false;
        };
        auto inc_num_points = [this](bool key_down) {
            if (key_down) {
                set_int_status(L"", ++opt_num_points, L" POINTS");
                init();
                return true;
            }
            return false;
        };
        auto dec_num_points = [this](bool key_down) {
            if (key_down) {
                set_int_status(L"", --opt_num_points, L" POINTS");
                init();
                return true;
            }
            return false;
        };
        auto inc_point_winding = [this](bool key_down) {
            if (key_down) {
                set_float_status(L"POINT WINDING = ", m_point_winding += 0.5);
                init();
                return true;
            }
            return false;
        };
        auto dec_point_winding = [this](bool key_down) {
            if (key_down) {
                set_float_status(L"POINT WINDING = ", m_point_winding -= 0.5);
                init();
                return true;
            }
            return false;
        };
        auto inc_max_depth = [this](bool key_down) {
            if (key_down) {
                set_int_status(L"MAX DEPTH = ", ++m_max_depth);
                init_hull();
                return true;
            }
            return false;
        };
        auto dec_max_depth = [this](bool key_down) {
            if (key_down) {
                set_int_status(L"MAX DEPTH = ", --m_max_depth);
                init_hull();
                return true;
            }
            return false;
        };

        bind_key(KeySym_p, std::move(toggle_points_display),
                 "Toggle display of points generating the convex hull.");
        bind_key(KeySym_d, std::move(toggle_point_labels_display),
                 "Toggle display of point labels.");
        bind_key(KeySym_c, std::move(toggle_colorize),
                 "Toggle colorization of the convex hull.");
        bind_key(KeySym_n, std::move(toggle_normals_display),
                 "Toggle display of face normals.");
        bind_key(KeySym_Insert, std::move(inc_num_points),
                 "Increment number of points.");
        bind_key(KeySym_Delete, std::move(dec_num_points),
                 "Decrement number of points.");
        bind_key(KeySym_Right, std::move(inc_point_winding),
                 "Increase point winding.");
        bind_key(KeySym_Left, std::move(dec_point_winding),
                 "Decrease point winding.");
        bind_key(KeySym_Page_Up, std::move(inc_max_depth),
                 "Increase maximum depth of algorithm.");
        bind_key(KeySym_Page_Down, std::move(dec_max_depth),
                 "Decrease maximum depth of algorithm.");

        m_label_formatter.set_text_color(color::yellow);

        init();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_NORMALIZE);
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_CULL_FACE);
        glPointSize(7);
    }



    void init_points()
    {
        m_points.clear();

/*
        // Minimal
        {
            m_points.push_back(Vec3(0, 0, 0));
            m_points.push_back(Vec3(1, 0, 0));
            m_points.push_back(Vec3(0, 1, 0));
            m_points.push_back(Vec3(0, 0,-1));
        }
*/

/*
        // Produce a set of points for testing purposes
        {
            int n = 3;
            for (int i = 0; i < n; ++i) {
                double f = 2*M_PI*i/double(n);
                m_points.push_back(Vec3(0.5*std::sin(f), -1, 0.5*std::cos(f)));
            }
            for (int i = 0; i < n; ++i) {
                double f = 2*M_PI*i/double(n);
                m_points.push_back(Vec3(sin(f), 0, std::cos(f)));
            }
            for (int i = 0; i < n; ++i) {
                double f = 2*M_PI*i/double(n);
                m_points.push_back(Vec3(0.5*std::sin(f), 1, 0.5*std::cos(f)));
            }
        }
*/

/*
        {
            m_points.push_back(Vec3(-1, 0, 0));
            m_points.push_back(Vec3(+1, 0, 0));
            int n = 19;
            for (int i = 0; i < n; ++i) {
                double f = 2*M_PI*i/double(n);
                m_points.push_back(Vec3(0, std::sin(f), std::cos(f)));
            }
        }
*/

/*
        // Axis elongated spiral
        {
            int n = 128;
            for (int i = 0; i < n; ++i) {
                double f = i/double(n);
                double a = std::sqrt(1-(f*2-1)*(f*2-1))/2/2;
                m_points.push_back(Vec3(a*std::sin(point_winding*f), f*2-1, a*std::cos(point_winding*f)));
            }
        }
*/

        // A set of random points
        {
            unsigned long seed = 11684281426618421174UL;
//            unsigned long seed = Random().get_uint<unsigned long>();
            std::cerr << "Random seed = " << seed << "\n";
            Random random(seed);

/*
            for (int i = 0; i < opt_num_points; ++i) {
                m_points.push_back(Vec3(random.get_uniform()-0.5,
                                        random.get_uniform()-0.5,
                                        random.get_uniform()-0.5));
            }
*/

            for (int i = 0; i < 117; ++i) {
                m_points.push_back(Vec3(random.get_uint<unsigned>(31)/31.0-0.5,
                                        random.get_uint<unsigned>(31)/31.0-0.5,
                                        random.get_uint<unsigned>(31)/31.0-0.5));
            }
//            m_points.push_back(Vec3(0.001, 0.01, -0.5));
        }


/*
        // A cube
        {
            m_points.push_back(Vec3(-1,-1,-1));
            m_points.push_back(Vec3(+1,-1,-1));
            m_points.push_back(Vec3(-1,+1,-1));
            m_points.push_back(Vec3(+1,+1,-1));
            m_points.push_back(Vec3(-1,-1,+1));
            m_points.push_back(Vec3(+1,-1,+1));
            m_points.push_back(Vec3(-1,+1,+1));
            m_points.push_back(Vec3(+1,+1,+1));
        }
*/

/*
        // A tetrahedron
        {
            m_points.push_back(Vec3(-1,+0,+1));
            m_points.push_back(Vec3(+1,+0,+1));
            m_points.push_back(Vec3(+0,-1,-1));
            m_points.push_back(Vec3(+0,+1,-1));
        }
*/

/*
        // Color gamut from image
        {
            Image::ConstRef image = Image::load("alley_baggett.png");
            ImageReader reader(image);
            int width = reader.get_width(), height = reader.get_height();
            m_points.resize(height * std::size_t(width));
            Array<double> buf(width * 3);
            for (int i = 0; i < height; ++i) {
                reader.set_pos(0,i).get_block_rgb(buf.get(), width, 1);
                std::size_t offset = i * width;
                for (int j = 0; j < width; ++j) {
                    double *const b = buf.get() + j*3;
                    m_points[offset + j].set(b[0], b[1], b[2]);
                }
            }
            sort(m_points.begin(), m_points.end());
            auto i = unique(points.begin(), points.end());
            m_points.erase(i, points.end());
        }
*/
    }

    void init_hull()
    {
        m_vertices.clear();
        m_trifans.clear();
        m_trifan_sets.clear();
        m_error = false;
        TrifanHandler trifan_handler{&m_vertices, &m_trifans, &m_trifan_sets};
        conv_hull::compute(m_points, trifan_handler, m_max_depth);

        std::size_t num_faces = std::accumulate(m_trifans.begin(), m_trifans.end(), 0);
        m_faces.resize(num_faces);

        std::size_t num_points = m_points.size();

        {
            std::size_t num_colors;
            std::vector<std::size_t> face_colors(num_faces);
            {
                std::list<std::size_t> colors;
                std::vector<std::vector<std::size_t>> used_colors(num_points);

                std::size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
                static_cast<void>(tri_idx);
                for (std::size_t n: m_trifan_sets) {
                    std::size_t vtx_1 = m_vertices[vtx_idx++];
                    std::size_t vtx_2 = m_vertices[vtx_idx++];
                    for (std::size_t i = 0; i < n; ++i) {
                        std::size_t m = m_trifans[fan_idx++];
                        for (;;) {
                            Face& face = m_faces[tri_idx];
                            std::size_t vtx_3 = m_vertices[vtx_idx++];
                            const Vec3& p_1 = m_points[vtx_1];
                            const Vec3& p_2 = m_points[vtx_2];
                            const Vec3& p_3 = m_points[vtx_3];
                            Vec3 v_1 = p_2 - p_1;
                            Vec3 v_2 = p_3 - p_1;
                            face.normal = unit(v_1 * v_2);
                            face.center = (p_1 + p_2 + p_3) / 3.0;
                            std::vector<std::size_t>& u_1 = used_colors[vtx_1];
                            std::vector<std::size_t>& u_2 = used_colors[vtx_2];
                            std::vector<std::size_t>& u_3 = used_colors[vtx_3];
                            std::set<std::size_t> u;
                            u.insert(u_1.begin(), u_1.end());
                            u.insert(u_2.begin(), u_2.end());
                            u.insert(u_3.begin(), u_3.end());
                            std::size_t color;
                            auto j = colors.begin();
                            for (;;) {
                                if (j == colors.end()) {
                                    color = colors.size();
                                    break;
                                }
                                color = *j;
                                if (u.find(color) == u.end()) {
                                    colors.erase(j);
                                    break;
                                }
                                ++j;
                            }
                            colors.push_back(color);
                            u_1.push_back(color);
                            u_2.push_back(color);
                            u_3.push_back(color);
                            face_colors[tri_idx] = color;
                            ++tri_idx;
                            if (--m == 0) {
                                vtx_1 = vtx_3;
                                break;
                            }
                            else {
                                vtx_2 = vtx_3;
                            }
                        }
                    }
                }

                num_colors = colors.size();
            }

            std::vector<std::size_t> permutation(num_colors);
            generate(permutation.begin(), permutation.end(), make_inc_generator<std::size_t>());
            random_shuffle(permutation.begin(), permutation.end());

            std::vector<Vec3F> colors(num_colors);
            for (std::size_t i = 0; i < num_colors; ++i)
                colors[i] = color::cvt_HSV_to_RGB(Vec3F(double(permutation[i])/num_colors, 0.3, 0.5));

            for (std::size_t i = 0; i < m_faces.size(); ++i)
                m_faces[i].color = colors[face_colors[i]];
        }

/*
        std::cerr << "Status:\n";
        for (std::size_t i = 0; i < m_faces.size(); ++i)
            std::cerr << "Face "<<i<<": normal = "<<m_faces[i].normal<<", color = "<<m_faces[i].color << "\n";
*/
    }

    void init()
    {
        init_points();
        init_hull();
    }

private:
    // Text rendering
    archon::render::TextFormatter m_label_formatter;
    TextLayout m_label_layout;

    // Convex hull input
    std::vector<Vec3> m_points;

    // Convex hull output
    std::vector<std::size_t> m_vertices;
    std::vector<std::size_t> m_trifans; // There is one triangle fan per entry in this vector. Each entry states how many triangles are in the fan.
    std::vector<std::size_t> m_trifan_sets; // There is one set of triangle fans per entry in this vector. Each entry states the number of fans in the set.

/*
      The triangles are generated as follows:

      For each entry in trifan_sets:
        Consume the next two vertices from 'vertices' and use them as the first two vertices of the first triangle fan in this set in the order they are extracted.
        For each triangle fan in the set:
          Consume as many vertices from 'vertices' as there are triangles in this fan.
          Use the last two vertices of this fan as the first two vertices of the next fan in reverse order.
*/

    // Render friendly representation of convex hull
    std::vector<Face> m_faces;

    bool m_points_display_on = true, m_point_labels_display_on = false;
    bool m_colorize_on = true, m_normals_display_on = false;

    double m_point_winding = 150;

    std::size_t m_max_depth = 1;

    bool m_error;
    std::size_t m_error_1, m_error_2, m_error_3;

    void render() override
    {
        std::size_t num_points = m_points.size();

        if (m_points_display_on) {
            glDisable(GL_LIGHTING);
            glColor3f(1,1,1);
            glBegin(GL_POINTS);
            for (std::size_t i = 0; i < num_points; ++i) {
                if (m_vertices.size() >= 1 && i == m_vertices[0]) {
                    glColor3f(1.0, 0.3, 0.3);
                }
                else if (m_vertices.size() >= 2 && i == m_vertices[1]) {
                    glColor3f(0.3, 1.0, 0.3);
                }
                else if (m_vertices.size() >= 3 && i == m_vertices[2]) {
                    glColor3f(0.3, 0.3, 1.0);
                }
                else {
                    glColor3f(1.0, 1.0, 1.0);
                }
                const Vec3& p = m_points[i];
                glVertex3d(p[0], p[1], p[2]);
            }
            glEnd();
        }

        if (m_error) {
            glDisable(GL_LIGHTING);
            glBegin(GL_TRIANGLES);
            const Vec3& p_1 = m_points[m_error_1];
            const Vec3& p_2 = m_points[m_error_2];
            const Vec3& p_3 = m_points[m_error_3];
            glColor3f(1,0,0);
            glVertex3d(p_1[0], p_1[1], p_1[2]);
            glVertex3d(p_2[0], p_2[1], p_2[2]);
            glVertex3d(p_3[0], p_3[1], p_3[2]);
            glEnd();
        }

        glEnable(GL_LIGHTING);

        glColor3f(0.5, 0.5, 0.5);
        bool render_as_individual_triangles = true; // Improved shading accuracy
        if (render_as_individual_triangles) {
            glShadeModel(GL_SMOOTH);
            std::size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
            for (std::size_t n: m_trifan_sets) {
                std::size_t vtx_1 = m_vertices[vtx_idx++];
                std::size_t vtx_2 = m_vertices[vtx_idx++];
                for (std::size_t i = 0; i < n; ++i) {
                    std::size_t m = m_trifans[fan_idx++];
                    for (;;) {
                        const Face& face = m_faces[tri_idx++];
                        std::size_t vtx_3 = m_vertices[vtx_idx++];
                        if (m_colorize_on) {
                            const Vec3F& color = face.color;
                            glColor3f(color[0], color[1], color[2]);
                        }
                        const Vec3& normal = face.normal;
                        glNormal3d(normal[0], normal[1], normal[2]);
                        const Vec3& p_1 = m_points[vtx_1];
                        const Vec3& p_2 = m_points[vtx_2];
                        const Vec3& p_3 = m_points[vtx_3];
                        glBegin(GL_TRIANGLES);
                        glVertex3d(p_1[0], p_1[1], p_1[2]);
                        glVertex3d(p_2[0], p_2[1], p_2[2]);
                        glVertex3d(p_3[0], p_3[1], p_3[2]);
                        glEnd();
                        if (--m == 0) {
                            vtx_1 = vtx_3;
                            break;
                        }
                        else {
                            vtx_2 = vtx_3;
                        }
                    }
                }
            }
        }
        else {
            glShadeModel(GL_FLAT);
            std::size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
            for (std::size_t n: m_trifan_sets) {
                std::size_t vtx_1 = m_vertices[vtx_idx++];
                std::size_t vtx_2 = m_vertices[vtx_idx++];
                for (std::size_t i = 0; i < n; ++i) {
                    glBegin(GL_TRIANGLE_FAN);
                    {
                        const Vec3& p_1 = m_points[vtx_1];
                        const Vec3& p_2 = m_points[vtx_2];
                        glVertex3d(p_1[0], p_1[1], p_1[2]);
                        glVertex3d(p_2[0], p_2[1], p_2[2]);
                    }
                    std::size_t m = m_trifans[fan_idx++];
                    for (;;) {
                        std::size_t vtx_3 = m_vertices[vtx_idx++];
                        const Face& face = m_faces[tri_idx++];
                        if (m_colorize_on) {
                            const Vec3F& color = face.color;
                            glColor3f(color[0], color[1], color[2]);
                        }
                        const Vec3& normal = face.normal;
                        glNormal3d(normal[0], normal[1], normal[2]);
                        const Vec3& p_3 = m_points[vtx_3];
                        glVertex3d(p_3[0], p_3[1], p_3[2]);
                        if (--m == 0) {
                            vtx_1 = vtx_3;
                            break;
                        }
                        else {
                            vtx_2 = vtx_3;
                        }
                    }
                    glEnd();
                }
            }
        }

        if (m_normals_display_on) {
            glDisable(GL_LIGHTING);
            glColor3f(1, 0, 0);
            glBegin(GL_LINES);
            std::size_t num_faces = m_faces.size();
            for (std::size_t i = 0; i < num_faces; ++i) {
                const Face& face = m_faces[i];
                const Vec3& p0 = face.center;
                Vec3 p1 = p0 + face.normal;
                glVertex3d(p0[0], p0[1], p0[2]);
                glVertex3d(p1[0], p1[1], p1[2]);
            }
            glEnd();
        }

        if (m_point_labels_display_on) {
            std::wstring buffer;
            const std::locale& locale = get_locale();
            GLint params[2];
            glGetIntegerv(GL_POLYGON_MODE, params);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_LIGHTING);
            glColor3f(1,1,1);
            for (std::size_t i = 0; i < num_points; ++i) {
                const Vec3& p = m_points[i];
                glPushMatrix();
                glTranslatef(p[0], p[1], p[2]);
                double angle = billboard::rotate();
                glScalef(0.05, 0.05, 0.05);
                format_int(i, buffer, locale);
                m_label_layout.set(m_label_formatter, buffer);
                Vec2 size{m_label_layout.get_width(), m_label_layout.get_height()};
                double excl_radius = 0.75;
                double corner_radius = 0.25;
                Vec2 q = calc_point_label_pos(excl_radius, angle, size, corner_radius);
                glTranslatef(q[0], q[1], 0);
                m_label_layout.render();
                glPopMatrix();
            }
            glPolygonMode(GL_FRONT, params[0]);
            glPolygonMode(GL_BACK, params[1]);
        }
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "render/test/");

    Application::Config app_cfg;
    CommandlineOptions opts;
    opts.add_help("Test application for the rendering application foundation");
    opts.check_num_args();
    opts.add_group(app_cfg);
    opts.add_param("n", "num-points", opt_num_points, "Set the number of random points to use");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    ConvHullApp(app_cfg).run();
}
