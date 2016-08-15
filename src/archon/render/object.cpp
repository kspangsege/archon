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

#include <GL/gl.h>

#include <archon/render/object.hpp>


using namespace std;
using namespace archon::Math;


namespace archon
{
  namespace Render
  {
    void Object::load(istream &in)
    {
      if(!in) throw runtime_error("Bad stream");
      size_t num_vertices, num_triangles;
      in >> num_vertices >> num_triangles;
      reserve(num_vertices, num_triangles);
      for(size_t i=0; i<num_vertices; ++i)
      {
        float x,y,z;
        in >> x >> y >> z;
        add_vertex(x,y,z);
      }
      for(size_t i=0; i<num_triangles; ++i)
      {
        size_t a,b,c;
        in >> a >> b >> c;
        if(num_vertices <= a ||
           num_vertices <= b ||
           num_vertices <= c) throw runtime_error("Bad index in triangle");
        if(a == b || a == c || b == c) throw runtime_error("Degenerate triangle");
        add_triangle(a,b,c);
      }
    }


    void Object::save(ostream &out) const
    {
      size_t const num_vertices = vertices.size(), num_triangles = triangles.size();
      out << num_vertices << " " << num_triangles << "\n";
      for(size_t i=0; i<num_vertices; ++i)
      {
        Vec3F const &v = vertices[i];
        out << v[0] << " " << v[1] << " " << v[2] << "\n";
      }
      for(size_t i=0; i<num_triangles; ++i)
      {
        Triangle const &t = triangles[i];
        out << t.a << " " << t.b << " " << t.c << "\n";
      }
    }


    void Object::render() const
    {
      glColor3f(0.5, 0.5, 0.5);
      glBegin(GL_TRIANGLES);
      size_t const num_triangles = triangles.size();
      for(size_t i=0; i<num_triangles; ++i)
      {
        Triangle const &tri = triangles[i];
        Vec3F const v1 = vertices[tri.a];
        Vec3F const v2 = vertices[tri.b];
        Vec3F const v3 = vertices[tri.c];
        Vec3F const n = (v2-v1) * (v3-v1);
        glNormal3f(n[0], n[1], n[2]);
        glVertex3f(v1[0], v1[1], v1[2]);
        glVertex3f(v2[0], v2[1], v2[2]);
        glVertex3f(v3[0], v3[1], v3[2]);
      }
      glEnd();
    }
  }
}
