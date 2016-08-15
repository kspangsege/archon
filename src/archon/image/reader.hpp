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

#ifndef ARCHON_IMAGE_READER_HPP
#define ARCHON_IMAGE_READER_HPP

#include <archon/math/vector.hpp>
#include <archon/image/reader_base.hpp>


namespace archon
{
  namespace image
  {
    /**
     * The purpose of this class is to provide the methods of the
     * ImageReader for various final classes. For example, an \c
     * ImageWriter is also an <tt>ImageReader</tt>, but in this case
     * we want the reader methods to return a reference to an \c
     * ImageWriter instead of a reference to an <tt>ImageReader</tt>.
     *
     *
     * <h3>Alpha channel elimination (and introduction)</h3>
     *
     * Some methods return an alpha component for each extracted
     * pixel, other methods do not, and yet other methods allow you to
     * specify whether you want an alpha component or not.
     *
     * In any case, if the underlying image has an alpha channel, and
     * the read operation does not return the alpha components, then
     * the extracted pixels will be blended with the current
     * background color before they are returned to the caller.
     *
     * On the other hand, if the underlying image does not have an
     * alpha channel, then any read operation that returns alpha
     * components, will set those components to maximum opacity.
     *
     *
     * <h3>Parameterized component types</h3>
     *
     * Many of the methods provided by this class are parameterized on
     * the type used to hold the individual components of each
     * pixel. Unless something else is stated for a method, the
     * accepted types are any unsigned integral type as well as any
     * floating point type.
     *
     * If the type is floating point, then 0 will correspond to zero
     * intencity/opacity, and 1 to full intencity/opacity. If it is an
     * unsigned integer type, the range will instead be from 0 to the
     * maximum representable value of that type.
     *
     * This is true regardless of whether the type is specified as a
     * template argument, or as a type descriptor argument of type
     * <tt>Image::WordType</tt>.
     *
     *
     * <h3>Tray buffers</h3>
     *
     * All methods that extract a block of pixels, takes a 'tray
     * buffer' as argument. The extracted pixels will always be store
     * in this buffer in row-major order (one horizontal strip after
     * another) starting from the lower left corner of the request
     * block.
     *
     * Each pixel will be stored as a sequence of \c n components
     * according to the specified color space (sometimes specified
     * implicitely through the name of the method), where \c n is the
     * number of primaries in the color space plus one if an alpha
     * component is requested too.
     *
     * The components of the individual pixel will be stored in the
     * canonical channel order specified by the color space. The alpha
     * channel, if requested, will always come last.
     *
     * Each component of each extracted pixel consumes exactly one
     * element of the tray buffer.
     *
     * What constitutes an element in the tray buffer, is either
     * specified as a template type paramter, or as a type descriptor
     * argument of type <tt>Image::WordType</tt>. In the former case,
     * the tray buffer argument will be a pointer to the templatized
     * type, and in the latter case it will be a void pointer.
     *
     * Some of the methods that extract a single pixel also take a
     * tray buffer as argument. Specifically, these are the methods
     * that allow you to specify an arbitarry color space, so the
     * number of returned components can vary. The above also applies
     * to these methods.
     */
    template<class Accessor, class Base> struct ReaderOps: Base
    {
      /**
       * Set the current position of positioned read and write
       * operations. Normally this is the position of the lower left
       * corner of the pixel block involved in the operation, but this
       * can be changed with set_pos_align().
       *
       * The position is specified in number of pixels, and the origin
       * is the lower left corner of the image. Fractional positions
       * can be set, but unless a non default positioning alignment is
       * used, the actual position is the integer part of the
       * specified position.
       */
      Accessor &set_pos(double x = 0, double y = 0);

      /**
       * Set the current position of positioned read and write
       * operations relative to the size of the accessed image.
       *
       * A position (0,0) corresponds with the lower left corner of
       * the image, while a position (1,1) corresponds with the upper
       * right corner.
       *
       * The specified values need not lie in the interval [0;1].
       */
      Accessor &set_rel_pos(double x = 0, double y = 0);

      /**
       * Set the aligment that applies to positioned operations such
       * as <tt>ImageReader::get_block</tt> (and
       * <tt>ImageWriter::put_block</tt>). The alignment is specified
       * as a displacement of the operation relative to the size of
       * the block being read or written. The effective horizontal
       * pixel poisition \c E of a block of width \c W is calculated
       * as follows:
       *
       * <pre>
       *
       *   E = floor(P - A*W)
       *
       * </pre>
       *
       * Where \c P is the horizontal component of the current
       * position as set by <tt>set_pos</tt>, and \c A is the
       * horizontal component of the alignment as set by this
       * function.
       *
       * Thus, an alignment of (0,0) means that the lower left corner
       * of the block is placed at the current position, and (1,1)
       * means that the upper right corner of the block is placed at
       * the current position.
       *
       * For example, you can place an image \c I at the center of
       * another image \c J as follows:
       *
       * <pre>
       *
       *   ImageWriter w(J);
       *   w.set_rel_pos(0.5, 0.5).set_pos_align(0.5, 0.5).put_image(I);
       *
       * </pre>
       *
       * The specified values need not lie in the interval [0;1].
       */
      Accessor &set_pos_align(double x = 0, double y = 0);

      /**
       * Set the clipping region for this reader.
       *
       * The clipping region restricts all image access through this
       * reader to the specified region.
       *
       * An attempt to read pixels lying outside the clipping region,
       * will be intercepted, and the offending pixels, as returned to
       * the caller, will be set according to the current falloff
       * mode.
       *
       * The specified region will be automatically clipped to the
       * boundary of the underlying image. So, the effective clipping
       * region cannot escape the image boundary.
       *
       * Initially, the clipping region coincides with the entire
       * image area.
       *
       * \param left, bottom The lower left corner of the clipping
       * region, measured in pixels from the lower left corner of the
       * image.
       *
       * \param width, height The size of the clipping region. If a
       * negative width is specified, the actual width is set to the
       * distance between the left clipping edge and the right edge of
       * the image. A negative height is handled correspondingly.
       *
       * \sa set_falloff
       */
      Accessor &set_clip(int left = 0, int bottom = 0, int width = -1, int height = -1);

      /**
       * Set the falloff mode.
       *
       * \sa set_clip
       */
      Accessor &set_falloff(Falloff f = falloff_Background);

      /**
       * Same as the one-argument versin, except that in this case,
       * different modes can be set for the horizontal and vertical
       * directions.
       */
      Accessor &set_falloff(Falloff horiz, Falloff vert);

      /**
       * Set the background color. The default is fully transparent black.
       *
       * When the falloff mode is
       * <tt>ReaderBase::falloff_Background</tt>, the background color
       * is used when reading pixels that lie outside the clipping
       * region.
       *
       * When reading pixels in a way that does not return
       * transparency information, from an image that has an alpha
       * channel, the extracted pixels will be automatically blended
       * with the background color.
       *
       * A reader, that is not also a writer, uses the background
       * color only in the ways mentioned above.
       *
       * For writers, the background color is also used when clearing
       * a region.
       *
       * \sa ReaderBase::falloff_Background
       * \sa WriterOps::clear
       */
      Accessor &set_background_color(util::PackedTRGB color);

      /**
       * Set the foreground color. The default is fully opaque white.
       *
       * A reader, that is not also a writer, never uses the
       * foreground color, but it is used by writers in various ways.
       *
       * \sa WriterOps::fill
       */
      Accessor &set_foreground_color(util::PackedTRGB color);

      /**
       * Get the color and transparency of the pixel at the current
       * reader position.
       *
       * If the current reader position is outside the current
       * clipping region, the outcome will depend on the current
       * falloff mode.
       *
       * \sa set_pos
       * \sa set_falloff
       */
      Accessor &get_pixel(util::PackedTRGB& pixel);

      /**
       * Same as the 4-argument version, except that this method does
       * not return the alpha component. Instead, if the image has an
       * alpha channel, the pixel will be blended with the current
       * background color before it is returned.
       *
       * \sa get_pixel_rgb(T &,T &,T &,T &)
       */
      template<typename T> Accessor &get_pixel_rgb(T &red, T &green, T &blue);

      /**
       * Same as get_pixel(util::PackedTRGB&), except that the color and transparency is now
       * returned as separate red, green, blue, and alpha components.
       *
       * The retreived component values are affected by the choice of the type
       * <tt>T</tt>. See the class level documentation for details.
       */
      template<typename T> Accessor &get_pixel_rgb(T &red, T &green, T &blue, T &alpha);

      template<typename T, class R, class I> Accessor &get_pixel_rgb(math::VecMem<3,T,R,I> &v);

      template<typename T, class R, class I> Accessor &get_pixel_rgb(math::VecMem<4,T,R,I> &v);

      template<typename T> Accessor &get_pixel(T *tray, ColorSpace::ConstRefArg c, bool has_alpha);

      Accessor &get_pixel(void *tray, ColorSpace::ConstRefArg c, bool has_alpha, WordType t);

      util::PackedTRGB get_pixel() { util::PackedTRGB p; get_pixel(p); return p; }

      math::Vec3F get_pixel_rgb()  { math::Vec3F p; get_pixel_rgb(p); return p; }

      math::Vec4F get_pixel_rgba() { math::Vec4F p; get_pixel_rgb(p); return p; };

      /**
       * Extract a block of pixels from the current reader position.
       *
       * The minimum acceptible size of the specified tray buffer, in
       * terms of the number of elements of type <tt>T</tt>, is:
       *
       * <pre>
       *
       *   min_buffer_size = height * width * n
       *
       *   where  n = 4 if has_alpha, else 3
       *
       * </pre>
       *
       * See the class level documentation for details about how the
       * extracted pixels are store in the tray buffer, and how the
       * choice of the type \c T affects the result.
       *
       * If the reader position is such that the requested block
       * escapes the current clipping region, then offending pixels
       * when stored in the tray buffer will be set according to the
       * current falloff mode.
       *
       * If the color space of the underlying pixel representation is
       * not RGB, this method will automatically perform the
       * conversion to RGB. If you need to extract them in their
       * native color space, use <tt>this->get_block(tray, width, height,
       * this->get_color_space(), this->has_alpha_channel())</tt>.
       *
       * \sa set_pos
       * \sa set_falloff
       */
      template<typename T>
      Accessor &get_block_rgb(T *tray, int width, int height, bool has_alpha = false);

      /**
       * Same as get_block_rgb(), except in this case the returned pixels are
       * decomposed according to the specified color space.
       */
      template<typename T>
      Accessor &get_block(T *tray, int width, int height,
                          ColorSpace::ConstRefArg c, bool has_alpha);

      template<typename T>
      Accessor &get_block(T *tray, ssize_t pitch, ssize_t stride, int width, int height,
                          ColorSpace::ConstRefArg c, bool has_alpha);

      Accessor &get_block(void *tray, ssize_t pitch, ssize_t stride, int width, int height,
                         ColorSpace::ConstRefArg c, bool has_alpha, WordType t);

    protected:
      ReaderOps(Image::ConstRefArg image): Base(image) {}
    };



