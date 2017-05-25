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

#include <archon/core/types.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/series.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/string.hpp>
#include <archon/core/text_table.hpp>
#include <archon/core/options.hpp>
#include <archon/core/file.hpp>
#include <archon/image/reader.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;

int main(int argc, const char* argv[])
{
    Series<4, int>         opt_clip(0, 0, -1, -1);
    Series<2, int>         opt_pos(0, 0);
    Series<2, FalloffEnum> opt_falloff(falloff_Background, falloff_Background);
    Series<2, int>         opt_block_size(1, 1);
    std::string            opt_save;
    bool                   opt_no_print(false);

    CommandlineOptions opts;
    opts.add_help("Test application for image readers", "SOURCE-IMAGE");
    opts.check_num_args(0,1);
    opts.add_param("p", "pos", opt_pos,
                   "Position in image to read from");
    opts.add_param("c", "clip", opt_clip,
                   "The region that the read opertation is clippped to");
    opts.add_param("f", "falloff", opt_falloff,
                   "The horizontal and vertical behavior when accessing pixels ouside the clipping region");
    opts.add_param("s", "block-size", opt_block_size,
                   "Size of block to read");
    opts.add_switch("", "save", opt_save, opt_save,
                    "Save the extracted block to this path", true);
    opts.add_switch("n", "no-print", opt_no_print, true,
                    "Do not print the extracted pixels to STDOUT.");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::string in_file = (argc < 2 ? file::dir_of(argv[0])+"../alley_baggett.png" : argv[1]);

    ColorSpace::ConstRef color_space = ColorSpace::get_RGB();
    bool has_alpha = true;
    int num_channels = color_space->get_num_primaries() + (has_alpha?1:0);
    int width = opt_block_size[0], height = opt_block_size[1];
    std::size_t n = height * std::size_t(width);
    std::unique_ptr<unsigned char[]> buffer = std::make_unique<unsigned char[]>(n * num_channels);
    std::cout << "Buffer size = " << n * num_channels * sizeof *buffer.get() << std::endl;

    ImageReader r(in_file);
    r.set_background_color(color::red).set_clip(opt_clip[0], opt_clip[1], opt_clip[2], opt_clip[3]);
    r.set_pos(opt_pos[0], opt_pos[1]).set_falloff(opt_falloff[0], opt_falloff[1]);
    r.get_block(buffer.get(), width, height, color_space, has_alpha);

    if (!opt_no_print) {
        Text::Table table;
        table.get_odd_row_attr().set_bg_color(Term::color_White);
        table.get_odd_col_attr().set_bold();
        table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();
        table.get_cell(0,0).set_text("Pixel");
        for (int i=0; i<num_channels; ++i)
            table.get_cell(0, i+1).set_text(color_space->get_channel_name(i));
        for (int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                std::size_t j = y * std::size_t(width) + x;
                table.get_cell(j+1, 0).set_text(format_int(x)+","+format_int(y));
                for (int i = 0; i < num_channels; ++i)
                    table.get_cell(j+1, i+1).set_val(to_num(buffer[j*num_channels + i]));
            }
        }

        std::cout << table.print();
    }

    if (!opt_save.empty())
        Image::new_image(buffer.get(), width, height, color_space, has_alpha)->save(opt_save);
}
