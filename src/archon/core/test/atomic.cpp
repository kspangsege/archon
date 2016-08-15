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
 * Testing the \c Atomic type.
 */

#include <pthread.h>

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iostream>

#include <archon/core/atomic.hpp>
#include <archon/core/term.hpp>
#include <archon/core/random.hpp>
#include <archon/core/text.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::Core;


namespace
{
  Atomic atomic = 0;

  Random random;

  void inc_and_dec()
  {
    int prev_n = 0;

    for(int i=0; i<1000; ++i)
    {
      int n = static_cast<int>(random.get_uniform() * 10000) + 1;
      for(int j=0; j<n; ++j) ++atomic;

      if(prev_n) for(int j=0; j<prev_n; ++j) --atomic;

      prev_n = n;
    }

    // Undo the last chunk of decrements
    if(prev_n) for(int j=0; j<prev_n; ++j) --atomic;
  }

  void add_and_sub()
  {
    int prev_n = 0;

    for(int i=0; i<1000; ++i)
    {
      int n = static_cast<int>(random.get_uniform() * 10000) + 1;
      for(int j=0; j<n; ++j) { atomic += 2; atomic += 3; }

      if(prev_n) for(int j=0; j<prev_n; ++j) atomic -= 5;

      prev_n = n;
    }

    // Undo the last chunk of decrements
    if(prev_n) for(int j=0; j<prev_n; ++j) atomic -= 5;
  }

  void test_inc_and_dec()
  {
    inc_and_dec();
    TEST(atomic == 0);
  }

  void test_add_and_sub()
  {
    add_and_sub();
    TEST(atomic == 0);
  }

  void test_dec_and_zero_test()
  {
    int n = 1000;
    atomic += n+1;
    for(int i=0; i<n; ++i) TEST(atomic.dec_and_zero_test() == false);
    TEST(atomic.dec_and_zero_test() == true);
    TEST(atomic == 0);
  }

  void test_inc_if_not_zero()
  {
    int n = 1000;
    for(int i=0; i<n; ++i) TEST(atomic.inc_if_not_zero() == 0);
    ++atomic;
    for(int i=0; i<n; ++i) TEST(atomic.inc_if_not_zero() == i+1);
    atomic -= n+1;
    TEST(atomic == 0);
  }

  void test_fetch_and_add()
  {
    int n = 1000;
    for(int i=0; i<n; ++i) TEST(atomic.fetch_and_add(3) == 3*i);
    for(int i=0; i<n; ++i) TEST(atomic.fetch_and_add(-3) == 3*(n-i));
    TEST(atomic == 0);
  }

  void test_test_and_set()
  {
    int n = 1000;
    int v = 0;
    for(int i=0; i<n; ++i)
    {
      bool want_match = random.get_uniform() < 0.5;
      int t = want_match ? v : static_cast<int>(random.get_uniform() * 10000);
      int w = static_cast<int>(random.get_uniform() * 10000);
      bool has_match = t == v;
      TEST(atomic.test_and_set(t,w) == has_match);
      if(has_match) v = w;
    }

    atomic = 0;
  }

  void *thread_inc_and_dec(void *)
  {
    inc_and_dec();
    return 0;
  }

  void *thread_add_and_sub(void *)
  {
    add_and_sub();
    return 0;
  }


/*
  int scale_int(int v, int from_max, int to_max)
  {
    return v <= 0 ? 0 : from_max <= v ? to_max : double(v) / from_max * (to_max + 1);
  }

  void print_history(vector<int> const &hist)
  {
    // Find max value
    int const max_val = *max_element(hist.begin(), hist.end());

    // Find out the maximum width of printed values
    int num_width = 0;
    {
      int v = max_val;
      while(v) { v /= 10; ++num_width; }
    }

    pair<int, int> term_size(80, 25);
    try { term_size = Term::get_terminal_size(); }
    catch(Term::NoTerminalException &) {}

    int const bar_max_width = max(term_size.first - num_width - 1, 0);

    // Print it
    ostringstream out;
    out.setf(ios_base::right, ios_base::adjustfield);
    int const n = hist.size();
    for(int i=0; i<n; ++i)
    {
      out.fill(' ');
      out.width(num_width);
      out << hist[i];
      out << '|';
      out.fill('#');
      out.width(scale_int(hist[i], max_val, bar_max_width));
      out << "" << endl;
    }

    cout << out.str();
  }
*/


