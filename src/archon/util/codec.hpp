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
 * This file provides a number of utility codecs.
 */

#ifndef ARCHON_UTIL_CODEC_HPP
#define ARCHON_UTIL_CODEC_HPP

#include <archon/core/codec.hpp>
#include <archon/core/unique_ptr.hpp>


namespace archon
{
  namespace Util
  {
    /**
     * This codec wraps a stream in an envelope consisting of a
     * sequence of blocks of up to 255 bytes. Each block is preceeded
     * by a byte that specifies the block size (number of bytes that
     * follow). It has an explicit end-of-input marker in the form of
     * a block of size zero. This is especially usefull when you want
     * to embed one stream inside another, since it becomes possible
     * to read the embedded stream without reading beyond the end of
     * it, and thus destoying the ability to read the remainder of the
     * outer stream.
     *
     * Unless the encoding output stream is flushed explicitely, all
     * chunks, except the last one, will have a size of 255. Flushing
     * the stream will in generall cause blocks of smaller size to be
     * generated, thus causing an increased size-wise overhead.
     *
     * This output stream does not support writing after a
     * flush. Thaat is, a flush is effectively a close.
     *
     * \note When closed, neither the decoding input stream nor the
     * encoding output stream will close the wrapped stream.
     *
     * \note Currently only the decoding input stream and the encoding
     * output stream is implemented.
     */
    Core::UniquePtr<Core::Codec const> get_block_codec();
  }
}

#endif // ARCHON_UTIL_CODEC_HPP
