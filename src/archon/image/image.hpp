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

#ifndef ARCHON_IMAGE_IMAGE_HPP
#define ARCHON_IMAGE_IMAGE_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <archon/core/unique_ptr.hpp>
#include <archon/util/tuple_grid.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/math/vector.hpp>
#include <archon/image/color_space.hpp>


namespace archon
{
  namespace Imaging
  {
    /**
     * This class is an abstract representation of an image as a
     * rectangular array of pixels.
     *
     * From the point of view of this class, an image is characterized
     * only by its witdh and height in number of pixels, and by a
     * particular pixel format used when chunks of pixels are read
     * from or written to the image.
     *
     * This class does not provide convenient methods for accessing
     * and manipulating pixels. It defines only the lowest level means
     * to do so. Convenient access to individual pixels are provided
     * by the \c ImageReader and \c ImageWriter classes.
     *
     * Both c ImageReader and \c ImageWriter wraps around a reference
     * to an instance of this class, but \c ImageWriter requires a
     * non-const reference, thus, image write access is intended to be
     * managed by means of constness of the image reference.
     *
     * Images can also be combined and manipulated in various ways by
     * a set of high-level compositing operators. See the \c
     * Image::Oper namesspace for more on this.
     *
     * For convenience, this class provides a \c load method for
     * loading images from files, and a \v save method for saving
     * image to files. These are simplified versions of the more
     * elaborate functionality provided in the <tt>Image::ImageIO</tt>
     * namespace.
     *
     * Further more it provides the \c new_image method, for creatig
     * new images backed by memory buffers, and a \c put_image method
     * for pasting one image onto another. See the \c BufferedImage
     * class for other ways to create bufefr backed images.
     *
     *
     *
     *
     * The rest of this comment is partially obsolete:
     *
     * All images have a specific width, height, and color
     * space. These properties can be accessed using
     * <tt>get_width</tt>, <tt>get_height</tt>, and
     * <tt>get_color_space</tt> respecitvely. Further more, each
     * individual channel of the color space is characterized by a
     * certain precision.
     *
     * It also provides a set of methods for creating alternative
     * views of the image. These methods all have the "View" suffix,
     * and are characterized by producing an alternative "live" view
     * of the original. Thus, changes made to the original image after
     * the point in time where the alternative view was created, still
     * have effect on what is seen throgh the alternative view. The
     * alternative views may optionally also be writable, in which
     * case changes made through the alternative view, are visible
     * when the image is views through its original interface.
     *
     * Most of the methods of this class take, either a pair of image
     * coordinates, or a rectangle specification, as argument. Unless
     * otherwise stated, it is not required that the coordinates, or
     * the specified rectangle, falls within the boundaries of the
     * image. When they escape the image boundary, they will either
     * wrap around to the opposite edge, or be clipped depending on
     * the specification of the repetition compound. See \c put_block
     * for a more thorough explanation of this.
     *
     * \todo More general image compositions with alpha blending.
     *
     * \todo Juxtaposition (horizontal) with align spec
     * (top/middle/bottom). Stack vertical with align spec
     * (left/middle/right). Must somehow handle differing color
     * spaces. Also, the region not covered would return alpha = 0.
     *
     * \todo Maybe repeat=-1 could be transparent beyond edge, but
     * then it will be impossible to have transparency outside a
     * repetition of 'n>1', but that might be ok.
     *
     * \todo Trouble, there is supposed to be a get_sub_view for Image
     * (a writable target), however it seems impossible to have it
     * support repetition due to aliasing (same pixels a represented
     * in several places). Aliassing is a thing to consider when
     * juxtaposing image with itself (maybe in disguise)
     *
     * \todo USE THIS in PNG SAVE and JPEG save
     * It shall be guaranteed that if this method returns true, then one format can be used in place of the other on this platform. However, the reverse shall not be guaranteed, that is, it could return false, even though the two formats are actually equivalent. (oppertunistic)
     * REquire integer of same bit size or float of same kind. (if a float and a double has same bit width on some flatform, is that a guarantee they have identical bit-level layout?)
     * An explicit endianess is equivalent to an implicit one if the native is.
     * Assuming a short is twice as wide as a char, then a format using short is equivalent with one using char endianess is right.
     * Might be a good idea to reintroduce flags (right_to_left, top_to_bottom, vertical_strips) such that load can prevent introducing a buffer hiding, turning image node which would force an image copy in save even though it would not have been necessary.
     */
    struct Image: virtual core::CntRefObjectBase, core::CntRefDefs<Image>
    {
      /**
       * Get the width of this image measured in pixels.
       *
       * \return The number of pixels between the left and the right
       * edges of this image.
       */
      virtual int get_width() const = 0;


