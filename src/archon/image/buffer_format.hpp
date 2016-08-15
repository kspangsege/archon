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

#ifndef ARCHON_IMAGE_BUFFER_FORMAT_HPP
#define ARCHON_IMAGE_BUFFER_FORMAT_HPP

#include <stdexcept>
#include <vector>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/refcnt.hpp>
#include <archon/image/buffer_codec.hpp>


namespace archon
{
  namespace Imaging
  {
    /**
     * Thrown when the requested dimmensions, number of channels, and
     * buffer format would cause the image buffer size to exceeds the
     * maximum size supported by the buffer format.
     */
    struct ImageSizeException;

    /**
     * \note A \c BufferFormat instance must be immutable. That is,
     * all non-static methods except \c get_codec must return the same
     * value throughout the lifetime of the instance. This includes
     * methods defined by derived classes. More over, if two codec
     * objects are retrieved for images of equal size, they must
     * perform exactly the same conversions.
     */
    struct BufferFormat: virtual Core::CntRefObjectBase,
                         Core::CntRefDefs<BufferFormat>
    {
      /**
       * The number of channels per pixel that is encoded by this
       * buffer format.
       *
       * \return The number of channels.
       */
      virtual int get_num_channels() const = 0;


      /**
       * Get the precision of the specified channel in this
       * format. That is, the number of bits of an integer that are
       * required to to hold any value from the specified channel
       * without loosing significant information.
       *
       * For integer base buffer formats, this is simply the number of
       * bits used to encode the channel. For floating point buffer
       * formats it is recommended that the bit width of the floating
       * point type is returned.
       *
       * \param channel_index The index of the channel whose width is
       * needed.
       *
       * \return The number of bits of an integer type that is
       * required to hold any value from the specified channel without
       * loosing significant information.
       */
      virtual int get_channel_width(int channel_index) const = 0;


      /**
       * Given the specified image dimmensions, determine whether the
       * specified format can be use in place of this one without
       * affecting the result of pixel encoding and decoding.
       *
       * This method is not required to detect equivalence in every
       * case. It shall be guaranteed, however, that if it returns
       * true, then the two formats are intechangable.
       *
       * \sa BufferedImage::has_equiv_buffer_format
       */
      virtual bool is_equiv_to(BufferFormat::ConstRefArg,
                               int image_width, int image_height) const = 0;


      /**
       * Compute the required size in bytes of a buffer that can hold
       * an image of the specified size using this buffer format.
       *
       * \param width, height The Size of the image that must fit in
       * the buffer.
       *
       * \return The required size of the buffer measured in number of
       * bytes.
       *
       * \throw ImageSizeExcpetion If this buffer format does not
       * support an image of the specified dimmensions. For example,
       * if the buffer would become too large.
       */
      virtual size_t get_required_buffer_size(int width, int height) const = 0;


      /**
       * \param buffer The pixel buffer to wrap. The ownership of this
       * buffer is not transferred to the callee. The pixels in the
       * buffer will be assumed to comply with this buffer format.
       *
       * \param width, height The Size of the image in the
       * buffer. Note that \c width always refers to the number of
       * pixels per strip, and \c height to the numner of strips in
       * the buffer.
       *
       * \return A codec object capable of encoding and decoding
       * pixels in the wrapped buffer. Ownership of the returned
       * buffer codec is transferred to the caller.
       */
      virtual Core::UniquePtr<BufferCodec>
      get_codec(void *buffer, int width, int height) const = 0;


      /**
       * Get a new default format. The default format uses exactly 8
       * bits per channel, and is based on words of type <tt>unsigned
       * char</tt> which in this context is the same as a byte. It
       * uses one word per channel. That is, each pixel uses as many
       * bytes as the specified number of channels. If a byte has more
       * than 8 bits, only the 8 bits of lowest significance are
       * used. The order of channels in memory is the same as the
       * canonical order, that is, the order in which they are
       * extraced from and passed to the codec object by the
       * application.
       *
       * \param num_channels The number of channels that the
       * format must encode.
       *
       * \return The default format for the specified number of
       * channels.
       */
      static Ref get_default_format(int num_channels = 4);


      /**
       * Get a new simple format where the channels are memory
       * consecutive and in canonical order. Each channel uses one
       * word, and uses all the bits of that word.
       */
      static Ref get_simple_format(WordType word_type, int num_channels);


      template<typename T> static Ref get_simple_format(int num_channels = 4);
    };






    // Implementation:

    struct ImageSizeException: std::runtime_error
    {
      ImageSizeException(std::string m): std::runtime_error(m) {}
    };


    template<typename T> inline BufferFormat::Ref BufferFormat::get_simple_format(int num_channels)
    {
      return get_simple_format(get_word_type_by_type<T>(), num_channels);
    }
  }
}

#endif // ARCHON_IMAGE_BUFFER_FORMAT_HPP
