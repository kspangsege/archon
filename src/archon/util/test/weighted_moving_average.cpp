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

#include <archon/core/assert.hpp>
#include <archon/util/statistics.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Util;


int main() throw()
{
  WeightedMovingAverage<double, 3> avg;

  cerr << avg.get() << endl;
  avg.add(1);
  cerr << avg.get() << endl;
  avg.add(1);
  cerr << avg.get() << endl;
  avg.add(1);
  cerr << avg.get() << endl;

  return 0;
}
