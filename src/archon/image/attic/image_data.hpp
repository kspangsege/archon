/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_IMAGE_DATA_HPP
#define ARCHON_IMAGE_IMAGE_DATA_HPP

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <vector>

#include <archon/core/memory.hpp>
#include <archon/math/functions.hpp>
#include <archon/image/buffer_format.hpp>

namespace archon
{
  namespace image
  {
    /**
     * A highly elaborate and configurable image data accessor with
     * the ability to describe image data layout in a very general and
     * flexible way.
     *
     *
     * <h3>Pixel strips</h3>
     *
     * At the top-most level image data is organized (stored in
     * memory) as a sequence of pixel strips. Strips are either meant
     * to be displayed horizontally (horizontal strips) or vertically
     * (vertical strips). This is controlled by a single flag.
     *
     * The pixels in a strip are stored consecutively in memory. Every
     * strip contains the same number of pixels and occupy the same
     * amount of memory (number of bytes or bits, depending on
     * format). The distance (in number of bytes or bits, depending on
     * format) between two consecutive strips is the 'stride' and is
     * constant throughout the image.
     *
     * When strips are meant to be displayed horizontally we say that
     * the pixel layout is y-major (or row-major), because an increase
     * in the y-coordinate signifies a major advance in memory address
     * comparend to an increase in the x-coordinate. Similarly, if
     * strips are meant to be displayed vertically, we say that the
     * pixel layout is x-major (or column-major).
     *
     *
     * <h3>Buffer format</h3>
     *
     * Coming soon...
     *
     * \sa BufferFormat
     *
     *
     * <h3>Pixel format</h3>
     *
     * Coming soon...
     *
     * \sa PixelFormat
     *
     *
     * <h3>Endianness</h3>
     *
     * Endianess is all about the way bytes are ordered when combined
     * into wider elements. Every hardware architecture has a specific
     * endianess. The most common are big endian and little endian
     * architectures:
     *
     * <dl>
     *
     * <dt>Little endian.</dt> <dd>The least significant part is
     * stored at the memory location with the lowest address (Intel
     * x86, MOS Technology 6502, DEC VAX).</dd>
     *
     * <dt>Big endian.</dt> <dd>The most significant part is stored at
     * the memory location with the lowest address (Motorola 68000,
     * SPARC, System/370).</dd>
     *
     * </dl>
     *
     * Unfortunately there are other type of endianness than
     * these. Some architectures order bytes in one way when combined
     * to double-bytes and order double-bytes in the opposite order
     * when combining them into quadruple bytes.
     *
     * For the sake of maximum flexibility this class supports all
     * types of endianness that can be described by a sequence of
     * ordering flags, one for each level of combination.
     *
     * Native endianness is the default and the best choise when you
     * want good performance, since it avoids the cumbersome byte
     * reordering.
     *
     * The reason one would ever specify an explicit endianness might
     * be to access data that was generated on a different
     * architecture, or to be able to inteface to systems/libraries
     * that for one reason or another deals with image data with an
     * alien endianess.
     *
     * The endianness specification affects only the formation of
     * words of the specified type from bytes in the image
     * buffer. Thus, it is ignored if the selected word type is
     * byte/char.
     *
     * \sa WordType
     * \sa detectNativeEndianness()
     * \sa http://en.wikipedia.org/wiki/Endianness
     *
     *
     * <h3>Frame of interest</h3>
     *
     * In some situations it is desireable to limit access to a
     * particular region in the image.  This is possible by means of
     * the "frame of interest" feature of this class. The frame is
     * specified by its width and height in pixels, and by the
     * displacement of its lower left corner from the lower left
     * corner of the full image.
     *
     * A good example is texture mapping where we sample colors from
     * an image, and we generally expect that a horizontal coordinate
     * of 1 corresponds with the right edge of the image. However
     * several textures are often combined into a single image
     * requireing coordinate transformations on our side to access a
     * specific sub-picture. A much better idea is to utilize the
     * frame of interest feature of this class - especially since it
     * also handles the problem of color bleeding near the edges.
     *
     * If you leave the frame of interest parameters at their defaults
     * it will always be coincident with the full image.
     *
     * The frame of interest is what defines the contents of the
     * principal image. See below.
     *
     * It is by no means possible to address pixels outside the frame
     * of interest, not even partially (no color bleeding).
     *
     *
     * <h3>Principal image</h3>
     *
     * The principal image is an important concept when reading pixels
     * from the buffer. It is defined by its position in the infinite
     * 2-D coordinate space. The principal image is always located in
     * coordinate space such that the lower left corner is at (0,
     * 0). With integer coordinates the upper right corner is always
     * at (width-1, height-1). When using continuous coordinates the
     * upper right corner is still at (width-1, height-1) by default
     * but is in general a function of the currently chosen coordinate
     * transformation.
     *
     * Many pixel access methods accept two optional arguments:
     * horizontalRepeat and verticalRepeat. Setting horizontalRepeat
     * to 0 has the effect of repeating the principal image infinitely
     * both to the right and to the left. Likewise, setting
     * verticalRepeat to 0 will repeat it infinitely in the vertical
     * direction. If both are set to zero, the entire infinit 2-D
     * plane will be tiled with the principal image.
     *
     * If you choose a non-zero value n for the horizontal repeat the
     * the principal image and its vertical replica will be repeated
     * n-1 times to the right. Reading pixels from coordinates beyond
     * the right edge of the rightmost replica, has the same efefct as
     * reading the last pixel directly to the left.
     *
     * In general, the right-most column of pixels in a finite array
     * of replica is repeated infinitely to the right, the top-most
     * row is repeated infinitely upwards, and so on. Each corner
     * pixel is used to fill the remaing four corner areas
     * respectively.
     *
     * When sampling with continuous coordinates near the right edge
     * of a finite array of replica no color bleeding from the
     * opposite edge will occur, which is the way it should be.
     *
     * When sampling with continuous coordinates near the transition
     * from one replica to the next color bleeding from the opposite
     * side will occur, which indeed is also the way it should be.
     *
     *
     * \note In the following the word 'byte' is used to mean exactly
     * the same thing as 'char'. That is, the smallest addressible
     * unit of memory. This very often corresponds to an octet, but
     * not always.
     *
     * \todo The following interesting feature regarding conversion
     * between different bit width of integer representations of the
     * closed continuous unit interval [0;1] might become usefull:
     * When converting from N bit representation to M bit
     * representation where N < M (widening conversion) use the N bits
     * as the most significant bits then repeat N for lower
     * significance bits as many times as is needed to reach M. the
     * last repetition will generally be truncated. For example b3 b2
     * b1 b0 where b3 is the most significant bit becomes b3 b2 b1 b0
     * b3 b2 b1 b0 b3 in 9-bit representation. When M < N simply cut
     * away the N-M least significant bits.
     *
     * <PRE>
     *
     *            --------------- ---------------
     *           | d   e   f   g | d   e   f   g |
     *    -------|       3       |       4       |
     *   | a   b | e   f   g   h | e   f   g   h |
     *   |        --------------- ---------------
     *   | b   c | d   e   f   g | d   e   f   g |
     *   |       |       1       |       2       |
     *   | c   d | e   f   g   h | e   f   g   h |
     *   |        --------------- ---------------
     *   | d   e   f   g   h   i   j   k |
     *    -------------------------------
     *     8 x 4 array of pixel data
     *
     *   1              -  frame of interest / principal module
     *   2, 3, 4        -  repeated modules
     *   1 + 2 + 3 + 4  -  repetition compound
     *
     * </PRE>
     *
     * 
     */
    struct ImageData
    {
      /**
       * \param buffer A pointer to the start of the underlaying pixel
       * buffer. This pointer must be aligned to the chosen word
       * type. The word size is specified through the pixel
       * format. The buffer must contain at least as many words as the
       * number returned by getMinimumBufferSizeInWords. The buffer is
       * not owned by this image data wrapper, so deallocation is the
       * responsibility of the caller.
       *
       * \param pixelsPerStrip The number of pixel in each strip (row
       * or column) in the buffer.
       *
       * \param numberOfStrips The number of pixel strips (rows or
       * columns) contained in the buffer.
       *
       * \param pixelFormat A description of the pixel format of the
       * specified buffer. See PixelFormat.
       *
       * \param bufferFormat A description of the buffer format. See
       * BufferFormat.
       *
       * \param left The offset in pixels of the left edge of the
       * frame of interest from the left edge of the underlaying
       * image. May not be negative. The sum of the left offset and
       * the width must not exceed the width of the underlaying image.
       *
       * \param bottom The offset in pixels of the bottom edge of the
       * frame of interest from the bottom edge of the underlaying
       * image. May not be negative. The sum of the bottom offset and
       * the height must not exceed the height of the underlaying
       * image.
       *
       * \param width The width in pixels of the frame of interest. A
       * value of zero adjusts the width so that the right edge
       * becomes aligned with the right edge of the underlaying
       * image. The sum of the left offset and the width must not
       * exceed the width of the underlaying image.
       *
       * \param height The height in pixels of the frame of
       * interest. A value of zero adjusts the height so that the top
       * edge becomes aligned with the top edge of the underlaying
       * image. The sum of the bottom offset and the height must not
       * exceed the height of the underlaying image.
       *
       * \param endianness The endianness of the image data. Please
       * see detectNativeEndianness() for a description of the format
       * used to describe endianness. An empty vector indicates that
       * the native endianness of your architecture applies.
       *
       *
       * FIXME: THE FOLLOWING IS WRONG !!!!!!!!!!! MUST INSTEAD CHECK left, bottom, width and height
       *
       * \throw invalid_argument If the specified majorPitch was too
       * small. For row-major buffers (default) majorPitch must
       * greater than or equal to the sum of the specified width and
       * the horizontal offset. For column-major buffers majorPitch
       * must greater than or equal to the sum of height and vertical
       * offset.
       *
       * \sa getMinimumBufferSizeInWords
       */
      ImageData(void *buffer, int pixelsPerStrip, int numberOfStrips,
                BufferFormat::ConstRefArg bufferFormat = BufferFormat::newDefaultFormat(),
                int left = 0, int bottom = 0, int width = 0, int height = 0,
                std::vector<bool> const &endianness = std::vector<bool>());