      /**
       * Get the geight of this image measured in pixels.
       *
       * \return The number of pixels between the bottom and the top
       * edges of this image.
       */
      virtual int get_height() const = 0;


      /**
       * Get the color space associated with this image, or more
       * precisely, the color space on which the native pixel block
       * format of this image is based.
       *
       * The native pixel block format is the one used when writing
       * to, or reading from the image via a codec object. For images
       * that encapsulate a pixel buffer, this format is often the
       * same as the one used to encode pixels in that buffer.
       *
       * In the native pixel format, the order of color channels is
       * the same as the natural channel order of the color space.
       *
       * \return The color space object associated with this
       * image.
       *
       * \sa acquire_codec
       */
      virtual ColorSpace::ConstRef get_color_space() const = 0;


      /**
       * Returns true if this image has transparency information in
       * the form of an alpha channel, or more precisely, if the
       * native pixel block format of this image includes an alpha
       * channel.
       *
       * The native pixel block format is the one used when writing
       * to, or reading from the image via a codec object. For images
       * that encapsulate a pixel buffer, this format is often the
       * same as the one used to encode pixels in that buffer.
       *
       * In the native pixel format, the alpha channel (if present) is
       * always the last one, and comes immediately after the color
       * channels.
       *
       * \return True iff this image includes an alpha channel in its
       * native pixel encoding.
       *
       * \sa acquire_codec
       */
      virtual bool has_alpha_channel() const = 0;


      /**
       * The number of channels present in this image, or more
       * precisely, in the native pixel block format of this
       * image. This is always equal to the number of channels of the
       * associated color space, plus one if this image has an alpha
       * channel.
       *
       * The native pixel block format is the one used when writing
       * to, or reading from the image via a codec object. For images
       * that encapsulate a pixel buffer, this format is often the
       * same as the one used to encode pixels in that buffer.
       *
       * \return The number of channels present in this image.
       *
       * \sa acquire_codec
       */
      virtual int get_num_channels() const = 0;


      /**
       * Get the precision of the specified channel in this
       * image. That is, the number of bits of an integer that are
       * required to to hold any value from the specified channel
       * without loosing significant information. If a negative
       * channel index is specified (the default), the result is the
       * precision of the widest channel, that is the maximum over all
       * the channels.
       *
       * For integer base pixel formats, this is simply the number of
       * bits used to encode the channel. For floating point pixel
       * formats it is recommended that the bit width of the floating
       * point type is returned.
       *
       * For purely implicit images, one must return the number of
       * bits that would be necessary if the result was to be stored
       * in a buffer without significant loss. A special case of this,
       * is a uniform image (same color everywhere) that might want to
       * returns 0 because in principle no bits are needed to store
       * it.
       *
       * \param channel_index The index of the channel whose width is
       * needed. If a negative index is specified, the width of the
       * widest channel is returned. The index refers to the canonical
       * channel order of the color space returned by
       * <tt>get_color_space</tt>.
       *
       * \return The number of bits of an integer type that is
       * required to hold any value from the specified channel without
       * loosing significant information. Note that 0 is a possible
       * return value.
       */
      virtual int get_channel_width(int channel_index = -1) const = 0;


      /**
       * Get the word type that is used in the native pixel block
       * format of this image, where each channel occupies precisely
       * one word of this type.
       *
       * The native pixel block format is the one used when writing
       * to, or reading from the image via a codec object. For images
       * that encapsulate a pixel buffer, this format is often the
       * same as the one used to encode pixels in that buffer.
       *
       * \return The word type used when reading and writing via a
       * codec object.
       *
       * \sa acquire_codec
       */
      virtual WordType get_word_type() const = 0;


      /**
       * Clear this image to black, and if it has an alpha channel,
       * then make it fully transparent. This is a shorthand for
       * calling `fill(Util::Color::transparent)`.
       *
       * \note Consider working through an \c ImageWriter instead,
       * since that provides greater flexibility and better
       * performance in cases where multiple accesses to image data
       * are needed.
       *
       * \sa ImageWriter::clear
       */
      void clear() { fill(Util::Color::transparent); }