  void *thread_visibility_test_1(void *)
  {
    TEST(atomic == 0);
    atomic = 1;
    for(;;) if(atomic == 2) break;
    atomic = 3;
    for(;;) if(atomic == 4) break;
    return 0;
  }

  void *thread_visibility_test_2(void *)
  {
    for(;;) if(atomic == 1) break;
    atomic = 2;
    for(;;) if(atomic == 3) break;
    atomic = 4;
    return 0;
  }


  Atomic zero_positives = 0;

  void *thread_dec_and_zero_test(void *)
  {
    vector<int> history;
    int n_up = 1000;
    int n = 2 * n_up;
    int c_up = 0, net = 0;
    for(int i=0; i<n; ++i)
    {
      bool up = random.get_uniform() < (net == 0 ? 1 : c_up == n_up ? 0 : 0.5);
      if(up)
      {
        atomic.inc();
        ++c_up;
        ++net;
      }
      else
      {
        if(atomic.dec_and_zero_test()) zero_positives.inc();
        --net;
      }
      history.push_back(net);
    }

    if(atomic.dec_and_zero_test())
    {
      zero_positives.inc();
      // print_history(history);
    }
    return 0;
  }

  int const inc_if_not_zero_threads = 25;
  Atomic inc_if_not_zero_flags[inc_if_not_zero_threads];
  Atomic inc_if_not_zero_started;

  void *thread_inc_if_not_zero(void *)
  {
    bool count = true;
    int r, i = 0;
    for(;;)
    {
      r = atomic.inc_if_not_zero();
      if(r) break;
      if(!count) continue;
      if(++i < 250) continue;
      inc_if_not_zero_started.inc();
      count = false;
    }
    --r;
    TEST_MSG(0 <= r && r < inc_if_not_zero_threads, "Bad orig value "+Text::print(r));
    inc_if_not_zero_flags[r].inc();
    return 0;
  }

  void *thread_inc_if_not_zero_2(void *)
  {
    TEST(atomic == 0);
    for(;;) if(inc_if_not_zero_started == inc_if_not_zero_threads) break;
    atomic.inc();
    return 0;
  }

  void run_threads(void *(*func)(void *), int n, void *(*func2)(void *) = 0)
  {
    int const n_threads = func2 ? n + 1 : n;
    pthread_t *threads = new pthread_t[n_threads];
    for(int i=0; i<n; ++i) pthread_create(threads+i, 0, func, 0);
    if(func2) pthread_create(threads+n, 0, func2, 0);
    for(int i=0; i<n_threads; ++i) pthread_join(threads[i], 0);
    delete[] threads;
  }

  void test_threaded()
  {
    run_threads(&thread_visibility_test_1, 1, &thread_visibility_test_2);
    TEST(atomic == 4);
    atomic = 0;
    TEST(atomic == 0);

    run_threads(&thread_inc_and_dec, 25);
    TEST(atomic == 0);

    run_threads(&thread_add_and_sub, 25);
    TEST(atomic == 0);

    atomic += 25;
    run_threads(&thread_dec_and_zero_test, 25);
    TEST(zero_positives == 1);
    TEST(atomic == 0);

    for(int i=0; i<25; ++i)
    {
      for(int j=0; j<inc_if_not_zero_threads; ++j) inc_if_not_zero_flags[j] = 0;
      inc_if_not_zero_started = 0;
      run_threads(&thread_inc_if_not_zero, inc_if_not_zero_threads, &thread_inc_if_not_zero_2);
      int n = 0;
      for(int j=0; j<inc_if_not_zero_threads; ++j) if(inc_if_not_zero_flags[j]) ++n;
      TEST(n == inc_if_not_zero_threads);
      atomic = 0;
    }
  }
}


int main() throw()
{
  test_inc_and_dec();
  test_add_and_sub();
  test_dec_and_zero_test();
  test_inc_if_not_zero();
  test_fetch_and_add();
  test_test_and_set();
  test_threaded();

  cout << "OK" << endl;

  return 0;
}