      /**
       * Get the width of this image or more precisely, the width of
       * the interest frame. See the description of interest frame
       * elsewhere.
       *
       * \return The width of the image in pixels.
       */
      int getWidth() const { return interestWidth; }

      /**
       * Get the height of this image or more precisely, the height of
       * the interest frame. See the description of interest frame
       * elsewhere.
       *
       * \return The height of the image in pixels.
       */
      int getHeight() const { return interestHeight; }

      /**
       * Get the pixel format of this image in its expanded form.
       *
       * \return The pixel format used by this image.
       *
       * \sa PixelFormat::getExpandedFormat
       */
//      PixelFormat const &getPixelFormat()  const { return pixelFormat;  }

      /**
       * Get the buffer format of this image.
       *
       * \return The buffer format used by this image.
       */
//      BufferFormat const &getBufferFormat() const { return bufferFormat; }

/*
      ImageData getSubImage(int left = 0, int bottom = 0,
                            int width = 0, int height = 0,
                            std::vector<int> const &channelMap = std::vector<int>(),
                            PixelFormat::ColorSpace colorSpace = PixelFormat::implied,
                            bool hasAlphaChannel = false) const;
*/

      /**
       * You may use this method to determine the size of the buffer
       * needed for an image with properties as specified for this
       * image data wrapper.
       *
       * In this case you would probably write something like the
       * following:
       *
       * <PRE>
       *
       *   PixelFormat f;
       *   f.wordType = PixelFormat::std_float;
       *   ImageData image(0, width, height, f); // No buffer yet
       *   void *buffer = new float[image.getMinimumBufferSizeInWords()];
       *   image.setBuffer(buffer);
       *
       *   // access your image data happily from here...
       *
       * </PRE>
       *
       * \sa setBuffer
       */
      size_t getMinimumBufferSizeInWords() const;

