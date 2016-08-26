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

#include <cstdlib>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>

#include <archon/core/functions.hpp>
#include <archon/core/random.hpp>
#include <archon/core/options.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/file.hpp>
#include <archon/math/vec_ops.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/kd_tree.hpp>
#include <archon/util/ticker.hpp>
#include <archon/util/color.hpp>
#include <archon/image/writer.hpp>
#include <archon/image/oper.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::math;
using namespace archon::image;


//bool palette_clamp(float const *pal, std::size_t pal_size, float *col);


namespace {

using ColorSpaceEnum = ColorSpace::TypeEnum;

std::string    opt_palette      = "";
ColorSpaceEnum opt_color_space  = ColorSpace::type_LAB;
bool           opt_no_pal_clamp = false;

std::string out_prefix;

/*
const int kern_rows = 3, kern_cols = 5, kern_offset = 2;
float kern[kern_rows * kern_cols] =
{
    0, 0, 0, 7, 5,
    3, 5, 7, 5, 3,
    1, 3, 5, 3, 1
};
*/

const int kern_rows = 2, kern_cols = 3, kern_offset = 1;
float kern[kern_rows * kern_cols] =
{
    0, 0, 7,
    3, 5, 1
};





// Returns the index image
Image::Ref color_quantize(Image::ConstRef image, Image::ConstRef palette,
                          ColorSpace::ConstRefArg cmp_color_space = ColorSpace::get_Lum())
{
    // Convert palette to comparison color space
    std::size_t pal_width = palette->get_width(), pal_height = palette->get_width();
    std::size_t pal_size = std::min<std::size_t>(pal_height * pal_width, 4096); // Why the 4096?
    if (!pal_size)
        throw std::invalid_argument("Empty palette");
    bool pal_has_alpha = palette->has_alpha_channel();
    int num_cmp_channels = cmp_color_space->get_num_channels(pal_has_alpha);
    std::unique_ptr<float[]> pal_buf = std::make_unique<float[]>((pal_size+2) * num_cmp_channels);
    float* pal_min = pal_buf.get();
    float* pal_max = pal_min + num_cmp_channels;
    float* pal     = pal_max + num_cmp_channels;
    ImageReader(palette).get_block(pal, pal_width, pal_height, cmp_color_space, pal_has_alpha);

    // Set up a kd-tree for efficient palette lookups (for 256 color palettes,
    // and a 3 component color space, the speedup is only about 2-3 times, more
    // colors meens more speedup)
    //
    // FIXME: The kd-tree had a bug that caused it to not be balanced, this may
    // be the reason the speedup was only 2-3 times.
    KdTreeMap<float, std::size_t> kd_tree(num_cmp_channels);
    kd_tree.add_contig(pal, 0, pal_size);

    for (std::size_t i = 0; i < pal_size; ++i) {
        for( int j = 0; j < num_cmp_channels; ++j) {
            float v = pal[i*num_cmp_channels + j];
            if (i == 0 || v < pal_min[j])
                pal_min[j] = v;
            if (i == 0 || pal_max[j] < v)
                pal_max[j] = v;
        }
        vec_print(std::cout << "pal["<<i<<"] = ", pal + i*num_cmp_channels, pal + i*num_cmp_channels + num_cmp_channels);
        std::cout << std::endl;
    }

    float kern_sum = float();
    for (int i = 0; i < kern_rows * kern_cols; ++i)
        kern_sum += kern[i];
    std::cout << "kernel sum = " << kern_sum << std::endl;

    // Generate the index image
    ImageReader reader(image);
    int width = reader.get_width(), height = reader.get_height();

    WordType idx_type = get_smallest_int_type_by_max_val(pal_size - 1);
    std::cout << "Index type = '" << get_word_type_name(idx_type) << "' based on a palette size of " << pal_size << std::endl;
    ColorSpace::ConstRef lum = ColorSpace::get_Lum();
    Image::Ref idx_img = Image::new_image(width, height, lum, false, idx_type);
    ImageWriter writer(idx_img);

    Image::Ref clamp_img = Image::new_image(width, height, ColorSpace::get_RGB(), pal_has_alpha);
    ImageWriter clamp_writer(clamp_img);

    std::size_t err_pitch  = num_cmp_channels;
    std::size_t err_stride = width * err_pitch;
    std::size_t err_buf_size = kern_rows * err_stride;
    std::size_t pix_buf_size = 2*num_cmp_channels + err_buf_size;
    std::unique_ptr<float[]> pix_buf = std::make_unique<float[]>(pix_buf_size);
    float* pix1    = pix_buf.get();
    float* pix2    = pix1 + num_cmp_channels;
    float* err_buf = pix2 + num_cmp_channels;

    std::fill(err_buf, err_buf+err_buf_size, float());

    std::unique_ptr<char[]> idx_buf =  std::make_unique<char[]>(get_bytes_per_word(idx_type));
    WordTypeConverter idx_cvt = get_word_type_clamp_converter(get_word_type_by_type<std::size_t>(), idx_type);
//    Random r;
    AdaptiveTicker ticker(10000);

    for (int i = 0; i < height; ++i) { // Top-down
        int y = height-1 - i; // y=0 is always at the bottom of the image
        float* err_row = err_buf + (i%kern_rows) * err_stride;
        bool reverse = i&1; // Right to left on every second scanline
        for (int j = 0; j < width; ++j) {
            int x = reverse ? width-1 - j : j; // x=0 is always at the left edge of the image
            float* err = err_row + x * err_pitch;
            reader.set_pos(x,y).get_pixel(pix1, cmp_color_space, pal_has_alpha); // pix1 = source[x,y]

            vec_add_assign(pix1, pix1 + num_cmp_channels, err);                  // pix1 += err

            if (!opt_no_pal_clamp) {
//                palette_clamp(pal, pal_size, pix1);
                clamp_writer.set_pos(x,y).put_pixel(pix1, cmp_color_space, pal_has_alpha);
            }

/*
            for (int k = 0; k < num_cmp_channels; ++k) {
                float v = pix1[k];
                if (pal_max[k] < v)
                    pix1[k] = pal_max[k];
                else if(v < pal_min[k]) {
                    pix1[k] = pal_min[k];
                }
            }
*/

            std::fill(err, err+num_cmp_channels, float());                       // err = 0

            // Find best match in palette
//            pix1[int(num_cmp_channels*r.get_uniform())] += 0.01;
            std::size_t idx = kd_tree[pix1];

            idx_cvt(&idx, idx_buf.get(), 1);
            writer.set_pos(x,y).put_pixel(idx_buf.get(), lum, false, idx_type);  // target[x,y] = inv(pal)(quant(pix1))
            vec_sub_assign(pix1, pix1 + num_cmp_channels, pal + idx*num_cmp_channels); // pix1 -= quant(pix1)

            // Clamp the error to limit color bleeding
/*
            float err_max = 0.4*1000;
            vec_unop_assign(pix1, pix1 + num_cmp_channels, Clamp<float>(-err_max, err_max));
*/

            // Distribute the error
            for (int k = 0; k < kern_rows; ++k) {
                int i2 = i + k;
                if (height <= i)
                    continue;

                const float* kern_row = kern + k * kern_cols;
                float* err_row2 = err_buf + (i2%kern_rows) * err_stride;
                for (int l = 0; l < kern_cols; ++l) {
                    int j2 = j - kern_offset + l;
                    if (j2 < 0 || width <= j2)
                        continue;
                    int x2 = reverse ? width-1 - j2 : j2;
                    float* err = err_row2 + x2 * err_pitch;
                    vec_add_scale_assign(err, err + num_cmp_channels,
                                         pix1, kern_row[l]/kern_sum);            // err[x2,y2] += kern[j,i] * pix1
                }
            }

            if (ticker.tick()) {
                std::size_t m = i*std::size_t(width)+j, n = height*std::size_t(width);
                std::cout << m<<"/"<<n<<" ("<<std::setw(3)<<int(double(m)/(n)*100)<<"%)" << std::endl;
                if (!opt_no_pal_clamp) {
                    Oper::color_map(idx_img, palette)->save(out_prefix+"-2-clamp-dither.png");
                    clamp_img->save(out_prefix+"-1-clamp.png");
                }
            }
        }
    }

    if (!opt_no_pal_clamp) {
        Oper::color_map(idx_img, palette)->save(out_prefix+"-2-clamp-dither.png");
        clamp_img->save(out_prefix+"-1-clamp.png");
    }

    return idx_img;
}

} // unnamed namespace


