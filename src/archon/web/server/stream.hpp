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

#ifndef ARCHON_WEB_SERVER_STREAM_HPP
#define ARCHON_WEB_SERVER_STREAM_HPP

#include <cstddef>
#include <limits>
#include <string>

#include <archon/core/blocking.hpp>
#include <archon/core/stream.hpp>


namespace archon
{
namespace web
{
namespace server
{
using core::ReadException;
using core::WriteException;
using core::InterruptException;


/**
 * An abstract endpoint of an input stream. That is, an abstract source from
 * which some a priori unknown amount of data can be read.
 *
 * Such a stream can be closed explicitely, such that errors caused by the close
 * operation can be detected. If it is not closed explicitely, it will be closed
 * by the destructor, but any errors will then be silently ignored.
 *
 * The effect of reading from a closed stream is unspecified, but
 * implementations are advised to detect this condition and throw an appropriate
 * exception. Closing a stream that is already closed shall have no efftect.
 *
 * A lot of implementations will exist, which are simply different kinds of
 * wrappers that simply forward the read request to another data source, and
 * maybe also does some processing on the fly. In such cases, implementations
 * are expected to handle the read requests in a 'lean' manner, meaning that
 * they should make no effort to fill the callers read buffer by making multiple
 * requests for data on the wrapped data source. The following 'rules of thumb'
 * are not mandatory, but recommended behaviour:
 *
 * - Avoid making a read request on the wrapped data source, if the callers
 *   buffer can be filled anyway.
 *
 * - Make multiple read requests on the wrapped data source only if it is
 *   required, to deliver at least one byte/character to the caller.
 *
 *
 * <h3>Thread-safty</h3>
 *
 * A stream implementation is thread-safe if all the methods defined below are
 * individually and mutually thread-safe. That is, if multiple threads can
 * safely call these methods simultaneously on a single instance. An
 * implementation does no have to provide thread-safety, but it must state
 * clearly whether or not it does.
 *
 *
 * <h3>Blocking behaviour</h3>
 *
 * A particular stream has either blocking or non-blocking behavior. If it has
 * blocking behavior, a read operation will block the caller, and return only
 * when at least one character is available to read from the source.
 *
 * On the other hand, a read operation on a non-blocking stream will return as
 * soon as possible, and will return only the characters that are immediately
 * available in the source. If no characters are available, InterruptException
 * will be thrown.
 *
 * A particular stream implementation must clearly state in its documentation
 * whether instances have blocking or non-blocking behavior. Some
 * implementations may provide instances of both kinds, but in all cases, it
 * must be possible for the application to know whether a particular stream is
 * blocking or not.
 */
template<class Ch, class Tr = std::char_traits<Ch> >
struct BasicInputStreamNew
{
    typedef Ch          char_type;
    typedef Tr          traits_type;
    typedef std::size_t size_type;


    /**
     * Read at most \c n characters from this stream into the specified buffer.
     *
     * \param buffer The buffer into which the retrieved characters will be
     * placed.
     *
     * \param n The maximum number of characters to read. The buffer must be
     * large enough to hold at least this number of characters.
     *
     * \return The number of characters that were actually read. This is always
     * a number between 0 and \a n. If \a n is not zero, a return value equal to
     * zero indicates that the end of input is reached. A blocking stream will
     * not return until at least one character can be extracted from the
     * source. A non-blocking stream throws InterruptException if no characters
     * are immediately available. If characters are available, and \a n is not
     * zero, the returned number is always at least 1, regardless of whether the
     * stream is blocking or non-blocking.
     *
     * \note It shall be legal to call this method again after it has indicated
     * end-of-input, but in that case it must return 0, and continue to do so.
     *
     * \throw ReadException If the read operation fails. For examle, if the
     * connection to the resource is severed, or corruption is detected withing
     * the state of the resource.
     *
     * \throw InterruptException If the caller was interrupted while blocked, or
     * if the stream is non-blocking and no data was immediately available.
     */
    virtual size_type read(char_type *buffer, size_type n)
        throw(ReadException, InterruptException) = 0;


    /**
     * Close this stream.
     *
     * Closing an input stream is an explicit way of indicating that you are not
     * interested in reading anything more from it.
     *
     * Individual stream implementations can use this opportunity to release
     * occupied resources, for example by recursively closing a wrapped stream.
     *
     * The stream shall be automatically closed when the stream object is
     * destroyed. This is not done by default. It is the responsibility of the
     * individual implementation to ensure it happens.
     */
    virtual void close() throw() = 0;


    virtual ~BasicInputStreamNew() {}
};



/**
 * An abstract endpoint of an output stream. That is, an abstract target to
 * which a generally infinite amount of data can be written.
 *
 * Such a stream can, and should, be closed explicitely, such that errors caused
 * by the implied flushing, or the close operation itself, can be detected. If
 * it is not closed explicitely, it will be closed by the destructor, but any
 * errors will then be silently ignored.
 *
 * The effect of writing to a closed stream is unspecified, but implementations
 * are advised to detect this condition and throw an appropriate
 * exception. Closing a stream that is already closed shall have no efftect.
 *
 * A lot of implementations will exist, which are simply different kinds of
 * wrappers that simply forward the write request to another data destination,
 * and maybe also does some processing on the fly. In such cases,
 * implementations are expected to handle the write requests in a 'lean' manner,
 * meaning that they should make no effort to flush internal buffers by issuing
 * multiple write requests to the wrapped data destination. The following 'rule
 * of thumb' is not mandatory, but recommended behaviour:
 *
 * - Make only as many write request on the wrapped data destination as are
 *   required to consume all the data in the callers buffer.
 *
 * This rule presumes that we are not willing to allocate extra buffer space
 * just to be able to consume the callers data.
 *
 *
 * <h3>Thread-safty</h3>
 *
 * A sthream implementation is thread-safe if all the methods defined below are
 * individually and mutually thread-safe. That is, if multiple threads can
 * safely call these methods simultaneously on a single instance. An
 * implementation does no have to provide thread-safety, but it must state
 * clearly whether or not it does.
 *
 *
 * <h3>Blocking behaviour</h3>
 *
 * A particular stream has either blocking or non-blocking behavior. If it has
 * blocking behavior, a write operation will block the caller, and return only
 * when at least one character could be delivered to the destination.
 *
 * On the other hand, a write operation on a non-blocking stream will return as
 * soon as possible, and will deliver only the characters that can be
 * immediately written to the destination. If no characters can be delivered,
 * InterruptException will be thrown.
 *
 * A particular stream implementation must clearly state in its documentation
 * whether instances have blocking or non-blocking behavior. Some
 * implementations may provide instances of both kinds, but in all cases, it
 * must be possible for the application to know whether a particular stream is
 * blocking or not.
 */
template<class Ch, class Tr = std::char_traits<Ch> >
struct BasicOutputStreamNew
{
    typedef Ch          char_type;
    typedef Tr          traits_type;
    typedef std::size_t size_type;

