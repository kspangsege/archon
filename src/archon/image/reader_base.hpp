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

#ifndef ARCHON_IMAGE_READER_BASE_HPP
#define ARCHON_IMAGE_READER_BASE_HPP

#include <cmath>
#include <algorithm>

#include <archon/core/functions.hpp>
#include <archon/core/memory.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/misc.hpp>
#include <archon/util/packed_trgb.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/image/image.hpp>
#include <archon/image/pixel_converter.hpp>


namespace archon
{
  namespace image
  {
    /**
     * This class provides all the low-level reading features that are
     * common to an <tt>ImageReader</tt> and and <tt>ImageWriter</tt>.
     */
    struct ReaderBase
    {
    protected:
      typedef util::TupleGrid TupleGrid;
      typedef util::ConstTupleGrid ConstTupleGrid;
      typedef PixelConverter::Format PixelFormat;

      ReaderBase(Image::ConstRefArg image);

      void set_pos(double x, double y) { pos_x = x; pos_y = y; }

      void set_pos_align(double x, double y) { pos_align_x = x; pos_align_y = y; }

      void set_clip(int left, int bottom, int width, int height);

      void set_falloff(Falloff h, Falloff v) { horiz_falloff = h; vert_falloff = v; }

      void set_color(util::PackedTRGB color, bool foreground);

      util::PackedTRGB get_pixel();

      /**
       * \param c If null, RGB is assumed.
       */
      template<typename T, bool has_alpha> void get_pixel_smart(T *p, ColorSpace const *c);

      /**
       * \param c If null, RGB is assumed.
       */
      template<bool has_alpha> void get_pixel(void *p, ColorSpace const *c, WordType t);

      void get_block(TupleGrid g, PixelFormat const &f, int width, int height);

      int get_block_pos_x(int block_width)  const;

      int get_block_pos_y(int block_height) const;

      template<class Op> static void subdivide_block_op(Op &, int width, int height,
                                                        size_t max_pixels_per_subblock);

      // 0 = nothing left (args are undefined), 1 = partial clip, 2 = nothing clipped (args are untouched)
      template<typename T>
      void clip_tray(util::BasicTupleGrid<T> &g, int &x, int &y, int &w, int &h);

      char *get_color_ptr(bool foreground) const;

      void prep_color_slots();

      char *get_rgba_ptr(bool foreground) const;

      // Simply a trick used by WriterOps::put_image to get access to
      // various protected attributes of the ImageReader argument.
      template<class WriterBase>
      static void put_image(ReaderBase const &r, WriterBase &w, int width, int height)
      {
        int x = r.get_block_pos_x(width);
        int y = r.get_block_pos_y(width);
	w.put_image(r.codec.get(), r.pixel_format, x, y, width, height);
      }

      Image::ConstRef const image;
      int const image_width, image_height; // Actual dimmensions of accessed image
      PixelFormat const pixel_format;
      WordType const best_float; // The floating point type that best covers pixel_format.word_type
      int bytes_per_best_float;
      ColorSpace::ConstRef const rgb;
      Image::CodecConstPtr codec;

      PixelConverter::Buffers buffers;
      bool read_cvt_initialized;
      bool write_cvt_initialized; // Placed here so that it can be cleared when backgound color changes

      // 'clip_left' is confined to the interval [0;image_width]
      // 'clip_right' is confined to the interval [clip_left;image_width]
      // 'clip_bottom' is confined to the interval [0;image_height]
      // 'clip_top' is confined to the interval [clip_bottom;image_height]
      int clip_left, clip_right, clip_bottom, clip_top;

      Falloff horiz_falloff, vert_falloff;

    private:
      core::MemoryBuffer color_slot_buffer;

      double pos_x, pos_y, pos_align_x, pos_align_y;

      template<bool noop, bool dense> struct ReadOp;
      typedef ReadOp<true,  false> NoopReadOp;
      typedef ReadOp<false, true>  DenseReadOp;
      typedef ReadOp<false, false> GridReadOp;

      void prep_read_cvt(PixelFormat const &);

      template<bool vertical>
      bool fix_falloff(int falloff_left, int falloff_right, int clip_width,
                       TupleGrid &g, int &x, int &width, int &width2,
                       int &rep_left1, int &rep_right1, int &rep_left2, int &rep_right2) const;

