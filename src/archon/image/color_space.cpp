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


#include <algorithm>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/util/color_space.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/color_space.hpp>


using namespace archon;
namespace impl = image::impl;


void impl::ColorSpaceDegen::from_rgb(const comp_type* rgb, comp_type* native, comp_type alpha) const
{
    static_cast<void>(rgb);
    static_cast<void>(native);
    static_cast<void>(alpha);
}


void impl::ColorSpaceDegen::to_rgb(const comp_type* native, comp_type* rgb, comp_type alpha) const
{
    static_cast<void>(native);
    static_cast<void>(alpha);
    for (int i = 0; i < 3; ++i)
        rgb[i] = comp_type(1);
}


void impl::ColorSpaceLum::from_rgb(const comp_type* rgb, comp_type* native, comp_type alpha) const
{
    // Conversion between RGB and Lum is linear, so alpha premultiplication does not have to
    // be undone.
    static_cast<void>(alpha);
    util::cvt_RGB_to_Lum(rgb, native);
}


void impl::ColorSpaceLum::to_rgb(const comp_type* native, comp_type* rgb, comp_type alpha) const
{
    // Conversion between RGB and Lum is linear, so alpha premultiplication does not have to
    // be undone.
    static_cast<void>(alpha);
    util::cvt_Lum_to_RGB(native, rgb);
}


void impl::ColorSpaceRGB::from_rgb(const comp_type* rgb, comp_type* native, comp_type alpha) const
{
    static_cast<void>(alpha); // Immaterial
    std::copy_n(rgb, 3, native);
}


void impl::ColorSpaceRGB::to_rgb(const comp_type* native, comp_type* rgb, comp_type alpha) const
{
    static_cast<void>(alpha); // Immaterial
    std::copy_n(native, 3, rgb);
}


auto image::ColorSpaceConverterRegistry::do_find(const image::ColorSpace& origin,
                                                 const image::ColorSpace& destin) const noexcept ->
    const image::ColorSpaceConverter*
{
    ARCHON_ASSERT(&origin != &destin);
    ARCHON_ASSERT(!origin.is_rgb());
    ARCHON_ASSERT(!destin.is_rgb());
    key_type key = std::make_pair(&origin, &destin);
    auto i = m_map.find(key);
    if (ARCHON_LIKELY(i == m_map.end()))
        return nullptr;
    return i->second;
}


void impl::color_space_convert(image::float_type* pixel, image::float_type alpha,
                               const image::ColorSpace& origin_color_space,
                               const image::ColorSpace& destin_color_space,
                               const image::ColorSpaceConverter* custom_converter)
{
    ARCHON_ASSERT(&origin_color_space != &destin_color_space);
    if (ARCHON_LIKELY(!custom_converter)) {
        // Via RGB
        image::float_type rgb[3] = {};
        origin_color_space.to_rgb(pixel, rgb, alpha); // Throws
        destin_color_space.from_rgb(rgb, pixel, alpha); // Throws
        return;
    }
    ARCHON_ASSERT(!origin_color_space.is_rgb());
    ARCHON_ASSERT(!destin_color_space.is_rgb());
    custom_converter->convert(pixel, alpha); // Throws
}