      /**
       * Use a new pixel buffer for this image data wrapper. The
       * passed pointer must be aligned to the chosen word type and
       * must contain at least as many words as indicated by
       * getMinimumBufferSizeInWords. The easiest way to get such a
       * bufefr is to use the allocateBuffer method of this class.
       *
       * \note This image data wrapper never owns the buffer, so any
       * previously specified buffer is not deallocated by this
       * object, neither is the new buffer.
       *
       * \param b The new buffer to be used by this image data
       * wrapper.
       *
       * \sa allocateBuffer
       * \sa getMinimumBufferSizeInWords
       */
      void setBuffer(void *b)
      {
        buffer = b;
      }

      /**
       * Allocate and return a buffer that is properly aligned for
       * this image format and is as big as is required.
       */
      void *allocateBuffer() const;

      /**
       * Fetch the color at the specified floating point
       * position. This will in general be a bilinear interpolation
       * over 4 (2 by 2) nearby pixels.
       *
       * This method is well suited for situations where images are
       * used as texture maps in such places as raytracers.
       *
       * \param x Floating point horizontal coordinate. '0' maps to the
       * left edge and '1' to the right edge.
       *
       * \param y Floating point vertical coordinate. '0' maps to the
       * bottom edge and '1' to the top edge.
       *
       * \todo FIXME: This does not makes sense for any color space, so
       * we must perform conversion to RGB when required.
       */
      void interpolateLinear(double x, double y, long double *pixel,
                             int horizontalRepeat = 0, int verticalRepeat = 0) const
      {
        // Scale to image size and align with integer coordinats
        x = x * interestWidth  - 0.5;
        y = y * interestHeight - 0.5;

        // Determine integer coordinates of lower left pixel of
        // relevant 2x2 pixel array within image.
        int const xi = static_cast<long>(floor(x));
        int const yi = static_cast<long>(floor(y));

        // Fetch all 4 pixels in the target 2x2 pixel array
        int const n = numberOfChannels;
        core::Array<long double> tray(4*n);
        getPixels(xi, yi, tray.get(), 2, 2, horizontalRepeat, verticalRepeat);

        // Get the fractional parts (intra pixel positions)
        long double const xf1 = x - xi;
        long double const yf1 = y - yi;
        long double const xf0 = 1 - xf1;
        long double const yf0 = 1 - yf1;

        // The weight w0 is determined as the area of intersection
        // between the real pixel p0 = (xi, yi) and the virtual pixel p
        // = (x, y).
        long double const w0 = xf0 * yf0;
        long double const w1 = xf1 * yf0;
        long double const w2 = xf0 * yf1;
        long double const w3 = xf1 * yf1;

        // Produce the weighted mean color
        long double *const c0 = tray.get();
        long double *const c1 = c0+n;
        long double *const c2 = c1+n;
        long double *const c3 = c2+n;
        for(int i=0; i<n; ++i)
          pixel[i] = w0 * c0[i] + w1 * c1[i] + w2 * c2[i] + w3 * c3[i];
      }

      /**
       * Fetch the color at the specified floating point
       * position. Each color component in the output pixel is is
       * computed as a bicubic interpolation over the corresponding
       * components of the 16 (4 by 4) nearby pixels.
       *
       * This method is well suited for situations where images are
       * used as texture maps in such places as raytracers.
       *
       * \param x Floating point horizontal coordinate. '0' maps to the
       * left edge and '1' to the right edge.
       *
       * \param y Floating point vertical coordinate. '0' maps to the
       * bottom edge and '1' to the top edge.
       *
       * \todo FIXME: This does not makes sense for any color space, so
       * we must perform conversion to RGB when required.
       */
      void interpolateCubic(double x, double y, long double *pixel,
                             int horizontalRepeat = 0, int verticalRepeat = 0) const
      {
        // Scale to image size and align with integer coordinats
        x = x * interestWidth  - 0.5;
        y = y * interestHeight - 0.5;

        // Determine integer coordinates of principal pixel of
        // relevant 4x4 pixel array within image.
        int const xi = static_cast<long>(floor(x));
        int const yi = static_cast<long>(floor(y));

        // Fetch all 16 pixels in the target 4x4 pixel array
        int const n = numberOfChannels;
        core::Array<long double> tray(16*n);
        getPixels(xi-1, yi-1, tray.get(), 4, 4,
                  horizontalRepeat, verticalRepeat);

        // Get the fractional parts (intra pixel positions)
        long double const xf = x - xi;
        long double const yf = y - yi;

        // Cubic weights for the horizontal direction
        long double const x0 = ((2-xf)*xf-1)*xf;    // -1
        long double const x1 = (3*xf-5)*xf*xf+2;    //  0
        long double const x2 = ((4-3*xf)*xf+1)*xf;  // +1
        long double const x3 = (xf-1)*xf*xf;        // +2

        // Cubic weights for the vertical direction
        long double const y0 = ((2-yf)*yf-1)*yf;    // -1
        long double const y1 = (3*yf-5)*yf*yf+2;    //  0
        long double const y2 = ((4-3*yf)*yf+1)*yf;  // +1
        long double const y3 = (yf-1)*yf*yf;        // +2

        for(int i=0; i<n; ++i)
        {
          long double *t = tray.get() + i;
          long double const z0 = x0 * t[0*n] + x1 * t[1*n] + x2 * t[2*n] + x3 * t[3*n];
          t += 4*n;
          long double const z1 = x0 * t[0*n] + x1 * t[1*n] + x2 * t[2*n] + x3 * t[3*n];
          t += 4*n;
          long double const z2 = x0 * t[0*n] + x1 * t[1*n] + x2 * t[2*n] + x3 * t[3*n];
          t += 4*n;
          long double const z3 = x0 * t[0*n] + x1 * t[1*n] + x2 * t[2*n] + x3 * t[3*n];
          pixel[i] = 0.25 * (y0*z0 + y1*z1 + y2*z2 + y3*z3);
        }
      }