      void get_restricted_block(TupleGrid g, PixelFormat const &f, int x, int y, int w, int h);

      void clear_tray(TupleGrid g, PixelFormat const &f, int w, int h);

      void store_color(char const *b, PixelFormat const &f, bool foreground);

      void cvt(PixelFormat const &source_format, char const *source,
               PixelFormat const &target_format, char *target, size_t n);

      static size_t const num_color_slots = 2;

      PixelConverter read_cvt;
      TransferFormat read_cvt_format;

      // Only initialized when not noop and assumes dense operation
      size_t read_cvt_max_pixels_per_block;


      struct ReaderCvtStepsBase
      {
        virtual bool is_bg_clean(ReaderBase const *) const = 0; // True if transparent or black
        virtual void background_blend(ReaderBase const *, char const *, char *, size_t) const = 0;
        virtual ~ReaderCvtStepsBase() {}
      };

      template<typename T, WordType> struct ReaderCvtSteps: ReaderCvtStepsBase
      {
        bool is_bg_clean(ReaderBase const *) const;
        void background_blend(ReaderBase const *, char const *, char *, size_t) const;
        ReaderCvtStepsBase const *operator()() const { return this; }
      };

      struct ReaderCvtStepsSwitch:
        WordTypeSwitch<ReaderCvtSteps, void, ReaderCvtStepsBase const *, true> {};

      static ReaderCvtStepsSwitch reader_cvt_steps_switch;

      ReaderCvtStepsBase const *const reader_cvt_steps;

    protected:
      // NOTE: It is the responsibility of the user to ensure that
      // neither of the two methods are called until after
      // prep_color_slots() has been called for the associated
      // ReaderBase.
      struct BackgroundBlender: PixelConverter::Manipulator
      {
        bool is_bg_clean() const // True if transparent or black
        {
          return reader->reader_cvt_steps->is_bg_clean(reader);
        }
        void manip(char const *s, char *t, size_t n) const
        {
          reader->reader_cvt_steps->background_blend(reader, s, t, n);
        }
        BackgroundBlender(ReaderBase const *r): reader(r) {}
        ReaderBase const *const reader;
      };

      BackgroundBlender const background_blender;
    };






    // Implementation:

    template<bool noop, bool dense> struct ReaderBase::ReadOp
    {
      void operator()(int x, int y, int w, int h) const
      {
        if(noop)
        {
          reader->codec->decode(TupleGrid(grid.origin + y * grid.stride + x * grid.pitch,
                                          grid.pitch, grid.stride), w, h, x0+x, y0+y);
          return;
        }

        PixelConverter const &cvt = reader->read_cvt;
        char *const source = cvt.get_internal_source();
        ssize_t const pitch = cvt.get_source_pixel_size();
        reader->codec->decode(TupleGrid(source, pitch, w*pitch), w, h, x0+x, y0+y);
        size_t const n = h * size_t(w);

        if(dense)
        {
          cvt(source, grid.origin + y * grid.stride + x * grid.pitch, n);
          return;
        }

        char *const target = cvt.get_internal_target();
        cvt(source, target, n);
        TupleGrid(grid.origin + y * grid.stride + x * grid.pitch,
                  grid.pitch, grid.stride).expand_from(target, cvt.get_target_pixel_size(), w, h);
      }

      ReadOp(ReaderBase *r, TupleGrid const &g, int x0, int y0):
        reader(r), grid(g), x0(x0), y0(y0)
      {
        // When reading to a sparse grid, we need the internal target buffer
        if(!noop && !dense) reader->read_cvt.ensure_internal_target(reader->buffers);
      }

    private:
      ReaderBase *const reader;
      TupleGrid const &grid;
      int const x0, y0;
    };



    inline ReaderBase::ReaderBase(Image::ConstRefArg image):
      image(image), image_width(image->get_width()), image_height(image->get_height()),
      pixel_format(image->get_color_space().get(), image->has_alpha_channel(), image->get_word_type()),
      best_float(get_smallest_float_cover(pixel_format.format.word_type)),
      bytes_per_best_float(get_bytes_per_word(best_float)), rgb(ColorSpace::get_RGB()),
      buffers(std::max((ColorSpace::max_num_primaries + 1) * get_max_bytes_per_word(), 2048)),
      read_cvt_initialized(false), write_cvt_initialized(false),
      reader_cvt_steps(reader_cvt_steps_switch(best_float)), background_blender(this)
    {
      codec.reset(image->acquire_codec().release());

      set_clip(0, 0, -1, -1);
      set_pos(0, 0);
      set_pos_align(0, 0);
      set_falloff(falloff_Background, falloff_Background);
    }


