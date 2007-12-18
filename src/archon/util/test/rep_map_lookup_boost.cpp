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

#include <map>
#include <iostream>

#include <archon/core/time.hpp>
#include <archon/util/rep_map_lookup_boost.hpp>
#include <archon/util/ticker.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;

namespace
{
  int const n = 10000;
  long const m = 1000000000L/n;

  unsigned test(int seq[], unsigned f(int), double &time, ProgressTicker *ticker)
  {
    unsigned v = 0;
    Time t = Time::now();
    for(long j=0; j<m; ++j)
    {
      for(int i=0; i<n; ++i) v += f(seq[i]);
      ticker->tick();
    }
    time = (Time::now()-t).get_as_seconds_float();
    return v;
  }

  typedef map<int, unsigned> Map;
  Map the_map;
  RepMapLookupBooster<Map> boost(&the_map);

  unsigned f1(int i) { return i; }
  unsigned f2(int i) { return the_map[i]; }
  unsigned f3(int i) { return boost[i]; }
}


int main()
{
  int seq[n];
  for(int i=0; i<n; ++i) seq[i] = i%2==0 ? 1 : 2;
  for(int i=0; i<n; ++i) the_map[i] = i;

  double t1, t2, t3;
  unsigned v1, v2, v3;

  {
    ProgressBar progress;
    ProgressTicker ticker(&progress, 3*m);
    v1 = test(seq, f1, t1, &ticker);
    v2 = test(seq, f2, t2, &ticker);
    v3 = test(seq, f3, t3, &ticker);
  }

  cout << "v1 = " << v1 << "  time = " << t1 << endl;
  cout << "v2 = " << v2 << "  time = " << t2 << endl;
  cout << "v3 = " << v3 << "  time = " << t3 << endl;

  cout << (t2-t1) / (t3-t1) << endl;

  return 0;
}
