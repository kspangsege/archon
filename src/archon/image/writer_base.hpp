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

#ifndef ARCHON_IMAGE_WRITER_BASE_HPP
#define ARCHON_IMAGE_WRITER_BASE_HPP

#include <archon/math/functions.hpp>
#include <archon/image/reader.hpp>


namespace archon
{
  namespace Imaging
  {
    struct WriterBase: ImageReader
    {
    protected:
      friend struct ReaderBase;


      WriterBase(Image::ConstRefArg image);


      void enable_blending(bool enable);


      void enable_color_mapping(bool enable);


      void fill(bool foreground);


      void put_pixel(Util::PackedTRGB color);


      /**
       * \param c If null, RGB is assumed.
       */
      template<typename T, bool has_alpha> void put_pixel_smart(T const *p, ColorSpace const *c);


      /**
       * \param c If null, RGB is assumed.
       */
      template<bool has_alpha> void put_pixel(void const *p, ColorSpace const *c, WordType t);


      void put_block(ConstTupleGrid g, PixelFormat const &f, int width, int height);


      /**
       * The region is x,y,w,h assumed to be confined to the source
       * image area. If may be empty though (w==0 || h==0).
       */
      void put_image(Image::Codec const *c, PixelFormat const &f, int x, int y, int w, int h);


      void blend_read(int x, int y, int w, int h);


      bool blending_enabled, color_mapping_enabled;


    private:
      template<bool noop, bool dense> struct WriteOp;
      typedef WriteOp<true,  false> NoopWriteOp;
      typedef WriteOp<false, true>  DenseWriteOp;
      typedef WriteOp<false, false> GridWriteOp;
      struct ImageToImageOp;

      void prep_write_cvt(PixelFormat const &);
      void prep_blend_read_cvt();

      ColorSpace::ConstRef const lum;
      PixelConverter write_cvt;
      TransferFormat write_cvt_format;
      size_t write_cvt_max_pixels_per_block; // Only initialized when not noop and assumes dense operation
      bool write_cvt_is_blending; // Only initialized when not noop

      PixelConverter blend_read_cvt; // Reads to RGB / RGBA in best_float word type.
      Core::MemoryBuffer blend_read_buffer;
      bool blend_read_cvt_initialized;


      struct WriterCvtStepsBase
      {
        virtual bool color_map_need_alpha(WriterBase const *) const = 0;
        virtual void color_map(WriterBase const *, char const *, char *, size_t) const = 0;
        virtual void alpha_blend(WriterBase const *, char const *, char *, size_t) const = 0;
        virtual ~WriterCvtStepsBase() {}
      };

      template<typename T, WordType> struct WriterCvtSteps: WriterCvtStepsBase
      {
        bool color_map_need_alpha(WriterBase const *) const;
        void color_map(WriterBase const *, char const *, char *, size_t) const;
        void alpha_blend(WriterBase const *, char const *, char *, size_t) const;
        WriterCvtStepsBase const *operator()() const { return this; }
      };

      struct WriterCvtStepsSwitch:
        WordTypeSwitch<WriterCvtSteps, void, WriterCvtStepsBase const *, true> {};

      static WriterCvtStepsSwitch writer_cvt_steps_switch;

      WriterCvtStepsBase const *const writer_cvt_steps;


      // NOTE: It is the responsibility of the user to ensure that
      // neither of the two methods are called until after
      // init_color_slots() has been called for the associated
      // WriterBase.
      struct ColorMapper: PixelConverter::Manipulator
      {
        bool need_alpha() const
        {
          return writer->writer_cvt_steps->color_map_need_alpha(writer);
        }
        void manip(char const *s, char *t, size_t n) const
        {
          writer->writer_cvt_steps->color_map(writer, s, t, n);
        }
        ColorMapper(WriterBase const *w): writer(w) {}
        WriterBase const *const writer;
      };

      ColorMapper const color_mapper;


      struct AlphaBlender: PixelConverter::Manipulator
      {
        void manip(char const *s, char *t, size_t n) const
        {
          writer->writer_cvt_steps->alpha_blend(writer, s, t, n);
        }
        AlphaBlender(WriterBase const *w): writer(w) {}
        WriterBase const *const writer;
      };

      AlphaBlender const alpha_blender;
    };






    // Implementation:

    template<bool noop, bool dense> struct WriterBase::WriteOp
    {
      void operator()(int x, int y, int w, int h) const
      {
        Image::Codec *const c = const_cast<Image::Codec *>(writer->codec.get());
        if(noop)
        {
          c->encode(ConstTupleGrid(grid.origin + y * grid.stride + x * grid.pitch,
                                   grid.pitch, grid.stride), w, h, x0+x, y0+y);
          return;
        }

        if(writer->write_cvt_is_blending) writer->blend_read(x0+x, y0+y, w, h);

        PixelConverter const &cvt = writer->write_cvt;
        char *const target = cvt.get_internal_target();
        size_t const n = h * size_t(w);
        if(dense) cvt(grid.origin + y * grid.stride + x * grid.pitch, target, n);
        else
        {
          char *const source = cvt.get_internal_source();
          ConstTupleGrid(grid.origin + y * grid.stride + x * grid.pitch, grid.pitch,
                         grid.stride).contract_to(source, cvt.get_source_pixel_size(), w, h);
          cvt(source, target, n);
        }

        ssize_t const pitch = cvt.get_target_pixel_size();
        c->encode(ConstTupleGrid(target, pitch, w*pitch), w, h, x0+x, y0+y);
      }

      WriteOp(WriterBase *w, ConstTupleGrid const &g, int x0, int y0):
        writer(w), grid(g), x0(x0), y0(y0)
      {
        // When writing from a sparse grid, we need the internal source buffer
        if(!noop && !dense) writer->write_cvt.ensure_internal_source(writer->buffers);
      }

    private:
      WriterBase *const writer;
      ConstTupleGrid const &grid; // The source data
      int const x0, y0;
    };



    struct WriterBase::ImageToImageOp
    {
      void operator()(int x, int y, int w, int h) const
      {
        PixelConverter const &cvt = writer->write_cvt;
        if(!cvt.is_noop() && writer->write_cvt_is_blending) writer->blend_read(x1+x, y1+y, w, h);

        char *const source = cvt.is_noop() ? writer->buffers.get_first() : cvt.get_internal_source();
        char *const target = cvt.is_noop() ? source : cvt.get_internal_target();
        ssize_t pitch = cvt.get_source_pixel_size();
        source_codec->decode(TupleGrid(source, pitch, w*pitch), w, h, x0+x, y0+y);

        if(!cvt.is_noop()) cvt(source, target, h * size_t(w));

        pitch = cvt.get_target_pixel_size();
        const_cast<Image::Codec *>(writer->codec.get())->
          encode(ConstTupleGrid(target, pitch, w*pitch), w, h, x1+x, y1+y);
      }

      ImageToImageOp(WriterBase *w, Image::Codec const *c, int x0, int y0, int x1, int y1):
        writer(w), source_codec(c), x0(x0), y0(y0), x1(x1), y1(y1) {}

    private:
      WriterBase *const writer;
      Image::Codec const *const source_codec;
      int const x0, y0, x1, y1;
    };



    inline WriterBase::WriterBase(Image::ConstRefArg image):
      ImageReader(image), blending_enabled(false), color_mapping_enabled(false),
      lum(ColorSpace::get_Lum()), blend_read_cvt_initialized(false),
      writer_cvt_steps(writer_cvt_steps_switch(best_float)),
      color_mapper(this), alpha_blender(this) {}


    inline void WriterBase::enable_blending(bool enable)
    {
      if(enable == blending_enabled) return;
      blending_enabled = enable;
      write_cvt_initialized = false;
    }


    inline void WriterBase::enable_color_mapping(bool enable)
    {
      if(enable == color_mapping_enabled) return;
      color_mapping_enabled = enable;
      write_cvt_initialized = false;
    }


    inline void WriterBase::fill(bool foreground)
    {
      int const w = clip_right - clip_left, h = clip_top - clip_bottom;
      if(0 < w && 0 < h)
        const_cast<Image::Codec *>(codec.get())->
          encode(ConstTupleGrid(get_color_ptr(foreground), 0, 0), w, h, clip_left, clip_bottom);
    }


    inline void WriterBase::put_pixel(Util::PackedTRGB color)
    {
      unsigned char b[4];
      color.unpack_rgba(b);
      put_pixel_smart<unsigned char, true>(b, 0);
    }


    template<typename T, bool has_alpha> inline void WriterBase::put_pixel_smart(T const *p, ColorSpace const *c)
    {
      if(has_alpha && Util::frac_complement(p[c ? c->get_num_primaries() : 3]) ||
         write_cvt_initialized && write_cvt_format.has_alpha &&
         write_cvt_format.color_space == (c ? c : rgb.get()))
      {
        if(has_alpha)
        {
          put_pixel<true>(p, c, get_word_type_by_type<T>());
          return;
        }

        // We would rather add the alpha channel, than re-initialize
        // the pixel converter
        int const n = c ? c->get_num_primaries() : 3;
        if(n < 9) // We will handle up to 8 primaries
        {
          T b[8];
          *std::copy(p, p+n, b) = Util::frac_full<T>();
          put_pixel<true>(b, c, get_word_type_by_type<T>());
          return;
        }
      }

      put_pixel<false>(p, c, get_word_type_by_type<T>());
    }