    inline void ReaderBase::set_clip(int l, int b, int w, int h)
    {
      clip_left   = core::clamp(l, 0, image_width);
      clip_bottom = core::clamp(b, 0, image_height);
      clip_right  = w < 0 ? image_width  : core::clamp(l + w, clip_left,   image_width);
      clip_top    = h < 0 ? image_height : core::clamp(b + h, clip_bottom, image_height);
    }


    inline void ReaderBase::set_color(util::PackedTRGB color, bool foreground)
    {
      unsigned char b[4];
      color.unpack_rgba(b);
      store_color(reinterpret_cast<char *>(b),
                  PixelFormat(rgb.get(), true, word_type_UChar), foreground);
    }


    inline util::PackedTRGB ReaderBase::get_pixel()
    {
      unsigned char rgba[4];
      get_pixel_smart<unsigned char, true>(rgba, 0);
      return util::PackedTRGB::pack_rgba(rgba);
    }


    template<typename T, bool has_alpha>
    inline void ReaderBase::get_pixel_smart(T *p, ColorSpace const *c)
    {
      if(has_alpha && (pixel_format.format.has_alpha || read_cvt_initialized &&
                       read_cvt_format.has_alpha && read_cvt_format.color_space == (c ? c : rgb.get()))) {
        get_pixel<true>(p, c, get_word_type_by_type<T>());
      }
      else {
        get_pixel<false>(p, c, get_word_type_by_type<T>());
        if(has_alpha) p[c ? c->get_num_primaries() : 3] = util::frac_full<T>();
      }
    }


    template<bool has_alpha>
    inline void ReaderBase::get_pixel(void *p, ColorSpace const *c, WordType t)
    {
      PixelFormat f(c ? c : rgb.get(), has_alpha, t);
      get_block(TupleGrid(reinterpret_cast<char *>(p), f.bytes_per_pixel, f.bytes_per_pixel), f, 1, 1);
    }