      /**
       * Fill this image with the specified color.
       *
       * \note Consider working through an \c ImageWriter instead,
       * since that provides greater flexibility and better
       * performance in cases where multiple accesses to image data
       * are needed.
       *
       * \sa ImageWriter::fill
       */
      void fill(Util::PackedTRGB color);


      /**
       * \note Consider working through an \c ImageWriter instead,
       * since that provides greater flexibility and better
       * performance in cases where multiple accesses to image data
       * are needed.
       *
       * \sa ImageWriter::put_image
       */
      void put_image(ConstRefArg image, int x = 0, int y = 0, bool blend = true);




      /**
       * Extract a sinlge pixel from the image at the specified
       * position.
       *
       * \param pixel An array into which the extracted pixel
       * components will be stored. The number of <tt>long
       * double</tt>s that will be writte to this array is precisely
       * the number of channels in the color space.
       *
       * \param x, y The position of the pixel to extract. Both are
       * measured in pixels and may exceed the edges of the image. \c
       * x is the offset from the left edge, and \c y is the offset
       * from the bottom edge.
       *
       * \param horizontal_repeat, vertical_repeat These have the same
       * meaning as in <tt>get_pixels</tt> assuming a target region of
       * 1 by 1 pixels.
       */
/*
      void get_pixel(double *pixel, int x, int y,
                    int horizontal_repeat = 0, int vertical_repeat = 0) const;
*/

      /**
       * Set the pixel at the specified coordinates to the specified
       * RGBA color. The color can be specified using a variety of
       * different data types:
       *
       * - <tt>unsigned long</tt>: On the form \c 0xTTRRGGBB, where \c
       *   TT is the transparency component whose value is 255 minus
       *   the corresponding alpha component. This makes it easier to
       *   specify RGB colors with full opacity. Thus, all four
       *   channels are packed into the 32 least significal bits, and
       *   each one uses 8 bits.
       *
       * - Any other integer type: Same as <tt>unsigned long</tt>.
       *
       * - <tt>Math::VecVal</tt>: Any vector value with 3 or 4
       *   components. Each component has a normalized range \c [0;1]
       *   where 0 is <i>off</i> and 1 is full intencity. If it has 3
       *   components, they will be interpreted as Red, Green, and
       *   Blue (in that order), and, 1, will be assumed for the alpha
       *   component. If it has four components, they will be
       *   interpreted as Red, Green, Blue, and Alpha (in that order).
       *
       * - <tt>double const *</tt>: Will be interpreted as a pointer
       *   to a buffer holding 4 <tt>double</tt>s. The interpretation
       *   of the values of these is the same as for
       *   <tt>Math::VecVal</tt>.
       *
       * - Any other pointer type: Same as <tt>double const
       *   *</tt>. Pointers to non-const elements are also accepted.
       *
       * \param color The color to store at the specified
       * coordinates. Please see above for a list of valid color
       * types.
       *
       * \param x, y The image coordinates of the target pixel. They
       * are specified in pixels, and are relative to the lower left
       * corner of the image.
       *
       * \note It is not a requirement that the coordinates must
       * address a pixel inside the boundaries of the image. Please
       * see \c put_pixels for a general explanation.
       */
/*
      template<typename Color>
      void put_pixel(Color color, int x, int y, int horizontal_repeat = 0, int vertical_repeat = 0);
*/



