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

#ifndef ARCHON_CORE_STREAM_HPP
#define ARCHON_CORE_STREAM_HPP

#include <cwchar>
#include <algorithm>
#include <stdexcept>
#include <string>

#include <archon/core/shared_ptr.hpp>
#include <archon/core/memory.hpp>


namespace Archon
{
  namespace Core
  {
    struct IOException;
    struct ReadException;
    struct WriteException;



    /**
     * An abstract endpoint of an input stream. That is, an abstract
     * source from which some a priori unknown amount of data can be
     * read.
     *
     * A lot of implementations will exist, which are simply different
     * kinds of wrappers that simply forward the read request to
     * another data source, and maybe also does some processing on the
     * fly. In such cases, implementations are expected to handle the
     * read requests in a 'lean' manner, meaning that they should make
     * no effort to fill the callers read buffer by making multiple
     * requests for data on the wrapped data source. The following
     * 'rules of thumb' are not mandatory, but recommended behaviour:
     *
     * - Avoid making a read request on the wrapped data source, if
     *   the callers buffer can be filled anyway.
     *
     * - Make multiple read requests on the wrapped data source only
     *   if it is required, to deliver at least one byte/character to
     *   the caller.
     *
     *
     * Thread safety: Implementations are not required to be strongly
     * thread-safe. Any public and/or general purpose input stream
     * implementation may be assumed to be weakly thread-safe,
     * however, unless something else is clearly stated in the
     * documentation of that implementation (see \ref ThreadSafety).
     *
     * \note \c <archon/util/stream.hpp> contains a few usefull
     * implementations.
     */
    template<typename C> struct BasicInputStream
    {
      typedef C char_type;

      /**
       * Read at most \c n characters from the stream into the
       * specified buffer.
       *
       * \param b The buffer into which the retrieved characters will
       * be placed.
       *
       * \param n The number of characters to read. The buffer must be
       * large enough to hold at least this number of characters.
       *
       * \return The number of characters that were actually
       * read. This may be anything between zero and <tt>n</tt>. Zero
       * implies end-of-input unless \c n was zero.
       *
       * \note It shall be legal to call this method again after it
       * has indicated end-of-input, but in that case it must return
       * zero, and continue to do so.
       *
       * \throw ReadException If the read operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking read is interrupted.
       */
      virtual std::size_t read(char_type *b, std::size_t n) = 0;

      /**
       * Keep reading until the end of file is reached or \c n
       * characters have been read.
       *
       * \param b The buffer in which retrieved characters will be
       * placed.
       *
       * \param n The number of characters to read. The buffer must be
       * large enough to hold at least this number of characters.
       *
       * \return The number of characters that were actually read. Any
       * value less than \c n means that end-of-input has been
       * reached.
       *
       * \throw ReadException If the read operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking read is interrupted.
       */
      std::size_t read_all(char_type *b, std::size_t n);

      /**
       * Read up to \c max characters from this stream and return them
       * as an STL string. The only thing that can cause the length of
       * the returned string to be less than <tt>max</tt>, is if
       * end-of-input is reached.
       *
       * \param max The maximum number of characters to read. You may
       * set this to zero, in which case there is no maximum, but be
       * carefull, since it may lead to unpredictable memory
       * consumption. You should only do this if the stream size is
       * somehow bounded by your application.
       *
       * \throw ReadException If the read operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking read is interrupted.
       *
       * \note This method is less efficient than the one that takes a
       * buffer as argument, so use with care.
       */
      std::basic_string<C> read_all(std::size_t max);

      /**
       * Read the remainder of the available data in this input
       * stream. Throw away that data, but return the number of bytes
       * read.
       *
       * \throw ReadException If the read operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking read is interrupted.
       */
      std::size_t discard_rest()
      {
        std::size_t const buf_size = 512;
        C buf[buf_size];
        size_t total = 0;
        while (size_t const n = read(buf, buf_size)) total += n;
        return total;
      }

      std::size_t discard_n(std::size_t n)
      {
        std::size_t const buf_size = 512;
        C buf[buf_size];
        size_t total = 0;
        while (size_t const m = 0<n ? read(buf, std::min(buf_size, n)) : 0) {
          total += m;
          n -= m;
        }
        return total;
      }

      virtual ~BasicInputStream() {}
    };