    inline void ReaderBase::get_block(TupleGrid g, PixelFormat const &f, int w, int h)
    {
      // Determine falloffs


      // "falloff" setting can bee "background", "edge", "repeat"

      // Since the requested block falls off the edges of the clipping
      // region, we need to consider the "falloff" settings.

      // If the horizontal falloff setting is "repeat", then
      // translate the block position horizontally by an integer
      // multiple of the width of the clipping region, such that the
      // left edge of the block falls within the infinite vertical
      // extension of the clipping region. If the setting is "edge"
      // and the block lies completely to the left or right of the
      // clipping region, translate the block horizontally such that
      // it gets a single pixel overlap with the infinite vertical
      // extension of the clipping region.

      // If the background color is needed anywhere, we simply start
      // by filling the tray with it.

      // Extract main region

      // To fill the callers tray (excluding the part that needs to be
      // filled with the background color), we use the following
      // procedure: First place in the tray a "proto block" that
      // contains all the required information, then expand that block
      // to fill the rest of the tray. This expansion has two phases,
      // first it is expanded by repeating the proto block, then it is
      // expanded further by repeating the edge of the previous
      // expansion. This expansion is handled conveniently by
      // util::BasicTupleGrid::extend.

      // We assemble the proto block as follows: If the tray is both wider and taller than the clipping region, the proto block is the entire clipping region. Otherwise if the tray is narrower than the clipping region, 

      // To assemble the proto block we need to read at most four
      // separate blocks from the image.

      if(w < 1 || h < 1) return;

      // Reinitialize the pixel converter if we have to
      if(!read_cvt_initialized || f.format != read_cvt_format) prep_read_cvt(f);

      int x = get_block_pos_x(w), y = get_block_pos_y(h);
      int const falloff_left   = clip_left   - x, falloff_right = x + w - clip_right;
      int const falloff_bottom = clip_bottom - y, falloff_top   = y + h - clip_top;

      bool const horiz = 0 < falloff_left   || 0 < falloff_right;
      bool const  vert = 0 < falloff_bottom || 0 < falloff_top;

      // Fast "track" for the case where the requested block is
      // already confined to the clipping region.
      if(!horiz && !vert)
      {
        get_restricted_block(g, f, x, y, w, h);
        return;
      }

      // If the clipping region is is empty, we must fill the tray
      // with the background color regardless of the faloff settings,
      // since we have no pixels to copy.
      int const clip_width = clip_right - clip_left, clip_height = clip_top - clip_bottom;
      bool const empty_clip = clip_width < 1 || clip_height < 1;
      if(horiz && horiz_falloff == falloff_Background ||
         vert  &&  vert_falloff == falloff_Background || empty_clip)
      {
        clear_tray(g,f,w,h);
        if(empty_clip) return;
      }

      int w2 = 0, h2 = 0; // Width and height of upper right quarter block

      int rep_left1 = 0, rep_right1 = 0, rep_bottom1 = 0, rep_top1 = 0;
      int rep_left2 = 0, rep_right2 = 0, rep_bottom2 = 0, rep_top2 = 0;

      if(horiz && fix_falloff<false>(falloff_left, falloff_right, clip_width,
                                     g, x, w, w2, rep_left1, rep_right1, rep_left2, rep_right2) ||
         vert  && fix_falloff<true>(falloff_bottom, falloff_top, clip_height,
                                    g, y, h, h2, rep_bottom1, rep_top1, rep_bottom2, rep_top2))
        return;

      // Now there is a non-empty intersection between the tray and
      // the clipping region.

      get_restricted_block(g, f, x, y, w, h);

      if(w2)
      {
        TupleGrid g2 = g;
        g2.move_right(w);
        get_restricted_block(g2, f, clip_left, y, w2, h);
      }

      if(h2)
      {
        TupleGrid g2 = g;
        g2.move_up(h);
        get_restricted_block(g2, f, x, clip_bottom, w, h2);

        if(w2)
        {
          g2.move_right(w);
          get_restricted_block(g2, f, clip_left, clip_bottom, w2, h2);
        }
      }

      if(horiz || vert)
      {
        int const proto_width  = w + w2;
        int const proto_height = h + h2;
        g.extend(f.bytes_per_pixel, proto_width, proto_height,
                 rep_left1, rep_right1, rep_bottom1, rep_top1,
                 rep_left2, rep_right2, rep_bottom2, rep_top2);
      }
    }


    inline int ReaderBase::get_block_pos_x(int block_width)  const
    {
      return std::floor(pos_x - pos_align_x * block_width);
    }


    inline int ReaderBase::get_block_pos_y(int block_height) const
    {
      return std::floor(pos_y - pos_align_y * block_height);
    }


    template<bool vert> inline
    bool ReaderBase::fix_falloff(int falloff_left, int falloff_right, int clip_width,
                                 TupleGrid &g, int &x, int &w, int &w2,
                                 int &rep_left1, int &rep_right1,
                                 int &rep_left2, int &rep_right2) const
    {
      typedef void (TupleGrid::*MoveRight)(int width);
      MoveRight const move_right = vert ? &TupleGrid::move_up : &TupleGrid::move_right;
      int const clip_left  = vert ? this->clip_bottom : this->clip_left;
      int const clip_right = vert ? this->clip_top    : this->clip_right;

      switch(vert ? vert_falloff : horiz_falloff)
      {
      case falloff_Background:
        if(0 < falloff_left)
        {
          w -= falloff_left;
          x += falloff_left;
          (g.*move_right)(falloff_left);
        }
        if(0 < falloff_right) w -= falloff_right;
        if(w < 1) return true; // Nothing is left
        break;

      case falloff_Edge:
        if(0 < falloff_left)
        {
          if(falloff_left < w) rep_left2 = falloff_left;
          else
          {
            // Slide tray to the right to get a 1-pixel overlap with
            // clipping region.
            rep_left2 = w - 1;
          }
          w -= rep_left2;
          x = clip_left;
          (g.*move_right)(rep_left2);
        }
        if(0 < falloff_right)
        {
          if(falloff_right < w) rep_right2 = falloff_right;
          else
          {
            // Slide tray to the left to get a 1-pixel overlap with
            // clipping region.
            rep_right2 = w - 1;
            x = clip_right - 1;
          }
          w -= rep_right2;
        }
        break;

      case falloff_Repeat:
        {
          int const l = core::modulo(falloff_left, clip_width);
          int const l2 = l ? l : clip_width;
          if(w <= l2)
          {
            // Tray spans only a single clipping module, so slide it
            // horizontally by an integer multiple of the clipping
            // width such that it falls within the clipping region.
            x = clip_right - l2;
          }
          else
          {
            int const r = w - l - clip_width;
            if(0 <= r)
            {
              // Horizontally we have a complete clipping module
              // within the tray.
              rep_left1 = l;
              rep_right1 = r;
              w = clip_width;
              x = clip_left;
              (g.*move_right)(rep_left1);
            }
            else
            {
              // Tray spans two clipping modules horizontally but does
              // not contain a complete module, so we are forced to
              // break the operation into two pieces.
              if(clip_width < w)
              {
                rep_right1 = w - clip_width;
                w2 = clip_width - l2;
              }
              else w2 = w - l2;
              w = l2;
              x = clip_width - l2;
            }
          }
        }
        break;
      }

      return false;
    }


