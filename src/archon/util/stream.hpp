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

#ifndef ARCHON_UTIL_STREAM_HPP
#define ARCHON_UTIL_STREAM_HPP

#include <string>
#include <algorithm>

#include <archon/core/stream.hpp>


namespace archon
{
  namespace Util
  {
    /**
     * Create a new input stream that reads from the specified string.
     *
     * \param s The string that must act as a data source.
     *
     * \return The new input stream.
     */
    template<class Ch> Core::SharedPtr<Core::BasicInputStream<Ch> >
    make_string_input_stream(typename std::basic_string<Ch> const &s);


    /**
     * This stream implementation allows you to limit the transfer
     * rate. This is especially usefull for testing the behaviour of
     * code when data arrives slowly and/or in small chunks.
     *
     * \param mean_transfer_rate The average transfer rate in 'bytes
     * per second'.
     *
     * \param mean_chunk_size The average chunk size that the read
     * method shall deliver if given a sufficiently large buffer. A
     * sufficiently large buffer is <tt>ceil(mean_chunk_size + 4.5 *
     * sqrt(mean_chunk_size)))</tt> due to the fact that the actual
     * chunk size will follow a Poisson distribution.
     */
    Core::UniquePtr<Core::InputStream>
    make_slow_stream(Core::InputStream &in,
                     double mean_transfer_rate, double mean_chunk_size);


    /**
     * Provides an input stream that can be repeatedly rewound to the
     * beginning. This is usefull in situations such as when you must
     * detect the MIME-type of a stream, and you must do this by
     * asking various distinct sub-systems to test for specific types.
     *
     * The Image library uses this class to determine the image format
     * of input streams.
     *
     * It is legal to rewind even after having seen the end-of-input.
     *
     * \sa Image::ImageIO
     */
    template<class Ch> struct BasicRewindableStream: Core::BasicInputStream<Ch>
    {
      BasicRewindableStream(Core::BasicInputStream<Ch> &in):
        in(in), buffer_start(0), released(false), eoi(false) {}

      std::size_t read(Ch *b, std::size_t n);

      /**
       * Rewind to the start of the stream.
       */
      void rewind();

      /**
       * Give up the rewind capability from this point and on. This
       * should be done before reading the entire contents of the
       * stream because the rewind capability requires every bit of
       * retreived data to be stored in a buffer. After calling this
       * method no new data will be buffered and when the current
       * rewind buffer gets empty it will be deallocated.
       */
      void release();

    private:
      Core::BasicInputStream<Ch> &in;
      std::basic_string<Ch> buffer;
      std::size_t buffer_start;
      bool released, eoi;
    };

    typedef BasicRewindableStream<char>     RewindableStream;
    typedef BasicRewindableStream<wchar_t>  WideRewindableStream;






    // Template implementations

    template<class Ch> struct BasicStringInputStream: Core::BasicInputStream<Ch>
    {
      std::size_t read(Ch *b, std::size_t n)
      {
        std::size_t m = std::min(n, s.size()-pos);
        if (m) {
          s.copy(b, m, pos);
          pos += m;
        }
        return m;
      }

      BasicStringInputStream(std::basic_string<Ch> s): s(s), pos(0) {}

    private:
      std::basic_string<Ch> s;
      std::size_t pos;
    };

    template<class Ch> Core::SharedPtr<Core::BasicInputStream<Ch> >
    make_string_input_stream(std::basic_string<Ch> const &s)
    {
      return Core::SharedPtr<Core::BasicInputStream<Ch> >(new BasicStringInputStream<Ch>(s));
    }



    template<class Ch> std::size_t BasicRewindableStream<Ch>::read(Ch *b, std::size_t n)
    {
      if (buffer_start == buffer.size()) {
        std::size_t m = eoi ? 0 : this->in.read(b,n);
        if (!m) eoi = true;
        if (!released) {
          buffer.append(b, m);
          buffer_start += m;
        }
        return m;
      }

      std::size_t left = buffer.size() - buffer_start;
      if (n < left) {
        buffer.copy(b, n, buffer_start);
        buffer_start += n;
        return n;
      }

      buffer.copy(b, left, buffer_start);
      if (released) {
        buffer.erase();
        buffer_start = 0;
      }
      else buffer_start += left;
      return left;
    }

    template<class Ch> void BasicRewindableStream<Ch>::rewind()
    {
      buffer_start = 0;
    }

    template<class Ch> void BasicRewindableStream<Ch>::release()
    {
      released = true;
    }
  }
}

#endif // ARCHON_UTIL_STREAM_HPP