    template<bool has_alpha> inline void WriterBase::put_pixel(void const *p, ColorSpace const *c, WordType t)
    {
      PixelFormat f(c ? c : rgb.get(), has_alpha, t);
      put_block(ConstTupleGrid(reinterpret_cast<char const *>(p),
                               f.bytes_per_pixel, f.bytes_per_pixel), f, 1, 1);
    }


    inline void WriterBase::put_block(ConstTupleGrid g, PixelFormat const &f, int w, int h)
    {
      // Clip the image region
      int x = get_block_pos_x(w), y = get_block_pos_y(h);
      clip_tray(g, x, y, w, h);

      // If all of the tray was clipped, we are done
      if(w <= 0 || h <= 0) return;

      // Reinitialize the pixel converter if we have to
      if(!write_cvt_initialized || f.format != write_cvt_format) prep_write_cvt(f);

      size_t const n = h * size_t(w);

      if(write_cvt.is_noop())
      {
        NoopWriteOp op(this, g, x, y);
        if(n <= Image::Codec::get_max_pixels_per_block()) op(0, 0, w, h);
        else subdivide_block_op(op, w, h, Image::Codec::get_max_pixels_per_block());
        return;
      }

      bool const dense_rows = g.pitch == f.bytes_per_pixel;
      bool const dense_grid = dense_rows && g.stride == w * g.pitch;

      // If the grid is dense, and we do not have to subdivide, we can
      // skip the first step that condenses pixels before convertion.
      if(dense_grid && n <= write_cvt_max_pixels_per_block) DenseWriteOp(this, g, x, y)(0, 0, w, h);
      else
      {
        GridWriteOp op(this, g, x, y);
        size_t max_pixels_per_block =
          std::min(buffers.get_size_of_buffers() / f.bytes_per_pixel, write_cvt_max_pixels_per_block);
        if(n <= max_pixels_per_block) op(0, 0, w, h);
        else subdivide_block_op(op, w, h, max_pixels_per_block);
      }
    }


    inline void WriterBase::put_image(Image::Codec const *c, PixelFormat const &f,
                                      int source_x, int source_y, int w, int h)
    {
      int x = get_block_pos_x(w), y = get_block_pos_y(h);

      // Clip against left edge
      int d = clip_left - x;
      if(0 < d)
      {
        source_x += d;
        x = clip_left;
        w -= d;
      }

      // Clip against right edge
      d = x + w - clip_right;
      if(0 < d) w -= d;

      // Clip against bottom edge
      d = clip_bottom - y;
      if(0 < d)
      {
        source_y += d;
        y = clip_bottom;
        h -= d;
      }

      // Clip against top edge
      d = y + h - clip_top;
      if(0 < d) h -= d;

      // If all of the image was clipped, we are done
      if(w <= 0 || h <= 0) return;

      // Reinitialize the pixel converter if we have to
      if(!write_cvt_initialized || f.format != write_cvt_format) prep_write_cvt(f);
      if(!write_cvt.is_noop()) write_cvt.ensure_internal_source(buffers);

      size_t const m = std::min(buffers.get_size_of_buffers() / f.bytes_per_pixel,
                                write_cvt.is_noop() ? Image::Codec::get_max_pixels_per_block() :
                                write_cvt_max_pixels_per_block);

      ImageToImageOp op(this, c, source_x, source_y, x, y);
      if(h * size_t(w) <= m) op(0, 0, w, h);
      else subdivide_block_op(op, w, h, m);
    }


    inline void WriterBase::prep_write_cvt(PixelFormat const &f)
    {
      prep_color_slots();

      PixelConverter::ConvSpec convs[2];
      int num_convs = 0;

      bool alpha;
      if(color_mapping_enabled)
      {
        alpha = color_mapper.need_alpha();
        convs[num_convs++] =
          PixelConverter::ConvSpec(&color_mapper,
                                   TransferFormat(lum.get(), false, best_float),
                                   TransferFormat(rgb.get(), alpha, best_float));
      }
      else alpha = f.format.has_alpha;

      write_cvt_is_blending = false;
      if(alpha)
      {
        if(blending_enabled)
        {
          if(!blend_read_cvt_initialized) prep_blend_read_cvt();
          bool const alpha = pixel_format.format.has_alpha;
          convs[num_convs++] =
            PixelConverter::ConvSpec(&alpha_blender,
                                     TransferFormat(rgb.get(), true,  best_float),
                                     TransferFormat(rgb.get(), alpha, best_float));
          write_cvt_is_blending = true;
        }
        // If the background color is black, it is better to ask the
        // color space converter to do the blending.
        else if(!pixel_format.format.has_alpha && !background_blender.is_bg_clean())
        {
          convs[num_convs++] =
            PixelConverter::ConvSpec(&background_blender,
                                     TransferFormat(rgb.get(), true,  best_float),
                                     TransferFormat(rgb.get(), false, best_float));
        }
      }

      write_cvt.init(f, pixel_format, buffers, convs, num_convs);

      if(!write_cvt.is_noop())
      {
        write_cvt.ensure_internal_target(buffers);

        int n = std::max(write_cvt.get_max_intermediate_pixel_size(), pixel_format.bytes_per_pixel);
        if(write_cvt_is_blending)
        {
          int const m =  blend_read_cvt.get_max_intermediate_pixel_size();
          if(n < m) n = m;
        }
        write_cvt_max_pixels_per_block =
          std::min(size_t(Image::Codec::get_max_pixels_per_block()),
                   buffers.get_size_of_buffers() / n);
      }

      write_cvt_format = f.format;
      write_cvt_initialized = true;
    }


