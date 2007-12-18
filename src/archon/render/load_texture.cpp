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

#include <stdexcept>
#include <limits>

#include <archon/platform.hpp> // Never include this one in header files

#include <GL/gl.h>
#ifdef ARCHON_HAVE_GLU
#include <GL/glu.h>
#endif

#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/image/integer_buffer_format.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/render/load_texture.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Imaging;
using namespace Archon::Render;

namespace
{
  struct ImageHandler
  {
    virtual void handle(GLint internal_format, GLenum format,
                        GLenum type, GLvoid const *buffer) const = 0;
    virtual ~ImageHandler() {}
  };


  struct TextureLoader: ImageHandler
  {
    void handle(GLint internal_format, GLenum format, GLenum type, GLvoid const *buffer) const
    {
      glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height,
                   with_border ? 1 : 0, format, type, buffer);
    }
    TextureLoader(int w, int h, bool b): width(w), height(h), with_border(b) {}
    int const width, height;
    bool const with_border;
  };


#ifdef ARCHON_HAVE_GLU
  struct MipmapLoader: ImageHandler
  {
    void handle(GLint internal_format, GLenum format, GLenum type, GLvoid const *buffer) const
    {
      gluBuild2DMipmaps(GL_TEXTURE_2D, internal_format, width, height, format, type, buffer);
    }
    MipmapLoader(int w, int h): width(w), height(h) {}
    int const width, height;
  };
#endif


#ifdef ARCHON_HAVE_GLU
  struct MipmapLevelLoader: ImageHandler
  {
    void handle(GLint internal_format, GLenum format, GLenum type, GLvoid const *buffer) const
    {
      gluBuild2DMipmapLevels(GL_TEXTURE_2D, internal_format, width, height,
                             format, type, level, base, max, buffer);
    }
    MipmapLevelLoader(int w, int h, int l, int b, int m):
      width(w), height(h), level(l), base(b), max(m) {}
    int const width, height, level, base, max;
  };
