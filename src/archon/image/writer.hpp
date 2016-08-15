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

#ifndef ARCHON_IMAGE_WRITER_HPP
#define ARCHON_IMAGE_WRITER_HPP

#include <archon/image/writer_base.hpp>


namespace archon
{
  namespace Imaging
  {
    /**
     * The purpose of this class is to provide the methods of the
     * ImageWriter for various final classes.
     *
     * \sa ReaderOps
     */
    template<class Accessor, class Base> struct WriterOps: ReaderOps<Accessor, Base>
    {
      /**
       * Enable or disable alpha blending when writing pixels to the
       * image. By default, blending is disabled.
       *
       * When blending is enabled, pixels written to the image will be
       * blended with the original contents. When the alpha component
       * of a written pixel is at full intensity, the original pixel
       * will be completely overwritten. On the other hand, when the
       * alpha component of a written pixel is at zero intensity, the
       * original pixel will be retained completely.
       *
       * In general, the result for a single channel will be:
       *
       * <pre>
       *
       *  a3  =  a1 + a2 - a1*a2  =  1 - (1-a1)*(1-a2)
       *  c3  =  (a2*c2 + a1*(1-a2)*c1) / a3  if  0 < a3  else c2
       *
       * </pre>
       *
       * Where \c aN is an alpha component and \c cN is a color
       * component. Suffix \c 1 refers to the original value in the
       * image, \c 2 refers the the value written, and \3 to the
       * result. Since blending always occurs in RGB space, the color
       * component is either red, green or blue.
       *
       * Note that this blending operation is associative, that is, if
       * we take <tt>blend(X,Y)</tt> to denote the result of blending
       * images <tt>X</tt> and <tt>Y</tt>, then the following two
       * compositions will yield the same result:
       *
       * <pre>
       *
       *   blend(X, blend(Y, Z))
       *   blend(blend(X, Y), Z)
       *
       * </pre>
       */
      Accessor &enable_blending(bool enabled = true);


      /**
       * When enabled, written pixels will first be converted to the
       * luminance color space, then the value of each component will
       * be looked up in the associated palette. If no explicit
       * palette has been assigned, a default palette will be
       * used. The default palette is a linear continuous gradient
       * (linear in RGBA space) from the background color to the
       * foreground color.
       *
       * Each incoming pixel is understood as an index into a
       * palette. The default palette is a linear continuous gradient
       * from background color to foreground color in RGBA space. A
       * custom palette can be set in which case zero will correspond
       * to the first color of that palette, and 1 to the last color
       * of it. If the incoming pixels have more than one component,
       * they will be impicitely converted to the 'luminance' color
       * space, and then the luminance component is used as the index.
       */
      Accessor &enable_color_mapping(bool enabled = true);


      /**
       * Fill the clipping region with the background color.
       */
      Accessor &clear();


      /**
       * Fill the clipping region with the foreground color.
       */
      Accessor &fill();


      /**
       * Set the pixel at the current writer position to the specified
       * color.
       *
       * If the current writer position is outside the current
       * clipping region, the image will not ba changed.
       *
       * If blending is enabled for this writer, and the transparency component
       * is not zero, then the resulting color will depend on the original color
       * of the target pixel.
       *
       * \sa set_pos
       * \sa set_clip
       * \sa enable_blending
       */
      Accessor &put_pixel(util::PackedTRGB color);


      /**
       * Same as the 4-argument version, except that this method
       * assumes that the alpha component is at its maximum
       * value. This means that blending never applies.
       *
       * \sa put_pixel_rgb(T,T,T,T)
       */
      template<typename T> Accessor &put_pixel_rgb(T red, T green, T blue);


      /**
       * Same as put_pixel(), except that the color and
       * transparency are now specified as separate red, green, blue,
       * and alpha components.
       *
       * If the components are specified using a floating point type,
       * then 0 will correspond to zero intencity/opacity, and 1 to
       * full intencity/opacity. If they are specified using an
       * integer type, the range will instead be from 0 to the maximum
       * representable value of that type. Note that this means a
       * signed integer type has half the resolution of the
       * corresponding unsigned type.
       *
       * \sa put_pixel()
       */
      template<typename T> Accessor &put_pixel_rgb(T red, T green, T blue, T alpha);


      template<typename T, class R, class I> Accessor &put_pixel_rgb(Math::VecMem<3,T,R,I> const &v);


      template<typename T, class R, class I> Accessor &put_pixel_rgb(Math::VecMem<4,T,R,I> const &v);


      template<typename T, class R, class I> Accessor &put_pixel_rgb(Math::VecVal<3,T,R,I> const &v);


      template<typename T, class R, class I> Accessor &put_pixel_rgb(Math::VecVal<4,T,R,I> const &v);


      template<typename T> Accessor &put_pixel(T const *p, ColorSpace::ConstRefArg c, bool has_alpha);


      Accessor &put_pixel(void const *p, ColorSpace::ConstRefArg c, bool has_alpha, WordType t);


      /**
       * Write a block of pixels at the current writer position.
       */
      template<typename T>
      Accessor &put_block_rgb(T const *tray, int width, int height, bool has_alpha = false);


      template<typename T>
      Accessor &put_block(T const *tray, int width, int height,
                          ColorSpace::ConstRefArg c, bool has_alpha);


      template<typename T>
      Accessor &put_block(T const *tray, ssize_t pitch, ssize_t stride, int width, int height,
                          ColorSpace::ConstRefArg c, bool has_alpha);

      Accessor &put_block(void const *tray, ssize_t pitch, ssize_t stride, int width, int height,
                          ColorSpace::ConstRefArg c, bool has_alpha, WordType t);


      /**
       * Transfer a block of pixels of the specified size from the specified
       * reader to this writer. The pixels that are read from the source image
       * are those that would be read when passing the same width and height to
       * get_block() on the reader, and the pixels that are overwritten in the
       * target image are those that would be overwritten when passing the same
       * width and height to put_block() on this writer.
       */
      Accessor &put_image(ImageReader &r, int width, int height);


      /**
       * Write the specified image as a block at the current writer
       * position.
       */
      Accessor &put_image(Image::ConstRefArg image);


      /**
       * Load the image from the specified file, and write it as a block
       * at the current writer position.
       */
      Accessor &put_image(std::string path);



    protected:
      WriterOps(Image::RefArg image): ReaderOps<Accessor, Base>(image) {}
    };