      /**
       * Write the pixels of the source image into this image buffer
       * at the specified position.
       *
       * \param source The source image.
       *
       * \param target_left, target_bottom The lower left corner of
       * the area into which the pixels will be written. This is
       * measured in pixels and relative to the lower left corner of
       * this image. The area need not lie within the bounds of this
       * image. Pixels will be transferred from \c tray as if \c
       * <tt>put_pixels(tray, target_left, target_bottom,
       * source_width, source_height, target_horizontal_repeat,
       * target_vertical_repeat)</tt> was called.
       *
       * \param source_left, source_bottom The lower left corner of
       * the area to extract from the source image. This is measured
       * in pixels and relative to the lower left corner of the source
       * image. The area need not lie within the bounds of the source
       * image. Pixels will be extracted into \c tray as if \c
       * <tt>get_pixels(tray, source_left, source_bottom,
       * source_width, source_height, source_horizontal_repeat,
       * source_vertical_repeat)</tt> was called.
       *
       * \param source_width, source_height The size of the pixel area
       * to transfer from the source image to this image buffer. A
       * value of zero will be automatically mapped to the actual
       * with/height of the specified source image.
       *
       * \sa get_pixels
       * \sa put_pixels
       *
       * Color space conversion: If source and target color spaces
       * are identical (including the precense of an alpha channel)
       * all color channels are copied one-to-one, otherwise the
       * source pixels are converted to RGBA then to the target color
       * space.
       *
       * Filling with pattern: This method can be used to fill a
       * region with a pattern. The trick is to pass a small image
       * (the pattern) as argument and then set \c source_width and \c
       * source_height to something larger than the pattern
       * dimensions. This also assumes that \c
       * source_horizontal_repeat and \c source_vertical_repeat are
       * 0. \c source_left and \c source_height can then be used to
       * control the alingment of the pattern with the lower left
       * corner of the target region.
       */
/*
      void put_image(Image::ConstRefArg source,
                     int target_left  = 0, int target_bottom = 0,
                     int source_left  = 0, int source_bottom = 0,
                     int source_width = 0, int source_height = 0,
                     int source_horizontal_repeat = 0, int source_vertical_repeat   = 0,
                     int target_horizontal_repeat = 0, int target_vertical_repeat   = 0);
*/



      /**
       * Load the image from the specified image file. This is nothing
       * more than a simple wrapper around the far more flexible load
       * method in <tt>ImageIO</tt>.
       *
       * \param file_path The file system path of the file to read.
       *
       * \return The loaded image.
       *
       * \sa ImageIO
       */
      static Ref load(std::string file_path);

      /**
       * Save this image to the the specified file. This is nothing
       * more than a simple wrapper around the far more flexible save
       * method in <tt>ImageIO</tt>.
       *
       * \param file_path The file system path of the file in which to
       * save the image.
       *
       * \sa ImageIO
       */
      void save(std::string file_path) const;





      /**
       * Create a new image of the specified size. The image will be
       * backed by a buffer, and the buffer will be cleared initially
       * if you ask for it.
       *
       * Take a look at \c BufferedImage::new_image if you need access
       * to the buffer and/or more control over the exact buffer
       * format to use.
       *
       * \param width, height The width and height of the new
       * image. If <tt>height</tt> is negative, it will be set equal
       * to <tt>width</tt>.
       *
       * \param color_space Use this color space in the new image.
       *
       * \param has_alpha Set to true if you want the new image to
       * have an alpha channel.
       *
       * \param word_type Use this word type in the new image.
       *
       * \sa BufferedImage::new_image
       */
      static Ref new_image(int width, int height = -1,
                           ColorSpace::ConstRefArg color_space = ColorSpace::get_RGB(),
                           bool has_alpha = false, WordType word_type = word_type_UChar);


      /**
       * Make an image whose buffer is the specified buffer and whose
       * pixel format is as follows: The word type is the type of the
       * buffer elements. The channel order is the canonical order of
       * the color space, and the channels are memory consecutive.
       *
       * The ownership of the buffer remains with the caller.
       *
       * The caller must make sure that the image is not accessed
       * after the bufefr is destroyed.
       *
       * Take a look at \c BufferedImage::new_image if you need more
       * control over the exact buffer format to use.
       *
       * \sa BufferedImage::new_image
       */
      template<typename T>
      static Ref new_image(T *buffer, int width, int height,
                           ColorSpace::ConstRefArg color_space = ColorSpace::get_RGB(),
                           bool has_alpha = false);


      /**
       * Make an image as a copy of the pixels in the specified
       * buffer. The pixel format of the specified buffer is assumed
       * to be as follows: The word type is the type of the buffer
       * elements. The channel order is the canonical order of the
       * color space, and the channels are memory consecutive.
       *
       * The ownership of the buffer remains with the caller.
       *
       * Take a look at \c BufferedImage::new_image if you need more
       * control over the exact buffer format to use.
       *
       * \sa BufferedImage::new_image
       */
      template<typename T>
      static Ref copy_image_from(T *buffer, int width, int height,
                                 ColorSpace::ConstRefArg color_space = ColorSpace::get_RGB(),
                                 bool has_alpha = false);


