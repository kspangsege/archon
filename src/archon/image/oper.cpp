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
#include <set>

#include <archon/core/memory.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/oper.hpp>
#include <archon/image/reader.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;
using namespace Archon::Imaging;

namespace
{
/*
  struct SimpleView: Image
  {
    int get_width()  const { return width;  }
    int get_height() const { return height; }

    ColorSpace::ConstRef get_color_space() const
    {
      return orig->get_color_space();
    }

    int get_channel_width(int channel) const
    {
      return orig->get_channel_width(channel);
    }


    Reader *acquire_reader() const
    {
      return new SimpleViewEncoder();
    }

    Writer *acquire_writer()
    {
      return this;
    }


    void read(Grid<double *> const &tray, int left, int bottom) const
    {
      // Adjust for direction of address growth
      Grid<double *> t = tray;
      if(horizontal_flip)
      {
        t.horizontal_flip();
        left = this->width - t.width - left;
      }
      if(vertical_flip)
      {
        t.vertical_flip();
        bottom = this->height - t.height - bottom;
      }
      if(diagonal_flip)
      {
        t.diagonal_flip();
        swap(left, bottom);
      }

      // Adjust for sub-view offset
      left   += this->left;
      bottom += this->bottom;

      if(escape) Image::read_block(orig.get(), t, left, bottom, horizontal_repeat, vertical_repeat);
      else Image::decode(orig.get(), t, left, bottom);
    }

    void write(Grid<double const *> const &tray, int left, int bottom)
    {
      // Adjust for direction of address growth
      Grid<double const *> t = tray;
      if(horizontal_flip)
      {
        t.horizontal_flip();
        left = this->width - t.width - left;
      }
      if(vertical_flip)
      {
        t.vertical_flip();
        bottom = this->height - t.height - bottom;
      }
      if(diagonal_flip)
      {
        t.diagonal_Flip();
        swap(left, bottom);
      }

      // Adjust for sub-view offset
      left   += this->left;
      bottom += this->bottom;

      if(escape) Image::write_block(orig.get(), t, left, bottom, horizontal_repeat, vertical_repeat);
      else Image::encode(orig.get(), t, left, bottom);
    }

*/
    /**
     * \todo FIXME: Part of obsolete scheme.
     */
/*
    void decode(Grid<double *> const &tray, int left, int bottom) const
    {
      // Adjust for direction of address growth
      Grid<double *> t = tray;
      if(horizontal_flip)
      {
        t.horizontal_flip();
        left = this->width - t.width - left;
      }
      if(vertical_flip)
      {
        t.vertical_flip();
        bottom = this->height - t.height - bottom;
      }
      if(diagonal_flip)
      {
        t.diagonal_flip();
        swap(left, bottom);
      }

      // Adjust for sub-view offset
      left   += this->left;
      bottom += this->bottom;

      if(escape) Image::read_block(orig.get(), t, left, bottom, horizontal_repeat, vertical_repeat);
      else Image::decode(orig.get(), t, left, bottom);
    }
*/
    /**
     * \todo FIXME: Part of obsolete scheme.
     */
/*
    void encode(Grid<double const *> const &tray, int left, int bottom)
    {
      // Adjust for direction of address growth
      Grid<double const *> t = tray;
      if(horizontal_flip)
      {
        t.horizontal_flip();
        left = this->width - t.width - left;
      }
      if(vertical_flip)
      {
        t.vertical_flip();
        bottom = this->height - t.height - bottom;
      }
      if(diagonal_flip)
      {
        t.diagonal_flip();
        swap(left, bottom);
      }

      // Adjust for sub-view offset
      left   += this->left;
      bottom += this->bottom;

      if(escape) Image::write_block(orig.get(), t, left, bottom, horizontal_repeat, vertical_repeat);
      else Image::encode(orig.get(), t, left, bottom);
    }


    SimpleView(Image *orig, int left, int bottom, int _width, int _height,
               bool horizontal_flip, bool vertical_flip, bool diagonal_flip,
               bool escape, int horizontal_repeat, int vertical_repeat):
      orig(orig), left(left), bottom(bottom),
      width(diagonal_flip  ? _height : _width),
      height(diagonal_flip ? _width  : _height),
      horizontal_flip(horizontal_flip), vertical_flip(vertical_flip), diagonal_flip(diagonal_flip),
      escape(escape), horizontal_repeat(horizontal_repeat), vertical_repeat(vertical_repeat) {}

    Image::Ref const orig;
    int const left, bottom;
    int const width, height;
    bool const horizontal_flip, vertical_flip, diagonal_flip;
    bool const escape;
    int const horizontal_repeat, vertical_repeat;
  };
*/