#endif




  struct ImageLoadInfo
  {
    bool have_word_type_8bit, have_word_type_16bit, have_word_type_32bit;
    WordType word_type_8bit, word_type_16bit, word_type_32bit;


    struct GlImgFmt
    {
      bool swap_bytes;
      GLenum format;
      GLenum type;
      GlImgFmt(bool swap_bytes, GLenum format, GLenum type):
        swap_bytes(swap_bytes), format(format), type(type) {}
    };

    IntegerBufferFormat::Map<GlImgFmt> format_map;


    static ImageLoadInfo const &get()
    {
      static ImageLoadInfo const info;
      return info;
    }


  private:
    typedef IntegerBufferFormat::Channel       Ch;
    typedef IntegerBufferFormat::ChannelLayout Layout;

    ImageLoadInfo()
    {
      try
      {
        have_word_type_8bit = false;
        word_type_8bit = get_word_type_by_bit_width(8);
        have_word_type_8bit = true;
      }
      catch(NoSuchWordTypeException &) {}
      try
      {
        have_word_type_16bit = false;
        word_type_16bit = get_word_type_by_bit_width(16);
        have_word_type_16bit = true;
      }
      catch(NoSuchWordTypeException &) {}
      try
      {
        have_word_type_32bit = false;
        word_type_32bit = get_word_type_by_bit_width(32);
        have_word_type_32bit = true;
      }
      catch(NoSuchWordTypeException &) {}


      // Add packed formats first.
      //
      // The general interpretation rule for packed OpenGL types is as
      // follows:
      //
      // The numbers in 'GL_UNSIGNED_INT_10_10_10_2' or
      // 'GL_UNSIGNED_INT_10_10_10_2_REV' states how many bits are
      // used for each channel. The first number always represent the
      // first channel, the second number represent the second
      // channel, and so forth.
      //
      // Meanings are assigned to channels by the 'format'. A format
      // of 'GL_RGBA' states that the first channel is the red
      // channel, and the final channel is the alpha channel. If
      // format is 'GL_BGRA' then the first channel is instrad the
      // blue channel, but the final channel is still the alpha
      // channel.
      //
      // When there is no '_REV' suffix, then the first channel
      // occupies the most significant bits of the pixel entity
      // (32-bit integer in the example above), and in general the
      // channels occupy bits in order of decreasing significance.
      //
      // On the other hand, when there is a '_REV' suffix, the the bit
      // order is reversed, and the first channel now occupies the
      // least significant bits of the pixel entity.
      //

      if(have_word_type_8bit)
      {
#if defined GL_UNSIGNED_BYTE_3_3_2 && defined GL_UNSIGNED_BYTE_2_3_3_REV
        add_packed_block(word_type_8bit, Layout(3, 3, 2),
                         Layout(Ch(6, 2), Ch(3, 3), Ch(0, 3)),
                         GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV);
#endif
      }

      if(have_word_type_16bit)
      {
#if defined GL_UNSIGNED_SHORT_5_6_5 && defined GL_UNSIGNED_SHORT_5_6_5_REV
        add_packed_block(word_type_16bit, Layout(5, 6, 5),
                         Layout(Ch(11, 5), Ch(5, 6), Ch(0, 5)),
                         GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_6_5_REV);
#endif

#if defined GL_UNSIGNED_SHORT_4_4_4_4 && defined GL_UNSIGNED_SHORT_4_4_4_4_REV
        add_packed_block(word_type_16bit, Layout(4, 4, 4, 4),
                         Layout(Ch(8, 4), Ch(4, 4), Ch(0, 4), Ch(12, 4)),
                         GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV);
#endif

#if defined GL_UNSIGNED_SHORT_5_5_5_1 && defined GL_UNSIGNED_SHORT_1_5_5_5_REV
        add_packed_block(word_type_16bit, Layout(5, 5, 5, 1),
                         Layout(Ch(10, 5), Ch(5, 5), Ch(0, 5), Ch(15, 1)),
                         GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_1_5_5_5_REV);
#endif
      }

      if(have_word_type_32bit)
      {
#if defined GL_UNSIGNED_INT_8_8_8_8 && defined GL_UNSIGNED_INT_8_8_8_8_REV
        add_packed_block(word_type_32bit, Layout(8, 8, 8, 8),
                         Layout(Ch(16, 8), Ch(8, 8), Ch(0, 8), Ch(24, 8)),
                         GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_8_8_8_8_REV);
#endif

#if defined GL_UNSIGNED_INT_10_10_10_2 && defined GL_UNSIGNED_INT_2_10_10_10_REV
        add_packed_block(word_type_32bit, Layout(10, 10, 10, 2),
                         Layout(Ch(20, 10), Ch(10, 10), Ch(0, 10), Ch(30, 2)),
                         GL_UNSIGNED_INT_10_10_10_2, GL_UNSIGNED_INT_2_10_10_10_REV);
#endif
      }


      // Add direct formats after the packed ones, such that if there
      // are correspondences, then the packed variant will be used.
      if(have_word_type_8bit)  add_direct_block(8,  word_type_8bit,  GL_UNSIGNED_BYTE);
      if(have_word_type_16bit) add_direct_block(16, word_type_16bit, GL_UNSIGNED_SHORT);
      if(have_word_type_32bit) add_direct_block(32, word_type_32bit, GL_UNSIGNED_INT);

//      format_map.dump_info(cerr);
    }


    void add_packed_block(WordType word_type, Layout const &rgb, Layout const &bgr,
                          GLenum gl_type, GLenum gl_type_rev)
    {
      vector<bool> const &nat_end = native_endianness;
      vector<bool> rev_end;
      {
        int const n = nat_end.size();
        rev_end.resize(n);
        for(int i=0; i<n; ++i) rev_end[i] = !nat_end[i];
      }
      bool const alpha = 3 < rgb.channels.size();

      int i = format_map.add_channel_layout(rgb);
      GLenum format = alpha ? GL_RGBA : GL_RGB;
      format_map.add_format(word_type, nat_end, true,  i, GlImgFmt(false, format, gl_type));
      format_map.add_format(word_type, nat_end, false, i, GlImgFmt(false, format, gl_type_rev));
      format_map.add_format(word_type, rev_end, true,  i, GlImgFmt(true,  format, gl_type));
      format_map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  format, gl_type_rev));

#if defined GL_BGR && defined GL_BGRA
      i = format_map.add_channel_layout(bgr);
      format = alpha ? GL_BGRA : GL_BGR;
      format_map.add_format(word_type, nat_end, true,  i, GlImgFmt(false, format, gl_type));
      format_map.add_format(word_type, nat_end, false, i, GlImgFmt(false, format, gl_type_rev));
      format_map.add_format(word_type, rev_end, true,  i, GlImgFmt(true,  format, gl_type));
      format_map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  format, gl_type_rev));
#endif
    }


    void add_direct_block(int w, WordType word_type, GLenum gl_type)
    {
      vector<bool> const &nat_end = native_endianness;
      vector<bool> rev_end;
      {
        int const n = nat_end.size();
        rev_end.resize(n);
        for(int i=0; i<n; ++i) rev_end[i] = !nat_end[i];
      }
      IntegerBufferFormat::Map<GlImgFmt> &map = format_map;

      int i = format_map.add_channel_layout(Layout(w));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_LUMINANCE, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_LUMINANCE, gl_type));

      i = format_map.add_channel_layout(Layout(w,w));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_LUMINANCE_ALPHA, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_LUMINANCE_ALPHA, gl_type));

      i = format_map.add_channel_layout(Layout(w,w,w));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_RGB, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_RGB, gl_type));

      i = format_map.add_channel_layout(Layout(w,w,w,w));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_RGBA, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_RGBA, gl_type));