    inline void ReaderBase::get_restricted_block(TupleGrid g, PixelFormat const &f,
                                                 int x, int y, int w, int h)
    {
      size_t const n = h * size_t(w);

      if(read_cvt.is_noop())
      {
        NoopReadOp op(this, g, x, y);
        if(n <= Image::Codec::get_max_pixels_per_block()) op(0, 0, w, h);
        else subdivide_block_op(op, w, h, Image::Codec::get_max_pixels_per_block());
        return;
      }

      bool const dense_rows = g.pitch == f.bytes_per_pixel;
      bool const dense_grid = dense_rows && g.stride == w * g.pitch;

      // If the grid is dense, and we do not have to subdivide, we can
      // skip the final step that spreads the converted pixels.
      if(dense_grid && n <= read_cvt_max_pixels_per_block) DenseReadOp(this, g, x, y)(0, 0, w, h);
      else
      {
        GridReadOp op(this, g, x, y);
        size_t max_pixels_per_block =
          std::min(read_cvt_max_pixels_per_block, buffers.get_size_of_buffers() / f.bytes_per_pixel);
        if(n <= max_pixels_per_block) op(0, 0, w, h);
        else subdivide_block_op(op, w, h, max_pixels_per_block);
      }
    }


    inline void ReaderBase::prep_read_cvt(PixelFormat const &f)
    {
      prep_color_slots();

      // If the background color is black, it is better to ask the
      // color space converter to do the blending.
      if(pixel_format.format.has_alpha && !f.format.has_alpha && !background_blender.is_bg_clean())
      {
        PixelConverter::ConvSpec b(&background_blender,
                                   TransferFormat(rgb.get(), true,  best_float),
                                   TransferFormat(rgb.get(), false, best_float));
        read_cvt.init(pixel_format, f, buffers, &b, 1);
        goto not_noop;
      }
      read_cvt.init(pixel_format, f, buffers);

      if(!read_cvt.is_noop())
      {
      not_noop:
        read_cvt.ensure_internal_source(buffers);
        read_cvt_max_pixels_per_block =
          std::min(size_t(Image::Codec::get_max_pixels_per_block()), buffers.get_size_of_buffers() /
                   std::max(pixel_format.bytes_per_pixel, read_cvt.get_max_intermediate_pixel_size()));
      }

      read_cvt_format = f.format;
      read_cvt_initialized = true;
    }


    template<typename T, WordType w>
    inline bool ReaderBase::ReaderCvtSteps<T,w>::is_bg_clean(ReaderBase const *r) const
    {
      T const *const rgba = reinterpret_cast<T *>(r->get_rgba_ptr(false));
      return rgba[3] == 0 || (rgba[0] == 0 && rgba[1] == 0 && rgba[2] == 0);
    }


    template<typename T, WordType w>
    inline void ReaderBase::ReaderCvtSteps<T,w>::background_blend(ReaderBase const *r,
                                                                  char const *src, char *tgt,
                                                                  size_t n) const
    {
      T const *s = reinterpret_cast<T const *>(src);
      T       *t = reinterpret_cast<T       *>(tgt);
      T const *const bg_rgba = reinterpret_cast<T *>(r->get_rgba_ptr(false));
      T bg[3] = { bg_rgba[0] * bg_rgba[3], bg_rgba[1] * bg_rgba[3], bg_rgba[2] * bg_rgba[3] };
      for(size_t i=0; i<n; ++i)
      {
        if(T const a = s[3])
        {
          if(T const b = 1 - a) for(int j=0; j<3; ++j) t[j] = a*s[j] + b*bg[j];
          else std::copy(s, s+3, t); // alpha = 1
        }
        else for(int j=0; j<3; ++j) t[j] = bg[j]; // alpha = 0
        s += 4;
        t += 3;
      }
    }


