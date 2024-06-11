// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <array>
#include <tuple>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/blend.hpp>
#include <archon/image/computed_image.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    fs::path path;

    cli::Spec spec;
    pat("<path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    using pixel_type = image::Pixel_RGB_F;
    using comp_type = pixel_type::comp_type;
    pixel_type black = std::array<comp_type, 3> { 0, 0, 0 };

    pixel_type colors[8] = {
        std::array<comp_type, 3> { 1, 0, 0 },
        std::array<comp_type, 3> { 0, 1, 0 },
        std::array<comp_type, 3> { 0, 0, 1 },
        std::array<comp_type, 3> { 0, 1, 1 },
        std::array<comp_type, 3> { 1, 0, 1 },
        std::array<comp_type, 3> { 1, 1, 0 },
        std::array<comp_type, 3> { 1, 1, 1 },
        std::array<comp_type, 3> { 1, 1, 1 },
    };

    comp_type factors[8] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5, 1.0 };

    image::Size image_size = { 8 * 32, 8 * 32 };
    image::ComputedImage image(image_size, [&](image::Pos pos) {
        int i = pos.x / 32;
        int j = pos.y /  4;
        comp_type f = comp_type(image::gamma_expand(factors[i] * double(1 + j) / 64));
        return image::Pixel_RGB_8(colors[i].blend(black, image::BlendMode::over, f));
    }); // Throws
    image::save(image, path, locale); // Throws
}