      /**
       * Extract a rectangular region of pixels into the specified
       * pixel tray.
       *
       * \param left, bottom The bottom left corner of the region to
       * extract. These are measured in pixels and the origin is the
       * bottom left corner of the image. Both are allowed to be
       * negative.
       *
       * \param tray The two dimensional array of pixels being
       * extracted from the image.
       *
       * \param width, height The width and height of the region to
       * extract. these are measuren in pixels.
       *
       * A 'pixel module' is either the pricipal image or one of its
       * repetitions.
       *
       * The 'repetition compound' is the combination of the pricipal image
       * and all its repetitions.
       *
       * The following illustrates how the part of the tray that falls
       * outside the principal image is filled in:
       *
       * <PRE>
       *
       *     principal      only
       *       image     repetition
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
       * </PRE>
       */
      void getPixels(int left, int bottom, long double *tray,
                     int width = 1, int height = 1,
                     int horizontalRepeat = 0,
                     int verticalRepeat = 0) const
      {
        int const pitch  = numberOfChannels;
        int const stride = pitch * width;

        // If pixel tray lies completely outside the repetition compound
        // translate tray appropriately to produce a one pixel overlap
        // with compound edge/corner.

        // Distance from left edge of tray to left edge of
        // compound/tray intersection
        int leftFallOff = 0;

        // Distance from right edge of tray to right edge of
        // compound/tray intersection
        int rightFallOff = 0;

        // Distance from bottom edge of tray to bottom edge of
        // compound/tray intersection
        int bottomFallOff = 0;

        // Distance from top edge of tray to top edge of compound/tray
        // intersection
        int topFallOff = 0;

        if(horizontalRepeat)
        {
          int fallOff = -left;
          if(0 < fallOff)
          {
            if(width <= fallOff)
            {
              leftFallOff = width - 1;
              width = 1;
            }
            else
            {
              leftFallOff = fallOff;
              width -= fallOff;
            }
            left = 0;
            tray += pitch * leftFallOff;
          }

          int const compound  = horizontalRepeat * interestWidth;
          fallOff = left + width - compound;
          if(0 < fallOff)
          {
            if(width <= fallOff)
            {
              rightFallOff = width - 1;
              left = compound - 1;
              width = 1;
            }
            else
            {
              rightFallOff = fallOff;
              width -= fallOff;
            }
          }
        }

        if(verticalRepeat)
        {
          int fallOff = -bottom;
          if(0 < fallOff)
          {
            if(height <= fallOff)
            {
              bottomFallOff = height - 1;
              height = 1;
            }
            else
            {
              bottomFallOff = fallOff;
              height -= fallOff;
            }
            bottom = 0;
            tray += stride * bottomFallOff;
          }

          int const compound = verticalRepeat * interestHeight;
          fallOff = bottom + height - compound;
          if(0 < fallOff)
          {
            if(height <= fallOff)
            {
              topFallOff = height - 1;
              bottom = compound - 1;
              height = 1;
            }
            else
            {
              topFallOff = fallOff;
              height -= fallOff;
            }
          }
        }

        // Now there is a non-empty intersection between the pixel
        // tray and the repetition compound.

        // Considder the bottom most, left most pixel module that has a
        // non-empty intersection with the pixel tray. Call this
        // intersection I.

        int transferLeft   = math::modulo<int>(left,   interestWidth);
        int transferBottom = math::modulo<int>(bottom, interestHeight);

        int transferWidth  = interestWidth  - transferLeft;
        int transferHeight = interestHeight - transferBottom;

        int const protoWidth  = std::min(interestWidth,  width);
        int const protoHeight = std::min(interestHeight, height);

        int const w = protoWidth  - transferWidth;
        int const h = protoHeight - transferHeight;

        if(w < 0) transferWidth  = protoWidth;
        if(h < 0) transferHeight = protoHeight;

        // If I is not left-aligned with module and the compound has
        // another module to the right and the intersection between
        // this module and the tray is non-empty and right-aligned
        // with the module then raise flag 'right' and step to the
        // right (new I).

        int protoLeft = 0; // Distance from left edge of compound/tray intersection to left edge of proto frame
        if(transferLeft && transferWidth + interestWidth <= width)
        {
          protoLeft = transferWidth;
          transferLeft = 0;
          transferWidth = interestWidth;
          tray += pitch * protoLeft;
        }

        // If I is not bottom-aligned with module and compound has
        // another module above and the intersection between this
        // module and the tray is non-empty and top-aligned with
        // module then raise flag 'up' and step up (new I).

        int protoBottom = 0; // Distance from bottom edge of compound/tray intersection to bottom edge of proto frame
        if(transferBottom && transferHeight + interestHeight <= height)
        {
          protoBottom = transferHeight;
          transferBottom = 0;
          transferHeight = interestHeight;
          tray += stride * protoBottom;
        }

        // Transfer I to pixel tray.

        decodePixelArray(transferLeft, transferBottom,
                         transferWidth, transferHeight, tray, stride);

        // If flag 'right' is not raised and I is not left-aligned
        // with module, and intersection to the right is non-empty then
        // raise flag 'right2' and transfer pixels to tray but at most
        // such that the total with of pixels transferred to tray
        // becomes equal to module width.

        // If flag 'up' is not raised and I is not bottom-aligned with
        // module, and intersection above is non-empty then raise flag
        // 'up2' and transfer pixels to tray but at most such that the
        // total height of pixels transferred to tray becomes equal to
        // module height.

        // If both flags 'right2' and 'up2' are raised then transfer
        // pixels to tray from module beyond the upper right corder of
        // I.

        if(!protoLeft && 0 < w)
        {
          decodePixelArray(0, transferBottom, w, transferHeight,
                           tray + pitch * transferWidth, stride);

          if(!protoBottom && 0 < h)
          {
            long double *const t = tray + stride * transferHeight;
            decodePixelArray(transferLeft, 0, transferWidth, h, t, stride);
            decodePixelArray(0, 0, w, h, t + pitch * transferWidth, stride);
          }
        }
        else if(!protoBottom && 0 < h)
          decodePixelArray(transferLeft, 0, transferWidth, h,
                           tray + stride * transferHeight, stride);

        // At this point we have stored a region R of pixels in the
        // tray whose width is protoWidth, and whose height is
        // protoHeight. Also 'tray' point to the tray pixel at the
        // lower left corner of the R.

        // If flag 'right' was raised then copy tray pixels from R to
        // fill gap to the left.

        if(protoLeft)
        {
          long double *t = tray;
          size_t const n = pitch * protoWidth;
          size_t const m = pitch * protoLeft;
          for(int i=0; i<protoHeight; ++i)
          {
            repeatBackward(t, n, m);
            t += stride;
          }
        }

        // Handle left compound edge fall-offs by copying tray pixels
        // from left compound edge.

        if(leftFallOff)
        {
          long double *t = tray - pitch * protoLeft;
          size_t const n = pitch;
          size_t const m = pitch * leftFallOff;
          for(int i=0; i<protoHeight; ++i)
          {
            repeatBackward(t, n, m);
            t += stride;
          }
        }

        // Copy tray pixels from R to fill up to the right tray-compond intersection edge.

        int const protoRight = width - protoLeft - protoWidth;
        if(protoRight)
        {
          long double *t = tray;
          size_t const n = pitch * protoWidth;
          size_t const m = pitch * protoRight;
          for(int i=0; i<protoHeight; ++i)
          {
            repeatForward(t, n, m);
            t += stride;
          }
        }

        // Handle right compound edge fall-offs by copying tray pixels
        // from right compound edge.

        if(rightFallOff)
        {
          long double *t = tray + pitch * (protoWidth + protoRight - 1);
          size_t const n = pitch;
          size_t const m = pitch * rightFallOff;
          for(int i=0; i<protoHeight; ++i)
          {
            repeatForward(t, n, m);
            t += stride;
          }
        }

        // At this point we have stored a region R2 of pixels in the
        // tray with full width.

        tray -= pitch * (leftFallOff + protoLeft);

        // If flag 'up' was raised then copy tray pixels from R2 to
        // fill gap below.

        if(protoBottom) repeatBackward(tray, stride * protoHeight,
                                       stride * protoBottom);

        // Handle bottom compound edge fall-offs by copying tray pixels
        // from bottom compound edge.

        if(bottomFallOff) repeatBackward(tray - stride * protoBottom,
                                         stride, stride * bottomFallOff);

        // Copy tray pixels from R2 to fill up to the upper
        // tray-compond intersection edge.

        int const protoTop = height - protoBottom - protoHeight;
        if(protoTop) repeatForward(tray, stride * protoHeight,
                                   stride * protoTop);

        // Handle upper compound edge fall-offs by copying tray pixels
        // from upper compound edge.

        if(topFallOff) repeatForward(tray + stride*(protoHeight+protoTop-1),
                                     stride, stride * topFallOff);

        // At this point we have filled the entire tray and we are
        // done.
      }


