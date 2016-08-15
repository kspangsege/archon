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

#include <iostream>

#include <archon/core/random.hpp>
#include <archon/render/billboard.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Math;
using namespace archon::Render;

int main() throw()
{
  set_terminate(&Cxx::terminate_handler);

  double values[] = {
    -1, +0, +0,
    +0, -1, +0,
    +0, +0, +1
  };

  Mat3 subframe_basis(values);
  Vec3 subframe_origin(0, 0, -1);
  Vec3 const rot_axis(0);
  Rotation3 rot;

  Random ran;
  Vec3 x(ran.get_uniform(), ran.get_uniform(), ran.get_uniform());
  x.unit();
  Vec3 y(ran.get_uniform(), ran.get_uniform(), ran.get_uniform());
  y.unit();
  Vec3 z = x*y;
  z.unit();
  y = z*x;

  subframe_basis.col(0) = x;
  subframe_basis.col(1) = y;
  subframe_basis.col(2) = z;
  subframe_origin.set(ran.get_uniform(), ran.get_uniform(), ran.get_uniform());

  Billboard::calculate_rotation(subframe_basis, subframe_origin, rot_axis, rot);

  cout << rot.axis << ":" << rot.angle << endl;

  return 0;
}
