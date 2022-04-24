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


#include <archon/util/colors.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/palette_image.hpp>
#include <archon/image/palettes.hpp>


using namespace archon;


namespace {


constexpr util::Color g_bw_colors[] = {
    util::colors::black,
    util::colors::white,
};


constexpr util::Color g_gray4_colors[] = {
    util::colors::black,
    util::colors::silver,
    util::colors::gray,
    util::colors::white,
};


constexpr util::Color g_css16_colors[] = {
    util::colors::black,
    util::colors::silver,
    util::colors::gray,
    util::colors::white,
    util::colors::maroon,
    util::colors::red,
    util::colors::purple,
    util::colors::fuchsia,
    util::colors::green,
    util::colors::lime,
    util::colors::olive,
    util::colors::yellow,
    util::colors::navy,
    util::colors::blue,
    util::colors::teal,
    util::colors::aqua,
};


template<std::size_t N> struct Palette {
    struct Colors {
        image::Pixel_RGBA_8 colors[N];

        Colors(const util::Color (&colors_2)[N])
        {
            for (std::size_t i = 0; i < N; ++i)
                colors[i] = image::Pixel_RGBA_8(colors_2[i]); // Throws
        }
    };

    Colors colors;

    image::PaletteImage_RGBA_8 image;

    Palette(const util::Color (&colors_2)[N])
        : colors(colors_2) // Throws
        , image(colors.colors)
    {
    }
};

template<std::size_t N> Palette(const util::Color (&)[N]) -> Palette<N>;


} // unnamed namespace



auto image::get_bw_palette() -> const image::PaletteImage_RGBA_8&
{
    static Palette palette(g_bw_colors); // Throws
    return palette.image;
}


auto image::get_gray4_palette() -> const image::PaletteImage_RGBA_8&
{
    static Palette palette(g_gray4_colors); // Throws
    return palette.image;
}


auto image::get_css16_palette() -> const image::PaletteImage_RGBA_8&
{
    static Palette palette(g_css16_colors); // Throws
    return palette.image;
}