#ifdef GL_BGR
      i = format_map.add_channel_layout(Layout(Ch(2*w, w), Ch(w, w), Ch(0, w)));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_BGR, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_BGR, gl_type));
#endif

#ifdef GL_BGRA
      i = format_map.add_channel_layout(Layout(Ch(2*w, w), Ch(w, w), Ch(0, w), Ch(3*w, w)));
      map.add_format(word_type, nat_end, false, i, GlImgFmt(false, GL_BGRA, gl_type));
      map.add_format(word_type, rev_end, false, i, GlImgFmt(true,  GL_BGRA, gl_type));
#endif
    }
  };
}




namespace Archon
{
  namespace Render
  {
    void load_image(Image::ConstRefArg,
                    int width, int height, ImageHandler const &);


    /**
     * \todo FIXME: Some OpenGL implementations will only accept
     * texture sizes whose width and height are \c 2**n+2*b where \c n
     * is some non-negative integer and \c b is 1 if \c with_border is
     * true, and 0 if it is false. In such cases we should manually
     * resize the image.
     */
    void load_texture(Image::ConstRefArg img, bool with_border, bool no_interp)
    {
      int const w = img->get_width(), h = img->get_height();
      load_image(img, w, h, TextureLoader(w, h, with_border));

      // Scale linearly when image is bigger than texture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, no_interp ? GL_NEAREST : GL_LINEAR);
      // Scale linearly when image is smalled than texture - disable mipmapping
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, no_interp ? GL_NEAREST : GL_LINEAR);
    }


#ifdef ARCHON_HAVE_GLU
    void load_mipmap(Image::ConstRefArg img)
    {
      int const w = img->get_width(), h = img->get_height();
      load_image(img, w, h, MipmapLoader(w,h));

      // Scale linearly when image is bigger than texture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // Enable mipmapping when image is smaller than texture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
#else
    void load_mipmap(Image::ConstRefArg img)
    {
      load_texture(img);
    }
#endif


#ifdef ARCHON_HAVE_GLU
    void load_mipmap_levels(Image::ConstRefArg img, int level,
                            int first, int last, int min_avail, int max_avail)
    {
      int const w = img->get_width(), h = img->get_height();
      if(w & w-1 != 0 || h & h-1 != 0)
        throw invalid_argument("Image width or height is not a power of two");
      if(first < 0) first = level;
      if(last  < 0) last  = first;
      if(min_avail < 0) min_avail = first;
      if(max_avail < 0) max_avail = last;
      if(last < first || first < level) throw invalid_argument("Bad level specification");
      load_image(img, w, h, MipmapLevelLoader(w, h, level, first, last));

      // Scale linearly when image is bigger than texture - disable mipmapping
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      // Scale linearly when image is smalled than texture - disable mipmapping
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

      // Set availble mipmap levels
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, min_avail);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  max_avail);
    }
#else
    void load_mipmap_levels(Image::ConstRefArg img, int, int, int, int, int)
    {
      load_texture(img);
    }