      void putPixels(int left, int bottom, long double const *tray,
                     int width = 1, int height = 1,
                     int horizontalRepeat = 0,
                     int verticalRepeat = 0) const
      {
        int const pitch  = numberOfChannels;
        int const stride = pitch * width;

        // Determine compound/tray intersection

        if(horizontalRepeat)
        {
          if(left < 0)
          {
            width += left;
            if(width < 1) return;
            tray -= pitch * left;
            left = 0;
          }

          int const right = left + width - horizontalRepeat * interestWidth;
          if(0 < right)
          {
            width -= right;
            if(width < 1) return;
          }
        }

        if(verticalRepeat)
        {
          if(bottom < 0)
          {
            height += bottom;
            if(height < 1) return;
            tray -= stride * bottom;
            bottom = 0;
          }

          int const top = bottom + height - verticalRepeat * interestHeight;
          if(0 < top)
          {
            height -= top;
            if(height < 1) return;
          }
        }

        // Now there is a non-empty intersection between the pixel
        // tray and the repetition compound.

        // Find module coordinates of lower left corner of intersection

        int const transferLeft   = math::modulo<int>(left,   interestWidth);
        int const transferBottom = math::modulo<int>(bottom, interestHeight);

        int transferWidth  = interestWidth  - transferLeft;
        int transferHeight = interestHeight - transferBottom;

        if(interestWidth  < width)  width  = interestWidth;
        if(interestHeight < height) height = interestHeight;

        int const w = width  - transferWidth;
        int const h = height - transferHeight;

        if(w < 0) transferWidth  = width;
        if(h < 0) transferHeight = height;

        encodePixelArray(transferLeft, transferBottom,
                         transferWidth, transferHeight, tray, stride);

        if(0 < w)
        {
          encodePixelArray(0, transferBottom, w, transferHeight,
                           tray + pitch * transferWidth, stride);

          if(0 < h)
          {
            tray += stride * transferHeight;
            encodePixelArray(transferLeft, 0, transferWidth, h, tray, stride);
            encodePixelArray(0, 0, w, h, tray + pitch * transferWidth, stride);
          }
        }
        else if(0 < h)
          encodePixelArray(transferLeft, 0, transferWidth, h,
                           tray + stride * transferHeight, stride);
      }


/*
      struct FramePartOperator
      {
        void operator()(const ImageData *image,
                        int partLeft, int partBottom,
                        int partWidth, int partHeight,
                        int frameLeft, int frameBottom) const;
      };

      void forEachFramedPart(int left, int bottom,
                             int width, int height,
                             int horizontalRepeat,
                             int verticalRepeat,
                             FramePartOperator oper) const
      {
//        IDEA IS TO USE THIS AS A BASIS OF BOTH putPixels AND NEW modifyPixels.

//      NEED TO DEAL WITH FRAMES WHICH ARE DISJOINT WITH THE REPETITION COMPOUND - IN THIS CASE WE MUST MAKE A ONE PIXEL WIDE OVERLAP WITH COMPOUND - THIS IS NOT CURRENTLY DONE IN putPixels - IS THIS NOT AN ERROR?

        int frameLeft   = 0;
        int frameBottom = 0;

        // Determine compound/frame intersection

        // If compound is horizontally finite
        if(horizontalRepeat)
        {
          int fallOff = -left;
          if(0 < fallOff)
          {
            if(width <= fallOff)
            {
              leftFallOff = width - 1;
              width = 1;
            }
            else
            {
              leftFallOff = fallOff;
              width -= fallOff;
            }
            left = 0;
            tray += pitch * leftFallOff;
          }

          int const compound  = horizontalRepeat * interestWidth;
          fallOff = left + width - compound;
          if(0 < fallOff)
          {
            if(width <= fallOff)
            {
              rightFallOff = width - 1;
              left = compound - 1;
              width = 1;
            }
            else
            {
              rightFallOff = fallOff;
              width -= fallOff;
            }
          }
        }

        // If compound is vertically finite
        if(verticalRepeat)
        {
          int fallOff = -bottom;
          if(0 < fallOff)
          {
            if(height <= fallOff)
            {
              bottomFallOff = height - 1;
              height = 1;
            }
            else
            {
              bottomFallOff = fallOff;
              height -= fallOff;
            }
            bottom = 0;
            tray += stride * bottomFallOff;
          }

          int const compound = verticalRepeat * interestHeight;
          fallOff = bottom + height - compound;
          if(0 < fallOff)
          {
            if(height <= fallOff)
            {
              topFallOff = height - 1;
              bottom = compound - 1;
              height = 1;
            }
            else
            {
              topFallOff = fallOff;
              height -= fallOff;
            }
          }
        }


        if(horizontalRepeat)
        {
          if(left < 0)
          {
            width += left;
            frameLeft = -left;
            left = 0;
            if(width < 1) width = 1;
          }

          int const right = left + width - horizontalRepeat * interestWidth;
          if(0 < right)
          {
            width -= right;
            if(width < 1)
            {
              left
              width = 1;
            }
          }
        }

        if(verticalRepeat)
        {
          if(bottom < 0)
          {
            height += bottom;
            if(height < 1) return;
            frameBottom = -bottom;
            bottom = 0;
          }

          int const top = bottom + height - verticalRepeat * interestHeight;
          if(0 < top)
          {
            height -= top;
            if(height < 1) return;
          }
        }

        // Now there is a non-empty intersection between the pixel
        // frame and the repetition compound.

        // Find module coordinates of lower left corner of intersection

        int const partLeft   = math::modulo<int>(left,   interestWidth);
        int const partBottom = math::modulo<int>(bottom, interestHeight);

        int partWidth  = interestWidth  - partLeft;
        int partHeight = interestHeight - partBottom;

        if(interestWidth  < width)  width  = interestWidth;
        if(interestHeight < height) height = interestHeight;

        int const w = width  - partWidth;
        int const h = height - partHeight;

        if(w < 0) partWidth  = width;
        if(h < 0) partHeight = height;

        (this->*operation)(partLeft, partBottom, partWidth, partHeight,
                           frameLeft, frameBottom);

        if(0 < w)
        {
          (this->*operation)(0, partBottom, w, partHeight,
                             frameLeft + partWidth, frameBottom);

          if(0 < h)
          {
            frameBottom += partHeight;
            (this->*operation)(partLeft, 0, partWidth, h, frameLeft, frameBottom);
            (this->*operation)(0, 0, w, h, frameLeft + partWidth, frameBottom);
          }
        }
        else if(0 < h)
          (this->*operation)(partLeft, 0, partWidth, h,
                             frameLeft, frameBottom + partHeight);
      }
*/