    /**
     * This class provides easy read access to image data.
     *
     * Pixel data can be read directly from an image object by
     * acquiring an \c Image::Codec object, but that is a combersome
     * and error prone endeavor due to the fact that data must be
     * transferred in a particular format specified by the image, and
     * this format generally varies from one image to another. The \c
     * ImageReader class transparently handles the conversion between
     * that format and the one that the application wishes to use.
     *
     * Most of the interesting methods are provided by the base class
     * <tt>ReaderOps</tt>.
     */
    struct ImageReader: ReaderOps<ImageReader, ReaderBase>
    {
    private:
      typedef ReaderOps<ImageReader, ReaderBase> Base;

    public:
      /**
       * Construct an image reader that accesses the image loaded from
       * the specified file system path.
       */
      ImageReader(std::string path): Base(Image::load(path)) {}


      /**
       * Construct an image reader that accesses the specified image.
       */
      ImageReader(Image::ConstRefArg image): Base(image) {}


      /**
       * Get the width (in pixels) of the accessed image.
       */
      int get_width()  const { return image_width;  }


      /**
       * Get the height (in pixels) of the accessed image.
       */
      int get_height() const { return image_height; }


      /**
       * Get the native color space of the accessed image, or more
       * precisely, the color space of the pixel transfer format that
       * applies when one accesses the image data directly through an
       * <tt>Image::Codec</tt> object.
       *
       * In general, access to image data through this
       * <tt>ImageReader</tt>, and throgh the <tt>ImageWriter</tt>, is
       * fastest when using this color space.
       *
       * \sa Image::get_transfer_format
       */
      ColorSpace::ConstRef get_color_space() const;


