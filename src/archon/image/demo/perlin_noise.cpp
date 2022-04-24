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


#include <cmath>
#include <memory>
#include <tuple>
#include <random>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/random.hpp>
#include <archon/cli.hpp>
#include <archon/math/vec.hpp>
#include <archon/util/perlin_noise.hpp>
#include <archon/image.hpp>
#include <archon/image/computed_image.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    namespace fs = std::filesystem;
    image::Size image_size;
    fs::path path;
    math::Vec2 feature_size = { 16, 16 };
    math::Vec2 feature_shift = { 0, 0 };

    cli::Spec spec;
    pat("<size>  <path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(image_size, path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-s, --feature-size", "<vec>", cli::no_attributes, spec,
        "The default feature size is @V.",
        cli::assign(core::as_list_a(feature_size.components(), 1, true))); // Throws

    opt("-i, --feature-shift", "<vec>", cli::no_attributes, spec,
        "The default feature shift is @V.",
        cli::assign(core::as_list_a(feature_shift.components(), 1, true))); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    using noise_type = util::PerlinNoise<2, double, util::PerlinNoiseBase::Interp::smoother>;
    using size_type = noise_type::size_type;
    using vec_type = noise_type::vec_type;
    vec_type grid_gauge = feature_size;
    vec_type grid_pos;
    size_type grid_size;
    for (int i = 0; i < 2; ++i) {
        grid_pos[i] = core::periodic_mod(feature_shift[i], -feature_size[i]); // Throws
        core::float_to_int(std::ceil((image_size.width - grid_pos[i]) / feature_size[i]),
                           grid_size[i]); // Throws
    }
    std::mt19937_64 random;
    core::seed_prng_nondeterministically(random); // Throws
    std::unique_ptr<vec_type[]> gradients = noise_type::alloc_gradients(grid_size); // Throws
    noise_type::init_gradients(grid_size, gradients.get(), random); // Throws
    noise_type noise(grid_size, grid_gauge, grid_pos, gradients.get());

    image::ComputedImage image(image_size, [&](image::Pos pos) {
        math::Vec2 pos_2 = { double(pos.x), double(pos.y) };
        double val = noise(pos_2, 0, 1);
        return image::Pixel_Lum_F({ image::float_type(val) });
    }); // Throws
    image::save(image, path, locale); // Throws
}