      /**
       * Color space conversion: If source and target color spaces
       * are identical (including the precense of an alpha channel)
       * all color channels are copied one-to-one, otherwise the
       * source pixels are converted to RGBA then to the target color
       * space.
       */
      void putImage(ImageData const *source,
                    int targetLeft  = 0, int targetBottom = 0,
                    int sourceLeft  = 0, int sourceBottom = 0,
                    int sourceWidth = 0, int sourceHeight = 0,
                    int sourceHorizontalRepeat = 0,
                    int sourceVerticalRepeat   = 0,
                    int targetHorizontalRepeat = 0,
                    int targetVerticalRepeat   = 0) const
      {
        if(!sourceWidth)  sourceWidth  = source->interestWidth;
        if(!sourceHeight) sourceHeight = source->interestHeight;

        int const trayWidth  = std::min(sourceWidth,  32);
        int const trayHeight = std::min(sourceHeight, 32);

        core::Array<long double> sourceTray, intermediateTray, targetTray;

        long double *effectiveSourceTray, *effectiveTargetTray;

        bool toRgba = false, fromRgba = false;

        if(source->pixelFormat.colorSpace == pixelFormat.colorSpace &&
           source->pixelFormat.hasAlphaChannel == pixelFormat.hasAlphaChannel &&
           (pixelFormat.colorSpace != PixelFormat::custom ||
            source->numberOfChannels == numberOfChannels))
        {
          // Compatible color spaces - only one tray is needed
          intermediateTray.reset(numberOfChannels*trayWidth*trayHeight);
          effectiveSourceTray = intermediateTray.get();
          effectiveTargetTray = intermediateTray.get();
        }
        else
        {
          // Incompatible color spaces - an intermediate tray for RGBA is needed
          intermediateTray.reset(4*trayWidth*trayHeight);
          effectiveSourceTray = intermediateTray.get();
          effectiveTargetTray = intermediateTray.get();
          if(source->nativeToRgba)
          {
            // Source color space is not RGBA - conversion is needed
            sourceTray.reset(source->numberOfChannels*trayWidth*trayHeight);
            effectiveSourceTray = sourceTray.get();
            toRgba = true;
          }
          if(rgbaToNative)
          {
            // Target color space is not RGBA - conversion is needed
            targetTray.reset(numberOfChannels*trayWidth*trayHeight);
            effectiveTargetTray = targetTray.get();
            fromRgba = true;
          }
        }

        int y = sourceHeight;
        do
        {
          int h = std::min(trayHeight, y);
          y -= h;

          int x = sourceWidth;
          do
          {
            int w = std::min(trayWidth, x);
            x -= w;

            source->getPixels(sourceLeft+x, sourceBottom+y,
                              effectiveSourceTray, w, h,
                              sourceHorizontalRepeat,
                              sourceVerticalRepeat);

            if(toRgba)
              (source->*source->nativeToRgba)(sourceTray.get(),
                                              intermediateTray.get(), w*h);

            if(fromRgba)
              (this->*rgbaToNative)(intermediateTray.get(),
                                    targetTray.get(), w*h);

            putPixels(targetLeft+x, targetBottom+y,
                      effectiveTargetTray, w, h,
                      targetHorizontalRepeat,
                      targetVerticalRepeat);
          }
          while(x);
        }
        while(y);
      }