  /////////////////////////   FlippedView   /////////////////////////

  struct FlippedView: Image
  {
    int get_width()  const { return width;  }
    int get_height() const { return height; }
    ColorSpace::ConstRef get_color_space() const { return orig->get_color_space(); }
    bool has_alpha_channel() const { return orig->has_alpha_channel(); }
    WordType get_word_type() const { return orig->get_word_type(); }
    int get_num_channels() const { return orig->get_num_channels(); }
    int get_channel_width(int channel) const { return orig->get_channel_width(channel); }
    CodecPtr acquire_codec();

    FlippedView(Image::RefArg orig, bool horizontal, bool vertical, bool diagonal):
      orig(orig),
      width(diagonal  ? orig->get_height() : orig->get_width()),
      height(diagonal ? orig->get_width()  : orig->get_height()),
      horizontal(horizontal), vertical(vertical), diagonal(diagonal) {}

    // Note: The referenced original image may actually be logically
    // const, but the constructing functions ensure that if the
    // original image was const, then the caller gets a const pointer
    // to this wrapping view object, and constness is thus protected
    // by the assumption that any const function on the view object
    // does not lead to invocation of a non-const function of the
    // original image.
    Image::Ref const orig;
    int const width, height;
    bool const horizontal, vertical, diagonal;
  };

  struct FlippedCodec: Image::Codec
  {
    void decode(TupleGrid const &g, int w, int h, int x, int y) const
    {
      // Adjust for direction of address growth
      TupleGrid f = g;
      if(view->horizontal)
      {
        f.horizontal_flip(w);
        x = view->width - w - x;
      }
      if(view->vertical)
      {
        f.vertical_flip(h);
        y = view->height - h - y;
      }
      if(view->diagonal)
      {
        f.diagonal_flip();
        swap(w, h);
        swap(x, y);
      }

      sub_codec->decode(f,w,h,x,y);
    }

    void encode(ConstTupleGrid const &g, int w, int h, int x, int y)
    {
      // Adjust for direction of address growth
      ConstTupleGrid f = g;
      if(view->horizontal)
      {
        f.horizontal_flip(w);
        x = view->width - w - x;
      }
      if(view->vertical)
      {
        f.vertical_flip(h);
        y = view->height - h - y;
      }
      if(view->diagonal)
      {
        f.diagonal_flip();
        swap(w, h);
        swap(x, y);
      }

      sub_codec->encode(f,w,h,x,y);
    }

    FlippedCodec(FlippedView const *view):
      view(view), sub_codec(view->orig->acquire_codec().release()) {}

    FlippedView const *const view;
    Image::CodecPtr const sub_codec;
  };

  Image::CodecPtr FlippedView::acquire_codec()
  {
    Image::CodecPtr c(new FlippedCodec(this));
    return c;
  }



  /////////////////////////   InvertedView   /////////////////////////

  struct InvertedView: Image
  {
    int get_width()  const { return orig->get_width();  }
    int get_height() const { return orig->get_height(); }
    ColorSpace::ConstRef get_color_space() const { return orig->get_color_space(); }
    bool has_alpha_channel() const { return orig->has_alpha_channel(); }
    WordType get_word_type() const { return orig->get_word_type(); }
    int get_num_channels() const { return orig->get_num_channels(); }
    int get_channel_width(int channel) const { return orig->get_channel_width(channel); }
    CodecPtr acquire_codec();
    template<typename, WordType> struct CodecInstantiator;

    InvertedView(Image::RefArg orig, int channel_index):
      orig(orig), channel_index(channel_index), word_type(orig->get_word_type()) {}

    // Note: The referenced original image may actually be logically
    // const, but the constructing functions ensure that if the
    // original image was const, then the caller gets a const pointer
    // to this wrapping view object, and constness is thus protected
    // by the assumption that any const function on the view object
    // does not lead to invocation of a non-const function of the
    // original image.
    Image::Ref const orig;
    int const channel_index;
    WordType word_type;
  };