#endif



    void load_image(Image::ConstRefArg img, int width, int height,
                    ImageHandler const &handler)
    {
      Image::ConstRef img_keep_alive;

      ImageLoadInfo const &info = ImageLoadInfo::get();

      int num_channels;
      bool swap_bytes;
      int row_align_bytes;
      GLenum gl_format;
      GLenum gl_type;
      GLvoid const *buffer;

      // Use RGB if it the color space is neither Lum nor RGB.
      ColorSpace::ConstRef color_space = img->get_color_space();
      bool const has_alpha = img->has_alpha_channel();
      {
        ColorSpace::Type const type = color_space->get_type();
        if(type != ColorSpace::type_RGB && type != ColorSpace::type_Lum)
        {
          color_space = ColorSpace::get_RGB();
          num_channels = has_alpha ? 4 : 3;

          goto convert;
        }
      }
      num_channels = color_space->get_num_channels(has_alpha);

      // Check for a match against a format supported by OpenGL
      if(BufferedImage const *const buf_img = dynamic_cast<BufferedImage const *>(img.get()))
      {
        BufferFormat::ConstRef const buf_fmt = buf_img->get_buffer_format();
        if(IntegerBufferFormat const *const int_buf_fmt =
           dynamic_cast<IntegerBufferFormat const *>(buf_fmt.get()))
        {
//cerr << "Detected buffer format '"<<int_buf_fmt->print(img->get_color_space(), has_alpha)<<"'" << endl;

          // Check strip aligment if we have to
          int align_bytes = 1;
          if(1 < height)
          {
            int const bits_per_byte = numeric_limits<unsigned char>::digits;
            int const bits_per_strip = int_buf_fmt->get_bits_per_strip(width);
            if(int_buf_fmt->get_word_align_strips())
            {
              int const used = width * int_buf_fmt->get_bits_per_pixel();
              int const skipped = bits_per_strip - used;
              if(skipped < bits_per_byte)
              {
                if(bits_per_strip % bits_per_byte != 0) goto convert;
              }
              else if(skipped < 2*bits_per_byte)
              {
                if(bits_per_strip % 2*bits_per_byte != 0) goto convert;
                align_bytes = 2;
              }
              else if(skipped < 4*bits_per_byte)
              {
                if(bits_per_strip % 4*bits_per_byte != 0) goto convert;
                align_bytes = 4;
              }
              else if(skipped < 8*bits_per_byte)
              {
                if(bits_per_strip % 8*bits_per_byte != 0) goto convert;
                align_bytes = 8;
              }
              else goto convert;
            }
            else if(bits_per_strip % bits_per_byte != 0) goto convert;
          }

          if(ImageLoadInfo::GlImgFmt const *gl_img_fmt =
             info.format_map.find(IntegerBufferFormat::ConstRef(int_buf_fmt)))
          {
            swap_bytes      = gl_img_fmt->swap_bytes;
            row_align_bytes = align_bytes;
            gl_format       = gl_img_fmt->format;
            gl_type         = gl_img_fmt->type;
            buffer          = buf_img->get_buffer_ptr();
            goto ready;
          }
        }
      }

    convert: // Incoming image can not be used directly, must be converted.
      {
        // Choose a word type based on the detected channel width and
        // the ability for strip alignment.
        int const cps = width * num_channels;
        int const bpb = numeric_limits<unsigned char>::digits;
        int channel_width = img->get_channel_width();
        if(channel_width <= 8)
        {
          if(info.have_word_type_8bit       && cps*8  % bpb == 0) channel_width =  8;
          else if(info.have_word_type_16bit && cps*16 % bpb == 0) channel_width = 16;
          else if(info.have_word_type_32bit && cps*32 % bpb == 0) channel_width = 32;
          else throw runtime_error("No suitable word type");
        }
        else if(channel_width <= 16)
        {
          if(info.have_word_type_16bit      && cps*16 % bpb == 0) channel_width = 16;
          else if(info.have_word_type_32bit && cps*32 % bpb == 0) channel_width = 32;
          else if(info.have_word_type_8bit  && cps*8  % bpb == 0) channel_width =  8;
          else throw runtime_error("No suitable word type");
        }
        else
        {
          if(info.have_word_type_32bit      && cps*32 % bpb == 0) channel_width = 32;
          else if(info.have_word_type_16bit && cps*16 % bpb == 0) channel_width = 16;
          else if(info.have_word_type_8bit  && cps*8  % bpb == 0) channel_width =  8;
          else throw runtime_error("No suitable word type");
        }
        WordType word_type = channel_width == 8 ? info.word_type_8bit :
          channel_width == 16 ? info.word_type_16bit : info.word_type_32bit;

        // Convert the image
        IntegerBufferFormat::ChannelLayout channel_layout;
        for(int i=0; i<num_channels; ++i) channel_layout.add(channel_width);
        IntegerBufferFormat::Ref const buf_fmt =
          IntegerBufferFormat::get_format(word_type, channel_layout, false, false);
        BufferedImage::Ref buf_img =
          BufferedImage::new_image(width, height, color_space, has_alpha, buf_fmt);
        buf_img->put_image(img, 0, 0, false);
        img_keep_alive = buf_img; // To keep the new image 'alive'

        // Post the format for OpenGL
        swap_bytes      = false;
        row_align_bytes = 1;
        gl_format       = (num_channels == 4 ? GL_RGBA : num_channels == 3 ? GL_RGB :
                           num_channels == 2 ? GL_LUMINANCE_ALPHA : GL_LUMINANCE);
        gl_type         = (channel_width == 8 ? GL_UNSIGNED_BYTE :
                           channel_width == 16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
        buffer          = buf_img->get_buffer_ptr();

//cerr << "Conversion to '"<<buf_fmt->print(color_space, has_alpha)<<"'" << endl;
      }

    ready:
      // Do the actual loading
      glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
      glPixelStorei(GL_UNPACK_SWAP_BYTES, swap_bytes);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
      glPixelStorei(GL_UNPACK_ALIGNMENT, row_align_bytes);

      ARCHON_ASSERT_1(0 < num_channels && num_channels <= 4, "Wrong number of channels");
      handler.handle(num_channels, gl_format, gl_type, buffer);

      glPopClientAttrib();
    }
  }
}