      /**
       * Extract a block of pixels from the image and place it into
       * the specified pixel tray.
       *
       * \param tray The two dimensional array into which the
       * extracted pixels will be stored. The number of <tt>long
       * double</tt>s that will be writte to this buffer is precisely
       * \c width times \c height times the number of channels in the
       * color space. Pixels are stored row wise from left to right,
       * and from bttom to top.
       *
       * \param left, bottom The bottom left corner of the region to
       * extract. These are measured in pixels and the origin is the
       * bottom left corner of the image. Both are allowed to be
       * negative.
       *
       * \param width, height The width and height of the region to
       * extract. These are measuren in pixels.
       *
       * \param horizontal_repeat, vertical_repeat Defines the size of
       * the repetition compound. In the case where both \c
       * horizontal_repeat and \c vertical_repeat are positive
       * numbers, the repetition compound is the combination of N by M
       * pixel modules where N = <tt>horizontal_repeat</tt> and M =
       * <tt>vertical_repeat</tt>, and aligned such that the lower
       * left module is coincident with the principal module. If \c
       * horizontal_repeat is zero (the default) then the compound
       * stretches infinitely to the left and to the right, and
       * likewise \c vertical_repeat in the up/down direction.
       *
       * A 'pixel module' is either the pricipal module or one of its
       * repetitions.
       *
       * The 'repetition compound' is the combination of the pricipal
       * module and all its repetitions.
       *
       * The following illustrates how the part of the tray that falls
       * outside the principal module is filled in:
       *
       * <pre>
       *
       *     principal      only
       *      module     repetition
       *    ----------- -----------                       extracted
       *   | 1   2   3 | 1   2   3 |   tray                 tray
       *   |           |           |-------        -----------------------
       *   | 4   5   6 | 4   5   6 |       |      | 6   4   5   6   6   6 |
       *   |           |           |       |      |                       |
       *   | 7   8   9 | 7   8   9 |       |      | 9   7   8   9   9   9 |
       *    ----------- -----------        |      |                       |
       *           |                       |      | 9   7   8   9   9   9 |
       *           |                       |      |                       |
       *           |                       |      | 9   7   8   9   9   9 |
       *            -----------------------        -----------------------
       *
       * </pre>
       */
/*
      void get_block(double *tray, int left, int bottom, int width, int height,
                     int horizontal_repeat = 0, int vertical_repeat = 0) const;
*/

      /**
       * Write a block of pixels to the image. The pixels are passed
       * in the 'tray' buffer.
       *
       * \param tray The two dimensional array holding the pixels that
       * should be written into the image. The number of <tt>long
       * double</tt>s that will be read from the tray buffer is
       * precisely \c width times \c height times the number of
       * channels in the color space.
       *
       * \param left, bottom The bottom left corner of the target
       * region into which the pixels be written. These are
       * measured in pixels and the origin is the bottom left corner
       * of the image. Both are allowed to be negative.
       *
       * \param width, height The width and height of the target
       * region into which the pixels be written. These are measured
       * in pixels.
       *
       * \param horizontal_repeat, vertical_repeat Defines the size of
       * the repetition compound. In the case where both \c
       * horizontal_repeat and \c vertical_repeat are positive
       * numbers, the repetition compound is the combination of M by N
       * pixel modules where N = <tt>horizontal_repeat</tt> and M =
       * <tt>vertical_repeat</tt>, and aligned such that the lower
       * left module is coincident with the principal module. If \c
       * horizontal_repeat is zero (the default) then the compound
       * stretches infinitely to the left and to the right, and
       * likewise \c vertical_repeat in the up/down direction. Please
       * also see the detail description below.
       *
       *
       * In the case where the target region does lie completely
       * within the principal image the result depends on \c
       * horizontal_repeat and \c vertical_repeat. To understand the
       * effect, we must first construct the repetition compound. In
       * the case where \c horizontal_repeat and \c vertical_repeat
       * are both non-zero, the repetition compound is an M by N array
       * of image aliases placed edge to edge, where M =
       * <tt>vertical_repeat</tt> and N =
       * <tt>horizontal_repeat</tt>. The coordinates of the specified
       * target region must then be interpreted relative to the bottom
       * left corner of the compound. Any part of the target region
       * that lies beyond the compound is ignored (not written to the
       * image.)
       *
       * The effect can now be defined as what we would get by writing
       * the pixels into the compound one at a time in the reverse of
       * the order they occur in the tray. The fact that the compound
       * consits of aliases (or modules), means that data written, for
       * example, just beyond the right edge of the principal image,
       * is in fact written just beyond its left edge. The fact that
       * we write pixels in reverse tray order, means for example,
       * that if the tray is a little wider than the principal image
       * then pixels near the right edge of the tray will be
       * overwritten by pixels near the left side.
       *
       * If \c horizontal_repeat is zero, the compound is unbounded
       * both to the right and to the left, and the origin for
       * horizontal coordinates can be chosen as the left edge of an
       * arbitrary alias column. Likewise, if \c vertical_repeat is
       * zero, the compound is unbounded both to the upwards and
       * downwards, and the origin for vertical coordinates can be
       * chosen as the bottom edge of an arbitrary alias row.
       *
       * <pre>
       *
       *     principal      only              resulting
       *      image      repetition             image
       *    ----------- -----------          -----------
       *   |           |           |        |           |
       *   |    ---------------    |        |           |
       *   |   | 1   2   3   4 |   |        | 3   1   2 |
       *   |   |               |   |        |           |
       *   |   | 2   3   4   5 |   |        | 4   2   3 |
       *    ---|               |---          -----------
       *       | 3   4   5   6 |
       *       |               |
       *       | 4   5   6   7 | tray
       *        ---------------
       *
       * </pre>
       */
/*
      void put_block(double const *tray, int left, int bottom, int width, int height,
                     int horizontal_repeat = 0, int vertical_repeat = 0);
*/




