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

#ifndef ARCHON_IMAGE_BUFFERED_IMAGE_HPP
#define ARCHON_IMAGE_BUFFERED_IMAGE_HPP

#include <archon/image/image.hpp>
#include <archon/image/buffer_format.hpp>

namespace archon
{
  namespace image
  {
    /**
     * A abstract base class for images that are directly backed by a
     * buffer and where the buffer, as well as the buffer format, is
     * available to the application.
     *
     * \todo FIXME: Consider adding a \c new_image method that takes a
     * const pointer to the buffer for cases where we want to wrap a
     * buffer to which we do not have write access. This method should
     * return an \c ImageSource::ConstRef.
     */
    struct BufferedImage: Image
    {
      // We need to define these manually to avoid ambiguity with the
      // cousins in Image.
      typedef core::CntRef<BufferedImage>        Ref;
      typedef core::CntRef<BufferedImage const>  ConstRef;
      typedef Ref                         const &RefArg;
      typedef ConstRef                    const &ConstRefArg;

      /**
       * Retrieve the pointer to the pixel buffer.
       */
      virtual void *get_buffer_ptr() = 0;

      /**
       * Retrieve the pointer to the pixel buffer.
       */
      virtual void const *get_buffer_ptr() const = 0;

      /**
       * Ask the buffer format for the buffer size.
       *
       * \return The size og the pixel buffer in number of bytes.
       *
       * \sa BufferFormat::get_required_buffer_size
       */
      size_t get_buffer_size() const;

      /**
       * Get the buffer format that governs how pixels are encoded in
       * the buffer.
       */
      virtual BufferFormat::ConstRef get_buffer_format() const = 0;

      /**
       * Check whether the specified buffer format is a valid
       * description of the actual format that governs the storage of
       * pixels in this image.
       *
       * This is trivially true if the specified buffer format is
       * identical to the one that is returned by
       * <tt>get_buffer_format</tt>, but it may also be true for
       * non-identical formats. Consider two integer based formats that
       * are identical except that one specifies that each pixel strip
       * is word aligned, and the other one does not. Then those two
       * formats would be equivalent for an image where the product of
       * its width and the number of bits per pixel is divisible by
       * the bit-width of the word type. Another case is where one
       * format specifies an explicit endianness and the other one
       * opts for the default endianness of the platform, then those
       * two would be equivalent on platforms whose native endianness
       * matches the one specified explicitly by one of the formats.
       *
       * It must always be the case that if this method returns true,
       * then the two formats are interchangeable, however the reverse
       * is not guaranteed. That is, implementations are not required
       * to detect all cases of equivalence.
       *
       * \note The result will in general depend on the image
       * dimmensions as well as the harware/software platform.
       */
      bool has_equiv_buffer_format(BufferFormat::ConstRefArg) const;

      /**
       * Create a new image backed by a buffer. The buffer is both
       * created and destroyed by the image object. The contents of
       * the buffer is not cleared initially.
       *
       * Note that although the memory format of individual pixels can
       * be controlled in great detail, there is no freedom in
       * choosing the order in which pixels must occur in memory. This
       * class assumes the following order: The buffer consists of a
       * sequence of horizontal pixel strips (rows). The bottom-most
       * strip is the first strip in the buffer. Within each strip the
       * left-most pixel is the first pixel in the buffer. The
       * encoding of a strip of pixels is controlled by the buffer
       * format, which also determines whether or not each strip is
       * aligned on a word boundary. Thus, the precise number of bits
       * per pixel and per strip is determined by the buffer format.
       *
       * If you consider the order of pixels to be different than what
       * is assumed by this class, you may want to wrap the instance
       * in a flipped and/or rotated view by using
       * <tt>Oper::flip</tt>, <tt>Oper::flip_diag</tt>, or
       * <tt>Oper::rotate</tt>. With those operators you will be able
       * to achieve almost any strip based pixel ordering.
       *
       * \param width, height The desired width and height of the
       * image.
       *
       * \param color_space The desired color space to be used in the
       * image.
       *
       * \param has_alpha Pass true if you want this image to have an
       * alpha channel.
       *
       * \param buffer_format The desired format of pixels when stored
       * in the image buffer. When \c null is passed (the default), a
       * default buffer format will be constructed to match the number
       * of channels in the requested color space, plus one if an
       * alpha channel is requested. In any case, the first channel
       * defined by the format describes the first channel of the
       * color space, and so on. This refers to the canonical channel
       * order of the color space.
       *
       * \throw ImageSizeException If the image buffer would become
       * larger than the maximum size supported by the specified
       * buffer format.
       */
      static Ref new_image(int width, int height,
                           ColorSpace::ConstRefArg color_space = ColorSpace::get_RGB(),
                           bool has_alpha = false,
                           BufferFormat::ConstRefArg buffer_format = core::CntRefNullTag());

      /**
       * Create a new image wrapping a pre-existing buffer. The
       * ownership of the buffer remains with the caller. The content
       * of the buffer is not cleared. apart from the difference in
       * buffer ownership, this method is identical to 4 argument
       * version of \c new_image.
       *
       * \param buffer The buffer to be wrapped. The ownership remains
       * with the caller. The size of the buffer measured in number of
       * bytes must be no less than what is returned by
       * <tt>BufferFormat::get_required_buffer_size</tt>.
       *
       * \param width The number of pixels per strip in the specified
       * buffer.
       *
       * \param height The number of strip in the specified buffer.
       *
       * \param color_space The color space in use in the specified
       * buffer.
       *
       * \param has_alpha Pass true if the specified buffer includes an
       * alpha channel.
       *
       * \param buffer_format The format of pixels as they are stored
       * in the specified buffer.
       *
       * \throw ImageSizeException If the size of the image buffer, as
       * calculated from the specified dimmensions, color space, and
       * buffer format, is larger than the maximum supported size.
       */
      static Ref new_image(void *buffer, int width, int height,
                           ColorSpace::ConstRefArg color_space, bool has_alpha,
                           BufferFormat::ConstRefArg buffer_format);
    };





    // Implementation:

    inline size_t BufferedImage::get_buffer_size() const
    {
      return get_buffer_format()->get_required_buffer_size(get_width(), get_height());
    }

    inline bool
    BufferedImage::has_equiv_buffer_format(BufferFormat::ConstRefArg f) const
    {
      return get_buffer_format()->is_equiv_to(f, get_width(), get_height());
    }
  }
}

#endif // ARCHON_IMAGE_IMAGE_HPP