    private:

      struct MemoryField
      {
        /**
         * -1 indicates that this is an unused field
         */
        int channelIndex;

        int bitWidth;

        /**
         * The value that maps to zero intensity for fields
         * representing floating point channels
         */
        long double min;

        /**
         * The value that maps to full intensity for fields
         * representing floating point channels
         */
        long double max;

        MemoryField(int channelIndex = -1, int bitWidth = 0):
          channelIndex(channelIndex), bitWidth(bitWidth),
          min(0), max(1) {}
      };

      template<typename T>
      static void repeatForward(T *begin, size_t n, size_t m)
      {
        T *end = begin + n;
        while(n < m)
        {
          end = std::copy(begin, end, end);
          m -= n;
          n <<= 1;
        }
        std::copy(begin, begin+m, end);
      }

      template<typename T>
      static void repeatBackward(T *begin, size_t n, size_t m)
      {
        T *end = begin + n;
        while(n < m)
        {
          begin = std::copy_backward(begin, end, begin);
          m -= n;
          n <<= 1;
        }
        std::copy_backward(end-m, end, begin);
      }


      /**
       * A reference to a method that tranfers a sequence pixels from
       * the image data buffer to the specified pixel tray
       * location. The transferred pixels are read memory consecutive
       * from the image data buffer, but are generally not stored
       * memory consecutive into the tray.
       *
       * \param data A word aligned pointer to the word that contains
       * the first pixel to be read or contains the first part of that
       * pixel if it spans several words.
       *
       * \param wordBitOffset A bit offset into the specified word. It
       * must be strictly less than bitsPerWord. If
       * mostSignificantBitsFirst is true, then zero corresponds with
       * the most significant bit of the specified word, otherwise
       * zero indicates the least significant bit. For direct pixel
       * formats this argument is ignored since it is assumed to be
       * zero.
       *
       * \param tray A pointer into a pixel tray being the target of
       * the pixel transfer. The first pixel will be transferred to
       * this location. The last pixel will be trasferred to
       * tray+pitch*(n-1). The pitch can be negative. Each pixel takes
       * up 'numberOfChannels' elements in the tray (long doubles).
       *
       * \param pitch The distance in tray elements (long doubles)
       * from the start of a pixel to the start of the pixel decoded
       * next. This may be any positive or negative multiple of the
       * number of channels specified by the effective pixel
       * format. It obviously should not be zero though.
       *
       * \param n The number of pixels to transfer.
       */
      void (ImageData::*decoder)(void const *data, int wordBitOffset,
                                 long double *tray, int pitch,
                                 long n) const;

      /**
       * A reference to a method that tranfers a sequence pixels from
       * the specified pixel tray location to the image data
       * buffer. The transferred pixels are stored memory consecutive
       * in the image data buffer, but are generally not read memory
       * consecutive from the tray.
       *
       * \param tray A pointer into a pixel tray being the source of
       * the pixel transfer. The first pixel will be transferred from
       * this location. The last pixel will be trasferred from
       * tray+pitch*(n-1). The pitch can be negative. For each pixel
       * 'numberOfChannels' elements (long doubles) are read from the
       * tray.
       *
       * \param pitch The distance in tray elements (long doubles)
       * from the start of a pixel to the start of the pixel encoded
       * next. This may be any positive or negative multiple of the
       * number of channels specified by the effective pixel
       * format. It obviously should not be zero though.
       *
       * \param n The number of pixels to transfer.
       *
       * \param data A word aligned pointer to the word where the
       * first pixel (or first part of first pixel) is to be written.
       *
       * \param wordBitOffset A bit offset into the specified word. It
       * must be strictly less than bitsPerWord. If
       * mostSignificantBitsFirst is true, then zero corresponds with
       * the most significant bit of the specified word, otherwise
       * zero indicates the least significant bit. For direct pixel
       * formats this argument is ignored since it is assumed to be
       * zero.
       */
      void (ImageData::*encoder)(long double const *tray,
                                 int pitch, long n,
                                 void *data, int wordBitOffset) const;

