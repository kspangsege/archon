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

#include <CGAL/Homogeneous.h>
#include <CGAL/Gmpzf.h>
#include <CGAL/Polytope_distance_d.h>
#include <CGAL/Polytope_distance_d_traits_3.h>

#include <vector>

using namespace std;

typedef CGAL::Homogeneous<CGAL::Gmpzf> K;
typedef CGAL::Polytope_distance_d_traits_3<K> TraitsPolytope;
typedef CGAL::Polytope_distance_d<TraitsPolytope> Polytope_distance;
typedef K::Point_3 Point_3;

bool palette_clamp(float const *pal, size_t pal_size, float *col)
{
  vector<Point_3> cloud;
  for(size_t i=0; i<pal_size; ++i)
  {
    float const *p = pal + i*3;
    cloud.push_back(Point_3(p[0], p[1], p[2]));
  }

  vector<Point_3> query;
  query.push_back(Point_3(col[0], col[1], col[2]));

  Polytope_distance d = Polytope_distance(cloud.begin(), cloud.end(), query.begin(), query.end());

  if(CGAL::to_double(d.squared_distance()) == 0) return false;
  Point_3 r = d.realizing_point_p();
  col[0] = CGAL::to_double(r[0]);
  col[1] = CGAL::to_double(r[1]);
  col[2] = CGAL::to_double(r[2]);

  return true;
}
