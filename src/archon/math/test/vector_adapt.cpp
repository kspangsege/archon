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
 *
 * Testing the vector adaptor.
 */

#include <iostream>

#include <archon/math/vector_adapt.hpp>


using namespace std;
using namespace archon::Math;


int main() throw()
{
  float v1[3] = {  1,  2,  3 };
  float v2[3] = { 10, 20, 30 };

  float v3[3];

  vec3_adapt(v3) = vec3_adapt(v2);

  cout << vec3_adapt(v1) + vec3_adapt(v3) << endl;



  Vec3F w1(2.0f), w2(3.0f);

  w1 = 2.0f * w2;

  return 0;
}
