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

#include <cmath>

#include <archon/core/time.hpp>
#include <archon/core/random.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/functions.hpp>
#include <archon/thread/thread.hpp>
#include <archon/util/stream.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Thread;

namespace
{
  struct SlowStream: InputStream
  {
    size_t read(char *b, size_t n)
    {
      if (first) {
        time = Time::now();
        first = false;
      }

      // Attempt to have at least 'max_chunk_size' bytes in buffer
      size_t left = buffer_end - buffer_begin;
      if (left < max_chunk_size && !eoi) {
        if(left) copy_backward(buffer_begin, buffer_end, buffer.get()+left);
        size_t m = in.read(buffer.get()+left, max_chunk_size);
        if(!m) eoi = true;
        left += m;
        buffer_begin = buffer.get();
        buffer_end   = buffer_begin + left;
      }

      // Randomly choose a chunk size
      n = min3<size_t>(1+chunk_size_generator->get(), n, left);

      if (b) copy(buffer_begin, buffer_begin+n, b);
      buffer_begin += n;

      // Randomly choose a wait time
      Time wait;
      wait.set_as_millis(n * wait_time_generator->get());

      time += wait;
      Thread::sleep_until(time);
      time = Time::now();
      return n;
    }

    size_t discard(size_t n)
    {
      return read(0, n);
    }

    SlowStream(InputStream &in, double mean_transfer_rate, double mean_chunk_size):
      in(in), max_chunk_size(min<size_t>(2048, ceil(mean_chunk_size +
                                                    4.5 * sqrt(mean_chunk_size-1)))),
      wait_time_generator(Random::get_poisson_distrib(1000/mean_transfer_rate).release()),
      chunk_size_generator(Random::get_poisson_distrib(mean_chunk_size-1).release()),
      buffer(2*max_chunk_size),
      buffer_begin(buffer.get()), buffer_end(buffer_begin),
      eoi(false), first(true) {}

    InputStream &in;
    size_t const max_chunk_size;
    // Wait time per byte in milli seconds
    UniquePtr<Random::Distribution> wait_time_generator;
    // Chunk size minus 1
    UniquePtr<Random::Distribution> chunk_size_generator;
    MemoryBuffer buffer;
    char *buffer_begin, *buffer_end;
    bool eoi, first;
    Time time;
  };
}


namespace archon
{
  namespace Util
  {
    UniquePtr<InputStream> make_slow_stream(InputStream &in,
                                            double mean_transfer_rate, double mean_chunk_size)
    {
      UniquePtr<InputStream> s(new SlowStream(in, mean_transfer_rate, mean_chunk_size));
      return s;
    }
  }
}
