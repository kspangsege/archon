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

#ifndef ARCHON_UTIL_CONV_HULL_HPP
#define ARCHON_UTIL_CONV_HULL_HPP

#include <cstddef>
#include <vector>

#include <archon/math/vector.hpp>


namespace archon
{
  namespace util
  {
    namespace ConvHull
    {
      struct TrifanHandler
      {
        virtual void add_vertex(std::size_t point_index) = 0; // The same index may get added multiple times
        virtual void close_trifan() = 0;
        virtual void close_trifan_set() = 0;

        virtual ~TrifanHandler() {}
      };


      void compute(std::vector<Math::Vec3> const &points, TrifanHandler &handler, int max_depth = 0);



      /**
       * Translates triangle fans to triangles.
       */
      struct TriangleHandler: TrifanHandler
      {
        virtual void add_triangle(std::size_t a, std::size_t b, std::size_t c) = 0;

        void add_vertex(std::size_t i);

        void close_trifan();

        void close_trifan_set();

        TriangleHandler(): state(0) {}

        size_t vertex0, vertex1, vertex2;
        int state;
      };
    }
  }
}

#endif // ARCHON_UTIL_CONV_HULL_HPP