int main(int argc, const char* argv[])
{
    CommandlineOptions opts;
    opts.add_help("Test application for image dithering feature", "SOURCE TARGET");
    opts.check_num_args(0,2);
    opts.add_param("p", "palette", opt_palette,
                   "Path to file holding palette image");
    opts.add_param("c", "color-space", opt_color_space,
                   "Match colors in this color space");
    opts.add_param("n", "no-pal-clamp", opt_no_pal_clamp,
                   "Do not clamp input pixels to the palette before dithering");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::string in_file  = argc < 2 ? file::dir_of(argv[0])+"../alley_baggett.png" : argv[1];
    Image::ConstRef image = Image::load(in_file);

    out_prefix = "dither-" + file::stem_of(in_file);
    out_prefix += "-" + (opt_palette.empty() ? "custom" : file::stem_of(opt_palette));
    out_prefix += "-" + ascii_tolower(opt_color_space.str());

    std::string out_file = argc < 3 ? out_prefix+(opt_no_pal_clamp ? "-3-direct_dither.png" : "-5-final.png") : argv[2];

    Image::ConstRef palette;
    if (!opt_palette.empty()) {
        palette = Image::load(opt_palette);
    }
    else {
        int m = 12, n = 12;
        Image::Ref p = Image::new_image(n, m);
        ImageWriter w(p);
        Random r;
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j)
                w.set_pos(j,i).put_pixel_rgb(r.get_uniform(), r.get_uniform(), r.get_uniform());
        }
        palette = p;
        p->save("/tmp/archon_image_palette.png");
    }

    Image::Ref idx_img = color_quantize(image, palette, ColorSpace::get(opt_color_space));

    Oper::color_map(idx_img, palette)->save(out_file);
    std::cout << "Image saved to: " << out_file << std::endl;
}
