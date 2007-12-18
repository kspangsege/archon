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

#include <archon/image/buffered_image.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;
using namespace Archon::Imaging;

namespace
{
  struct ImageImpl: BufferedImage, private Image::Codec
  {
    int get_width() const
    {
      return width;
    }

    int get_height() const
    {
      return height;
    }

    ColorSpace::ConstRef get_color_space() const
    {
      return colorSpace;
    }

    bool has_alpha_channel() const
    {
      return hasAlpha;
    }

    int get_num_channels() const
    {
      return colorSpace->get_num_primaries() + (hasAlpha?1:0);
    }

    int get_channel_width(int channel) const
    {
      if(channel < 0)
      {
        int n = bufferFormat->get_num_channels(), w = 0;
        for(int i = 0; i<n; ++i)
        {
          int v = bufferFormat->get_channel_width(i);
          if(w < v) w = v;
        }
        return w;
      }
      return bufferFormat->get_channel_width(channel);
    }

    void *get_buffer_ptr()
    {
      return codec->getBufferPtr();
    }

    void const *get_buffer_ptr() const
    {
      return codec->getBufferPtr();
    }

    BufferFormat::ConstRef get_buffer_format() const
    {
      return bufferFormat;
    }


    CodecPtr acquire_codec()
    {
      CodecPtr c(this);
      return c;
    }


    WordType get_word_type() const
    {
      return codec->getTrayWordType();
    }

    // Implementing Image::Codec method
    void decode(TupleGrid const &g, int w, int h, int x, int y) const
    {
      codec->decode(g, w, h, x, y);
    }

    // Implementing Image::Codec method
    void encode(ConstTupleGrid const &g, int w, int h, int x, int y)
    {
      codec->encode(g, w, h, x, y);
    }

    // Implementing Image::Codec method
    void release() const {}



    ImageImpl(void *buffer, int width, int height,
              ColorSpace::ConstRef colorSpace, bool hasAlpha,
              BufferFormat::ConstRefArg bufferFormat):
      width(width), height(height),
      colorSpace(colorSpace), hasAlpha(hasAlpha), bufferFormat(bufferFormat)
    {
      int const n = colorSpace->get_num_primaries() + (hasAlpha?1:0);
      if(n != bufferFormat->get_num_channels())
        throw invalid_argument("Mismatching number of channels in "
                               "color space (plus alpha) and buffer format");

      if(!buffer)
      {
        // According to the standard, any block of memory allocated as
        // an array of characters through operator new[] is properly
        // aligned for any object of that size or smaller.
        buffer = new char[bufferFormat->get_required_buffer_size(width, height)];
        bufferOwned = true;
      }
      else
      {
        // We still need to verify the dimmensions against the buffer
        // format
        bufferFormat->get_required_buffer_size(width, height);
        bufferOwned = false;
      }

      codec.reset(bufferFormat->get_codec(buffer, width, height).release());
    }

    ~ImageImpl()
    {
      if(bufferOwned) delete[] reinterpret_cast<char *>(codec->getBufferPtr());
    }

    UniquePtr<BufferCodec> codec;
    bool bufferOwned;
    int const width, height;
    ColorSpace::ConstRef const colorSpace;
    bool const hasAlpha;
    BufferFormat::ConstRef const bufferFormat;
  };
}


namespace Archon
{
  namespace Imaging
  {
    BufferedImage::Ref BufferedImage::new_image(int width, int height,
                                                ColorSpace::ConstRefArg colorSpace, bool hasAlpha,
                                                BufferFormat::ConstRefArg bufferFormat)
    {
      BufferFormat::ConstRef f = bufferFormat;
      if(!f)
      {
        int n = colorSpace->get_num_primaries() + (hasAlpha?1:0);
        f = BufferFormat::get_default_format(n);
      }
      return new_image(0, width, height, colorSpace, hasAlpha, f);
    }

    BufferedImage::Ref BufferedImage::new_image(void *buffer, int width, int height,
                                                ColorSpace::ConstRefArg colorSpace, bool hasAlpha,
                                                BufferFormat::ConstRefArg bufferFormat)
    {
      return Ref(new ImageImpl(buffer, width, height, colorSpace, hasAlpha, bufferFormat));
    }
  }
}
