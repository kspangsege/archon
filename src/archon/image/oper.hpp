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

#ifndef ARCHON_IMAGE_OPER_HPP
#define ARCHON_IMAGE_OPER_HPP

#include <vector>

#include <archon/image/image.hpp>


namespace Archon
{
  namespace Imaging
  {
    namespace Oper
    {
      /**
       * Get an alterntively bouded view of this image which can be
       * smaller or larger than the original.
       *
       * \param left, bottom Defines the bottom left corner of the
       * selected region measured in pixles from the lower left corner
       * of this image. Either may be negative. See \c
       * horizontal_repeat for more details.
       *
       * \param width, height Defines the size of the selected region
       * measured in pixels. The selected region may extend beyond the
       * bounds of this image. See \c horizontal_repeat for more
       * details.
       *
       * \param horizontal_repeat, vertical_repeat If the selected
       * region extends (or lies entirely) beyond the bounds of this
       * image, it is the values passed for these arguments that
       * determine the outcome, which will be as if \c get_pixels was
       * called with the same arguments.
       */
/*
      Ref get_sub_view(int left, int bottom, int width, int height,
                       int horizontal_repeat = 0, int vertical_repeat = 0);
*/

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
/*
      ConstRef get_sub_view(int left, int bottom, int width, int height,
                            int horizontal_repeat = 0, int vertical_repeat = 0) const;
*/


      /**
       * Get a flipped view of the specified image. The flipped view
       * is "live" in the sense that writing to either causes a change
       * in the other.
       */
      Image::Ref flip(Image::RefArg i, bool horizontal, bool vertical);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef flip(Image::ConstRefArg i, bool horizontal, bool vertical);


      /**
       * Get a diagonally flipped view of the specified image. The
       * flipped view is "live" in the sense that writing to either
       * causes a change in the other.
       */
      Image::Ref flip_diag(Image::RefArg i, bool even, bool odd);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef flip_diag(Image::ConstRefArg i, bool even, bool odd);


      /**
       * Get a rotated view of the specifed image. The rotation is
       * counter clock-wise. The flipped view is "live" in the sense
       * that writing to either causes a change in the other.
       *
       * \param ninety, one_eighty The angle to turn by. If both are
       * true the angle will be 270.
       *
       * \return The rotated view of the specified image.
       */
      Image::Ref rotate(Image::RefArg i, bool ninety, bool one_eighty);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef rotate(Image::ConstRefArg i, bool ninety, bool one_eighty);


      /**
       * Invert all the color channels of this image (produce the
       * negative), or if a channel index is specified, ivert only
       * that channel, which may be the alpha channel.
       *
       * \param channel_index The index of the desired channel. It
       * refers to the natural channel order of the color space of the
       * image. A negative value means 'all color channels'.
       */
      Image::Ref invert(Image::RefArg i, int channel_index = -1);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef invert(Image::ConstRefArg i, int channel_index = -1);


      /**
       * Get a view of the specified image that reinterprets the
       * meaning of its channels. The new view has the specified color
       * space and an alpha channel if requested. Each of the channels
       * of the new view can be selected individually among the
       * channels of the original image using the <tt>channel_map</tt>
       * argument.
       *
       * \param color_space The color space that should be reported as
       * being used natively by the new view.
       *
       * \param has_alpha True iff the new view should identify itself
       * as having an alpha channel.
       *
       * \param channel_map If <tt>channel_map[i] == j</tt>, it means
       * that the <tt>j</tt>'th channel of the original image is used
       * as the <tt>i</tt>'th channel of the new color space, thus,
       * the number of elements in \c channel_map must be eaqual to the
       * number of channels of the new view, which is the number of
       * color channels in the specified color space, plus one if an
       * alpha channel is requested. Channel indexes between 0 and N-1
       * refer to color channels in the natural channel order of the
       * specified color space, where N is the number of color
       * channels of this color space. A channel index equal to N,
       * refers to the alpha channel. The map must be an injection,
       * that is, <tt>channel_map[i]</tt> must be different from
       * <tt>channel_map[j]</tt> when <tt>i</tt> is different from
       * <tt>j</tt>.
       *
       * \return The new view of the original image.
       */
      Image::Ref remap_channels(Image::RefArg i,
                                ColorSpace::ConstRefArg color_space, bool has_alpha,
                                std::vector<int> const &channel_map);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef remap_channels(Image::ConstRefArg i,
                                     ColorSpace::ConstRefArg color_space, bool has_alpha,
                                     std::vector<int> const &channel_map);