    /**
     * An abstract endpoint of an output stream. That is, an abstract
     * target to which a generally infinite amount of data can be
     * written.
     *
     * Such a stream can, and should, be closed explicitely, such that
     * errors caused by the implied flushing, or the close operation
     * itself, can be detected. If it is not closed explicitely, it
     * will be closed by the destructor, but any errors will then be
     * silently ignored (note that, in general, destructors should
     * never throw exceptions).
     *
     * The effect of writing to a closed stream is unspecified, but
     * implementations are advised to detect this condition and throw
     * an appropriate WriteException. Closing a stream that is already
     * closed shall have no efftect.
     *
     * A lot of implementations will exist, which are simply different
     * kinds of wrappers that simply forward the write request to
     * another data destination, and maybe also does some processing
     * on the fly. In such cases, implementations are expected to
     * handle the write requests in a 'lean' manner, meaning that they
     * should make no effort to flush internal buffers by issuing
     * multiple write requests to the wrapped data destination. The
     * following 'rule of thumb' is not mandatory, but recommended
     * behaviour:
     *
     * - Make only as many write request on the wrapped data
     *   destination as are required to consume all the data in the
     *   callers buffer.
     *
     * This rule presumes that we are not willing to allocate extra
     * buffer space just to be able to consume the callers data.
     *
     *
     * Thread safety: Implementations are not required to be strongly
     * thread-safe. Any public and/or general purpose output stream
     * implementation may be assumed to be weakly thread-safe,
     * however, unless something else is clearly stated in the
     * documentation of that implementation (see \ref ThreadSafety).
     *
     * \note \c <archon/util/stream.hpp> contains a few usefull
     * implementations.
     */
    template<typename C> struct BasicOutputStream
    {
      typedef C char_type;

      /**
       * Write \c n characters from the specified buffer onto the
       * stream.
       *
       * \param b The buffer holding the data to be written.
       *
       * \param n The number of characters from the buffer to write.
       *
       * \throw WriteException If the write operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking write is interrupted.
       */
      virtual void write(char_type const *b, std::size_t n) = 0;

      /**
       * Same as the two argument version of <tt>write</tt>, only the
       * buffer comes in the form of an STL string.
       *
       * \param s The string to be written.
       *
       * \throw WriteException If the write operation fails in any way
       * that prevents it from doing what it is specified to do.
       *
       * \throw InterruptException If a blocking write is interrupted.
       */
      template<class T, class A> void write(std::basic_string<C,T,A> const &s);

      /**
       * Write everything that can be read from the specified input
       * stream onto this output stream.
       *
       * \param in The input stream whose contents is to be written to
       * this output stream.
       *
       * \throw ReadException If the reading from the specified input
       * stream fails.
       *
       * \throw WriteException If the writing to this output stream
       * fails.
       *
       * \throw InterruptException If a blocking read or a blocking
       * write is interrupted.
       */
      void write(BasicInputStream<C> &in);

      /**
       * Flush any buffered data down the stream. This implies that
       * any wrapped stream is also flushed.
       *
       * The flush() method must work in such a way that if the
       * writing agent calls it just before destroying the stream, he
       * will either get notified or the writing will be 100%
       * successfull.
       *
       * A flush must always be attempted by the destructor, but no
       * errors may be reported in this case. Note that it is
       * virtually always a bad idea to throw an exception from a
       * destructor.
       *
       * Some implementations will not be able to honour a flush()
       * without assuming that the end of the stream has been
       * reached. In such cases, the implementation must make that
       * assumption, and should probably report an error if writing is
       * attempted after a flush. Such a constraint should also be
       * clearly documented by the implementation. Crucially, an
       * implementation must never choose to skip a flush completely
       * or partially, because in that case the writing agent will no
       * longer have a reliable way of detecting errors. One examples
       * of such a implementation is a character transcoder where the
       * writing agent uses a multi-byte encoding. Another example is
       * the Block codec available via Util::get_block_codec().
       *
       * \throw WriteException If flushing of any buffered data fails.
       *
       * \throw InterruptException If the flushing blocks and is
       * interrupted.
       */
      virtual void flush() = 0;

      /**
       * Close the output stream, but ignore any write error thay may
       * occur due to the implied flushing of buffered data.
       *
       * Note: Writing agents that do not call flush() before destroying an
       * output stream, will never know for sure whether writing was
       * successful.
       *
       * Implementations that bufefr data in any way, should override
       * the destructor and flush any buffered data down the stream,
       * but this must not cause any exceptions to be thrown.
       *
       * Implementations that do not buffer data, should normally not
       * need to override the destructor.
       *
       * \sa flush()
       */
      virtual ~BasicOutputStream() {}
    };



