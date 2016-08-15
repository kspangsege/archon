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

#include <limits>

#include <archon/image/buffer_format.hpp>
#include <archon/image/integer_buffer_format.hpp>


using namespace std;

namespace archon
{
  namespace image
  {
    BufferFormat::Ref BufferFormat::get_default_format(int num_channels)
    {
      int const bits_per_byte  = numeric_limits<unsigned char>::digits;
      IntegerBufferFormat::ChannelLayout channels;
      channels.bits_per_pixel = num_channels * bits_per_byte;
      for(int i=0; i<num_channels; ++i)
        channels.add(IntegerBufferFormat::Channel(i*bits_per_byte, 8));
      return IntegerBufferFormat::get_format(word_type_UChar, channels);
    }


    BufferFormat::Ref BufferFormat::get_simple_format(WordType word_type, int num_channels)
    {
      int const bits_per_word = get_bits_per_word(word_type);
      IntegerBufferFormat::ChannelLayout channels;
      for(int i=0; i<num_channels; ++i) channels.add(bits_per_word);
      return IntegerBufferFormat::get_format(word_type, channels);
    }
  }
}