  template<typename T> struct InvertedCodec: Image::Codec
  {
    void decode(TupleGrid const &g, int w, int h, int x, int y) const
    {
      ssize_t const p = num_channels * ssize_t(sizeof(T));
      sub_codec->decode(TupleGrid(reinterpret_cast<char *>(buffer.get()), p, w*p), w, h, x, y);
      invert(h * size_t(w));
      g.expand_from(buffer.get(), num_channels, w, h);
    }


    void encode(ConstTupleGrid const &g, int w, int h, int x, int y)
    {
      g.contract_to(buffer.get(), num_channels, w, h);
      invert(h * size_t(w));
      ssize_t const p = num_channels * ssize_t(sizeof(T));
      sub_codec->encode(ConstTupleGrid(reinterpret_cast<char const *>(buffer.get()),
                                       p, w*p), w, h, x, y);
    }


    void invert(size_t n) const
    {
      T *p = buffer.get();
      int const channel_index = view->channel_index;
      if(num_channels == 1 || channel_index < 0)
      {
        if(num_primaries == num_channels)
        {
          // Invert every channel
          frac_complement(p, p, n*num_channels);
        }
        else
        {
          // Invert every channel except the alpha channel
          for(size_t i=0; i<n; ++i)
          {
            frac_complement(p, p, num_primaries);
            p += num_channels;
          }
        }
      }
      else
      {
        // Invert a specific channel only
        p += channel_index;
        for(size_t i=0; i<n; ++i)
        {
          *p = frac_complement(*p);
          p += num_channels;
        }
      }
    }


    InvertedCodec(InvertedView const *view):
      view(view), sub_codec(view->orig->acquire_codec().release()),
      num_primaries(view->orig->get_color_space()->get_num_primaries()),
      num_channels(view->orig->get_num_channels()),
      buffer(Image::Codec::get_max_pixels_per_block() * num_channels) {}

    InvertedView const *const view;
    Image::CodecPtr const sub_codec;
    int const num_primaries, num_channels;
    Array<T> const buffer;
  };

  template<typename T, WordType> struct InvertedView::CodecInstantiator
  {
    Codec *operator()(InvertedView const *view) const
    {
      return new InvertedCodec<T>(view);
    }
  };

  Image::CodecPtr InvertedView::acquire_codec()
  {
    CodecPtr a(WordTypeSwitch<CodecInstantiator,
               InvertedView const *, Codec *>()(word_type, this));
    return a;
  }



  /////////////////////////   ReinterpretedChannelsView   /////////////////////////

  struct ReinterpretedChannelsView: Image
  {
    int get_width()  const { return orig->get_width();  }
    int get_height() const { return orig->get_height(); }

    ColorSpace::ConstRef get_color_space() const
    {
      return color_space;
    }

    bool has_alpha_channel() const
    {
      return has_alpha;
    }

    WordType get_word_type() const
    {
      return orig->get_word_type();
    }

    int get_num_channels() const
    {
      return channel_map.size();
    }

    int get_channel_width(int channel) const
    {
      if(channel < 0)
      {
        int m = 0;
        for(vector<int>::const_iterator i=channel_map.begin(); i!=channel_map.end(); ++i)
        {
          int w = orig->get_channel_width(*i);
          if(m < w) m = w;
        }
        return m;
      }
      return orig->get_channel_width(channel_map[channel]);
    }

    CodecPtr acquire_codec();

    template<typename, WordType> struct CodecInstantiator;

    ReinterpretedChannelsView(Image::RefArg orig, ColorSpace::ConstRefArg color_space,
                              bool has_alpha, vector<int> const &channel_map):
      orig(orig), color_space(color_space), has_alpha(has_alpha), word_type(orig->get_word_type()),
      channel_map(channel_map) {}

    // Note: The referenced original image may actually be logically
    // const, but the constructing functions ensure that if the
    // original image was const, then the caller gets a const pointer
    // to this wrapping view object, and constness is thus protected
    // by the assumption that any const function on the view object
    // does not lead to invocation of a non-const function of the
    // original image.
    Image::Ref const orig;
    ColorSpace::ConstRef color_space;
    bool has_alpha;
    WordType word_type;
    vector<int> const channel_map;
  };