    typedef BasicInputStream<char>     InputStream;
    typedef BasicOutputStream<char>    OutputStream;
    typedef BasicInputStream<wchar_t>  WideInputStream;
    typedef BasicOutputStream<wchar_t> WideOutputStream;




    /**
     * Create an input stream that reads from STDIN.
     *
     * \param close If true, STDIN will be closed when the last
     * reference to the returned stream is dropped.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<InputStream> make_stdin_stream(bool close = true);

    /**
     * Create an output stream that writes to STDOUT.
     *
     * \param close If true, STDOUT will be closed when the last
     * reference to the returned stream is dropped.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<OutputStream> make_stdout_stream(bool close = true);

    /**
     * Create an output stream that writes to STDERR.
     *
     * \param close If true, STDERR will be closed when the last
     * reference to the returned stream is dropped.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<OutputStream> make_stderr_stream(bool close = true);

    /**
     * Create an input stream that uses a previously opened file as
     * the byte source.
     *
     * \param fildes The file desciptor of the open file to read from.
     *
     * \param close If true, the passed file descriptor will be closed
     * when the last reference to the returned stream is dropped.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<InputStream> make_file_input_stream(int fildes, bool close = true);

    /**
     * Create an input stream that uses a file in the file system as
     * the byte source.
     *
     * The file is closed when the last reference to the returned
     * stream is dropped.
     *
     * \param file_path The file system path of the file to read from.
     *
     * \throw File::NotFoundException When the file could not be
     * found or it was a directory.
     *
     * \throw File::PermissionException When access to the directory
     * is forbidden.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<InputStream> make_file_input_stream(std::string file_path);

    /**
     * Create an output stream that uses a previously opened file as
     * the byte target.
     *
     * \param fildes The file desciptor of the open file to write to.
     *
     * \param close If true, the passed file descriptor will be closed
     * when the last reference to the returned stream is dropped.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<OutputStream> make_file_output_stream(int fildes, bool close = true);

    /**
     * Create an output stream that uses a file in the file system as
     * the byte target.
     *
     * The file is closed when the last reference to the returned
     * stream is dropped.
     *
     * \param file_path The file system path of the file to write to.
     *
     * \throw FileOpenException If the file could not be opened.
     *
     * \note This implementation does not support thread
     * termination, so you should probably _not_ use it with file
     * descriptors on which read operations can block.
     *
     * \todo FIXME: Consider honoring thread interruption/termination
     * by using \c select to wait for readyness.
     */
    UniquePtr<OutputStream> make_file_output_stream(std::string file_path);







    // Implementation:

    struct IOException: std::runtime_error
    {
      IOException(std::string m): std::runtime_error(m) {}
    };

    struct ReadException: IOException
    {
      ReadException(std::string m): IOException(m) {}
    };

    struct WriteException: IOException
    {
      WriteException(std::string m): IOException(m) {}
    };


    template<typename C> std::size_t BasicInputStream<C>::read_all(char_type *b, std::size_t n)
    {
      std::size_t m = 0;
      while(m < n) {
        std::size_t r = read(b + m, n - m);
        if(!r) break;
        m += r;
      }
      return m;
    }

    template<typename C> std::basic_string<C> BasicInputStream<C>::read_all(std::size_t max)
    {
      std::basic_string<C> s;
      std::size_t n = 1024;
      if(max && max < n) n = max;
      Array<C> b(n);
      for(;;) {
        std::size_t m = read_all(b.get(), n);
        s.append(b.get(), m);
        if(m < n) break; // EOI
        if(max) {
          max -= m;
          if(!max) break; // max reached
          if(max < n) n = max;
        }
      }
      return s;
    }


    template<typename C> template<class T, class A>
    void BasicOutputStream<C>::write(std::basic_string<C,T,A> const &s)
    {
      write(s.c_str(), s.size());
    }

    template<typename C> void BasicOutputStream<C>::write(BasicInputStream<C> &in)
    {
      std::size_t const n = 1024;
      Array<C> b(n);
      for(;;) {
        std::size_t m = in.read(b.get(), n);
        if(!m) break;
        write(b.get(), m);
      }
    }
  }
}

#endif // ARCHON_CORE_STREAM_HPP