    inline void WriterBase::prep_blend_read_cvt()
    {
      PixelFormat f(rgb.get(), pixel_format.format.has_alpha, best_float);
      blend_read_cvt.init(pixel_format, f, buffers);
      if(!blend_read_cvt.is_noop()) blend_read_cvt.ensure_internal_source(buffers);
      blend_read_buffer.reset(buffers.get_size_of_buffers());
      blend_read_cvt_initialized = true;
    }


    inline void WriterBase::blend_read(int x, int y, int w, int h)
    {
      char *const t = blend_read_buffer.get();
      char *const s = blend_read_cvt.is_noop() ? t : blend_read_cvt.get_internal_source();
      ssize_t const pitch = pixel_format.bytes_per_pixel;
      codec->decode(TupleGrid(s, pitch, w*pitch), w, h, x, y);
      if(!blend_read_cvt.is_noop()) blend_read_cvt(s, t, h * size_t(w));
    }


    template<typename T, WordType v>
    inline bool WriterBase::WriterCvtSteps<T,v>::color_map_need_alpha(WriterBase const *w) const
    {
      T const *rgba = reinterpret_cast<T *>(w->get_rgba_ptr(false)); // Background
      if(rgba[3] < 1) return true;
      rgba = reinterpret_cast<T *>(w->get_rgba_ptr(true)); // Foreground
      return rgba[3] < 1;
    }


    template<typename T, WordType v>
    inline void WriterBase::WriterCvtSteps<T,v>::color_map(WriterBase const *w,
                                                           char const *src, char *tgt,
                                                           size_t n) const
    {
      T const *const bg = reinterpret_cast<T *>(w->get_rgba_ptr(false));
      T const *const fg = reinterpret_cast<T *>(w->get_rgba_ptr(true));
      T const *s = reinterpret_cast<T const *>(src);
      T       *t = reinterpret_cast<T       *>(tgt);
      int const m = color_map_need_alpha(w) ? 4 : 3;
      for(size_t i=0; i<n; ++i)
      {
        for(int j=0; j<m; ++j) t[j] = Math::lin_interp(*s, 0, 1, bg[j], fg[j]);
        s += 1;
        t += m;
      }
    }


    template<typename T, WordType v>
    inline void WriterBase::WriterCvtSteps<T,v>::alpha_blend(WriterBase const *w,
                                                             char const *src, char *tgt,
                                                             size_t n) const
    {
      T const *r = reinterpret_cast<T const *>(w->blend_read_buffer.get());
      T const *s = reinterpret_cast<T const *>(src);
      T       *t = reinterpret_cast<T       *>(tgt);
      if(w->pixel_format.format.has_alpha)
      {
        // The target buffer has an alpha channel, so we need the full
        // version.
        for(size_t i=0; i<n; ++i)
        {
          if(T const a2 = s[3])
          {
            if(T const b2 = 1 - a2)
            {
              T const a1 = r[3];
              T const b1 = 1-a1;
              T const a3 = 1 - b1*b2;
              if(a3)
              {
                T const f1 = a1 * b2 / a3, f2 = a2 / a3;
                for(int j=0; j<3; ++j) t[j] = f1*r[j] + f2*s[j];
                t[3] = a3;
              }
              else std::copy(s, s+4, t);
            }
            else std::copy(s, s+4, t); // alpha = 1
          }
          else std::copy(r, r+4, t); // alpha = 0
          r += 4;
          s += 4;
          t += 4;
        }
      }
      else
      {
        // The target buffer has no alpha channel, so we can do with a
        // reduced version.
        for(size_t i=0; i<n; ++i)
        {
          if(T const a = s[3])
          {
            if(T const b = 1 - a) for(int j=0; j<3; ++j) t[j] = b*r[j] + a*s[j];
            else std::copy(s, s+3, t); // alpha = 1
          }
          else std::copy(r, r+3, t); // alpha = 0
          r += 3;
          s += 4;
          t += 3;
        }
      }
    }
  }
}


#endif // ARCHON_IMAGE_WRITER_BASE_HPP
