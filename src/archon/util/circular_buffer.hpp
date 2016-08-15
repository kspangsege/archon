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

#ifndef ARCHON_UTIL_CICULAR_BUFFER_HPP
#define ARCHON_UTIL_CICULAR_BUFFER_HPP

#include <cstddef>
#include <limits>
#include <algorithm>

#include <archon/core/memory.hpp>


namespace archon
{
  namespace util
  {
    template<class T> struct CircularBuffer
    {
      CircularBuffer(size_t max_size):
        buffer_size(max_size), buffer(buffer_size), buffer_used_size(0) {}

      bool full() const { return buffer_used_size == buffer_size; }

      bool empty() const { return buffer_used_size == 0; }

      size_t size() const { return buffer_used_size; }

      bool get_span(T const *&begin, size_t &size, size_t offset = 0) const
      {
        if(buffer_used_size <= offset) return false;
        size_t b;
        empty_pre(b, size, offset);
        begin = buffer.get() + b;
        return true;
      }

      bool get_span(T *&begin, size_t &size, size_t offset = 0)
      {
        if(buffer_used_size <= offset) return false;
        size_t b;
        empty_pre(b, size, offset);
        begin = buffer.get() + b;
        return true;
      }

      template<class Iter> Iter copy_to(Iter target, size_t offset, size_t n) const
      {
        T const *p;
        size_t m, i = offset;
        while(get_span(p,m,i))
        {
          if(n <= m) return std::copy(p, p+n, target);
          target = std::copy(p, p+m, target);
          n -= m;
          i += m;
        }
        return target;
      }

      void discard(size_t n)
      {
        if(buffer_used_size <= n) buffer_used_size = 0;
        else
        {
          buffer_used_begin += n;
          buffer_used_size -= n;
          if(buffer_size <= buffer_used_begin) buffer_used_begin -= buffer_size;
        }
      }

      /**
       * Read as much as possible from the specified source and write
       * it to this buffer. The read() method of the specified source
       * will be called at most once.
       *
       * If the read() method of the specified source returns 0, it is
       * interpreted as end-of-input.
       *
       * \return True if, and only if the source was empty.
       */
      template<class Source> bool fill_from_stream(Source &src)
      {
        size_t begin, size;
        fill_pre(begin, size);
        return size == 0 ? false : fill_post(src.read(buffer.get() + begin, size));
      }


      /**
       * Transfer as much data as possible from this buffer to the
       * specified target. The write() method of the specified target
       * will be called at most once.
       *
       * \return True if, and only if the buffer is left empty.
       */
      template<class Target> bool empty_to_stream(Target &tgt)
      {
        size_t begin, size;
        empty_pre(begin, size);
        return empty_post(tgt.write(buffer.get() + begin, size));
      }


    private:
      void fill_pre(size_t &begin, size_t &size)
      {
        if(buffer_used_size == 0)
        {
          buffer_used_begin = 0;
          begin = 0;
          size = buffer_size;
          return;
        }

        begin = buffer_used_begin + buffer_used_size;
        if(begin < buffer_size)
        {
          size = buffer_size - begin;
          return;
        }

        begin -= buffer_size;
        size = buffer_size - buffer_used_size;
      }


      bool fill_post(size_t n)
      {
        if(n == 0) return true;
        buffer_used_size += n;
        return false;
      }

      void empty_pre(size_t &begin, size_t &size, size_t offset = 0) const
      {
        begin = buffer_used_begin + offset;
        size = buffer_used_size - offset;
        if(buffer_size <= begin) begin -= buffer_size;
        else
        {
          size_t const end = buffer_used_begin + buffer_used_size;
          if(buffer_size < end) size -= end - buffer_size;
        }
      }

      bool empty_post(size_t n)
      {
        buffer_used_begin += n;
        buffer_used_size  -= n;
        return buffer_used_size == 0;
      }

      size_t const buffer_size;
      core::Array<T> const buffer;
      size_t buffer_used_begin;
      size_t buffer_used_size;
    };
  }
}

#endif // ARCHON_UTIL_CIRCULAR_BUFFER_HPP