    /**
     * Write at most \c n characters from the specified buffer onto the stream.
     *
     * \param buffer The buffer that holds the data, that is to be written.
     *
     * \param n The maximum number of characters to write.
     *
     * \return The number of characters that were actually written. This is
     * always a number between 0 and \a n. If \a n is not zero, a return value
     * is always at least 1. A blocking stream will not return until at least
     * one character could be handed off to the destination. A non-blocking
     * stream throws InterruptException if no characters could be handed off
     * immediately.
     *
     * \throw WriteException If the write operation fails. For examle, if the
     * connection to the resource is severed, or if written data did not conform
     * to a required protocol.
     *
     * \throw InterruptException If the caller was interrupted while blocked, or
     * if the stream is non-blocking and no data could be handed off
     * immediately.
     */
    virtual size_type write(char_type const *buffer, size_type n)
        throw(WriteException, InterruptException) = 0;


    /**
     * Flush as much as possible of any buffered data 'down the stream'.
     *
     * This implies that a stream implementation that wraps another stream,
     * should recursively flush the wrapped stream.
     *
     * This method shall not guarantee that all buffered data is flushed, nor
     * that the stream will generate no more output in case it is closed. There
     * are some types of streams where a complete flush is undesirable, because
     * it would cause any further writing to be meaningless. An example is a
     * stream that wraps its input into an envelope stream consisting of a
     * series of blocks that each specify its own size, and is terminated by a
     * block of size zero. If a flush had to guarantee that the stream would
     * generate no more output in case it was closed, it would be forced to
     * write the terminating block of size zero, but then it would not be
     * possible to add more data later.
     *
     * Another example, is a stream that transcodes charcters from one multibye
     * encoding to another. If only a partial character is available in the
     * buffer, these bytes cannot be flushed unless it is assumed that the
     * stream is about to be closed, in which case these bytes should be handled
     * as invalid input.
     *
     * \throw WriteException If flusing fails. For examle, if the connection to
     * the resource is severed, or if buffered data did not conform to a
     * required protocol.
     *
     * \throw InterruptException If the caller was interrupted while blocked, or
     * if the stream is non-blocking and the buffered data could not be handed
     * off immediately.
     */
    virtual void flush() throw(WriteException, InterruptException) = 0;


    /**
     * Close this stream.
     *
     * Closing an output stream is an explicit way of indicating that you are
     * not going to write anything more to it. Closing an output strean also
     * involves an inmplicit 'flush', which forces intermediate buffers to be
     * emptied, and thereby pushing any remaining data 'down the stream'.
     *
     * In contrast to flush(), this method <u>shall</u> guarantee that all
     * remaining buffered data is flushed, and therefore, if this is impossible
     * due to incomplete data, some action must be taken, for example by
     * throwing <tt>WriteException</tt>. An example of such a case, is a
     * transcoding stream accepting multi-byte characters as a flat stream of
     * bytes, and if at the time of closing, the last character is incomplete,
     * the stream might choose to indicate an error.
     *
     * The closing operation should also be used as an opportunity to release
     * occupied resources.
     *
     * The stream shall be automatically closed when the stream object is
     * destroyed. This is not done by default. It is the responsibility of the
     * individual implementation to ensure it happens. In this case, a failure
     * to close shall be silently ignored.
     *
     * \throw WriteException If closing fails. For examle, if buffered data
     * could not be flushed.
     *
     * \throw InterruptException If the caller was interrupted while flushing,
     * or if the stream is non-blocking and the buffered data could not be
     * handed off immediately.
     */
    virtual void close() throw(WriteException, InterruptException) = 0;


    virtual ~BasicOutputStreamNew() {}
};



typedef BasicInputStreamNew<char>     InputStreamNew;
typedef BasicOutputStreamNew<char>    OutputStreamNew;
typedef BasicInputStreamNew<wchar_t>  WideInputStreamNew;
typedef BasicOutputStreamNew<wchar_t> WideOutputStreamNew;




/**
 * This stream implementation is not thread-safe.
 */
struct FileInputStream: InputStreamNew
{
    void open(std::string filesys_path);

    void open(int filedes, bool close = true);

    /**
     * \return The underlying file descriptor.
     */
    int get_fildes() const { return fildes; }

    FileInputStream(): closed(true) {}
    ~FileInputStream() throw() { close(); }

    size_type read(char_type *buffer, size_type n) throw(ReadException, InterruptException);
    void close() throw();

private:
    int fildes;
    bool must_close, closed;
};
}
}
}

#endif // ARCHON_WEB_SERVER_STREAM_HPP
