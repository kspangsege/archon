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

#ifndef ARCHON_UTIL_COMPRESS_HPP
#define ARCHON_UTIL_COMPRESS_HPP

#include <archon/core/codec.hpp>
#include <archon/core/unique_ptr.hpp>


namespace Archon
{
  namespace Util
  {
    /**
     * Variable-Length-Code LZW Compression with, 'clear' and 'eof'
     * codes, and with a maximum of 12 bits per code. This is the
     * variant used in the GIF image file format.
     *
     * \param bits_per_byte Some number, <tt>N</tt>, such that the value
     * of all encoded bytes are less than <tt>2**N</tt>. The lower the
     * number, the better the compression. It must be in the range
     * <tt>[2;min(11,M)]</tt>, where \c M is the number of bits in a
     * byte. Also, the compressor and the decompressor must agree on
     * this value for decompression to be successfull. The compressor
     * will raise an error if it encounteres an input byte whose value
     * is not in the range <tt>[0;2**N-1]</tt> (assuming bytes of
     * unsigned type).
     */
    Core::UniquePtr<Core::Codec const> get_lempel_ziv_welch_codec(int bits_per_byte = 8);
  }
}

#endif // ARCHON_UTIL_COMPRESS_HPP