  template<typename T> struct ReinterpretedChannelsCodec: Image::Codec
  {
    void decode(TupleGrid const &g, int w, int h, int x, int y) const
    {
      TupleGrid f = g;
      int n = view->channel_map.size(), m = view->orig->get_num_channels();
      ssize_t pitch = m;
      ssize_t stride = w * pitch;
      sub_codec->decode(TupleGrid(reinterpret_cast<char *>(buffer.get()),
                                  pitch, stride), w, h, x, y);
      T const *source = buffer.get();
      T *target = reinterpret_cast<T *>(f.origin);
      for(int i=0; i<h; ++i)
      {
        for(int j=0; j<n; ++j)
        {
          T const *s = source + view->channel_map[j], *e = s + stride;
          T *t = target + j;
          while(s < e)
          {
            *t = *s;
            s += pitch;
            t += f.pitch;
          }
        }
        source += stride;
        target += f.stride;
      }
    }

    void encode(ConstTupleGrid const &g, int w, int h, int x, int y)
    {
      ConstTupleGrid f = g;
      int n = view->channel_map.size(), m = view->orig->get_num_channels();
      ssize_t pitch = m;
      ssize_t stride = w * pitch;
      // If there are as less channels in this view than in the
      // original image, then some channels will be un-touched by
      // the incoming block, so to prevent writing undefined data to
      // the image, we are forced to decode the block first.
      if(n != m)
        sub_codec->decode(TupleGrid(reinterpret_cast<char *>(buffer.get()),
                                    pitch, stride), w, h, x, y);
      T const *source = reinterpret_cast<T const *>(f.origin);
      T *target = buffer.get();
      for(int i=0; i<h; ++i)
      {
        for(int j=0; j<n; ++j)
        {
          T const *s = source + j;
          T *t = target + view->channel_map[j], *e = t + stride;
          while(t < e)
          {
            *t = *s;
            s += f.pitch;
            t += pitch;
          }
        }
        source += f.stride;
        target += stride;
      }

      sub_codec->encode(ConstTupleGrid(reinterpret_cast<char const *>(buffer.get()),
                                       pitch, stride), w, h, x, y);
    }

    ReinterpretedChannelsCodec(ReinterpretedChannelsView const *view):
      view(view), sub_codec(view->orig->acquire_codec().release()),
      buffer(Image::Codec::get_max_pixels_per_block() * view->orig->get_num_channels()) {}

    ReinterpretedChannelsView const *const view;
    Image::CodecPtr const sub_codec;
    Array<T> const buffer;
  };

  template<typename T, WordType> struct ReinterpretedChannelsView::CodecInstantiator
  {
    Codec *operator()(ReinterpretedChannelsView const *view) const
    {
      return new ReinterpretedChannelsCodec<T>(view);
    }
  };

  Image::CodecPtr ReinterpretedChannelsView::acquire_codec()
  {
    CodecPtr a(WordTypeSwitch<CodecInstantiator,
               ReinterpretedChannelsView const *, Codec *>()(word_type, this));
    return a;
  }



  /////////////////////////   ColorMappedView   /////////////////////////

  struct ColorMappedView: Image
  {
    int get_width()  const { return orig->get_width();  }
    int get_height() const { return orig->get_height(); }
    ColorSpace::ConstRef get_color_space() const { return palette->get_color_space(); }
    bool has_alpha_channel() const { return palette->has_alpha_channel(); }
    WordType get_word_type() const { return palette->get_word_type(); }
    int get_num_channels() const { return palette->get_num_channels(); }
    int get_channel_width(int channel) const { return palette->get_channel_width(channel); }
    CodecPtr acquire_codec();

    ColorMappedView(Image::ConstRefArg orig, Image::ConstRefArg palette):
      orig(orig), palette(palette) {}

    Image::ConstRef const orig;
    Image::ConstRef const palette;
  };

