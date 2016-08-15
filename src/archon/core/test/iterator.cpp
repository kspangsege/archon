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
 * Testing the utility iterators.
 */

#include <iostream>
#include <iomanip>
#include <algorithm>

#include <archon/core/iterator.hpp>


using namespace std;
using namespace archon::Core;

int main() throw()
{
  int seq[60];
  int buffer[100];

  for(int i=0; i<60; ++i) seq[i] = i;
  fill(buffer, buffer+100, 0);

  PeriodIter<int *> grid(buffer, 3, 2);
  copy(seq, seq+60, grid);

  for(int i=0; i<10; ++i)
  {
    for(int j=0; j<10; ++j) cout << setw(4) << buffer[10*i+j];
    cout << endl;
  }

  return 0;
}
