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

#ifndef ARCHON_RENDER_OBJECT_HPP
#define ARCHON_RENDER_OBJECT_HPP

#include <cstddef>
#include <vector>
#include <ios>

#include <archon/math/vector.hpp>


namespace Archon
{
  namespace Render
  {
    struct Object
    {
      struct Triangle
      {
        std::size_t a,b,c;
        Triangle(std::size_t a, std::size_t b, std::size_t c): a(a), b(b), c(c) {}
      };

      void reserve(std::size_t num_vertices, std::size_t num_triangles)
      {
        vertices.reserve(num_vertices);
        triangles.reserve(num_triangles);
      }

      std::size_t add_vertex(Math::Vec3F v)
      {
        size_t i = vertices.size();
        vertices.push_back(v);
        return i;
      }

      std::size_t add_vertex(float x, float y, float z)
      {
        return add_vertex(Math::Vec3F(x,y,z));
      }

      std::size_t add_triangle(Triangle t)
      {
        size_t i = triangles.size();
        triangles.push_back(t);
        return i;
      }

      std::size_t add_triangle(std::size_t a, std::size_t b, std::size_t c)
      {
        return add_triangle(Triangle(a,b,c));
      }


      void load(std::istream &in);

      void save(std::ostream &out) const;

      void render() const;


    private:
      typedef std::vector<Math::Vec3F>::iterator       vertex_iterator;
      typedef std::vector<Math::Vec3F>::const_iterator const_vertex_iterator;
      typedef std::vector<Triangle>::iterator          triangle_iterator;
      typedef std::vector<Triangle>::const_iterator    const_triangle_iterator;


    public:
      vertex_iterator         vertices_begin()        { return vertices.begin();  }
      const_vertex_iterator   vertices_begin()  const { return vertices.begin();  }
      vertex_iterator         vertices_end()          { return vertices.end();    }
      const_vertex_iterator   vertices_end()    const { return vertices.end();    }
      triangle_iterator       triangles_begin()       { return triangles.begin(); }
      const_triangle_iterator triangles_begin() const { return triangles.begin(); }
      triangle_iterator       triangles_end()         { return triangles.end();   }
      const_triangle_iterator triangles_end()   const { return triangles.end();   }


    private:
      std::vector<Math::Vec3F> vertices;
      std::vector<Triangle>    triangles;
    };
  }
}

#endif // ARCHON_RENDER_OBJECT_HPP