  struct ColorMappedCodec: Image::Codec
  {
    void decode(TupleGrid const &g, int w, int h, int x, int y) const
    {
      unsigned char *p = buffer.get();
      TupleGrid f = g;
      orig_reader.set_pos(x,y).get_block(p, w, h, lum, false);
      for(int i=0; i<h; ++i)
      {
        char *t = f.origin;
        for(int j=0; j<w; ++j)
        {
          size_t index = size_t(*p++) % palette_size;
          int pal_x = index % palette_width, pal_y = index / palette_width;          
          palette_codec->decode(TupleGrid(t, 0, 0), 1, 1, pal_x, pal_y);
          t += f.pitch;
        }
        f.origin += f.stride;
      }
    }

    void encode(ConstTupleGrid const &, int, int, int, int)
    {
      throw runtime_error("Forbidden call");
    }

    ColorMappedCodec(ColorMappedView const *view):
      view(view), orig_reader(view->orig),
      palette_codec(view->palette->acquire_codec().release()),
      palette_width(view->palette->get_width()),
      palette_size(view->palette->get_height() * size_t(palette_width)),
      buffer(Image::Codec::get_max_pixels_per_block()),
      lum(ColorSpace::get_Lum()) {}

    ColorMappedView const *const view;
    mutable ImageReader orig_reader;
    Image::CodecConstPtr const palette_codec;
    int const palette_width;
    size_t const palette_size;
    Array<unsigned char> buffer;
    ColorSpace::ConstRef const lum;
  };

  Image::CodecPtr ColorMappedView::acquire_codec()
  {
    Image::CodecPtr c(new ColorMappedCodec(this));
    return c;
  }
}


namespace Archon
{
  namespace Imaging
  {
    namespace Oper
    {
/*
    Image::Ref Image::get_sub_view(int left, int bottom, int width, int height,
                                   int horizontal_repeat, int vertical_repeat)
    {
      int w = get_width(), h = get_height();
      if(width <= 0 || height <= 0)
        throw invalid_argument("Negative or zero box size");
      bool escape = left < 0 || bottom < 0 ||
        w < left + width || h < bottom + height;
      return Ref(new SimpleView(this, left, bottom, width, height,
                                false, false, false, escape,
                                horizontal_repeat, vertical_repeat));
    }
*/

      Image::Ref flip(Image::RefArg i, bool horizontal, bool vertical)
      {
        return horizontal || vertical ?
          Image::Ref(new FlippedView(i, horizontal, vertical, false)) : i;
      }

      Image::Ref flip_diag(Image::RefArg i, bool even, bool odd)
      {
        return even || odd ? Image::Ref(new FlippedView(i, odd, odd, even != odd)) : i;
      }

      Image::Ref rotate(Image::RefArg i, bool ninety, bool one_eighty)
      {
        return ninety || one_eighty ?
          Image::Ref(new FlippedView(i, ninety != one_eighty, one_eighty, ninety)) : i;
      }

      Image::Ref invert(Image::RefArg i, int channel_index)
      {
        if(i->get_num_channels() <= channel_index)
          throw invalid_argument("Channel index out of range");
        return Image::Ref(new InvertedView(i, channel_index));
      }

      Image::Ref remap_channels(Image::RefArg image, ColorSpace::ConstRefArg color_space,
                                bool has_alpha, vector<int> const &channel_map)
      {
        int n = color_space->get_num_primaries() + (has_alpha?1:0);
        if(channel_map.size() != unsigned(n))
          throw invalid_argument("Size of channel_map must mtach number of channels in new vew");
        set<int> seen;
        ColorSpace::ConstRef orig_color_space = image->get_color_space();
        bool orig_has_alpha = image->has_alpha_channel();
        int m = orig_color_space->get_num_primaries() + (orig_has_alpha?1:0);
        bool trivial = true;
        for(int i=0; i<n; ++i)
        {
          int j = channel_map[i];
          if(j < 0 || m <= j) throw invalid_argument("Index out of range in channel_map");
          if(!seen.insert(j).second)
            throw invalid_argument("Multiple occurences of same index in channel_map");
          if(j != i) trivial = false;
        }
        return has_alpha == orig_has_alpha && color_space == orig_color_space && trivial ? image :
          Image::Ref(new ReinterpretedChannelsView(image, color_space, has_alpha, channel_map));
      }

      Image::ConstRef color_map(Image::ConstRefArg index_image, Image::ConstRefArg palette)
      {
        return Image::ConstRef(new ColorMappedView(index_image, palette));
      }
    }
  }
}
