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
 * Testing the ray/object intersection computations.
 */

#include <stdexcept>
#include <string>
#include <iostream>

#include <archon/math/intersect.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::Core;
using namespace archon::Math;

namespace
{
  void assert_val(double v, double w, string msg)
  {
    double const e = 100*numeric_limits<double>::epsilon();
    TEST_MSG(w-e < v && v < w+e, msg);
  }
}


int main() throw()
{
  double dist;


  TEST_MSG(intersect_sphere<false>(Line3(Vec3(0, 0, 2), Vec3(0, 0, -1)), dist), "sphere hit 1");
  assert_val(dist, 1, "sphere dist 1");

  TEST_MSG(intersect_sphere<false>(Line3(Vec3(0.5, 0, 2), Vec3(0, 0, -1)), dist), "sphere hit 2");
  assert_val(dist, 2-sqrt(3)/2, "sphere dist 2");

  TEST_MSG(intersect_sphere<false>(Line3(Vec3(0, 0.5, 2), Vec3(0, 0, -1)), dist), "sphere hit 3");
  assert_val(dist, 2-sqrt(3)/2, "sphere dist 3");

  TEST_MSG(intersect_sphere<true>(Line3(Vec3(0, 0.5, 2), Vec3(0, 0, -1)), dist), "sphere hit 4");
  assert_val(dist, 2+sqrt(3)/2, "sphere dist 4");



  TEST_MSG(intersect_box<false>(Line3(Vec3(0, 0, 2), Vec3(0, 0, -1)), dist) == 6, "box hit 1");
  assert_val(dist, 1, "box dist 1");

  TEST_MSG(intersect_box<false>(Line3(Vec3(0.5, 0, 2), Vec3(0, 0, -1)), dist) == 6, "box hit 2");
  assert_val(dist, 1, "box dist 2");

  TEST_MSG(intersect_box<true>(Line3(Vec3(0.5, 0, 2), Vec3(0, 0, -1)), dist) == 5, "box hit 3");
  assert_val(dist, 3, "box dist 3");

  TEST_MSG(intersect_box<false>(Line3(Vec3(0.75, 2, 0.25), Vec3(0, -1, 0)), dist) == 4, "box hit 4");
  assert_val(dist, 1, "box dist 4");

  TEST_MSG(intersect_box<false>(Line3(Vec3( 1.1, 0, 2), Vec3(0, 0, -1)), dist) == 0, "box hit 5");
  TEST_MSG(intersect_box<false>(Line3(Vec3(-1.1, 0, 2), Vec3(0, 0, -1)), dist) == 0, "box hit 6");
  TEST_MSG(intersect_box<false>(Line3(Vec3(0,  1.1, 2), Vec3(0, 0, -1)), dist) == 0, "box hit 7");
  TEST_MSG(intersect_box<false>(Line3(Vec3(0, -1.1, 2), Vec3(0, 0, -1)), dist) == 0, "box hit 8");

  TEST_MSG(intersect_box<false>(Line3(Vec3(0,0,2), Vec3(-1.1, 0, -1)), dist) == 0, "box hit 9");
  TEST_MSG(intersect_box<false>(Line3(Vec3(0,0,2), Vec3( 1.1, 0, -1)), dist) == 0, "box hit 10");
  TEST_MSG(intersect_box<false>(Line3(Vec3(0,0,2), Vec3(0, -1.1, -1)), dist) == 0, "box hit 11");
  TEST_MSG(intersect_box<false>(Line3(Vec3(0,0,2), Vec3(0,  1.1, -1)), dist) == 0, "box hit 12");


  cout << "OK" << endl;

  return 0;
}