    // Assumes that the incoming block is so big that it needs to be subdivided 
    template<class Op> inline
    void ReaderBase::subdivide_block_op(Op &op, int width, int height, size_t max_pixels_per_subblock)
    {
      int subblock_width, subblock_height;
      if(width < height)
      {
        // Tall tray
        subblock_width  = std::min<int>(sqrtf(max_pixels_per_subblock), width);
        subblock_height = std::min<int>(max_pixels_per_subblock / subblock_width, height);
      }
      else
      {
        // Wide tray
        subblock_height = std::min<int>(sqrtf(max_pixels_per_subblock), height);
        subblock_width  = std::min<int>(max_pixels_per_subblock / subblock_height, width);
      }

      int y = 0, h = subblock_height;
      for(;;)
      {
        int x = 0, w = subblock_width;
        for(;;)
        {
          op(x, y, w, h);
          x += w;
          int const l = width - x;
          if(l <= 0) break;
          if(l < w) w = l; // Adjust width for final column
        }

        y += h;
        int const l = height - y;
        if(l <= 0) break;
        if(l < h) h = l; // Adjust height for final row
      }
    }


    template<typename T>
    inline void ReaderBase::clip_tray(util::BasicTupleGrid<T> &g, int &x, int &y, int &w, int &h)
    {
      // Clip against left edge
      int d = clip_left - x;
      if(0 < d)
      {
        g.origin += d * g.pitch;
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
        g.origin += d * g.stride;
        y = clip_bottom;
        h -= d;
      }

      // Clip against to edge
      d = y + h - clip_top;
      if(0 < d) h -= d;
    }


    // Get either background or foreground color expressed in RGBA
    // using one 'best_float' per component. NOTE: It is the
    // responsibility of the caller to ensure that this method is not
    // called until after prep_color_slots() has been called.
    inline char *ReaderBase::get_rgba_ptr(bool foreground) const
    {
      char *p = color_slot_buffer.get();
      if(foreground) p += 4*bytes_per_best_float;
      return p;
    }


    // Get a pointer to either the backgound or the foreground
    // color. The pixel format will be identical to that of the held
    // image codec. Both internal buffers may get clobbered.
    inline char *ReaderBase::get_color_ptr(bool foreground) const
    {
      const_cast<ReaderBase *>(this)->prep_color_slots();
      char *p = color_slot_buffer.get() + 2*4*bytes_per_best_float;
      if(foreground) p += pixel_format.bytes_per_pixel;
      return p;
    }


    inline void ReaderBase::prep_color_slots()
    {
      if(color_slot_buffer) return;
      color_slot_buffer.reset(2 * 4 * bytes_per_best_float +
                              num_color_slots * pixel_format.bytes_per_pixel);
      set_color(util::Color::transparent, false); // Background color
      set_color(util::Color::black, true);        // Foreground color
    }


    inline void ReaderBase::store_color(char const *b, PixelFormat const &f, bool foreground)
    {
      cvt(f, b, pixel_format,                             get_color_ptr(foreground), 1);
      cvt(f, b, PixelFormat(rgb.get(), true, best_float), get_rgba_ptr(foreground),  1);
      write_cvt_initialized = read_cvt_initialized = false;
    }


    inline void ReaderBase::clear_tray(TupleGrid g, PixelFormat const &f, int w, int h)
    {
      cvt(pixel_format, get_color_ptr(false), f, g.origin, 1); // Convert to tray format
      g.extend(f.bytes_per_pixel, 1, 1, 0, 0, 0, 0, 0, w-1, 0, h-1);
    }

    inline void ReaderBase::cvt(PixelFormat const &f, char const *s,
                                PixelFormat const &g, char *t, size_t n)
    {
      PixelConverter c(f, g, buffers);
      if(c.is_noop()) std::copy(s, s + n*f.bytes_per_pixel, t);
      else c(s,t,n);
    }
  }
}


#endif // ARCHON_IMAGE_READER_BASE_HPP