      /**
       * Enquire about the precense of an alpha channel in the
       * accessed image, or more precisely, about the precense of an
       * alpha channel in the pixel transfer format that applies when
       * one accesses the image data directly through an
       * <tt>Image::Codec</tt> object.
       *
       * In general, access to image data through this
       * <tt>ImageReader</tt>, and throgh the <tt>ImageWriter</tt>, is
       * fastest if it has an alpha channel when, and only when, this
       * method returns true.
       *
       * \sa Image::get_transfer_format
       */
      bool has_alpha_channel() const;


      /**
       * Get the native word type of the accessed image, or more
       * precisely, the word type of the pixel transfer format that
       * applies when one accesses the image data directly through an
       * <tt>Image::Codec</tt> object.
       *
       * In general, access to image data through this
       * <tt>ImageReader</tt>, and throgh the <tt>ImageWriter</tt>, is
       * fastest when using this word type.
       *
       * \sa Image::get_transfer_format
       */
      WordType get_word_type() const;
    };







    // Implementation:

    template<class A, class B> inline A &ReaderOps<A,B>::set_pos(double x, double y)
    {
      ReaderBase::set_pos(x,y);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_rel_pos(double x, double y)
    {
      ReaderBase::set_pos(this->image_width * x, this->image_height * y);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_pos_align(double x, double y)
    {
      ReaderBase::set_pos_align(x,y);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_clip(int l, int b, int w, int h)
    {
      ReaderBase::set_clip(l,b,w,h);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_falloff(Falloff f)
    {
      ReaderBase::set_falloff(f,f);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_falloff(Falloff h, Falloff v)
    {
      ReaderBase::set_falloff(h,v);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_background_color(util::PackedTRGB c)
    {
      ReaderBase::set_color(c, false);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::set_foreground_color(util::PackedTRGB c)
    {
      ReaderBase::set_color(c, true);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &ReaderOps<A,B>::get_pixel(util::PackedTRGB &p)
    {
      p = ReaderBase::get_pixel();
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_pixel_rgb(T &red, T &green, T &blue)
    {
      T b[3];
      ReaderBase::get_pixel_smart<T, false>(b,0);
      red = b[0]; green = b[1]; blue = b[2];
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_pixel_rgb(T &red, T &green, T &blue, T &alpha)
    {
      T b[4];
      ReaderBase::get_pixel_smart<T, true>(b,0);
      red = b[0]; green = b[1]; blue = b[2]; alpha = b[3];
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &ReaderOps<A,B>::get_pixel_rgb(math::VecMem<3,T,R,I> &v)
    {
      ReaderBase::get_pixel_smart<T, false>(v.get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &ReaderOps<A,B>::get_pixel_rgb(math::VecMem<4,T,R,I> &v)
    {
      ReaderBase::get_pixel_smart<T, true>(v.get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_pixel(T *g, ColorSpace::ConstRefArg c, bool a)
    {
      if(a) ReaderBase::get_pixel_smart<T, true>(g, c.get());
      else ReaderBase::get_pixel_smart<T, false>(g, c.get());
      return static_cast<A &>(*this);
    }


    template<class A, class B>
    inline A &ReaderOps<A,B>::get_pixel(void *g, ColorSpace::ConstRefArg c, bool a, WordType t)
    {
      ssize_t const p = (c->get_num_primaries() + (a?1:0)) * ssize_t(get_bytes_per_word(t));
      return get_block(g,p,p,1,1,c,a,t);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_block_rgb(T *g, int w, int h, bool a)
    {
      ssize_t const p = (a ? 4 : 3) * ssize_t(sizeof(T));
      return get_block(g, p, w*p, w, h, this->rgb, a);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_block(T *g, int w, int h, ColorSpace::ConstRefArg c, bool a)
    {
      ssize_t const p = (c->get_num_primaries() + (a?1:0)) * ssize_t(sizeof(T));
      return get_block(g, p, w*p, w, h, c, a);
    }


    template<class A, class B> template<typename T>
    inline A &ReaderOps<A,B>::get_block(T *g, ssize_t p, ssize_t s, int w, int h,
                                        ColorSpace::ConstRefArg c, bool a)
    {
      return get_block(g, p, s, w, h, c, a, get_word_type_by_type<T>());
    }


    template<class A, class B>
    inline A &ReaderOps<A,B>::get_block(void *g, ssize_t p, ssize_t s, int w, int h,
                                        ColorSpace::ConstRefArg c, bool a, WordType t)
    {
      ReaderBase::get_block(ReaderBase::TupleGrid(reinterpret_cast<char *>(g), p, s),
                            ReaderBase::PixelFormat(c.get(), a, t), w, h);
      return static_cast<A &>(*this);
    }



    inline ColorSpace::ConstRef ImageReader::get_color_space() const
    {
      return ColorSpace::ConstRef(pixel_format.format.color_space);
    }


    inline bool ImageReader::has_alpha_channel() const
    {
      return pixel_format.format.has_alpha;
    }


    inline WordType ImageReader::get_word_type() const
    {
      return pixel_format.format.word_type;
    }
  }
}


#endif // ARCHON_IMAGE_READER_HPP