      struct Codec;
      struct Release;
      typedef core::UniquePtr<Codec, Release> CodecPtr;
      typedef core::UniquePtr<Codec const, Release> CodecConstPtr;

      /**
       * All reading from an image and writing to an image must go
       * through a \c Codec object, thus a codec object must be
       * aquired before any access to the image data can happen. When
       * reading and/or writing is complete, the codec object must be
       * released by calling its \c release method. If multiple
       * successive accesses are needed, the codec object should be
       * kept, and reused for each operation, and only be released
       * when the last operation is done. This ensures maximum
       * efficiency.
       *
       * This scheme allows for extra resources (buffers) to be
       * allocated such that the operations can excute efficiently,
       * and also prevents these resources from having to be
       * permanently allocated in the image object.
       *
       * In the case of the standard buffered image implementation, no
       * extra resources are needed, and the method will simply return
       * a pointer to the image object itself, leading to minimal
       * overhead in this case.
       *
       * The lifetime of a codec object must not extend beyond the end
       * of the lifetime of the image from which it was
       * acquired. Thus, it is required that the codec object be
       * released before the image is destroyed.
       *
       * \note The codec object must not be used after the \c
       * Codec::release method has been called. This is because it
       * may have "committed suicide" upon return from the \c release
       * method (an entirely legal thing for it to do).
       */
      virtual CodecPtr acquire_codec() = 0;

      /**
       * Same as the non-const version, but can be called for const
       * images, and allows only read access.
       */
      CodecConstPtr acquire_codec() const;



    protected:

      static Ref new_image(void *buffer, WordType word_type, int width, int height,
                           ColorSpace::ConstRefArg color_space, bool has_alpha);



/*
      void read_block(core::Grid<double *> const &tray, int left, int bottom,
                      int horizontal_repeat, int vertical_repeat) const;
*/

      /**
       * This method is equivalent to <tt>put_block</tt>, except for
       * that more general tray structure having \c pitch and \c
       * stride properties. These are usefull when you want to address
       * a "submatrix" of the tray, or for filling the indicated
       * region with a single pixel. In the latter case you would set
       * both \c pitch and \c stride to 0, and \c tray points to the
       * fill pixel, which is the only pixel that will be fetched from
       * the tray.
       *
       * \sa put_block
       */
/*
      void write_block(core::Grid<double const *> const &tray, int left, int bottom,
                       int horizontal_repeat, int vertical_repeat);
*/



      /**
       * \todo FIXME: Part of obsolete scheme.
       *
       * This method serves only to allow derived classes to call the
       * virtual method of the same name on a different
       * instance.
       */
/*
      static void read_block(Image const *, core::Grid<double *> const &, int, int, int, int);
*/

      /**
       * \todo FIXME: Part of obsolete scheme.
       *
       * This method serves only to allow derived classes to call the
       * virtual method of the same name on a different
       * instance.
       */
/*
      static void write_block(Image *, core::Grid<double const *> const &, int, int, int, int);
*/


//      struct PixelOld;
    };


