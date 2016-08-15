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

#ifndef ARCHON_IMAGE_BUFFER_CODEC_HPP
#define ARCHON_IMAGE_BUFFER_CODEC_HPP

#include <archon/util/tuple_grid.hpp>
#include <archon/image/word_type.hpp>


namespace archon
{
  namespace Imaging
  {
    /**
     * An abstract buffer codec. That is, an encapsulation of a
     * particular way of storing pixel data in a memory buffer, and a
     * set of methods for reading and writing data in and out of the
     * buffer.
     *
     * A buffer codec object knows the pointer to the memory buffer,
     * but does not own the memory.
     *
     * Buffer codecs are normally created by a buffer format object by
     * request from an image object, and are then used transparently
     * by that image object to read and write data. This class is not
     * intended to be used directly by the application, but the
     * application may wich to implement a custom buffer codec which
     * requires that the application also implements a custom buffer
     * format.
     *
     * \sa BufferFormat
     */
    struct BufferCodec
    {
      /**
       * Get a pointer to the memory buffer that is accessed by this
       * buffer codec.
       */
      virtual void *getBufferPtr() const = 0;



      /**
       * The \c decode and \c encode methods expect the pixel data in
       * the tray buffer to be of this type. That is, each channel
       * occupies precisely one such word.
       */
      virtual WordType getTrayWordType() const = 0;

      /**
       * Extract a rectangular block of pixels from the image
       * buffer. This generally involves some degree of decoding or
       * unpacking.
       *
       * \param tray Defines a grid of target memory addresses into
       * which the decoded pixels will be placed. The width and height
       * of this grid determines the size of the block of pixels to be
       * decoded. The width must be less than or equal to the width of
       * the image minus <tt>left</tt>. Likewise, the height must be
       * less than or equal to the height of the image minus
       * <tt>bottom</tt>. Each pixel in the tray consists of N memory
       * consecutive words of the type returned by
       * <tt>getTrayWordType</tt> where N is the number of channels in
       * the color space used by the image.
       *
       * \param left, bottom The bottom left corner of the block of
       * pixels to be decoded. These are measured in pixels and the
       * origin is the bottom left corner of the image. \c left must
       * be less than or equal to the width of the image minus the
       * width of the tray. Likewise \c bottom must be less than or
       * equal to the height of the image minus the height of the
       * tray.
       */
      virtual void decode(Util::TupleGrid const &g, int w, int h, int x, int y) const = 0;

      /**
       * Write a rectangular block of pixels into the image
       * buffer. This generally involves some degree of encoding or
       * packing.
       *
       * \param tray Defines a grid of source memory addresses holding
       * the "raw" pixels to be encoded. The width and height of this
       * grid determines the size of the affected block in the
       * image. The width must be less than or equal to the width of
       * the image minus <tt>left</tt>. Likewise, the height must be
       * less than or equal to the height of the image minus
       * <tt>bottom</tt>. Each pixel in the tray consists of N memory
       * consecutive words of the type returned by
       * <tt>getTrayWordType</tt> where N is the number of channels in
       * the color space used by the image.
       *
       * \param left, bottom The bottom left corner of the target
       * block in the image. These are measured in pixels and the
       * origin is the bottom left corner of the image. \c left must
       * be less than or equal to the width of the image minus the
       * width of the tray. Likewise \c bottom must be less than or
       * equal to the height of the image minus the height of the
       * tray.
       */
      virtual void encode(Util::ConstTupleGrid const &g, int w, int h, int x, int y) = 0;


      virtual ~BufferCodec() {}
    };
  }
}

#endif // ARCHON_IMAGE_BUFFER_CODEC_HPP
