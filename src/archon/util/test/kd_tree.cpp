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
 * Testing the kd-tree.
 */

#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/random.hpp>
#include <archon/core/time.hpp>
#include <archon/core/iterator.hpp>
#include <archon/math/vec_ops.hpp>
#include <archon/util/kd_tree.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Math;
using namespace archon::Util;


namespace
{
  template<typename T> void test(int num_points, int num_components, int num_searches)
  {
    cout << "test_dyn: Find "<<num_searches<<" points in cloud of "<<num_points<<" points "
      "each with "<<num_components<<" components" << endl;

    KdTreeSet<T> kd(num_components);

    cout << "Generating random points" << endl;
    size_t m = num_points * size_t(num_components);
    size_t n = num_searches * size_t(num_components);
    Array<T> buffer(m+n);
    Random r;
    for(size_t i=0; i<m+n; ++i) buffer[i] = r.get_uniform();

    T const *const points = buffer.get();
    T const *const needles = points + m;

    cout << "Balancing kd-tree" << endl;
    RowIter<T const *> begin(points, num_components);
    kd.add(begin, begin + num_points);

    Array<T> const first_results(num_searches), second_results(num_searches);

    cout << "Searching... " << flush;
    Time start = Time::now();
    for(int i=0; i<num_searches; ++i)
    {
      T const *const needle = needles + i*size_t(num_components);
      T const *const vec = kd.find_nearest(needle);
      first_results[i] = vec_sq_dist(needle, needle + num_components, vec);
    }
    Time time_first = Time::now() - start;
    cout << time_first.get_as_millis()<<"ms" << endl;


    cout << "Brute force check... " << flush;
    start = Time::now();
    for(int i=0; i<num_searches; ++i)
    {
      T const *const needle = needles + i*size_t(num_components);
      T min = vec_sq_dist(needle, needle + num_components, points);
      for(int j=1; j<num_points; ++j)
      {
        T const d = vec_sq_dist(needle, needle + num_components,
                                points + j*size_t(num_components));
        if(d < min) min = d;
      }
      second_results[i] = min;
    }
    Time time_second = Time::now() - start;
    cout << time_second.get_as_millis()<<"ms" << endl;

    cout << "Speedup: " << (time_second.get_as_micros()/
                            double(time_first.get_as_micros())) << endl;


    int failures = 0;
    for(int i=0; i<num_searches; ++i)
      if(first_results[i] != second_results[i])
      {
        ++failures;
        if(failures <= 10) cout << "Failure "<<failures<<": "<<first_results[i]<<" != "<<second_results[i] << endl;
      }
    if(failures) cout << "FAILURES: "<<failures<<"/"<<num_searches << endl;
    else cout << "SUCCESS!!!" << endl;
  }
}


int main() throw()
{
  test<double>(250000, 5, 5000);
  test<float>(256, 3, 5000000);

  return 0;
}
