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

#ifndef ARCHON_IMAGE_WORD_BASED_BUFFER_FORMAT_HPP
#define ARCHON_IMAGE_WORD_BASED_BUFFER_FORMAT_HPP

#include <limits>
#include <vector>
#include <string>

#include <archon/image/word_type.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/buffer_format.hpp>


namespace archon
{
  namespace image
  {
    /**
     * A buffer format where pixels are layed out in a sequence of
     * words of a type that is not necessarily chars/bytes. This is
     * important in relation to endianess, but of course only when the
     * word type is not actually \c char. One example would be buffer
     * formats based on floating point components. Another example is
     * some packed formats where three components (RGB) are stored as
     * bit-fields in a single 16 bit integer. Both these formats are
     * in general affected by endiannes issues. Note however that this
     * class says nothing about how pixels are layed out in the word
     * sequence, that is the job of derived classes such as \c
     * IntegerBufferFormat or \c DirectBufferFormat.
     *
     * \sa IntegerBufferFormat
     * \sa DirectBufferFormat
     */
    struct WordBasedBufferFormat: BufferFormat
    {
      // We need to define these manually to avoid ambiguity with the
      // cousins in BufferFormat.
      typedef core::CntRef<WordBasedBufferFormat>        Ref;
      typedef core::CntRef<WordBasedBufferFormat const>  ConstRef;
      typedef Ref                                 const &RefArg;
      typedef ConstRef                            const &ConstRefArg;


      virtual WordType get_word_type() const = 0;

      virtual std::vector<bool> get_endianness() const = 0;

      virtual int get_bytes_per_word() const = 0;

      int get_bits_per_word() const;

      bool is_floating_point_words() const;

      std::string get_word_type_name() const;


      /**
       * Produce a string representation of this buffer format
       * combined with the specified color space.
       *
       * If the color space is not specified, this method will choose
       * an appropriate color space. Also, in this case \c has_alpha
       * will be ignored, and this method will decide whether or not
       * an alpha channel is present.
       */
      virtual std::string print(ColorSpace::ConstRefArg color_space = core::CntRefNullTag(),
                                bool has_alpha = false) const = 0;
    };






    // Implementation

    inline int WordBasedBufferFormat::get_bits_per_word() const
    {
      return get_bytes_per_word() * std::numeric_limits<unsigned char>::digits;
    }

    inline bool WordBasedBufferFormat::is_floating_point_words() const
    {
      return archon::image::is_floating_point(get_word_type());
    }

    inline std::string WordBasedBufferFormat::get_word_type_name() const
    {
      return archon::image::get_word_type_name(get_word_type());
    }
  }
}

#endif // ARCHON_IMAGE_WORD_BASED_BUFFER_FORMAT_HPP
