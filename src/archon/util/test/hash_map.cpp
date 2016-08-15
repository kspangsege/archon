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
 * Testing the hash map.
 */

#include <stdexcept>
#include <cmath>
#include <limits>
#include <map>
//#include <unordered_map>
#include <iostream>

#include <archon/core/text_hist.hpp>
#include <archon/core/random.hpp>
#include <archon/util/hash_map.hpp>
#include <archon/util/statistics.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::core;
using namespace archon::Util;


int main() throw()
{
  Random ran;

  typedef int Key;
  typedef double Val;
  typedef map<Key, Val> StdMap;
  typedef HashMap<Key, Val> HashMap;
//  typedef unordered_map<Key, Val> HashMap;


  {
    StdMap  std_map;
    HashMap hash_map;

    for(size_t i=0; i<0x1000000; ++i)
    {
      Key key = int(ldexp(ran.get_uniform(), 10));
      Val val = ldexp(ran.get_uniform(), 15);
      std_map[key] = val;
      hash_map[key] = val;
    }
cerr << hash_map.size() << endl;

    TEST_MSG(hash_map.size() == std_map.size(), "Size mismatch");

    {
      StdMap::const_iterator const end = std_map.end();
      for(StdMap::const_iterator i=std_map.begin(); i!=end; ++i)
        TEST_MSG(hash_map[i->first] == i->second, "Value mismatch");
    }

    {
      StdMap  m;
      size_t n = 0;
      HashMap::const_iterator const end = hash_map.end();
      for(HashMap::const_iterator i=hash_map.begin(); i!=end; ++i) {
        TEST_MSG(std_map[i->first] == i->second, "Reverse value mismatch");
        ++n;
        m[i->first] = i->second;
      }
      TEST_MSG(n == std_map.size(), "Iteration count mismatch");
      TEST_MSG(m == std_map, "Map mismatch");
    }

    while (size_t const n = std_map.size()) {
      vector<Key> v;
      {
        HashMap::const_iterator const end = hash_map.end();
        for(HashMap::const_iterator i=hash_map.begin(); i!=end; ++i) {
          if (ran.get_frac(1,3)) v.push_back(i->first);
        }
      }
cerr << v.size() << endl;
      for (vector<Key>::iterator i=v.begin(); i!=v.end(); ++i) {
        std_map.erase(*i);
        hash_map.erase(*i);
      }
      TEST_MSG(hash_map.size() == n - v.size(), "Wrong size after erase");
      {
        StdMap::const_iterator const end = std_map.end();
        for(StdMap::const_iterator i=std_map.begin(); i!=end; ++i)
          TEST_MSG(hash_map[i->first] == i->second, "Value mismatch after erase");
      }
      {
        StdMap  m;
        size_t n = 0;
        HashMap::const_iterator const end = hash_map.end();
        for(HashMap::const_iterator i=hash_map.begin(); i!=end; ++i) {
          TEST_MSG(std_map[i->first] == i->second, "Reverse value mismatch after erase");
          ++n;
          m[i->first] = i->second;
        }
        TEST_MSG(n == std_map.size(), "Iteration count mismatch after erase");
        TEST_MSG(m == std_map, "Map mismatch after erase");
      }
    }

    cerr << "OK" << endl;
  }


  // Measurement of hash function quality
/*
  {
    double acc_num_buckets = 0;
    double max_bucket_size = 0;
    Variance<double> bucket_size_variance;

    size_t const num_keys = 0xC000;
    HashMap hash_map;
    size_t a = ran.get_uniform() * 0x10000;
    for(size_t i=0; i<num_keys; ++i)
    {
      int key = (a^i^(a*i)) % 19157; // 39139;
      hash_map[key] = 1;
    }

    size_t const n = hash_map.bucket_count();
    acc_num_buckets += n;
    for(size_t i=0; i<n; ++i)
    {
      size_t const m = hash_map.bucket_size(i);
      if(max_bucket_size < m) max_bucket_size = m;
      bucket_size_variance.add(m);
    }

    cerr << "number of entries     = " << hash_map.size() << endl;
    cerr << "number of buckets     = " << acc_num_buckets << endl;
    cerr << "maximum bucket size   = " << max_bucket_size << endl;
    cerr << "average bucket size   = " << bucket_size_variance.get_arith_mean() << endl;
    cerr << "bucket size variance  = " << bucket_size_variance.get_variance() << endl;
    cerr << "hash function quality = " << bucket_size_variance.get_arith_mean()/bucket_size_variance.get_variance() << "  (1 is good, 0 is very bad)" << endl;

    Histogram hist(0, max_bucket_size+1);
    for(size_t i=0; i<n; ++i) hist.add(hash_map.bucket_size(i));
    hist.print(cerr);
  }
*/


  // Measurement of speed
/*
  {
    double acc_num_buckets = 0;
    double max_bucket_size = 0;
    Variance<double> bucket_size_variance;

    size_t const num_maps = 0x1000, num_keys = 0xC00;
    for(size_t k=0; k<num_maps; ++k)
    {
      HashMap hash_map;
      size_t a = ran.get_uniform() * 0x10000;
      for(size_t i=0; i<num_keys; ++i)
      {
        int    key = a+i;
        hash_map[key] = 1;
      }
    }
  }
*/

  return 0;
}
