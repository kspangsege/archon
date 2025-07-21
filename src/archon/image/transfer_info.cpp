// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <cstddef>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/image.hpp>


using namespace archon;
using image::TransferInfo;


auto TransferInfo::determine_palette_size() const noexcept -> std::size_t
{
    if (!palette)
        return 0;

    image::Size palette_image_size = palette->get_size();
    std::size_t palette_size = 1;
    bool overflow = (!core::try_int_mul(palette_size, std::max(palette_image_size.width, 0)) ||
                     !core::try_int_mul(palette_size, std::max(palette_image_size.height, 0)));
    if (ARCHON_UNLIKELY(overflow))
        palette_size = core::int_max<std::size_t>();

    // Clamp palette size to available index range
    std::size_t max_index = core::int_mask<std::size_t>(std::min(index_depth, core::int_width<std::size_t>()));
    if (ARCHON_LIKELY(palette_size == 0 || max_index >= std::size_t(palette_size - 1)))
        return palette_size;
    return std::size_t(max_index + 1);
}