    /**
     * This class represets the lowest level of access to image
     * data. All image data access must ultimately go through one of
     * the methods of this class.
     *
     * The pixel format expected by both of the methods of this class
     * is as follows. The origin of the specified grid points to the
     * first byte of the pixel at the lower left corner of the
     * transferred block. The pixel at block coordinates <tt>x, y</tt>
     * is expected to be found at <tt>y * g.stride + x * g.pitch +
     * g.origin</tt>, where <tt>g</tt> is the specified grid. Each
     * pixel is expected to consist of N memory consecutive words of
     * type T, where N is the number returned by
     * <tt>Image::get_num_channels</tt>, and T is the type returned
     * by <tt>Image::get_word_type</tt>. If
     * <tt>Image::has_alpha_channel</tt> returns true, then the last
     * word always contains the alpha component, and the rest of the
     * words contain the components of the color space returned by
     * <tt>Image::get_color_space</tt> in their canonical order.
     *
     * Both methods assume that the addressed region falls entirely
     * within the image area, so the caller must ensure this in order
     * to prevent memory corruption.
     */
    struct Image::Codec
    {
      /**
       * The maximum number of pixels that may be requested to be
       * decoded or encoded during a single call to either of the two
       * methods. A value of 24*24 = 576 corresponds to a 2.25KB
       * buffer for RGBA using one byte per channel.
       */
      static size_t constexpr get_max_pixels_per_block() { return 24*24; }

      /**
       * Read a block of pixels from the image at the specified
       * position. In general this envolves a translation from the
       * format used in the pixel buffer to pixel transfer format.
       *
       * The source region is assumed to not be empty, and lie fully
       * within image area.
       */
      virtual void decode(Util::TupleGrid const &g, int w, int h, int x, int y) const = 0;

      /**
       * Write a block of pixels into the image at the specified
       * position. In general this envolves a translation from the
       * pixel transfer format to the format used in the pixel buffer.
       *
       * The target region is assumed to not be empty, and lie fully
       * within image area.
       */
      virtual void encode(Util::ConstTupleGrid const &g, int w, int h, int x, int y) = 0;

      /**
       * This method must be called to release the codec after
       * use. This is supposed to happen automatically by means of the
       * "smart pointer" <tt>CodecPtr</tt>.
       */
      virtual void release() const { delete this; }


      virtual ~Codec() {}
    };


    struct Image::Release
    {
      void operator()(Image::Codec const *p) const { if(p) p->release(); }
    };







    // Implementation

    inline Image::Ref Image::new_image(int w, int h, ColorSpace::ConstRefArg c, bool a, WordType t)
    {
      if(h < 0) h = w;
      return new_image(0,t,w,h,c,a);
    }

    template<typename T>
    inline Image::Ref Image::new_image(T *b, int w, int h, ColorSpace::ConstRefArg c, bool a)
    {
      return new_image(b, get_word_type_by_type<T>(), w, h, c, a);
    }

