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

#ifndef ARCHON_X_IMAGE_X_TEST_X_PIXEL_UTILS_HPP
#define ARCHON_X_IMAGE_X_TEST_X_PIXEL_UTILS_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>

#include <archon/core/features.h>
#include <archon/check/test_context.hpp>
#include <archon/check/check_macros.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/writer.hpp>


namespace archon::image::test {


template<class R> bool check_approx_equal_pixels(check::TestContext&, const image::Pixel<R>&, const image::Pixel<R>&);


template<class R> bool check_color_index(check::TestContext&, image::Writer&, std::size_t index,
                                         const image::Pixel<R>& expected_color);








// Implementation


template<class R> bool check_approx_equal_pixels(check::TestContext& test_context, const image::Pixel<R>& a,
                                                 const image::Pixel<R>& b)
{
    using repr_type = R;
    using comp_type = typename repr_type::comp_type;
    auto check = [&](comp_type a, comp_type b) {
        if constexpr (std::is_integral_v<comp_type>) {
            return ARCHON_CHECK_DIST_LESS_EQUAL(a, b, 1);
        }
        else {
            static_assert(std::is_floating_point_v<comp_type>);
            comp_type eps = std::numeric_limits<comp_type>::epsilon();
            return ARCHON_CHECK_APPROXIMATELY_EQUAL(a, b, 10 * eps);
        }
    };
    if (a.opacity() == 0 || b.opacity() == 0)
        return check(a.opacity(), b.opacity()); // Throws
    for (int i = 0; i < repr_type::num_channels; ++i) {
        if (ARCHON_UNLIKELY(!check(a[i], b[i]))) // Throws
            return false;
    }
    return true;
}


template<class R> bool check_color_index(check::TestContext& test_context, image::Writer& writer, std::size_t index,
                                         const image::Pixel<R>& expected_color)
{
    // Due to limited numeric precision, all we can require is that the distance from the
    // directly expected color (`expected_color`) to the palette entry corresponding with
    // the produced color index in the image (`sqdist`) is not significantly greater than
    // the distance from the directly expected color to the palette entry closest to the
    // directly expected color (`expected`).
    std::size_t index_2 = writer.reverse_palette_lookup(expected_color);
    image::Pixel<R> color_1, color_2;
    writer.palette_lookup(index, color_1);
    writer.palette_lookup(index_2, color_2);
    image::float_type sqdist   = writer.color_sqdist(expected_color, color_1);
    image::float_type expected = writer.color_sqdist(expected_color, color_2);
    image::float_type eps = std::numeric_limits<image::float_type>::epsilon();
    return ARCHON_CHECK_NOT_DEFINITELY_GREATER(sqdist, expected, 10 * eps);
}


} // namespace archon::image::test

#endif // ARCHON_X_IMAGE_X_TEST_X_PIXEL_UTILS_HPP