      /**
       * Isolate a single channel of an image and access it as a
       * one-channeled image which will be idetified as a <i>luminance
       * channel<i>. You may optionally request to preserve the
       * transparency information available in the original image, in
       * which case the resulting view will have an alpha channel if,
       * and only if the original image has one, and the channel you
       * request to isolate, is not the alpha channel itself.
       *
       * \param channel_index The index of the desired channel. It
       * refers to the natural channel order of the color space of the
       * image.
       *
       * \param preserve_alpha Pass true if you would like to include
       * the alpha channel of the original image, if it has one.
       */
      Image::Ref pick_channel(Image::RefArg i, int channel_index, bool preserve_alpha = true);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef pick_channel(Image::ConstRefArg i, int channel_index,
                                  bool preserve_alpha = true);


      /**
       * If the specified image has an alpha channel, it will be
       * discarded. That is, the alpha channel will not be merged into
       * the image, but just simply forgotten about. If the specified
       * image does not have an alpha channel, this method simply
       * returns that image.
       */
      Image::Ref discard_alpha(Image::RefArg i);

      /**
       * Same as the non-const version, except that the returned view
       * is now "read only".
       */
      Image::ConstRef discard_alpha(Image::ConstRefArg i);


      /**
       * Assuming that this image is an indirect color image, present
       * it as a direct color image where each pixel is mapped through
       * the specified palette. In an indirect color image each pixel
       * is an index into the color table / palette.
       *
       * The palette is itself an image. The number of colors in the
       * palette is equal to the number of pixels in the palette
       * image. The order is row major starting from the lower left
       * corner. That is, the first color is the one in the lower left
       * corner, and number two is immediately to the right of the
       * first one, or immediately above it, if the image has width 1.
       *
       * The color space of the resulting image, is the same as that
       * of the palette.
       */
      Image::ConstRef color_map(Image::ConstRefArg index_image, Image::ConstRefArg palette);





      // Implementation

      inline Image::ConstRef flip(Image::ConstRefArg i, bool horizontal, bool vertical)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return flip(j, horizontal, vertical);
      }

      inline Image::ConstRef flip_diag(Image::ConstRefArg i, bool even, bool odd)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return flip_diag(j, even, odd);
      }

      inline Image::ConstRef rotate(Image::ConstRefArg i, bool ninety, bool one_eighty)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return rotate(j, ninety, one_eighty);
      }

      inline Image::ConstRef remap_channels(Image::ConstRefArg i,
                                            ColorSpace::ConstRefArg color_space, bool has_alpha,
                                            std::vector<int> const &channel_map)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return remap_channels(j, color_space, has_alpha, channel_map);
      }


      inline Image::Ref pick_channel(Image::RefArg i, int channel_index, bool preserve_alpha)
      {
        std::vector<int> channel_map;
        channel_map.push_back(channel_index);
        bool add_alpha = false;
        if(preserve_alpha && i->has_alpha_channel())
        {
          int alpha_index = i->get_num_channels()-1;
          if(channel_index != alpha_index)
          {
            add_alpha = true;
            channel_map.push_back(alpha_index);
          }
        }
        return remap_channels(i, ColorSpace::get_Lum(), add_alpha, channel_map);
      }

      inline Image::ConstRef pick_channel(Image::ConstRefArg i, int channel_index,
                                          bool preserve_alpha)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return pick_channel(j, channel_index, preserve_alpha);
      }


      inline Image::Ref discard_alpha(Image::RefArg i)
      {
        if(!i->has_alpha_channel()) return i;
        std::vector<int> channel_map;
        for(int j=0; j<i->get_num_channels()-1; ++j) channel_map.push_back(j);
        return remap_channels(i, i->get_color_space(), false, channel_map);
      }

      inline Image::ConstRef discard_alpha(Image::ConstRefArg i)
      {
        Image::Ref j(const_cast<Image *>(i.get()));
        return discard_alpha(j);
      }
    }
  }
}


#endif // ARCHON_IMAGE_OPER_HPP