    template<typename T>
    inline Image::Ref Image::copy_image_from(T *b, int w, int h, ColorSpace::ConstRefArg c, bool a)
    {
      WordType const t = get_word_type_by_type<T>();
      Image::Ref img = new_image(w,h,c,a,t);
      img->put_image(new_image(b, get_word_type_by_type<T>(), w, h, c, a));
      return img;
    }



/*
    struct Image::PixelOld
    {
      PixelOld(unsigned long p, ColorSpace const *c, bool a)
      {
        double q[] = { (p>>16&0xFF)/255.0L, (p>>8&0xFF)/255.0L,
                       (p&0xFF)/255.0L, (255-(p>>24&0xFF))/255.0L };
        store(q,c,a);
      }
      PixelOld(double const *p, ColorSpace const *c, bool a)
      {
        if(a && c->is_rgb())
        {
          ptr = p;
          dyn = false;
        }
        else cvt(p,c,a);
      }
      template<typename T> PixelOld(T const *p, ColorSpace const *c, bool a)
      {
        double q[4] = { p[0], p[1], p[2], p[3] };
        store(q,c,a);
      }
      PixelOld(Math::BasicVector<4, double> const &p, ColorSpace const *c, bool a)
      {
        if(a && c->is_rgb())
        {
          ptr = &p[0];
          dyn = false;
        }
        else cvt(&p[0], c, a);
      }
      template<typename T, class R, class F>
      PixelOld(Math::_VecImpl::Base<3,T,R,F> const &p, ColorSpace const *c, bool a)
      {
        double q[4] = { p[0], p[1], p[2], 1 };
        store(q,c,a);
      }
      template<typename T, class R, class F>
      PixelOld(Math::_VecImpl::Base<4,T,R,F> const &p, ColorSpace const *c, bool a)
      {
        double q[4] = { p[0], p[1], p[2], p[3] };
        store(q,c,a);
      }
      void store(double const *p, ColorSpace const *c, bool a)
      {
        if(a && c->is_rgb())
        {
          std::copy(p, p+4, stat);
          ptr = stat;
          dyn = false;
        }
        else cvt(p,c,a);
      }
      void cvt(double const *p, ColorSpace const *c, bool a)
      {
        int const n = c->get_num_primaries() + (a?1:0);
        double *q;
        if(5 < n)
        {
          q = new double[n];
          dyn = true;
        }
        else
        {
          q = stat;
          dyn = false;
        }
        ptr = q;
        c->from_rgb(word_type_Double, true, a).cvt(p, q, 1);
      }
      bool dyn;
      double const *ptr;
      double stat[5];
      double const *get() const { return ptr; }
      ~PixelOld() { if(dyn) delete[] ptr; }
    };



    inline void Image::clear(int left, int bottom, int width, int height,
                             int horizontal_repeat, int vertical_repeat)
    {
      fill(0xFF000000, left, bottom, width, height, horizontal_repeat, vertical_repeat);
    }

    template<typename T>
    inline void Image::fill(T pixel, int left, int bottom, int width, int height,
                            int horizontal_repeat, int vertical_repeat)
    {
      PixelOld p(pixel, get_color_space().get(), has_alpha_channel());
      core::Grid<double const *> g(p.get(), width ? width : get_width(),
                                   height ? height : get_height(), 0, 0);
      write_block(g, left, bottom, horizontal_repeat, vertical_repeat);
    }



    inline void Image::get_pixel(double *pixel, int x, int y,
                                 int horizontal_repeat, int vertical_repeat) const
    {
      get_block(pixel, x, y, 1, 1, horizontal_repeat, vertical_repeat);
    }

    template<typename T>
    inline void Image::put_pixel(T pixel, int x, int y, int horizontal_repeat, int vertical_repeat)
    {
      put_block(PixelOld(pixel, get_color_space().get(), has_alpha_channel()).get(),
                x, y, 1, 1, horizontal_repeat, vertical_repeat);
    }



    inline void Image::get_block(double *tray, int left, int bottom, int width, int height,
                                 int horizontal_repeat, int vertical_repeat) const
    {
      int const pitch  = get_num_channels();
      read_block(core::Grid<double *>(tray, width, height, pitch, pitch*width),
                 left, bottom, horizontal_repeat, vertical_repeat);
    }

    inline void Image::put_block(double const *tray, int left, int bottom,
                                 int width, int height, int horizontal_repeat, int vertical_repeat)
    {
      int const pitch  = get_num_channels();
      write_block(core::Grid<double const *>(tray, width, height, pitch, pitch*width),
                  left, bottom, horizontal_repeat, vertical_repeat);
    }

*/


    inline Image::CodecConstPtr Image::acquire_codec() const
    {
      CodecConstPtr a(const_cast<Image *>(this)->acquire_codec().release());
      return a;
    }


/*
    inline void Image::read_block(Image const *source, core::Grid<double *> const &grid,
                                  int left, int bottom, int horizontal_repeat, int vertical_repeat)
    {
      source->read_block(grid, left, bottom, horizontal_repeat, vertical_repeat);
    }

    inline void Image::write_block(Image *target, core::Grid<double const *> const &grid,
                                   int left, int bottom, int horizontal_repeat, int vertical_repeat)
    {
      target->write_block(grid, left, bottom, horizontal_repeat, vertical_repeat);
    }



    inline void
    Image::decode(Image const *source, core::Grid<double *> const &grid, int left, int bottom)
    {
      source->decode(grid, left, bottom);
    }

    inline void
    Image::encode(Image *target, core::Grid<double const *> const &grid, int left, int bottom)
    {
      target->encode(grid, left, bottom);
    }
*/
  }
}

#endif // ARCHON_IMAGE_IMAGE_HPP
