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
///
/// Testing the \c BasicTupleGrid::repeat function.

#include <iostream>
#include <iomanip>
#include <algorithm>

#include <archon/util/tuple_grid.hpp>


using namespace std;
using namespace archon::util;

int main() throw()
{
    int buffer[400];

    for (int i = 0; i < 400; ++i)
        buffer[i] = i;

    BasicTupleGrid<int *>(buffer,
                          1,   // int pitch
                          20). // int stride
        extend(8,  // int width
               4,  // int height
               1,  // int n
               0,  // int left
               4,  // int right
               0,  // int down
               0,  // int up
               0,  // int left2
               0,  // int right2
               0,  // int down2
               0); // int up2

    for (int i = 19; -1 < i; --i) {
        for (int j = 0; j < 20; ++j)
            cout << setw(4) << buffer[20*i+j];
        cout << endl;
    }
}
