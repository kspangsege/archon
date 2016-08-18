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

/// \file
///
/// \author Kristian Spangsege

#include <archon/image/imageio.hpp>
#include <archon/image/writer.hpp>


using namespace std;
using namespace archon::util;
using namespace archon::image;

namespace archon {
namespace image {

void Image::fill(PackedTRGB color)
{
    ImageWriter(Ref(this)).set_foreground_color(color).fill();
}


void Image::put_image(Image::ConstRefArg image, int x, int y, bool blend)
{
    ImageWriter(Ref(this)).set_pos(x,y).enable_blending(blend).put_image(image);
}


Image::Ref Image::load(string file_path)
{
    return load_image(file_path);
}


void Image::save(string file_path) const
{
    save_image(Image::ConstRef(this), file_path);
}


Image::Ref Image::new_image(void* b, WordType t, int w, int h,
                            ColorSpace::ConstRefArg c, bool a)
{
    BufferFormat::Ref f = BufferFormat::get_simple_format(t, c->get_num_channels(a));
    return b ? BufferedImage::new_image(b,w,h,c,a,f) : BufferedImage::new_image(w,h,c,a,f);
}


/*
/// \todo Do not write a region that is larger than what will fit into the
/// target image (possible - what about non-repeated source - and non-repeated
/// target) That will hopefully also allow us to work with forward iterations.
///
/// \todo Use just one buffer with room for an extra pixel such that color space
/// transformations do not clobber themselves.
///
/// \todo Make performance test to see whether 36KB transfer buffer is a good
/// size.
void Image::put_image(Image::ConstRefArg source,
                      int target_left, int target_bottom,
                      int source_left, int source_bottom,
                      int source_width, int source_height,
                      int source_horizontal_repeat, int source_vertical_repeat,
                      int target_horizontal_repeat, int target_vertical_repeat)
{
    // This method copies a source_width x source_height block of pixels from
    // the specified source image to this image, but to prevent excessive memory
    // allocation the pixels are transfered in smaller blocks using a so called
    // 'tray' buffer. Each block is read using the get_pixels method of the
    // source image which allows us to read beyond the images edges taking the
    // source repetition specification into account. The block is then
    // transcoded from the color space of the source image to the color space of
    // this image unless they are the same, and then written using the
    // put_pixels method of this image which again allows writting beyond the
    // image edges and taking the target repetition specification into account.

    int principal_source_width  = source->get_width();
    int principal_source_height = source->get_height();
    int principal_target_width  = get_width();
    int principal_target_height = get_height();

    if (!source_width)
        source_width = principal_source_width;
    if (!source_height)
        source_height = principal_source_height;

    // We don't want to read any more data from the source than we have to, so
    // if the transfer size is larger than the target image or extends beyond
    // its repetition compound, we cut it down accordingly
    if (target_horizontal_repeat) {
        if (target_left < 0) {
            source_width += target_left;
            if (source_width < 1)
                return;
            source_left -= target_left;
            target_left = 0;
        }
        int right = target_left + source_width -
            target_horizontal_repeat * principal_target_width;
        if (0 < right) {
            source_width -= right;
            if (source_width < 1)
                return;
        }
        target_horizontal_repeat = 0; // 'write_pixels' need not re-check
    }

    if (target_vertical_repeat) {
        if (target_bottom < 0) {
            source_height += target_Bottom;
            if (source_height < 1)
                return;
            source_bottom -= target_bottom;
            target_bottom = 0;
        }
        int top = target_bottom + source_height -
            target_vertical_repeat * principal_target_height;
        if (0 < top) {
            source_height -= top;
            if (source_height < 1)
                return;
        }
        target_vertical_repeat = 0; // 'write_pixels' need not re-check
    }

    if (principal_target_width < source_width)
        source_width = principal_target_width;
    if (principal_target_height < source_height)
        source_height = principal_target_height;

    // If the source image is small, then choose a tray size that holds an
    // integer number of repetitions of the source image, since then we only
    // need to read from the source image once, even when the source _region_ is
    // large. The tray size is limited to 24 x 24 pixels which for a 4 component
    // color space corresponds to 18KB on GNU/i386 platform due to the fact that
    // a double takes up 8 bytes.
    int max_tray_width  = 24;
    int max_tray_height = 24;
    int tray_width  = max_tray_width;
    int tray_height = max_tray_height;
    bool read_once;
    if (source_horizontal_repeat==0 && source_vertical_repeat==0) {
        tray_width  -= max_tray_width  % principal_source_width;
        tray_height -= max_tray_height % principal_source_height;
        read_once = tray_width && tray_height;
        if (!read_once) {
            tray_width  = max_tray_width;
            tray_height = max_tray_height;
        }
    }
    else {
        read_once = false;
    }
    if (source_width  < tray_width)
        tray_width  = source_width;
    if (source_height < tray_height)
        tray_height = source_height;

    Array<double> source_tray, intermediate_tray, target_tray;
    double* effective_source_tray;
    double* effective_target_tray;
    const ColorSpace::Converter* to_rgba = nullptr;
    const ColorSpace::Converter* from_rgba = nullptr;

    ColorSpace::ConstRef source_color_space = source->get_color_space();
    ColorSpace::ConstRef target_color_space = get_color_space();
    bool source_has_alpha = source->has_alpha_channel();
    bool target_has_alpha = has_alpha_channel();
    int source_pitch = source_color_space->get_num_primaries() + (source_has_alpha?1:0);
    int target_pitch = target_color_space->get_num_primaries() + (target_has_alpha?1:0);
    int source_stride = source_pitch*tray_width;
    int target_stride = target_pitch*tray_width;
    if (source_color_space == target_color_space) {
        // Compatible color spaces - only one tray is needed
        intermediate_tray.reset(source_stride*tray_height);
        effective_source_tray = intermediate_tray.get();
        effective_target_tray = intermediate_tray.get();
    }
    else {
        // Incompatible color spaces - an intermediate tray for RGBA is needed
        intermediate_tray.reset(4*tray_width*tray_height);
        effective_source_tray = intermediate_tray.get();
        effective_target_tray = intermediate_tray.get();
        if (!(source_color_space->is_rgb() && source_has_alpha)) {
            // Source color space is not RGBA - conversion is needed
            source_tray.reset(source_stride*tray_height);
            effective_source_tray = source_tray.get();
            to_rgba = &source_color_space->to_rgb(word_type_Double, source_has_alpha, true);
        }
        if (!(target_color_space->is_rgb() && target_has_alpha)) {
            // Target color space is not RGBA - conversion is needed
            target_tray.reset(target_stride*tray_height);
            effective_target_tray = target_tray.get();
            from_rgba = &target_color_space->from_rgb(word_type_Double, true, target_has_alpha);
        }
    }

    bool r = true;
    int y = 0, h = tray_height, read_width;
    for (;;) {
        int x = 0, w = tray_width;
        for (;;) {
            if (r) {
                source->read_block(Grid<double*>(effective_source_tray, w, h,
                                                 source_pitch, source_pitch*w),
                                   source_left+x, source_bottom+y,
                                   source_horizontal_repeat, source_vertical_repeat);
                read_width = w;
                if (to_rgba)
                    to_rgba->cvt(source_tray.get(), intermediate_tray.get(), w*h);
                if (from_rgba)
                    from_rgba->cvt(intermediate_tray.get(), target_tray.get(), w*h);
                if (read_once)
                    r = false;
            }

            write_block(Grid<const double*>(effective_target_tray, w, h,
                                            target_pitch, target_pitch*read_width),
                        target_left+x, target_bottom+y, target_horizontal_repeat, target_vertical_repeat);

            x += w;
            int l = source_width - x;
            if (l <= 0)
                break;
            if (l < w)
                w = l; // Adjust width for final column
        }

        y += h;
        int l = source_height - y;
        if (l <= 0)
            break;
        if (l < h)
            h = l; // Adjust height for final row
    }
}


void Image::write_block(const Grid<const double*>& tray, int left, int bottom,
                        int horizontal_repeat, int vertical_repeat)
{
    Grid<const double*> t = tray;

    // This method uses the low-level write_pixel_array method to write the
    // specified pixels into the image buffer. Since that method requires the
    // addressed region to lie strictly within the principal image area, it is
    // the responsibility (and in fact, the main objective) of this method to
    // figure out which region (or regions) to write to, based on the repetition
    // specification.

    int principal_width  = get_width();
    int principal_height = get_height();

    // Determine compound/tray intersection

    if (horizontal_repeat) {
        if (left < 0) {
            t.width += left;
            if (t.width < 1)
                return;
            t.lower_left -= t.pitch * left;
            left = 0;
        }

        int right = left + t.width - horizontal_repeat * principal_width;
        if (0 < right) {
            t.width -= right;
            if (t.width < 1)
                return;
        }
    }

    if (vertical_repeat) {
        if (bottom < 0) {
            t.height += bottom;
            if (t.height < 1)
                return;
            t.lower_left -= t.stride * bottom;
            bottom = 0;
        }

        int top = bottom + t.height - vertical_repeat * principal_height;
        if (0 < top) {
            t.height -= top;
            if (t.height < 1)
                return;
        }
    }

    // Now there is a non-empty intersection between the pixel tray and the
    // repetition compound.

    // Find module coordinates of lower left corner of intersection

    int transfer_left   = modulo<int>(left,   principal_width);
    int transfer_bottom = modulo<int>(bottom, principal_height);

    int transfer_width  = principal_width  - transfer_left;
    int transfer_height = principal_height - transfer_bottom;

    if (principal_width  < t.width)
        t.width = principal_width;
    if (principal_height < t.height)
        t.height = principal_height;

    int w = t.width  - transfer_width;
    int h = t.height - transfer_height;

    if (w < 0)
        transfer_width = t.width;
    if (h < 0)
        transfer_height = t.height;

    encode(const Grid<double*>(t.lower_left, transfer_width, transfer_height,
                               t.pitch, t.stride), transfer_left, transfer_bottom);

    if (0 < w) {
        encode(const Grid<double*>(t.lower_left + t.pitch * transfer_width,
                                   w, transfer_height, t.pitch, t.stride), 0, transfer_bottom);

        if (0 < h) {
            t.lower_left += t.stride * transfer_height;
            encode(Grid<const double*>(t.lower_left, transfer_width, h,
                                       t.pitch, t.stride), transfer_left, 0);
            encode(Grid<const double*>(t.lower_left + t.pitch * transfer_width,
                                       w, h, t.pitch, t.stride), 0, 0);
        }
    }
    else if (0 < h) {
        encode(Grid<const double*>(t.lower_left + t.stride * transfer_height,
                                   transfer_width, h, t.pitch, t.stride), transfer_left, 0);
    }
}
*/

} // namespace image
} // namespace archon