      /**
       * Convert a pixel component in normalized floating point
       * representation [0;1] to integer representation [0;maxInt]
       * where maxInt = (1<<intBits)-1.
       *
       * This method handles clamping and avoids the rounding problem
       * when converting 1 (and values close to 1) from narrow
       * floating point types to wide interger types. Considder the
       * following code snippet:
       *
       * <PRE>
       *
       *   my_float f = 1;
       *   my_int max = numeric_limits<my_int>::max();
       *   my_int i = static_cast<my_int>(f * m);
       *
       * </PRE>
       *
       * As long as my_float is significantly wider in terms of bits
       * than my_int the above code will always complete with i ==
       * numeric_limits<my_int>::max(). If, on the other hand, my_int
       * has more bits than my_float the code may complete with i ==
       * 0. This seems surprising at first, but is a simple
       * consequence of the fact that my_float cannot represent
       * numeric_limits<my_int>::max() exactly. Somtimes the result of
       * f * m ends up being greater than
       * numeric_limits<my_int>::max() and may in fact be interpreted
       * as 1+numeric_limits<my_int>::max() when converting to my_int.
       */
      template<typename Float, typename Int>
      static Int normFloatToInt(Float v, int intBits);

      /**
       * Convert a pixel component in integer representation
       * [0;maxInt] where maxInt = (1<<intBits)-1 to normalized
       * floating point representation [0;1].
       */
      template<typename Int, typename Float, bool maskInput>
      static Float intToNormFloat(Int v, int intBits);

      template<typename Word, bool isInteger> struct DirectWordAccess;

      template<typename Word, bool nativeEndianness>
      Word readWord(Word const *) const;

      template<typename Word, bool nativeEndianness>
      void writeWord(Word, Word *) const;

      template<typename Word, bool nativeEndianness>
      void decodePixelSequenceDirect(void const *data, int,
                                     long double *pixels,
                                     int pitch, long n) const;

      template<typename Word, bool nativeEndianness>
      void encodePixelSequenceDirect(long double const *pixels,
                                     int pitch, long n,
                                     void *data, int) const;

      /**
       * Decode a sequence of memory consecutive pixels for tight and
       * packed pixel formats.
       *
       * The parameters are explained elsewhere.
       */
      template<typename Word, typename WordAssemble,
               typename ChannelAssemble, bool nativeEndianness>
      void decodePixelSequencePackedTight(void const *data, int wordBitOffset,
                                          long double *pixels,
                                          int pitch, long n) const;

      /**
       * Encode a sequence of memory consecutive pixels for tight and
       * packed pixel formats.
       *
       * The parameters are explained elsewhere.
       */
      template<typename Word, typename WordAssemble,
               typename ChannelAssemble, bool nativeEndianness>
      void encodePixelSequencePackedTight(long double const *pixels,
                                          int pitch, long n,
                                          void *data, int wordBitOffset) const;



      void blendWithBackground(long double const *source,
                               long double *target) const;

      template<bool alpha, bool custom>
      void decodeFromRgb(long double const *source,
                         long double *target, size_t n) const;

      template<bool alpha, bool custom>
      void encodeToRgb(long double const *source,
                       long double *target, size_t n) const;

      template<bool alpha, bool custom>
      void decodeFromLuminance(long double const *source,
                               long double *target, size_t n) const;

      template<bool alpha, bool custom>
      void encodeToLuminance(long double const *source,
                             long double *target, size_t n) const;

      template<bool alpha>
      void decodeFromHsv(long double const *source,
                         long double *target, size_t n) const;

      template<bool alpha>
      void encodeToHsv(long double const *source,
                       long double *target, size_t n) const;



      template<typename Word>
      void setupCodecDirect();

      template<typename Word, typename WordAssemble, typename ChannelAssemble>
      void setupCodecPackedTight();

      template<typename Word, typename WordAssemble>
      void setupCodecPackedTight();

      template<typename Word, typename WordAssemble>
      void setupCodec();


      void *buffer; ///< Must be aligned on a word boundary
      int const pixelsPerStrip;
      int const numberOfStrips;
      BufferFormat::ConstRef bufferFormat;
      int interestLeft;
      int interestBottom;
      int interestWidth;
      int interestHeight;
      std::vector<bool> const endianness;


      // Quatities derived from the pixel and buffer format specifications

      int bytesPerWord;
      int numberOfChannels;

      /**
       * Zero length indicates native endianness. Otherwise the size of
       * this permuation is equals to the number of bytes per word of
       * the specified word type. This permutation is always a
       * symmetric map. That is, if you apply it twice the result will
       * be identical to the original.
       */
      std::vector<int> bytePermutation;

      /**
       * For direct formats a field always correspond to one word. For
       * other formats a field is any number of bits and may span
       * multiple words.
       */
      std::vector<MemoryField> memoryFields;


      /**
       * The distance in bits between memory consecutive pixels.
       */
      long bitsPerPixel;

      /**
       * The distance in bits between memory consecutive pixel strips.
       */
      long bitsPerStrip;

      /**
       * This is the bit-level index of the principal bit.
       *
       * The principal bit is the bit with the lowest bit-level index
       * among all the bits that are part of pixels that fall within
       * the frame of interest.
       *
       * Inside words a bit-level index increases with significance of
       * bit position. This is always the case (in this context)
       * regardless of the pixel order specified by the pixel format.
       */
      long principalBitOffset;
    };
  }
}

#endif // ARCHON_IMAGE_IMAGE_DATA_HPP