    /**
     * This class extends the ImageReader with easy write access to
     * image data.
     */
    struct ImageWriter: WriterOps<ImageWriter, WriterBase>
    {
    private:
      typedef WriterOps<ImageWriter, WriterBase> Base;

    public:
      /**
       * A short hand for <tt>ImageWriter(Image::new_image(width,
       * height, ColorSpace::get_RGB(), has_alpha));</tt> followed by
       * an optional clearing of the new image buffer.
       *
       * \param width, height Create a new image of this width and
       * height.
       *
       * \param clear Set to false if you do not want the image to be
       * cleared to the background color (black/transparent) initially.
       */
      ImageWriter(int width, int height, bool has_alpha = false, bool clear = true);

      /**
       * A short hand for <tt>ImageWriter(Image::load(path))</tt>.
       *
       * \param path The file system path of the file to be loaded.
       */
      ImageWriter(std::string path): Base(Image::load(path)) {}

      /**
       * Create a new image writer wrapping the specified image.
       *
       * \param image The wrapped image.
       */
      ImageWriter(Image::RefArg image): Base(image) {}

      /**
       * Save the wrapped image to the specified file.
       *
       * \param path The file system path of the file to
       * create/overwrite.
       */
      void save(std::string path) { image->save(path); }
    };







    // Implementation:

    template<class A, class B> inline A &WriterOps<A,B>::enable_blending(bool e)
    {
      WriterBase::enable_blending(e);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::enable_color_mapping(bool e)
    {
      WriterBase::enable_color_mapping(e);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::clear()
    {
      WriterBase::fill(false);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::fill()
    {
      WriterBase::fill(true);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::put_pixel(util::PackedTRGB color)
    {
      WriterBase::put_pixel(color);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_pixel_rgb(T r, T g, T b)
    {
      T rgb[3] = { r,g,b };
      WriterBase::put_pixel_smart<T, false>(rgb, 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_pixel_rgb(T r, T g, T b, T a)
    {
      T rgba[4] = { r,g,b,a };
      WriterBase::put_pixel_smart<T, true>(rgba, 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &WriterOps<A,B>::put_pixel_rgb(Math::VecMem<3,T,R,I> const &v)
    {
      WriterBase::put_pixel_smart<T, false>(v.get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &WriterOps<A,B>::put_pixel_rgb(Math::VecMem<4,T,R,I> const &v)
    {
      WriterBase::put_pixel_smart<T, true>(v.get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &WriterOps<A,B>::put_pixel_rgb(Math::VecVal<3,T,R,I> const &v)
    {
      WriterBase::put_pixel_smart<T, false>(Math::BasicVec<3,T>(v).get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T, class R, class I>
    inline A &WriterOps<A,B>::put_pixel_rgb(Math::VecVal<4,T,R,I> const &v)
    {
      WriterBase::put_pixel_smart<T, true>(Math::BasicVec<3,T>(v).get(), 0);
      return static_cast<A &>(*this);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_pixel(T const *g, ColorSpace::ConstRefArg c, bool a)
    {
      if(a) WriterBase::put_pixel_smart<T, true>(g, c.get());
      else WriterBase::put_pixel_smart<T, false>(g, c.get());
      return static_cast<A &>(*this);
    }


    template<class A, class B>
    inline A &WriterOps<A,B>::put_pixel(void const *g, ColorSpace::ConstRefArg c, bool a, WordType t)
    {
      ssize_t const p = (c->get_num_primaries() + (a?1:0)) * ssize_t(get_bytes_per_word(t));
      return put_block(g,p,p,1,1,c,a,t);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_block_rgb(T const *g, int w, int h, bool a)
    {
      ssize_t const p = (a ? 4 : 3) * ssize_t(sizeof(T));
      return put_block(g, p, w*p, w, h, this->rgb, a);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_block(T const *g, int w, int h, ColorSpace::ConstRefArg c, bool a)
    {
      ssize_t const p = (c->get_num_primaries() + (a?1:0)) * ssize_t(sizeof(T));
      return put_block(g, p, w*p, w, h, c, a);
    }


    template<class A, class B> template<typename T>
    inline A &WriterOps<A,B>::put_block(T const *g, ssize_t p, ssize_t s, int w, int h,
                                        ColorSpace::ConstRefArg c, bool a)
    {
      return put_block(g, p, s, w, h, c, a, get_word_type_by_type<T>());;
    }


    template<class A, class B>
    inline A &WriterOps<A,B>::put_block(void const *g, ssize_t p, ssize_t s, int w, int h,
                                        ColorSpace::ConstRefArg c, bool a, WordType t)
    {
      WriterBase::put_block(WriterBase::ConstTupleGrid(reinterpret_cast<char const *>(g), p, s),
                            WriterBase::PixelFormat(c.get(), a, t), w, h);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::put_image(ImageReader &r, int width, int height)
    {
      ReaderBase::put_image(r, *static_cast<WriterBase *>(this), width, height);
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::put_image(Image::ConstRefArg image)
    {
      Image::CodecConstPtr c(image->acquire_codec().release());
      WriterBase::put_image(c.get(), WriterBase::PixelFormat(image->get_color_space().get(),
                                                             image->has_alpha_channel(),
                                                             image->get_word_type()),
                            0, 0, image->get_width(), image->get_height());
      return static_cast<A &>(*this);
    }


    template<class A, class B> inline A &WriterOps<A,B>::put_image(std::string path)
    {
      return put_image(Image::load(path));
    }



    inline ImageWriter::ImageWriter(int width, int height, bool has_alpha, bool do_clear):
      Base(Image::new_image(width, height, ColorSpace::get_RGB(), has_alpha))
    {
      if(do_clear) clear();
    }
  }
}


#endif // ARCHON_IMAGE_WRITER_HPP
