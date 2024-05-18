// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <random>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/format_as.hpp>
#include <archon/core/random.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/rectangle_packer.hpp>
#include <archon/image.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    namespace fs = std::filesystem;
    fs::path path;
    std::size_t num_rectangles = 400;
    // width range = 1-40            
    // height range = 1-40            
    int spacing = 1;
    int margin = 1;
    bool verbose = false;

    cli::Spec spec;
    pat("<path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-n, --num-rectangles", "<number>", cli::no_attributes, spec,
        "Set the number of rectangles to generate. The default number is @V.",
        cli::assign(core::as_int(num_rectangles))); // Throws

    opt("-s, --spacing", "<size>", cli::no_attributes, spec,
        "Set spacing between rectiangles in number of pixels. The default spacing is @V.",
        cli::assign(core::as_int(spacing))); // Throws

    opt("-m, --margin", "<size>", cli::no_attributes, spec,
        "Set margin (minimum distance bweteen rectangles and edge of bin) in number of pixels. The default margin is "
        "@V.",
        cli::assign(core::as_int(margin))); // Throws

    opt("-v, --verbose", "", cli::no_attributes, spec,
        "Reveal some information about the packing process.",
        cli::raise_flag(verbose)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    // Generate the rectangles
    std::mt19937_64 random;
    core::seed_prng_nondeterministically(random); // Throws
    std::vector<image::Box> rects;
    for (std::size_t i = 0; i < num_rectangles; ++i) {
        image::Box rect = {};
        rect.size.width  = core::rand_int(random, 1, 40); // Throws        
        rect.size.height = core::rand_int(random, 1, 40); // Throws        
        rects.push_back(rect); // Throws
    }

    // Pack them into a bin of the suggested width
    image::Size image_size;
    {
        util::RectanglePacker packer(spacing, margin);
        for (const image::Box& rect : rects)
            packer.add_rect(rect.size.width, rect.size.height); // Throws
        int width = packer.suggest_bin_width();
        bool success = packer.pack(width); // Throws
        ARCHON_ASSERT(success);
        image_size.width  = packer.get_utilized_width();
        image_size.height = packer.get_utilized_height();
        for (std::size_t i = 0; i < num_rectangles; ++i) {
            image::Box& rect = rects[i];
            packer.get_rect_pos(i, rect.pos.x, rect.pos.y);
        }
    }

    if (verbose) {
        double avail_width  = double(image_size.width  - 2 * margin) + spacing;
        double avail_height = double(image_size.height - 2 * margin) + spacing;
        double avail_area = avail_width * avail_height;
        double used_area = 0;
        for (const image::Box& rect : rects) {
            double width  = double(rect.size.width)  + spacing;
            double height = double(rect.size.height) + spacing;
            used_area += width * height;
        }
        double coverage = used_area / avail_area;
        log::FileLogger logger(core::File::get_cout(), locale);
        logger.info("Efficiency: %s", core::as_percent(coverage, 1));
    }

    // Generate the image
    image::BufferedImage_RGB_8 image(image_size); // Throws
    image::Writer writer(image); // Throws
    for (const image::Box& rect : rects) {
        image::Pixel_RGB_F color;
        for (int i = 0; i < color.num_channels; ++i)
            color[i] = image::float_type(1 - 0.9 * core::rand_float<double>(random)); // Throws
        writer.set_foreground_color_a(color);
        writer.fill(rect); // Throws
    }
    image::save(image, path, locale); // Throws
}
